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

#include <math.h>
#include "tiny/audio/capture.h"
#include "tiny/audio/sincapture.h"
#include "tiny/time.h"

using namespace tiny;
using namespace tiny::audio;

namespace
{
	static const float twoPi = 6.283185307f;

	class SinCaptureDevice : public ICaptureDevice
	{
	public:
		SinCaptureDevice(float frequency, int sampleRate)
			: deltaAngle(twoPi * frequency / static_cast<float>(sampleRate))
			, deviceSamplesPer10ms(sampleRate / 100)
			, buffer(new float[sampleRate/100])
			, outputFrequency(timestampFrequency() / 100)
		{
		}

		virtual void release()
		{
			delete this;
		}

		virtual int sampleRate() const
		{
			return deviceSamplesPer10ms * 100;
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
			angle = 0.0f;
			nextOutputTime = timestampCurrent();
			return true;
		}

		virtual void stop()
		{
		}

		virtual const float* get10msOfSamples()
		{
			const uint64_t now = timestampCurrent();
			if (now < nextOutputTime)
			{
				return nullptr;
			}

			float localAngle = angle;
			for (int ii = 0, nn = deviceSamplesPer10ms; ii != nn; ++ii, localAngle += deltaAngle)
			{
				if (localAngle > twoPi)
				{
					localAngle -= twoPi;
				}

				buffer[ii] = sinf(localAngle);
			}

			angle = localAngle;
			nextOutputTime += outputFrequency;
			return buffer;
		}

	private:
		SinCaptureDevice(const SinCaptureDevice&); // = delete
		SinCaptureDevice& operator=(const SinCaptureDevice&); // = delete

		float* const buffer;
		float angle;
		uint64_t nextOutputTime;
		
		const float deltaAngle;
		const int deviceSamplesPer10ms;
		const uint64_t outputFrequency;
	};
}

ICaptureDevice* audio::createSinCaptureDevice(float frequency, int sampleRate)
{
	return new SinCaptureDevice(frequency, sampleRate);
}
