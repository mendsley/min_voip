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

#include <stdint.h>
#include <string.h>
#include "peer/ice/candidate.h"

using namespace tiny;
using namespace tiny::peer;
using namespace tiny::net;

bool peer::candidateShouldUseHostAddress(const Address& addr)
{
	switch (addr.family)
	{
	case AddressFamily::IPv4:
		return !addressIsLoopback(addr.u.v4);

	case AddressFamily::IPv6:
		return !addressIsLoopback(addr.u.v6) && !addressIsLinkLocal(addr.u.v6);

	default:
		return false;
	}
}

uint32_t peer::candidatesEncodeLength(const Candidate& c)
{
	switch (c.address.family)
	{
	case AddressFamily::IPv4:
		return 14;
	case AddressFamily::IPv6:
		return 26;
	default:
		return 10;
	}
}

uint8_t* peer::candidatesEncode(uint8_t* out, const Candidate& c)
{
	memcpy(out, &c.foundation, sizeof(c.foundation));
	out += sizeof(c.foundation);

	uint32_t priority = c.priority & ~0x03;
	switch (c.address.family)
	{
	case AddressFamily::IPv4:
		priority |= 0x01;
		break;
	case AddressFamily::IPv6:
		priority |= 0x02;
		break;
	default:
		break;
	}
	memcpy(out, &priority, sizeof(priority));
	out += sizeof(priority);

	memcpy(out, &c.port, sizeof(c.port));
	out += sizeof(c.port);

	switch (c.address.family)
	{
	case AddressFamily::IPv4:
		memcpy(out, &c.address.u.v4, sizeof(c.address.u.v4));
		out += sizeof(c.address.u.v4);
		break;

	case AddressFamily::IPv6:
		memcpy(out, &c.address.u.v6, sizeof(c.address.u.v6));
		out += sizeof(c.address.u.v6);
		break;
	}

	return out;
}

uint32_t peer::candidateDecode(Candidate* out, const uint8_t* in, uint32_t n)
{
	if (n < 10)
		return 0;

	memcpy(&out->foundation, in, sizeof(out->foundation));
	in += sizeof(out->foundation);

	uint32_t priorityAndType;
	memcpy(&priorityAndType, in, sizeof(priorityAndType));
	in += sizeof(priorityAndType);

	out->priority = priorityAndType & ~0x03;
	const uint32_t addressType = priorityAndType & 0x03;

	memcpy(&out->port, in, sizeof(out->port));
	in += sizeof(out->port);

	switch (addressType)
	{
	case 0x01: // IPv4
		if (n < 14)
			return 0;
		out->address.family = AddressFamily::IPv4;
		memcpy(&out->address.u.v4, in, sizeof(out->address.u.v4));
		return 14;

	case 0x02: // IPv6
		if (n < 26)
			return 0;
		out->address.family = AddressFamily::IPv6;
		memcpy(&out->address.u.v6, in, sizeof(out->address.u.v6));
		return 26;

	default:
		return 0;
	}
}
