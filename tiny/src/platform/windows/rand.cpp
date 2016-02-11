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

#include "tiny/platform.h"
#include "tiny/crypto/rand.h"

#if TINY_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wincrypt.h>

using namespace tiny;
using namespace tiny::crypto;

static_assert(sizeof(HCRYPTPROV) <= sizeof(void*), "WinCrypt provider handle will not reinterpret_cast into RandomGenerator::platformHandle");

void crypto::crandInit(CryptoRandSource* s)
{
	// attempt to use cryptographic provider
	HCRYPTPROV crypto;
	if (CryptAcquireContext(&crypto, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
	{
		s->platformHandle = reinterpret_cast<void*>(crypto);
	}
	else
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);

		crypto = 0;
		s->platformHandle = 0;
		s->fallback.seed(li.LowPart);
	}
}

void crypto::crandDestroy(CryptoRandSource* s)
{
	if (s->platformHandle)
	{
		CryptReleaseContext(reinterpret_cast<HCRYPTPROV>(s->platformHandle), 0);
		s->platformHandle = 0;
	}
}

void crypto::crandFill(CryptoRandSource* s, uint8_t* p, uint32_t n)
{
	if (!s->platformHandle || !CryptGenRandom(reinterpret_cast<HCRYPTPROV>(s->platformHandle), n, p))
	{
		for (uint32_t ii = 0; ii < n; ++ii)
		{
			p[ii] = static_cast<uint8_t>(s->fallback() & 0xFF);
		}
	}
}

#endif // TINY_PLATFORM_WINDOWS
