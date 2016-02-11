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

#include <tiny/platform.h>
#include <tiny/audio/capture.h>
#include <tiny/audio/render.h>
#include <tiny/voice/engine.h>
#include <tiny/voice/source.h>

#include <stdio.h>
#include <time.h>

using namespace tiny;
using namespace tiny::audio;

int main()
{
	if (!platformStartup())
		return -1;

	ICaptureDevice* mic = acquireDefaultCaptureDevice();
	if (!mic)
		return -1;

	IRenderDevice* speaker = acquireDefaultRenderDevice(48000);
	if (!speaker)
		return -1;

	voice::Engine engine(mic, speaker->sampleRate());
	
	if (!speaker->start() || !mic->start())
	{
		return -1;
	}

	voice::Source vsource;
	engine.addSource(&vsource);

	uint8_t packet[4000];
	const time_t end = time(nullptr)+10;
	while (time(nullptr) < end)
	{
		for (;;)
		{
			uint32_t npacket = engine.generatePacket(packet, sizeof(packet));
			if (npacket)
			{
				engine.processPacket(&vsource, packet, npacket);
			}
			else
			{
				break;
			}
		}

		float* buffer;
		const uint32_t nsamples = speaker->acquireBuffer(&buffer);

		const float* samples;
		uint32_t samplesAvailable = vsource.getSourceAudio(&samples);
		if (samplesAvailable > nsamples)
		{
			samplesAvailable = nsamples;
		}

		for (uint32_t ii = 0; ii < nsamples; ++ii)
		{
			if (ii < samplesAvailable)
			{
				buffer[2*ii] = samples[ii];
			}
			else
			{
				buffer[2*ii] = 0.0f;
			}
			buffer[2*ii+1] = buffer[2*ii];
		}
		vsource.consumeSourceAudio(samplesAvailable);

		speaker->commitBuffer();
	}

	engine.removeSource(&vsource);
	speaker->release();
	mic->release();
}
