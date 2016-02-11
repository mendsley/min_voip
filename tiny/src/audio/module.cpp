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

#include "audio/module.h"
#include "platform/windows/wasapi/wasapi.h"
#include "platform/windows/xaudio2/xaudio2.h"

using namespace tiny;
using namespace audio;

static bool nullModule_enumerateDevice(DeviceType::E /*type*/, uint32_t /*moduleId*/, IDeviceEnumeration* /*e*/)
{
	return false;
}

static ICaptureDevice* nullModule_acquireCaptureDevice(uint32_t /*deviceId*/)
{
	return nullptr;
}

static IRenderDevice* nullModule_acqureRenderDevice(uint32_t /*deviceId*/, int /*desiredSampleRate*/)
{
	return nullptr;
}

static const DeviceModule nullModule = {
	nullModule_enumerateDevice,
	nullModule_acquireCaptureDevice,
	nullModule_acqureRenderDevice,
};

static DeviceModule g_modules[] = {
	nullModule,
	nullModule,
};

namespace
{
	struct Init
	{
		Init()
		{
			wasapi::makeModule(&g_modules[0]);
			xaudio2::makeModule(&g_modules[1]);
		}
	};

	static Init initModules;
}

int audio::modulesCount()
{
	return sizeof(g_modules)/sizeof(g_modules[0]);
}

const DeviceModule* audio::modules()
{
	return g_modules;
}
