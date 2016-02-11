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

#ifndef TINY_AUDIO__RESAMPLE_H
#define TINY_AUDIO__RESAMPLE_H

namespace tiny
{
	namespace audio
	{
		namespace resample
		{
			// Utitlity to resample audio linearly
			class Linear
			{
			public:
				Linear();
				Linear(int inputRate, int outputRate);

				void reset(int inputRate, int outputRate);

				int inputSamples(int outputSamples) const;
				int outputSamples(int inputSamples) const;

				void resampleMono(const float* in, int samplesIn, float* out, int samplesOut);
				void resampleStereoToMono(const float* in, int samplesIn, float* out, int samplesOut);
				void resampleMonoToStereo(const float* in, int samplesIn, float* out, int samplesOut);
				void resampleStereo(const float* in, int samplesIn, float* out, int samplesOut);

			private:
				float idealRate;
				float prevSamples[2];
			};
		}
	}
}

#endif // TINY_AUDIO__RESAMPLE_H
