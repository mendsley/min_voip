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
#include <string.h>
#include "tiny/audio/resample.h"

using namespace tiny;
using namespace tiny::audio;
using namespace tiny::audio::resample;

Linear::Linear()
	: idealRate(0.0f)
{
}

Linear::Linear(int inputRate, int outputRate)
{
	reset(inputRate, outputRate);
}

void Linear::reset(int inputRate, int outputRate)
{
	idealRate = static_cast<float>(inputRate)/static_cast<float>(outputRate);
	prevSamples[0] = prevSamples[1] = 0.0f;
}

int Linear::inputSamples(int outputSamples) const
{
	return static_cast<int>(ceilf(idealRate*static_cast<float>(outputSamples)));
}

int Linear::outputSamples(int inputSamples) const
{
	return static_cast<int>(ceilf(static_cast<float>(inputSamples)/idealRate));
}

void Linear::resampleMono(const float* in, int samplesIn, float* out, int samplesOut)
{
	const float rate = static_cast<float>(samplesIn)/static_cast<float>(samplesOut);
	float* outEnd = out + samplesOut - 1;

	float pos = rate;
	while (pos < 1.0f)
	{
		out[0] = prevSamples[0] + pos*(in[0] - prevSamples[0]);
		++out;
		pos += rate;
	}

	while (out < outEnd)
	{
		float posFloor = floorf(pos);
		int index = static_cast<int>(floorf(posFloor));
		out[0] = in[index-1] + (in[index] - in[index-1]) * (pos - posFloor);

		++out;
		pos += rate;
	}

	out[0] = in[samplesIn-1];

	// copy last sample for next run
	prevSamples[0] = out[0];
}

void Linear::resampleStereoToMono(const float* in, int samplesIn, float* out, int samplesOut)
{
	const float rate = static_cast<float>(samplesIn)/static_cast<float>(samplesOut);
	float* outEnd = out + (samplesOut) - 1;

	float pos = rate;
	while (pos < 1.0f)
	{
		out[0] = prevSamples[0] + pos*(in[0] - prevSamples[0]);
		out[0] += prevSamples[1] + pos*(in[1] - prevSamples[1]);
		out[0] /= 2.0f;
	}

	while (out < outEnd)
	{
		float posFloor = floorf(pos);
		int index = 2*static_cast<int>(floorf(posFloor));
		out[0] = in[index-2] + (in[index+0] - in[index-2]) * (pos - posFloor);
		out[0] += in[index-1] + (in[index+1] - in[index-1]) * (pos - posFloor);
		out[0] /= 2.0f;

		++out;
		pos += rate;
	}

	out[0] = in[2*samplesIn-2];
	out[0] += in[2*samplesIn-1];
	out[0] /= 2.0f;

	// copy last sample for next run
	prevSamples[0] = in[2*samplesIn-2];
	prevSamples[1] = in[2*samplesIn-1];
}

void Linear::resampleMonoToStereo(const float* in, int samplesIn, float* out, int samplesOut)
{
	const float rate = static_cast<float>(samplesIn)/static_cast<float>(samplesOut);
	float* outEnd = out + (2*samplesOut) - 2;

	float pos = rate;
	while (pos < 1.0f)
	{
		out[0] = prevSamples[0] + pos*(in[0] - prevSamples[0]);
		out[1] = out[0];
		out += 2;
		pos += rate;
	}

	while (out < outEnd)
	{
		float posFloor = floorf(pos);
		int index = static_cast<int>(floorf(posFloor));
		out[0] = in[index-1] + (in[index] - in[index-1]) * (pos - posFloor);
		out[1] = out[0];

		out += 2;
		pos += rate;
	}

	out[0] = in[samplesIn-1];
	out[1] = out[0];

	// copy last sample for next run
	prevSamples[0] = out[0];
}

void Linear::resampleStereo(const float* in, int samplesIn, float* out, int samplesOut)
{
	const float rate = static_cast<float>(samplesIn)/static_cast<float>(samplesOut);
	float* outEnd = out + (2*samplesOut) - 2;

	float pos = rate;
	while (pos < 1.0f)
	{
		out[0] = prevSamples[0] + pos*(in[0] - prevSamples[0]);
		out[1] = prevSamples[1] + pos*(in[1] - prevSamples[1]);
		out += 2;
		pos += rate;
	}

	while (out < outEnd)
	{
		float posFloor = floorf(pos);
		int index = 2*static_cast<int>(posFloor);
		out[0] = in[index-2] + (in[index+0] - in[index-2]) * (pos - posFloor);
		out[1] = in[index-1] + (in[index+1] - in[index-1]) * (pos - posFloor);

		out += 2;
		pos += rate;
	}

	out[0] = in[2*samplesIn-2];
	out[1] = in[2*samplesIn-1];

	// copy last sample for next run
	prevSamples[0] = out[0];
	prevSamples[1] = out[1];
}
