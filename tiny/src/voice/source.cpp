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

#include <stdlib.h>
#include <string.h>
#include <opus.h>
#include "tiny/voice/source.h"

using namespace tiny;
using namespace tiny::voice;

Source::Decoder::Decoder()
	: p(nullptr)
{
	int requiredSpace = opus_decoder_get_size(1);
	if (requiredSpace > sizeof(space))
	{
		abort();
	}
}

Source::Decoder::Decoder(const Decoder& other)
{
	if (other.p)
	{
		p = reinterpret_cast<OpusDecoder*>(&space);
		memcpy(&space, &other.space, sizeof(space));
	}
	else
	{
		p = nullptr;
	}
}

Source::Decoder& Source::Decoder::operator=(const Decoder& other)
{
	if (other.p)
	{
		p = reinterpret_cast<OpusDecoder*>(&space);
		memcpy(&space, &other.space, sizeof(space));
	}
	else
	{
		p = nullptr;
	}
	return *this;
}

void Source::Decoder::reset()
{
	p = reinterpret_cast<OpusDecoder*>(&space);
	if (OPUS_OK != opus_decoder_init(p, 48000, 1))
	{
		p = nullptr;
	}
}

bool Source::valid()
{
	return decoder.p != nullptr;
}

void Source::reset(uint32_t sampleRate)
{
	incomingSequence = 0;
	decoder.reset();
	outputResampler.reset(48000, sampleRate);
}

void Source::swap(Source& other)
{
	incomingData.swap(other.incomingData);
	outputResampler = other.outputResampler;
	incomingSequence = other.incomingSequence;
	decoder = std::move(other.decoder);
	other.decoder.p = nullptr;
}

uint32_t Source::getSourceAudio(const float** monoSamples)
{
	*monoSamples = incomingData.data();
	return static_cast<uint32_t>(incomingData.size());
}

void Source::consumeSourceAudio(uint32_t samples)
{
	incomingData.erase(incomingData.begin(), incomingData.begin() + samples);
}

std::vector<float> Source::takeAllSourceAudio()
{
	std::vector<float> temp;
	temp.swap(incomingData);
	return std::move(temp);
}
