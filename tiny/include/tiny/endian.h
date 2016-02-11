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

#ifndef TINY__ENDIAN_H
#define TINY__ENDIAN_H

#include <stdint.h>

#if defined(_MSC_VER)
#	include <stdlib.h>
#	pragma push_macro("__builtin_bswap16")
#	pragma push_macro("__builtin_bswap32")
#	pragma push_macro("__builtin_bswap64")
#	define __builtin_bswap16 _byteswap_ushort
#	define __builtin_bswap32 _byteswap_ulong
#	define __builtin_bswap64 _byteswap_uint64
#endif

namespace tiny
{
	namespace detail
	{
		union endian_ckeck
		{
			uint32_t i;
			uint8_t little;
		};
	}

	static const detail::endian_ckeck endian = { 0x01 };

	static inline uint16_t endianFromBig(uint16_t v)
	{
		return endian.little ? __builtin_bswap16(v) : v;
	}

	static inline uint32_t endianFromBig(uint32_t v)
	{
		return endian.little ? __builtin_bswap32(v) : v;
	}

	static inline uint64_t endianFromBig(uint64_t v)
	{
		return endian.little ? __builtin_bswap64(v) : v;
	}

	static inline uint16_t endianToBig(uint16_t v)
	{
		return endian.little ? __builtin_bswap16(v) : v;
	}

	static inline uint32_t endianToBig(uint32_t v)
	{
		return endian.little ? __builtin_bswap32(v) : v;
	}

	static inline uint64_t endianToBig(uint64_t v)
	{
		return endian.little ? __builtin_bswap64(v) : v;
	}

	static inline uint16_t endianFromLittle(uint16_t v)
	{
		return endian.little ? v : __builtin_bswap16(v);
	}

	static inline uint32_t endianFromLittle(uint32_t v)
	{
		return endian.little ? v : __builtin_bswap32(v);
	}

	static inline uint64_t edianFromLittle(uint64_t v)
	{
		return endian.little ? v : __builtin_bswap64(v);
	}

	static inline uint16_t endianToLittle(uint16_t v)
	{
		return endian.little ? v : __builtin_bswap16(v);
	}

	static inline uint32_t endianToLittle(uint32_t v)
	{
		return endian.little ? v : __builtin_bswap32(v);
	}

	static inline uint64_t endianToLittle(uint64_t v)
	{
		return endian.little ? v : __builtin_bswap64(v);
	}
}

#if defined(_MSC_VER)
#	pragma pop_macro("__builtin_bswap16")
#	pragma pop_macro("__builtin_bswap32")
#	pragma pop_macro("__builtin_bswap64")
#endif

#endif // TINY__ENDIAN_H
