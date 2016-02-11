/**
 * Copyright 2011-2015 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "audio/config.h"
#include "platform/windows/wasapi/wasapi.h"

using namespace tiny;
using namespace tiny::audio;

#if TINY_AUDIO_ENABLE_WASAPI

#include <InitGuid.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <vector>
#include <stdint.h>
#include <string.h>
#include "audio/module.h"
#include "audio/types.h"
#include "tiny/audio/capture.h"
#include "tiny/audio/enum.h"
#include "tiny/audio/render.h"
#include "tiny/hash/fnv.h"

namespace
{
	class WasapiCaptureDevice : public ICaptureDevice
	{
	public:
		WasapiCaptureDevice(IAudioClient* client, IAudioCaptureClient* captureClient, int sampleRate, int samplesPer10ms, int channels)
			: client(client)
			, captureClient(captureClient)
			, deviceSampleRate(sampleRate)
			, deviceSamplesPer10ms(samplesPer10ms)
			, deviceChannels(channels)
		{
			buffer.reserve(channels*samplesPer10ms);
		}

		virtual void release()
		{
			delete this;
		}

		virtual int sampleRate() const
		{
			return deviceSampleRate;
		}

		virtual int samplesPer10ms() const
		{
			return deviceSamplesPer10ms;
		}

		virtual int channels() const
		{
			return 1;
		}

		virtual bool start()
		{
			HRESULT hr = client->Start();
			if (FAILED(hr))
			{
				return false;
			}
			
			return true;
		}

		virtual void stop()
		{
			client->Stop();
		}

		virtual const float* get10msOfSamples()
		{
			// prune any data still in the buffer
			int currentSize = static_cast<int>(buffer.size());
			if (currentSize >= deviceSamplesPer10ms)
			{
				buffer.erase(buffer.begin(), buffer.begin() + deviceSamplesPer10ms);
				currentSize -= deviceSamplesPer10ms;

				// if we still have enough samples, return those
				if (currentSize >= deviceSamplesPer10ms)
				{
					return buffer.data();
				}
			}

			// do we have any pending data
			UINT nextPacketSize;
			HRESULT hr = captureClient->GetNextPacketSize(&nextPacketSize);
			if (FAILED(hr))
			{
				return nullptr;
			}

			// no data to read, don't do the COM call
			if (nextPacketSize == 0)
			{
				return nullptr;
			}

			// pull data from device
			BYTE* data;
			UINT framesAvailable;
			DWORD flags;
			hr = captureClient->GetBuffer(&data, &framesAvailable, &flags, NULL, NULL);
			if (FAILED(hr))
			{
				return nullptr;
			}

			const float* floatData = reinterpret_cast<const float*>(data);
			buffer.reserve(currentSize+framesAvailable);
			for (int ii = 0; ii != framesAvailable; ++ii)
			{
				buffer.push_back(floatData[ii*deviceChannels]);
			}
			hr = captureClient->ReleaseBuffer(framesAvailable);
			if (FAILED(hr))
			{
				return nullptr;
			}

			// enough data to return to caller?
			if (currentSize + static_cast<int>(framesAvailable) < deviceSamplesPer10ms)
			{
				return nullptr;
			}

			return buffer.data();
		}

	private:
		WasapiCaptureDevice(const WasapiCaptureDevice&); // = delete
		WasapiCaptureDevice& operator=(const WasapiCaptureDevice&); // = delete

		~WasapiCaptureDevice()
		{
			captureClient->Release();
			client->Release();
		}

		IAudioClient* const client;
		IAudioCaptureClient* const captureClient;
		std::vector<float> buffer;
		const int deviceSampleRate;
		const int deviceSamplesPer10ms;
		const int deviceChannels;
	};

	class WasapiRenderDevice : public IRenderDevice
	{
	public:
		WasapiRenderDevice(IAudioClient* client, IAudioRenderClient* renderClient, HANDLE event, int sampleRate, UINT bufferSize)
			: client(client)
			, renderClient(renderClient)
			, event(event)
			, deviceSampleRate(sampleRate)
			, bufferSize(bufferSize)
		{
		}

		virtual void release()
		{
			delete this;
		}

		virtual int sampleRate() const
		{
			return deviceSampleRate;
		}

		virtual bool start()
		{
			HRESULT hr = client->Start();
			if (FAILED(hr))
			{
				return false;
			}

			bufferIndex = 0;
			frameReadySem = CreateSemaphore(nullptr, c_npackets, c_npackets, nullptr);
			frameCompleteSem = CreateSemaphore(nullptr, 0, c_npackets, nullptr);
			threadShutdown = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			thread = CreateThread(nullptr, 0, threadEntry, this, 0, nullptr);
			return thread != nullptr;
		}

		virtual void stop()
		{
			SetEvent(threadShutdown);
			WaitForSingleObject(thread, INFINITE);
			CloseHandle(threadShutdown);
			CloseHandle(thread);
			CloseHandle(frameReadySem);
			CloseHandle(frameCompleteSem);
		}

		virtual int acquireBuffer(float** buffer)
		{
			// wait for a packet
			HANDLE wait[2] = {frameReadySem, thread};
			DWORD waitResult = WaitForMultipleObjects(2, wait, FALSE, 100);
			switch (waitResult)
			{
			case WAIT_OBJECT_0:
				*buffer = buffers[bufferIndex % c_npackets];
				return c_nsamples;

			case WAIT_TIMEOUT:
				return 0;
			}

			return -1;
		}

		virtual void commitBuffer()
		{
			// notify the processing thread
			++bufferIndex;
			ReleaseSemaphore(frameCompleteSem, 1, nullptr);
		}

		virtual void discardBuffer()
		{
			ReleaseSemaphore(frameReadySem, 1, nullptr);
		}

	private:
		WasapiRenderDevice(const WasapiRenderDevice&); // = delete
		WasapiRenderDevice& operator=(const WasapiRenderDevice&); // = delete

		~WasapiRenderDevice()
		{
			stop();
			CloseHandle(event);
			renderClient->Release();
			client->Release();
		}

		void threadProc()
		{
			// render a single frame of silence to begin
			BYTE* data;
			HRESULT hr = renderClient->GetBuffer(bufferSize, &data);
			if (SUCCEEDED(hr))
			{
				hr = renderClient->ReleaseBuffer(bufferSize, AUDCLNT_BUFFERFLAGS_SILENT);
			}
			if (FAILED(hr))
			{
				client->Stop();
				return;
			}

			HANDLE wait[2] = {frameCompleteSem, threadShutdown};
			int packetIndex = 0;
			for(;;)
			{
				// wait for a packet to be available, or a shutdown to be signaled
				DWORD waitResult = WaitForMultipleObjects(2, wait, FALSE, INFINITE);
				if (waitResult == WAIT_OBJECT_0+1)
				{
					client->Stop();
					return;
				}

				const float* packet = buffers[packetIndex % c_npackets];
				++packetIndex;
				UINT remainingSamples = c_nsamples;
				while (remainingSamples)
				{
					UINT currentPadding;
					hr = client->GetCurrentPadding(&currentPadding);
					if (FAILED(hr))
					{
						client->Stop();
						return;
					}

					UINT bufferAvailable = bufferSize - currentPadding;
					if (0 == bufferAvailable)
					{
						// wait for buffer space to become available
						WaitForSingleObject(event, 20);
						continue;
					}

					if (bufferAvailable > remainingSamples)
					{
						bufferAvailable = remainingSamples;
					}

					if (FAILED(renderClient->GetBuffer(bufferAvailable, &data)))
					{
						client->Stop();
						return;
					}

					memcpy(data, packet, bufferAvailable*2*sizeof(float));
					renderClient->ReleaseBuffer(bufferAvailable, 0);

					packet += bufferAvailable*2;
					remainingSamples -= bufferAvailable;
				}

				// release the packet back to the client thread
				ReleaseSemaphore(frameReadySem, 1, nullptr);
			}
		}

		static DWORD CALLBACK threadEntry(void* ctx)
		{
			reinterpret_cast<WasapiRenderDevice*>(ctx)->threadProc();
			return 0;
		}

		static const int c_nsamples = 2048;
		static const int c_npackets = 2;

		HANDLE event;
		float buffers[c_npackets][2*c_nsamples];
		IAudioClient* const client;
		IAudioRenderClient* const renderClient;
		uint32_t bufferIndex;
		HANDLE frameReadySem;
		HANDLE frameCompleteSem;
		HANDLE thread;
		HANDLE threadShutdown;
		const int deviceSampleRate;
		const UINT bufferSize;
	};
}

static IMMDevice* findDeviceById(EDataFlow flow, uint32_t deviceId)
{
	IMMDeviceEnumerator* e;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&e));
	if (FAILED(hr))
	{
		return nullptr;
	}

	if (!deviceId)
	{
		IMMDevice* device;
		hr = e->GetDefaultAudioEndpoint(flow, eMultimedia, &device);
		e->Release();
		if (FAILED(hr))
		{
			return nullptr;
		}

		return device;
	}

	IMMDeviceCollection* devices;
	hr = e->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &devices);
	e->Release();
	if (FAILED(hr))
	{
		return nullptr;
	}

	UINT numDevices;
	hr = devices->GetCount(&numDevices);
	if (FAILED(hr))
	{
		return nullptr;
	}

	for (UINT ii = 0; ii < numDevices; ++ii)
	{
		IMMDevice* device;
		hr = devices->Item(ii, &device);
		if (FAILED(hr))
		{
			continue;
		}

		LPWSTR id;
		hr = device->GetId(&id);
		if (FAILED(hr))
		{
			continue;
		}

		const uint32_t deviceIdHash = fnv1a(id, 2*wcslen(id));
		CoTaskMemFree(id);

		if (deviceIdHash == deviceId)
		{
			devices->Release();
			return device;
		}
	}
	devices->Release();

	return nullptr;
}

static ICaptureDevice* wasapiAcquireCaptureDevice(uint32_t deviceId)
{
	IMMDevice* device = findDeviceById(eCapture, deviceId);
	if (!device)
	{
		return nullptr;
	}

	IAudioClient* client;
	HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&client));
	device->Release();
	if (FAILED(hr))
	{
		return nullptr;
	}

	// attempt to get a suitable format
	WAVEFORMATEX format;
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	format.wBitsPerSample = 8*sizeof(float);
	format.cbSize = 0;
	
	// find a compatible frequency
	const int frequenciesToCheck[] = {48000, 44100, 16000, 96000, 32000, 8000};
	for (int ii = 0; ii < sizeof(frequenciesToCheck)/sizeof(frequenciesToCheck[0]); ++ii)
	{
		const int frequency = frequenciesToCheck[ii];
		format.nSamplesPerSec = frequency;

		for (WORD channels = 1; channels <= 2; ++channels)
		{
			format.nChannels = channels;
			format.nBlockAlign = format.nChannels*format.wBitsPerSample/8;
			format.nAvgBytesPerSec = format.nSamplesPerSec*format.nBlockAlign;

			WAVEFORMATEX* close = 0;
			hr = client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &format, &close);
			if (close)
			{
				CoTaskMemFree(close);
			}
			if (hr == S_OK)
			{
				break;
			}
		}
		if (hr == S_OK)
		{
			break;
		}
	}
	if (FAILED(hr))
	{
		client->Release();
		return nullptr;
	}

	// request 60ms buffer
	static const REFERENCE_TIME bufferTime = 60 * 10000;
	hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_NOPERSIST, bufferTime, 0, &format, NULL);
	if (FAILED(hr))
	{
		client->Release();
		return nullptr;
	}

	UINT32 bufferSize;
	hr = client->GetBufferSize(&bufferSize);
	if (FAILED(hr))
	{
		client->Release();
		return nullptr;
	}

	IAudioCaptureClient* captureClient;
	hr = client->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&captureClient));
	if (FAILED(hr))
	{
		client->Release();
		return nullptr;
	}

	const UINT framesPer10ms = format.nSamplesPerSec*10/1000;
	return new WasapiCaptureDevice(client, captureClient, format.nSamplesPerSec, framesPer10ms, static_cast<int>(format.nChannels));
}

static IRenderDevice* wasapiAcquireRenderDevice(uint32_t deviceId, int desiredSampleRate)
{
	IMMDevice* device = findDeviceById(eRender, deviceId);
	if (!device)
	{
		return nullptr;
	}

	IAudioClient* client;
	HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&client));
	device->Release();
	if (FAILED(hr))
	{
		return nullptr;
	}

	// attempt to get a suitable format
	WAVEFORMATEX format;
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	format.nChannels = 2;
	format.wBitsPerSample = 8*sizeof(float);
	format.nBlockAlign = format.nChannels*format.wBitsPerSample/8;
	format.cbSize = 0;
	format.nSamplesPerSec = desiredSampleRate;
	format.nAvgBytesPerSec = format.nSamplesPerSec*format.nBlockAlign;

	WAVEFORMATEX* close = 0;
	hr = client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &format, &close);
	if (close)
	{
		format.nSamplesPerSec = close->nSamplesPerSec;
		format.nAvgBytesPerSec = format.nSamplesPerSec*format.nBlockAlign;
		CoTaskMemFree(close);
	}
	if (FAILED(hr))
	{
		client->Release();
		return nullptr;
	}

	hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK|AUDCLNT_STREAMFLAGS_NOPERSIST, 0, 0, &format, NULL);
	if (FAILED(hr))
	{
		client->Release();
		return nullptr;
	}

	UINT32 bufferSize;
	hr = client->GetBufferSize(&bufferSize);
	if (FAILED(hr))
	{
		client->Release();
		return nullptr;
	}

	IAudioRenderClient* renderClient;
	hr = client->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&renderClient));
	if (FAILED(hr))
	{
		client->Release();
		return nullptr;
	}

	HANDLE event = CreateEvent(NULL, FALSE, FALSE, NULL);
	hr = client->SetEventHandle(event);
	if (FAILED(hr))
	{
		CloseHandle(event);
		renderClient->Release();
		client->Release();
		return nullptr;
	}

	return new WasapiRenderDevice(client, renderClient, event, static_cast<int>(format.nSamplesPerSec), bufferSize);
}

static bool wasapiEnumerateDevices(DeviceType::E type, uint32_t moduleId, IDeviceEnumeration* e)
{
	IMMDeviceEnumerator* deviceEnum;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&deviceEnum));
	if (FAILED(hr))
	{
		return false;
	}

	EDataFlow flow;
	switch (type)
	{
	case DeviceType::Render:
		flow = eRender;
		break;

	case DeviceType::Capture:
		flow = eCapture;
		break;

	default:
		return false;
	}

	IMMDeviceCollection* devices;
	hr = deviceEnum->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &devices);
	deviceEnum->Release();
	if (FAILED(hr))
	{
		return false;
	}

	UINT numDevices;
	if (FAILED(devices->GetCount(&numDevices)))
	{
		return false;
	}

	for (UINT ii = 0; ii < numDevices; ++ii)
	{
		IMMDevice* device;
		if (SUCCEEDED(devices->Item(ii, &device)))
		{
			LPWSTR id;
			if (FAILED(device->GetId(&id)))
			{
				device->Release();
				continue;
			}

			const uint32_t deviceId = fnv1a(id, 2*wcslen(id));

			IPropertyStore* props;
			if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &props)))
			{
				PROPVARIANT friendlyName;
				PropVariantInit(&friendlyName);
				if(SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &friendlyName)))
				{
					static_assert(sizeof(friendlyName.pwszVal[0]) == sizeof(int16_t), "WASAPI returning non utf-16 data");
					e->onDevice_utf16(moduleId, deviceId, reinterpret_cast<const int16_t*>(friendlyName.pwszVal), reinterpret_cast<const int16_t*>(id));
				}

				PropVariantClear(&friendlyName);
				props->Release();
			}

			CoTaskMemFree(id);
			device->Release();
		}
	}

	devices->Release();
	return true;
}


void wasapi::makeModule(DeviceModule* m)
{
	m->enumerateDevices = wasapiEnumerateDevices;
	m->acquireCaptureDevice = wasapiAcquireCaptureDevice;
	m->acquireRenderDevice = wasapiAcquireRenderDevice;
}

#else // TINY_AUDIO_ENABLE_WASAPI

void wasapi::makeModule(DeviceModule* m)
{
}

#endif // TINY_AUDIO_ENABLE_WASAPI
