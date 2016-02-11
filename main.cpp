
#include <stdio.h>
#include <OVR_CAPI_Audio.h>
#include <tiny/audio/enumutil.h>
#include <tiny/audio/render.h>
#include <tiny/platform.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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
	device->release();
}
