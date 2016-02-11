/**
 * Copyright 2011-2015 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
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

#ifndef TINY_VOICE__SOURCE_H
#define TINY_VOICE__SOURCE_H

#include <stdint.h>
#include <vector>
#include <tiny/audio/resample.h>

struct OpusDecoder;

namespace tiny
{
	namespace voice
	{
		class Engine;

		// Voice engine source
		class Source
		{
			friend Engine;
		public:
			
			bool valid();
			void reset(uint32_t sampleRate);

			void swap(Source& other);

			uint32_t getSourceAudio(const float** monoSamples);
			void consumeSourceAudio(uint32_t samples);

			std::vector<float> takeAllSourceAudio();

		private:
			std::vector<float> incomingData;
			audio::resample::Linear outputResampler;
			uint32_t incomingSequence;
			
			struct Decoder
			{
				Decoder();
				Decoder(const Decoder& other);
				Decoder& operator=(const Decoder& other);

				OpusDecoder* p;

				void reset();

				union
				{
					uint8_t block[18220];
					void* align;
				} space;
			};

			Decoder decoder;
		};
	}
}

#endif // TINY_VOICE__SOURCE_H
