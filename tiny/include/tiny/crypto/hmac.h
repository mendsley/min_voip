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

#ifndef TINY_CRYPTO__HMAC_H
#define TINY_CRYPTO__HMAC_H

#include <stdint.h>
#include <tiny/crypto/sha1.h>

namespace tiny
{
	namespace crypto
	{
		struct hmac_sha1_state
		{
			static const uint32_t BLOCK_SIZE = 64;
			static const uint32_t DIGEST_SIZE = sha1_state::DIGEST_SIZE;

			sha1_state inner;
			uint8_t buffer[DIGEST_SIZE+BLOCK_SIZE];
		};

		void hmac_sha1_begin(hmac_sha1_state* st, const uint8_t* key, uint32_t nkey);
		void hmac_sha1_add(hmac_sha1_state* st, const void* p, uint32_t n);
		void hmac_sha1_end(hmac_sha1_state* st, uint8_t digest[hmac_sha1_state::DIGEST_SIZE]);
		bool hmac_sha1_digest_equal(const uint8_t* digest1, uint32_t ndigest1, const uint8_t* digest2, uint32_t ndigest2);
	}
}

#endif // TINY_CRYPTO__HMAC_H
