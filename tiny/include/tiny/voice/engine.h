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

#ifndef TINY_VOICE__ENGINE_H
#define TINY_VOICE__ENGINE_H

#include <stdint.h>

struct OpusEncoder;

namespace webrtc { class AudioProcessing; }

namespace tiny
{
	namespace audio { class ICaptureDevice; }

	namespace voice
	{
		class Source;

		class Engine
		{
		public:

			explicit Engine(audio::ICaptureDevice* microphone, uint32_t sampleRate = 48000);
			~Engine();

			void addSource(Source* s);
			void removeSource(Source* s);

			uint32_t generatePacket(uint8_t* packet, uint32_t npacket);
			void processPacket(Source* s, const uint8_t* packet, uint32_t npacket);

		private:
			Engine(const Engine&); // = delete
			Engine& operator=(const Engine&); // = delete

			static const int c_monoSamples = 480; // 10ms @ 48khz

			audio::ICaptureDevice* const mic;
			OpusEncoder* encoder;
			webrtc::AudioProcessing* outgoingProcessor;
			const uint32_t samplesPer10ms;
			const uint32_t outputSampleRate;
			uint32_t outgoingSequence;
			const int micSampleRate;
			float monoBuffer[c_monoSamples];

			union
			{
				uint8_t block[47888];
				void* align;
			} encoderSpace;
		};
	}
}

#endif // TINY_VOICE__ENGINE_H
