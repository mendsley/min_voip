
#include <stdio.h>
#include <OVR_CAPI_Audio.h>
#include <tiny/audio/enumutil.h>
#include <tiny/audio/render.h>
#include <tiny/platform.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void output_sin440_10sec(tiny::audio::IRenderDevice* device, int sourceSampleRate);

int main()
{
	if (!tiny::platformStartup())
	{
		printf("Failed to start platform\n");
		return -1;
	}

	static const int sampleRate = 48000;
	tiny::audio::IRenderDevice* device = nullptr;

	WCHAR deviceName[OVR_AUDIO_MAX_DEVICE_STR_SIZE];
	auto res = ovr_GetAudioDeviceOutGuidStr(deviceName);
	if (OVR_SUCCESS(res))
	{
		printf("Searching for OVR audio device %S\n", deviceName);
		device = tiny::audio::findRenderDeviceById_utf16(reinterpret_cast<const int16_t*>(deviceName), sampleRate);
		if (!device)
		{
			printf("Failed to find OVR device %S falling back to default audio device\n", device);
		}
	}
	else
	{
		ovrErrorInfo err;
		ovr_GetLastErrorInfo(&err);
		printf("Failed to query OVR audio device: %d %s\n", err.Result, err.ErrorString);
	}

	// fallback to default device
	if (!device)
	{
		device = tiny::audio::acquireDefaultRenderDevice(sampleRate);
	}

	if (!device)
	{
		printf("Failed to acquire audio device\n");
		return -1;
	}

	printf("Audio device [%p]\n", device);
	output_sin440_10sec(device, sampleRate);
	device->release();
}

// from tiny/examples/audio_sin
#include <vector>
#include <time.h>
#include <tiny/audio/resample.h>
void output_sin440_10sec(tiny::audio::IRenderDevice* device, int sourceSampleRate)
{
	using namespace tiny::audio;

	resample::Linear resampler(sourceSampleRate, device->sampleRate());
	std::vector<float> scratch;

	static const float frequency = 440.0f;
	static const float twoPi = 6.283185307f;
	const float deltaAngle = twoPi * frequency / sourceSampleRate;

	if (device->start())
	{
		float angle = 0.0f;

		time_t end = time(NULL) + 10;
		while (time(NULL) < end)
		{
			float* samples;
			int nsamples = device->acquireBuffer(&samples);

			const int nrawsamples = resampler.inputSamples(nsamples);
			scratch.resize(nrawsamples);
			float* rawSamples = scratch.data();

			for (int remaining = nrawsamples; remaining; --remaining, rawSamples += 1, angle += deltaAngle)
			{
				// wrap angle to [0.0, 2pi]
				if (angle > twoPi)
					angle -= twoPi;

				const float sinAngle = sinf(angle);
				rawSamples[0] = sinAngle;
			}

			resampler.resampleMonoToStereo(scratch.data(), nrawsamples, samples, nsamples);
			device->commitBuffer();
		}

		device->stop();
	}
}
