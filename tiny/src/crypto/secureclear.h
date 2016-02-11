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

#ifndef TINYCRYPTO_SRC__SECURECLEAR_H
#define TINYCRYPTO_SRC__SECURECLEAR_H

#include "tiny/platform.h"

#if TINY_PLATFORM_WINDOWS|TINY_PLATFORM_XB1|TINY_PLATFORM_X360
#	if !defined(WIN32_LEAN_AND_MEAN)
#		define WIN32_LEAN_AND_MEAN
#	endif // WIN32_LEAN_AND_MEAN
#	include <Windows.h>
#	define secureClearMemory(ptr, size) SecureZeroMemory((ptr), (size))
#elif TINY_PLATFORM_PS4
#	include <stdint.h>
	static inline void secureClearMemory(void* ptr, uint32_t size)
	{
		volatile uint8_t *v = static_cast<volatile uint8_t*>(ptr);
		while (size)
		{
			*v = 0;
			++v;
			--size;
		}
	}
#else
#	include <string.h>
#	define secureClearMemory(ptr, size) memset_s((ptr), 0, (size))
#endif // TINY_PLATFORM_

#endif // TINYCRYPTO_SRC__SECURECLEAR_H
