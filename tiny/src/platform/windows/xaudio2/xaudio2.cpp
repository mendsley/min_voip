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
#include "platform/windows/xaudio2/xaudio2.h"

using namespace tiny;
using namespace tiny::audio;

#if TINY_AUDIO_ENABLE_XAUDIO2

#if !defined(TINY_AUDIO_XAUDIO2_USE_DYNAMIC_LOADING)
#	if !defined(_XBOX) && !defined(_DURANGO)
#		define TINY_AUDIO_XAUDIO2_USE_DYNAMIC_LOADING 1
#	else
#		define TINY_AUDIO_XAUDIO2_USE_DYNAMIC_LOADING 0
#	endif
#endif // ... TINYAUDIO_XAUDIO2_USE_DYNAMIC_LOADING

#define WIN32_LEAN_AND_MEAN
#include <XAudio2.h>
#include <Windows.h>
#include <stdint.h>
#include <memory>
#include <utility>
#include "audio/module.h"
#include "audio/types.h"
#include "tiny/audio/enum.h"
#include "tiny/audio/render.h"

namespace
{
	// Helper class to acquire an IXAudio2 device
	class Context
	{
	public:
		Context()
			: device(nullptr)
			, library(nullptr)
		{
		}

		Context(Context&& other)
			: device(other.device)
			, library(other.library)
		{
			other.device = nullptr;
			other.library = nullptr;
		}

		~Context()
		{
			if (device) device->Release();
			if (library) FreeLibrary(library);
		}

		bool load();

		IXAudio2* device;

	private:
		Context(const Context&); // = delete
		Context& operator=(const Context&); // = delete

		HMODULE library;
	};

	class XAudio2RenderDevice : public IRenderDevice, private IXAudio2VoiceCallback
	{
	public:
		XAudio2RenderDevice(Context&& ctx, IXAudio2MasteringVoice* masterVoice, int deviceSampleRate);

		bool createSourceVoice();

		virtual void release();

		virtual int sampleRate() const;

		virtual bool start();
		virtual void stop();

		virtual int acquireBuffer(float** buffer);
		virtual void commitBuffer();
		virtual void discardBuffer();

	private:
		virtual ~XAudio2RenderDevice();

		virtual void CALLBACK OnBufferEnd(void* context);

		virtual void CALLBACK OnBufferStart(void*) {}
		virtual void CALLBACK OnLoopEnd(void*) {}
		virtual void CALLBACK OnStreamEnd() {}
		virtual void CALLBACK OnVoiceError(void*, HRESULT) {}
		virtual void CALLBACK OnVoiceProcessingPassEnd() {}
		virtual void CALLBACK OnVoiceProcessingPassStart(UINT32) {}

		static const int c_npackets = 2;
		static const int c_nsamples = 2048;

		IXAudio2SourceVoice* sourceVoice;
		float sampleBuffer[c_npackets][c_nsamples*2];
		HANDLE bufferSema;
		uint32_t bufferIndex;

		const Context context;
		IXAudio2MasteringVoice* const masterVoice;
		const int deviceSampleRate;
	};
}

XAudio2RenderDevice::XAudio2RenderDevice(Context&& ctx, IXAudio2MasteringVoice* masterVoice, int sampleRate)
	: context(std::move(ctx))
	, masterVoice(masterVoice)
	, deviceSampleRate(sampleRate)
	, bufferSema(nullptr)
{
}

XAudio2RenderDevice::~XAudio2RenderDevice()
{
	sourceVoice->Stop();
	sourceVoice->DestroyVoice();
	masterVoice->DestroyVoice();

	if (bufferSema)
	{
		CloseHandle(bufferSema);
	}
}

