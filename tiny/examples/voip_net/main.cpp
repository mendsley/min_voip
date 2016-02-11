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
#include <mutex>
#include <thread>
#include <vector>
#include <tiny/audio/capture.h>
#include <tiny/audio/render.h>
#include <tiny/audio/resample.h>
#include <tiny/platform.h>
#include <tiny/sleep.h>
#include <tiny/time.h>
#include <tiny/voice/engine.h>
#include <tiny/voice/source.h>

using namespace tiny;

static const int c_sampleRate = 48000;

static std::vector<float> g_chatSamples;
static std::mutex g_chatSamplesLock;

static void mixVoice(float* samples, int nsamples)
{
	std::unique_lock<std::mutex> l(g_chatSamplesLock);

	int samplesAvailable = static_cast<int>(g_chatSamples.size());
	if (samplesAvailable == 0)
		return;

	int samplesToRead = nsamples;
	if (samplesToRead > samplesAvailable)
	{
		samplesToRead = samplesAvailable;
	}

	float* stereo = samples;
	const float* chatSamples = g_chatSamples.data();
	for (int ii = 0; ii != samplesToRead; ++ii)
	{
		stereo[0] += chatSamples[ii];
		stereo[1] += chatSamples[ii];
		stereo += 2;
	}

	g_chatSamples.erase(g_chatSamples.begin(), g_chatSamples.begin() + samplesToRead);

}

static void renderThread()
{
	platformStartup();

	audio::resample::Linear resampler;
	std::vector<float> mixerbuffer;
	audio::IRenderDevice* device = nullptr;
	for (;;)
	{
		if (!device)
		{
			device = audio::acquireDefaultRenderDevice(c_sampleRate);
			if (!device || !device->start())
			{
				return;
			}
			resampler.reset(c_sampleRate, device->sampleRate());
		}

		float* samples;
		int nsamples = device->acquireBuffer(&samples);
		if (nsamples < 0)
		{
			device->release();
			device = nullptr;
			continue;
		}

		const int nmixersamples = resampler.inputSamples(nsamples);
		mixerbuffer.resize(2*nmixersamples);

		mixerbuffer.assign(mixerbuffer.size(), 0.0f);
		mixVoice(mixerbuffer.data(), nmixersamples);

		resampler.resampleStereo(mixerbuffer.data(), nmixersamples, samples, nsamples);
		device->commitBuffer();
	}
}

#include <tiny/audio/sincapture.h>

int main()
{
	platformStartup();

	std::thread(renderThread).detach();

	audio::ICaptureDevice* microphone = audio::acquireDefaultCaptureDevice();
	if (!microphone || !microphone->start())
		return -1;

	voice::Engine engine(microphone, c_sampleRate);
	voice::Source source;
	engine.addSource(&source);

	uint8_t voicePacket[1200];
	for (;;)
	{
		uint32_t nvoicePacket = engine.generatePacket(voicePacket, sizeof(voicePacket));
		if (nvoicePacket)
		{
			engine.processPacket(&source, voicePacket, nvoicePacket);
		}

		std::vector<float> data = source.takeAllSourceAudio();
		{
			std::unique_lock<std::mutex> l(g_chatSamplesLock);
			g_chatSamples.insert(g_chatSamples.end(), data.data(), data.data() + data.size());
		}

		sleep(16);
	}
}
