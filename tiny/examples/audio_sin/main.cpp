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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <tiny/platform.h>
#include <tiny/audio/enum.h>
#include <tiny/audio/render.h>
#include <tiny/audio/resample.h>

using namespace tiny;
using namespace tiny::audio;

int main()
{
	if (!platformStartup())
	{
		return -1;
	}

	int sampleRate = 48000;
	IRenderDevice* render = acquireDefaultRenderDevice(sampleRate);
	if (!render)
	{
		return -1;
	}

	resample::Linear resampler(sampleRate, render->sampleRate());
	std::vector<float> scratch;

	static const float frequency = 440.0f;
	static const float twoPi = 6.283185307f;
	const float deltaAngle = twoPi * frequency / sampleRate;

	if (render->start())
	{
		float angle = 0.0f;

		time_t end = time(NULL) + 10;
		while (time(NULL) < end)
		{
			float* samples;
			int nsamples = render->acquireBuffer(&samples);

			const int nrawsamples = resampler.inputSamples(nsamples);
			scratch.resize(nrawsamples);
			float* rawSamples = scratch.data();

			for (int remaining = nrawsamples; remaining; --remaining, rawSamples += 1, angle += deltaAngle)
			{
				// wrapte angle to [0.0, 2pi]
				if (angle > twoPi)
					angle -= twoPi;

				const float sinAngle = sinf(angle);
				rawSamples[0] = sinAngle;
			}

			resampler.resampleMonoToStereo(scratch.data(), nrawsamples, samples, nsamples);
			render->commitBuffer();
		}

		render->stop();
	}
	render->release();

	platformShutdown();
}
