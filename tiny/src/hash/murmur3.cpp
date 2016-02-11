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

#include "rotate.h"
#include "tiny/hash/murmur3.h"

using namespace tiny;
using namespace tiny::hash;

static const uint32_t c_murmur3_c1 = 0xCC9E2D51;
static const uint32_t c_murmur3_c2 = 0x1B873593;

// avalanche all bits in h
static inline uint32_t avalance(uint32_t h)
{
	h ^= (h >> 16);
	h *= 0x85EBCA6B;
	h ^= (h >> 13);
	h *= 0xC2B2AE35;
	h ^= (h >> 16);
	return h;
}

static uint32_t add_block(uint32_t hash, uint32_t b)
{
	b *= c_murmur3_c1;
	b = rotate_left<15>(b);
	b *= c_murmur3_c2;

	hash ^= b;
	hash = rotate_left<13>(hash);
	hash *= 5;
	hash += 0xE6546B64;
	return hash;
}

static uint32_t append_tail(murmur3_state::tail_u& tail, uint32_t prevLen, const uint8_t* p, uint32_t n)
{
	uint32_t nextIndex = prevLen&3;
	uint32_t remaining = 4-nextIndex;
	uint32_t ii;
	for (ii = 0; ii < remaining && ii < n; ++ii, ++nextIndex)
	{
		tail.b[nextIndex] = p[ii];
	}

	return ii;
}

void hash::murmur3_begin(murmur3_state* st, uint32_t seed)
{
	st->hash = seed;
	st->len = 0u;
	st->tail.u = 0u;
}

void tiny::hash::murmur3_add(murmur3_state* st, const void* p, uint32_t n)
{
	const uint8_t* data = static_cast<const uint8_t*>(p);

	uint32_t hash = st->hash;

	// complete previous partial block in st->tail
	if (st->tail.u)
	{
		uint32_t read = append_tail(st->tail, st->len, data, n);
		n -= read;
		data += read;
		st->len += read;

		if ((st->len&3) != 0)
		{
			return;
		}

		// add partial block
		hash = add_block(hash, st->tail.u);
		st->tail.u = 0u;
	}

	// process complete 32-bit blocks
	const uint32_t nblocks = n/4;
	const uint32_t* blocks = reinterpret_cast<const uint32_t*>(data);
	for (uint32_t ii = 0; ii < nblocks; ++ii)
	{
		hash = add_block(hash, blocks[ii]);
	}

	st->len += nblocks*4;

	// append remaining partial block to tail
	const uint8_t* tail = reinterpret_cast<const uint8_t*>(blocks + nblocks);
	st->len += append_tail(st->tail, st->len, tail, n&3);
	st->hash = hash;
}

uint32_t tiny::hash::murmur3_end(const murmur3_state* st)
{
	uint32_t hash = st->hash;

	// process trailing data
	uint32_t k1 = st->tail.u;
	if (k1)
	{
		k1 *= c_murmur3_c1;
		k1 = rotate_left<15>(k1);
		k1 *= c_murmur3_c2;

		hash ^= k1;
	}

	// finalize hash
	hash ^= st->len;
	return avalance(hash);
}
