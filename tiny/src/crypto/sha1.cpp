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
#include "crypto/secureclear.h"
#include "tiny/endian.h"
#include "tiny/crypto/sha1.h"
#include "rotate.h"

using namespace tiny;
using namespace tiny::crypto;

#define blk0(i) (block->l[i] = endianToBig(block->l[i]))
#define blk(i) (block->l[i&15] = rotate_left<1>(block->l[(i+13) & 0x0F] ^ block->l[(i+8) & 0x0F] ^ block->l[(i+2) & 0x0F] ^ block->l[i & 0x0F]))

// (R0+R1), R2, R3, R4 are the different operations used in SHA1
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rotate_left<5>(v);w=rotate_left<30>(w);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rotate_left<5>(v);w=rotate_left<30>(w);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rotate_left<5>(v);w=rotate_left<30>(w);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rotate_left<5>(v);w=rotate_left<30>(w);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rotate_left<5>(v);w=rotate_left<30>(w);

// Hash a single 512-bit block. This is the core of the algorithm
static void process_block(uint32_t state[5], uint8_t buffer[64])
{
	union Char64Long16
	{
		uint8_t  c[64];
		uint32_t l[16];
	};

	Char64Long16* block = reinterpret_cast<Char64Long16*>(buffer);

	// copy state to working vars
	uint32_t a = state[0];
	uint32_t b = state[1];
	uint32_t c = state[2];
	uint32_t d = state[3];
	uint32_t e = state[4];

	// 4 rounds of 20 operations each. Loop unrolled.
	R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
	R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
	R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
	R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
	R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
	R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
	R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
	R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
	R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
	R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
	R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
	R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
	R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
	R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
	R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
	R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
	R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
	R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
	R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
	R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

	// add the working vars back to state
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
}

void crypto::sha1_begin(sha1_state* st)
{
	st->state[0] = 0x67452301;
	st->state[1] = 0xEFCDAB89;
	st->state[2] = 0x98BADCFE;
	st->state[3] = 0x10325476;
	st->state[4] = 0xC3D2E1F0;
	st->count = 0;
}

void crypto::sha1_add(sha1_state* st, const void* p, uint32_t n)
{
	uint32_t j = (st->count/8) % 64;
	st->count += (n*8);

	// have a full 512 bit block?
	uint32_t i;
	if ((j + n) >= 64)
	{
		// fill remaining buffer
		i = 64-j;
		memcpy(&st->buffer[j], p, i);
		process_block(st->state, st->buffer);

		// copy remaining full blocks
		for ( ; i + 64 <= n; i += 64)
		{
			memcpy(st->buffer, static_cast<const uint8_t*>(p) + i, 64);
			process_block(st->state, st->buffer);
		}
		j = 0;

	}
	else
	{
		i = 0;
	}

	// copy remaining to buffer
	memcpy(&st->buffer[j], static_cast<const uint8_t*>(p) + i, n-i);
}

void crypto::sha1_end(sha1_state* st, uint8_t digest[sha1_state::DIGEST_SIZE])
{
	uint64_t beCount = endianToBig(st->count);

	// pad message
	//  RFFC 3174: Append a 1 bit, followed by zeros, followed by the 64-bit
	//  message length until a full 512-bit block is reached
	static const uint8_t c_finalOne = 0x80;
	static const uint8_t c_finalNul = 0x00;
	sha1_add(st, &c_finalOne, 1);
	while (st->count%512 != (512-64)) // leave room for length
		sha1_add(st, &c_finalNul, 1);

	// should cause a sha1_transform call
	sha1_add(st, &beCount, sizeof(beCount));

	for (uint32_t ii = 0; ii < sha1_state::DIGEST_SIZE; ++ii)
	{
		digest[ii] = static_cast<uint8_t>((st->state[ii>>2] >> ((3 - (ii&3)) * 8)) & 0xFF);
	}

	// wipe variables
	secureClearMemory(st->buffer, sizeof(st->buffer));
	secureClearMemory(st->state, sizeof(st->state));
	secureClearMemory(&st->count, sizeof(st->count));
	secureClearMemory(&beCount, sizeof(beCount));
}
