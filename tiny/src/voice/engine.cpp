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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <opus.h>
#include <webrtc/modules/audio_processing/include/audio_processing.h>
#include "tiny/audio/capture.h"
#include "tiny/audio/resample.h"
#include "tiny/endian.h"
#include "tiny/voice/engine.h"
#include "tiny/voice/source.h"

using namespace tiny;
using namespace tiny::audio;
using namespace tiny::voice;

Engine::Engine(ICaptureDevice* mic, uint32_t sampleRate)
	: mic(mic)
	, encoder(nullptr)
	, outgoingProcessor(nullptr)
	, samplesPer10ms(mic ? mic->samplesPer10ms() : 0)
	, outputSampleRate(sampleRate)
	, outgoingSequence(0)
	, micSampleRate(mic ? mic->sampleRate() : 0)
{
	if (mic)
	{
		assert(mic->channels() == 1);
		int requiredSpce = opus_encoder_get_size(1);
		if (requiredSpce > sizeof(encoderSpace))
		{
			abort();
		}

		OpusEncoder* encoder = reinterpret_cast<OpusEncoder*>(&encoderSpace);
		if (OPUS_OK == opus_encoder_init(encoder, 48000, 1, OPUS_APPLICATION_VOIP))
		{
			webrtc::AudioProcessing* processor = webrtc::AudioProcessing::Create();
	
			processor->high_pass_filter()->Enable(true);
	
			processor->echo_cancellation()->enable_drift_compensation(false);
			processor->echo_cancellation()->Enable(false /*true*/);

			processor->noise_suppression()->set_level(webrtc::NoiseSuppression::kHigh);
			processor->noise_suppression()->Enable(true);

			processor->gain_control()->set_analog_level_limits(0, 255);
			processor->gain_control()->set_mode(webrtc::GainControl::kAdaptiveAnalog);
			processor->gain_control()->Enable(true);

			processor->voice_detection()->Enable(true);

			this->encoder = encoder;
			this->outgoingProcessor = processor;
		}
	}
}

Engine::~Engine()
{
	if (outgoingProcessor)
	{
		delete outgoingProcessor;
	}
}

void Engine::addSource(Source* s)
{
	s->reset(outputSampleRate);
}

void Engine::removeSource(Source* /*s*/)
{
}

uint32_t Engine::generatePacket(uint8_t* packet, uint32_t npacket)
{
	if (!mic || !outgoingProcessor || !encoder)
		return 0;

	if (npacket < 4)
		return 0;

	uint32_t sequence = endianToLittle(outgoingSequence);
	memcpy(packet, &sequence, sizeof(sequence));
	packet += sizeof(sequence);
	npacket -= sizeof(sequence);

	uint32_t packetWritten = sizeof(uint32_t);
	uint32_t packetsGenerated = 0;
	while (npacket > 200)
	{
		// do we have 10ms of data to encode?
		const float* incoming = mic->get10msOfSamples();
		if (!incoming)
			break;

		// process the audio
		float* out[1] = {monoBuffer};
		if (webrtc::AudioProcessing::kNoError == outgoingProcessor->ProcessStream(&incoming, webrtc::StreamConfig(micSampleRate, 1, false), webrtc::StreamConfig(48000, 1, false), out))
		{
			if (outgoingProcessor->voice_detection()->stream_has_voice())
			{
				// encode the audio
				int32_t packetData = opus_encode_float(encoder, monoBuffer, c_monoSamples, packet+2, npacket-2);
				if (packetData < 1) // no need to transmit this data
				{
					break;
				}

				uint16_t encodedPacketData = endianToLittle(static_cast<uint16_t>(packetData));
				memcpy(packet, &encodedPacketData, sizeof(encodedPacketData));

				packet += sizeof(encodedPacketData);
				npacket -= sizeof(encodedPacketData);
				packetWritten += sizeof(encodedPacketData);

				npacket -= packetData;
				packet += packetData;
				packetWritten += packetData;
				++packetsGenerated;
			}
		}
	}

	outgoingSequence += packetsGenerated;

	// did we actually write any audio data?
	if (packetWritten == sizeof(uint32_t))
	{
		return 0;
	}

	return packetWritten;
}

void Engine::processPacket(Source* s, const uint8_t* packet, uint32_t npacket)
{
	if (npacket < 6)
		return;

	uint32_t incomingSequence;
	memcpy(&incomingSequence, packet, sizeof(incomingSequence));
	incomingSequence = endianFromLittle(incomingSequence);
	packet += sizeof(incomingSequence);
	npacket -= sizeof(incomingSequence);

	// first packet? reset incoming sequence
	if (s->incomingSequence == 0)
	{
		s->incomingSequence = incomingSequence;
	}

	// notify opus of missing audio packets
	if (s->valid())
	{
		for (uint32_t ii = s->incomingSequence; ii < incomingSequence; ++ii)
		{
			const int nsamples = opus_decode_float(s->decoder.p, nullptr, 0, monoBuffer, c_monoSamples, 0);
			if (nsamples > 0)
			{
				const uint32_t outputSamples = s->outputResampler.outputSamples(nsamples);
				const size_t existingSize = s->incomingData.size();
				s->incomingData.reserve(existingSize + outputSamples);
				s->outputResampler.resampleMono(monoBuffer, nsamples, s->incomingData.data() + existingSize, outputSamples);
			}
		}
	}

	// process incoming data
	while (npacket)
	{
		if (npacket < 3)
		{
			break;
		}

		uint16_t audioPacketSize;
		memcpy(&audioPacketSize, packet, sizeof(audioPacketSize));
		audioPacketSize = endianFromLittle(audioPacketSize);
		packet += sizeof(audioPacketSize);
		npacket -= sizeof(audioPacketSize);

		if (s->valid() && incomingSequence >= s->incomingSequence)
		{
			const int nsamples = opus_decode_float(s->decoder.p, packet, audioPacketSize, monoBuffer, c_monoSamples, 0);
			if (nsamples > 0)
			{
				const uint32_t outputSamples = s->outputResampler.outputSamples(nsamples);
				const size_t existingSize = s->incomingData.size();
				s->incomingData.resize(existingSize + outputSamples);
				s->outputResampler.resampleMono(monoBuffer, nsamples, s->incomingData.data() + existingSize, outputSamples);
			}
		}
		
		++incomingSequence;
		packet += audioPacketSize;
		npacket -= audioPacketSize;
	}

	s->incomingSequence = incomingSequence;
}