bool XAudio2RenderDevice::createSourceVoice()
{
	WAVEFORMATEX mixFormat;
	memset(&mixFormat, 0, sizeof(mixFormat));
	mixFormat.nChannels = 2;
	mixFormat.wBitsPerSample = sizeof(float)*8;
	mixFormat.nSamplesPerSec = deviceSampleRate;
	mixFormat.nBlockAlign = mixFormat.wBitsPerSample*mixFormat.nChannels/8;
	mixFormat.nAvgBytesPerSec = mixFormat.nSamplesPerSec*mixFormat.nBlockAlign;
	mixFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;

	// create source voice
	static const UINT32 mixFlags = 0U;
	if (FAILED(context.device->CreateSourceVoice(&sourceVoice, &mixFormat, mixFlags, 1.0f, this, nullptr, nullptr)))
	{
		return false;
	}

	// reset buffer semaphore
	if (bufferSema)
	{
		CloseHandle(bufferSema);
	}
	bufferSema = CreateSemaphore(nullptr, c_npackets, c_npackets, nullptr);

	bufferIndex = 0;
	sourceVoice->Discontinuity();
	return true;
}

void CALLBACK XAudio2RenderDevice::OnBufferEnd(void* /*context*/)
{
	ReleaseSemaphore(bufferSema, 1, nullptr);
}

void XAudio2RenderDevice::release()
{
	delete this;
}

int XAudio2RenderDevice::sampleRate() const
{
	return deviceSampleRate;
}

bool XAudio2RenderDevice::start()
{
	if (FAILED(sourceVoice->Start()))
	{
		return false;
	}

	return true;
}

void XAudio2RenderDevice::stop()
{
	sourceVoice->Stop();
}

int XAudio2RenderDevice::acquireBuffer(float** buffer)
{
	WaitForSingleObject(bufferSema, INFINITE);
	*buffer = sampleBuffer[bufferIndex % c_npackets];
	return c_nsamples;
}

void XAudio2RenderDevice::commitBuffer()
{
	XAUDIO2_BUFFER packet;
	memset(&packet, 0, sizeof(packet));
	packet.AudioBytes = sizeof(sampleBuffer[bufferIndex % c_npackets]);
	packet.pAudioData = reinterpret_cast<BYTE*>(sampleBuffer[bufferIndex % c_npackets]);
	++bufferIndex;

	sourceVoice->SubmitSourceBuffer(&packet);
}

void XAudio2RenderDevice::discardBuffer()
{
	ReleaseSemaphore(bufferSema, 1, nullptr);
}

#if TINY_AUDIO_XAUDIO2_USE_DYNAMIC_LOADING
// XAudio2.7 guids
static const GUID c_CLSID_XAudio2 = { 0x5a508685, 0xa254, 0x4fba, {0x9b, 0x82, 0x9a, 0x24, 0xb0, 0x03, 0x06, 0xaf} };
static const GUID c_IID_IXaudio2 = { 0x8bcf1f58, 0x9fe7, 0x4583, {0x8a, 0xc6, 0xe2, 0xad, 0xc4, 0x65, 0xc8, 0xbb} };

// Create an IXAudio interface
bool Context::load()
{
	if (device || library)
	{
		return false;
	}

	HMODULE lib = LoadLibraryA("XAudio2_7.dll");
	if (!lib)
	{
		lib = LoadLibraryA("XAudio2_dist.dll");
		if (!lib)
		{
			return false;
		}
	}

	typedef HRESULT (STDMETHODCALLTYPE *DllGetClassObjectFunc)(const IID&, const IID&, LPVOID*);
	DllGetClassObjectFunc dllGetClassObject = reinterpret_cast<DllGetClassObjectFunc>(GetProcAddress(lib, "DllGetClassObject"));
	if (!dllGetClassObject)
	{
		FreeLibrary(lib);
		return false;
	}

	IClassFactory* factory;
	HRESULT hr = dllGetClassObject(c_CLSID_XAudio2, IID_IClassFactory, reinterpret_cast<void**>(&factory));
	if (FAILED(hr))
	{
		FreeLibrary(lib);
		return false;
	}

	IXAudio2* device;
	hr = factory->CreateInstance(nullptr, c_IID_IXaudio2, reinterpret_cast<void**>(&device));
	if (FAILED(hr))
	{
		FreeLibrary(lib);
		return false;
	}

	hr = device->Initialize(0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		device->Release();
		FreeLibrary(lib);
		return false;
	}

	this->library = lib;
	this->device = device;
	return true;
}
#else // TINY_AUDIO_XAUDIO2_USE_DYNAMIC_LOADING

