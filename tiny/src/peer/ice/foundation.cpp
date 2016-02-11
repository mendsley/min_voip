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

#include "peer/ice/foundation.h"
#include "tiny/hash/murmur3.h"
#include "tiny/net/address.h"

using namespace tiny;
using namespace tiny::hash;
using namespace tiny::net;
using namespace tiny::peer;

uint32_t peer::foundationForHostAddress(const Address& addr)
{
	murmur3_state st;
	murmur3_begin(&st);
	murmur3_add(&st, "LOCALUDP", 8);

	switch (addr.family)
	{
	case AddressFamily::IPv4:
		murmur3_add(&st, &addr.u.v4, sizeof(addr.u.v4));
		break;

	case AddressFamily::IPv6:
		murmur3_add(&st, &addr.u.v6, sizeof(addr.u.v6));
		break;
	}

	return murmur3_end(&st);
}

uint32_t peer::foundationForServerReflexiveAddress(uint32_t hostFoundation, const Address& addr)
{
	murmur3_state st;
	murmur3_begin(&st, hostFoundation);
	murmur3_add(&st, "SERVRFLX", 8);

	switch (addr.family)
	{
	case AddressFamily::IPv4:
		murmur3_add(&st, &addr.u.v4, sizeof(addr.u.v4));
		break;

	case AddressFamily::IPv6:
		murmur3_add(&st, &addr.u.v6, sizeof(addr.u.v6));
		break;
	}

	return murmur3_end(&st);
}

uint32_t peer::foundationForPeerReflexiveAddress(const Address& addr)
{
    murmur3_state st;
	murmur3_begin(&st);
	murmur3_add(&st, "PEERRFLX", 8);

	switch (addr.family)
	{
	case AddressFamily::IPv4:
		murmur3_add(&st, &addr.u.v4, sizeof(addr.u.v4));
		break;

	case AddressFamily::IPv6:
		murmur3_add(&st, &addr.u.v6, sizeof(addr.u.v6));
		break;
	}

    return murmur3_end(&st);
}
