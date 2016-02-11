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

#include <algorithm>
#include "tiny/audio/capture.h"
#include "tiny/audio/enum.h"
#include "tiny/audio/enumutil.h"
#include "tiny/audio/render.h"

using namespace tiny;
using namespace tiny::audio;

namespace
{
	static inline size_t utf16_len(const int16_t* str)
	{
		size_t len = 0;
		while (*str)
		{
			++str, ++len;
		}
		return len;
	}

	class ByNameEnumerator : public IDeviceEnumeration
	{
	public:
		ByNameEnumerator(const int16_t* query, size_t nquery)
			: moduleId(0)
			, deviceId(0)
			, query(query)
			, nquery(nquery)
		{
		}

		virtual void onDevice_utf16(uint32_t moduleId, uint32_t deviceId, const int16_t* description, const int16_t* hardwareIdentifier)
		{
			const size_t len = utf16_len(description);
			const int16_t* descriptionEnd = description+len;
			const int16_t* found = std::search(description, descriptionEnd, query, query+nquery);
			if (found != descriptionEnd)
			{
				this->moduleId = moduleId;
				this->deviceId = deviceId;
			}
		}

		uint32_t moduleId;
		uint32_t deviceId;
	private:
		ByNameEnumerator(const ByNameEnumerator&); // = delete
		ByNameEnumerator& operator=(const ByNameEnumerator&); // = delete

		const int16_t* const query;
		const size_t nquery;
	};

	class ByIdEnumerator : public IDeviceEnumeration
	{
	public:
		ByIdEnumerator(const int16_t* query, size_t nquery)
			: moduleId(0)
			, deviceId(0)
			, query(query)
			, nquery(nquery)
		{
		}

		virtual void onDevice_utf16(uint32_t moduleId, uint32_t deviceId, const int16_t* description, const int16_t* hardwareIdentifier)
		{
			const size_t len = utf16_len(hardwareIdentifier);
			if (len == nquery)
			{
				if (0 == memcmp(hardwareIdentifier, query, len))
				{
					this->moduleId = moduleId;
					this->deviceId = deviceId;
				}
			}
		}

		uint32_t moduleId;
		uint32_t deviceId;
	private:
		ByIdEnumerator(const ByIdEnumerator&); // = delete
		ByIdEnumerator& operator=(const ByIdEnumerator&); // = delete

		const int16_t* const query;
		const size_t nquery;
	};
}

IRenderDevice* audio::findRenderDeviceBySubstring_utf16(const int16_t* substr, int sampleRate)
{
	ByNameEnumerator e(substr, utf16_len(substr));
	enumerateRenderDevices(&e);
	if (e.moduleId && e.deviceId)
	{
		return acquireRenderDevice(e.moduleId, e.deviceId, sampleRate);
	}

	return nullptr;
}

IRenderDevice* audio::findRenderDeviceById_utf16(const int16_t* identifier, int sampleRate)
{
	ByIdEnumerator e(identifier, utf16_len(identifier));
	enumerateRenderDevices(&e);
	if (e.moduleId && e.deviceId)
	{
		return acquireRenderDevice(e.moduleId, e.deviceId, sampleRate);
	}

	return nullptr;
}

ICaptureDevice* audio::findCaptureDeviceBySubstring_utf16(const int16_t* substr)
{
	ByNameEnumerator e(substr, utf16_len(substr));
	enumerateRenderDevices(&e);
	if (e.moduleId && e.deviceId)
	{
		return acquireCaptureDevice(e.moduleId, e.deviceId);
	}

	return nullptr;
}

ICaptureDevice* audio::findCaptureDeviceById_utf16(const int16_t* identifier)
{
	ByIdEnumerator e(identifier, utf16_len(identifier));
	enumerateCapureDevices(&e);
	if (e.moduleId && e.deviceId)
	{
		return acquireCaptureDevice(e.moduleId, e.deviceId);
	}

	return nullptr;
}
