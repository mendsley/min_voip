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

#include <string.h>
#include "tiny/crypto/constant.h"
#include "tiny/crypto/hmac.h"
#include "tiny/crypto/sha1.h"
#include "secureclear.h"

using namespace tiny;
using namespace tiny::crypto;

static inline void maskBuffer(uint8_t* buffer, uint32_t nbuffer, uint8_t value)
{
	for (uint32_t ii = 0; ii < nbuffer; ++ii)
		buffer[ii] ^= value;
}

void tiny::crypto::hmac_sha1_begin(hmac_sha1_state* st, const uint8_t* key, uint32_t nkey)
{
	// prepare key
	if (nkey > hmac_sha1_state::BLOCK_SIZE)
	{
		sha1_begin(&st->inner);
		sha1_add(&st->inner, key, nkey);
		sha1_end(&st->inner, st->buffer);
		nkey = sha1_state::DIGEST_SIZE;
	}
	else
	{
		memcpy(st->buffer, key, nkey);
		if (nkey < hmac_sha1_state::BLOCK_SIZE)
			memset(st->buffer + nkey, 0, hmac_sha1_state::BLOCK_SIZE-nkey);
	}

	maskBuffer(st->buffer, hmac_sha1_state::BLOCK_SIZE, 0x36);

	sha1_begin(&st->inner);
	sha1_add(&st->inner, st->buffer, hmac_sha1_state::BLOCK_SIZE);
}

void tiny::crypto::hmac_sha1_add(hmac_sha1_state* st, const void* p, uint32_t n)
{
	sha1_add(&st->inner, p, n);
}

void tiny::crypto::hmac_sha1_end(hmac_sha1_state* st, uint8_t digest[hmac_sha1_state::DIGEST_SIZE])
{
	sha1_end(&st->inner, st->buffer + hmac_sha1_state::BLOCK_SIZE);

	maskBuffer(st->buffer, hmac_sha1_state::BLOCK_SIZE, 0x36^0x5C);

	sha1_begin(&st->inner);
	sha1_add(&st->inner, st->buffer, sizeof(st->buffer));
	sha1_end(&st->inner, digest);

	secureClearMemory(st->buffer, sizeof(st->buffer));
}

bool tiny::crypto::hmac_sha1_digest_equal(const uint8_t* digest1, uint32_t ndigest1, const uint8_t* digest2, uint32_t ndigest2)
{
	if (ndigest1 != ndigest2)
		return false;

	return 1 == constCompareRange(digest1, digest2, ndigest1);
}