bool Context::load()
{
	if (device)
	{
		return false;
	}

	IXAudio2* device;
	HRESULT hr = XAudio2Create(&device, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		return false;
	}

	this->device = device;
	return true;
}

#endif // TINY_AUDIO_XAUDIO2_USE_DYNAMIC_LOADING

static bool xaudio2EnumerateDevices(DeviceType::E type, uint32_t moduleId, IDeviceEnumeration* e)
{
	// XAudio2 does not support audio capture
	if (type == DeviceType::Capture)
	{
		return false;
	}

	Context ctx;
	if (!ctx.load())
	{
		return false;
	}

	UINT32 numDevices;
	if (FAILED(ctx.device->GetDeviceCount(&numDevices)))
	{
		return false;
	}

	for (UINT32 ii = 0; ii < numDevices; ++ii)
	{
		XAUDIO2_DEVICE_DETAILS details;
		if (SUCCEEDED(ctx.device->GetDeviceDetails(ii, &details)))
		{
			static_assert(sizeof(details.DisplayName[0]) == sizeof(int16_t), "XAudio2 is returning non utf16 data");
			static_assert(sizeof(details.DeviceID[0]) == sizeof(int16_t), "XAudio2 is returning non utf16 data");
			e->onDevice_utf16(moduleId, ii+ii, reinterpret_cast<const int16_t*>(details.DisplayName), reinterpret_cast<const int16_t*>(details.DeviceID));
		}
	}

	return true;
}

static IRenderDevice* xaudio2AcquireRenderDevice(uint32_t deviceId, int /*desiredSampleRate*/)
{
	Context ctx;
	if (!ctx.load())
	{
		return nullptr;
	}

	// find the appropriate device index
	if (deviceId == 0)
	{
		UINT32 numDevices;
		if (FAILED(ctx.device->GetDeviceCount(&numDevices)))
		{
			return nullptr;
		}

		for (UINT32 ii = 0; ii < numDevices; ++ii)
		{
			XAUDIO2_DEVICE_DETAILS details;
			if (SUCCEEDED(ctx.device->GetDeviceDetails(ii, &details)))
			{
				if ((details.Role & GlobalDefaultDevice) != 0)
				{
					deviceId = ii+1;
					break;
				}
			}
		}

		if (deviceId == 0)
		{
			return nullptr;
		}
	}

	// convert device id to index
	--deviceId;

	XAUDIO2_DEVICE_DETAILS deviceDetails;
	if (FAILED(ctx.device->GetDeviceDetails(deviceId, &deviceDetails)))
	{
		return nullptr;
	}

	// create mastering voice
	IXAudio2MasteringVoice* masterVoice;
	if (FAILED(ctx.device->CreateMasteringVoice(&masterVoice, 2, deviceDetails.OutputFormat.Format.nSamplesPerSec, 0, deviceId, nullptr)))
	{
		return nullptr;
	}

	XAudio2RenderDevice* device = new XAudio2RenderDevice(std::move(ctx), masterVoice, deviceDetails.OutputFormat.Format.nSamplesPerSec);
	if (!device->createSourceVoice())
	{
		device->release();
		return nullptr;
	}

	return device;
}

void xaudio2::makeModule(DeviceModule* m)
{
	m->enumerateDevices = xaudio2EnumerateDevices;
	m->acquireRenderDevice = xaudio2AcquireRenderDevice;
}

#else // TINY_AUDIO_ENABLE_XAUDIO2

void xaudio2::makeModule(DeviceModule* m)
{
}

#endif // TINY_AUDIO_ENABLE_XAUDIO2
