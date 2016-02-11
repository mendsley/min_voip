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

#ifndef TINY_CRYPTO__CONSTANT_H
#define TINY_CRYPTO__CONSTANT_H

#include <stdint.h>

namespace tiny
{
	namespace crypto
	{
		static inline uint8_t constCompareByte(uint8_t a, uint8_t b)
		{
			uint8_t result = ~(a ^ b);
			result &= (result >> 4);
			result &= (result >> 2);
			result &= (result >> 1);
			return result;
		}

		static inline uint8_t constCompareRange(const uint8_t* a, const uint8_t* b, uint32_t n)
		{
			uint8_t value = 0;
			for (uint32_t ii = 0; ii < n; ++ii)
				value |= (a[ii] ^ b[ii]);

			return constCompareByte(value, 0);
		}
	}
}

#endif // TINY_CRYPTO__CONSTANT_H
