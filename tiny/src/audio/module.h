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

#ifndef TINY_SRC_AUDIO__MODULE_H
#define TINY_SRC_AUDIO__MODULE_H

#include <stdint.h>
#include "audio/types.h"

namespace tiny
{
	namespace audio
	{
		class ICaptureDevice;
		class IRenderDevice;
		struct IDeviceEnumeration;

		// services provided by individual device modules
		struct DeviceModule
		{
			bool (*enumerateDevices)(DeviceType::E type, uint32_t moduleId, IDeviceEnumeration* e);
			ICaptureDevice* (*acquireCaptureDevice)(uint32_t deviceId);
			IRenderDevice* (*acquireRenderDevice)(uint32_t deviceId, int desiredSampleRate);
		};

		int modulesCount();
		const DeviceModule* modules();
		static inline bool moduleIdValid(uint32_t m) { return (m > 0 && static_cast<int>(m - 1) < modulesCount()); }
		static inline uint32_t moduleIndexToId(int index) { return static_cast<uint32_t>(index) + 1; }
		static inline const DeviceModule* moduleFromId(uint32_t m) { if (!moduleIdValid(m)) return nullptr; return &(modules()[m-1]); }
	}
}

#endif // TINY_SRC_AUDIO__MODULE_H
