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

#ifndef TINY_HASH__FNV_H
#define TINY_HASH__FNV_H

#include <stdint.h>

namespace tiny
{
	namespace detail
	{
		static const uint32_t FNV_prime32 = 16777619; // 2^24 + 2^8 + 0x93
		static const uint32_t FNV_offset32 = 2166136261;
	}

	// fnv-1a
	static inline uint32_t fnv1a(const void* ptr, size_t n)
	{
		using namespace detail;

		const uint8_t* p = static_cast<const uint8_t*>(ptr);
		uint32_t v = FNV_offset32;
		for (size_t ii = 0; ii < n; ++ii, ++p)
		{
			v = (v ^ *p) * FNV_prime32;
		}

		return v;
	}

	template<uint32_t size>
	static inline uint32_t fnv1a_cstr(const char (&str)[size])
	{
		return fnv1a(str, size - 1);
	}
}

#endif // TINY_HASH__FNV_H
