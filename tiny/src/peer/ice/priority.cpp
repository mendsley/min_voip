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

#include "peer/ice/priority.h"
#include "tiny/net/address.h"

using namespace tiny;
using namespace tiny::net;
using namespace tiny::peer;

static uint32_t localPreference(const Address& addr)
{
	switch (addr.family)
	{
	case AddressFamily::IPv4:
		return 30000;

	case AddressFamily::IPv6:
		if (addressIsSiteLocal(addr.u.v6))
			return 1000;
		if (addressIsV4Compatible(addr.u.v6))
			return 1000;
		if (addressIs6Bone(addr.u.v6))
			return 1000;
		if (addressIsTeredo(addr.u.v6))
			return 10000;
		if (addressIs6to4(addr.u.v6))
			return 20000;
		if (addressIsV4Mapped(addr.u.v6))
			return 30000;
		if (addressIsUniqueLocal(addr.u.v6))
			return 50000;
		if (addressIsLoopback(addr.u.v6))
			return 60000;

		// all other IPv6 addresses
		return 40000;

	default:
		return 0;
	}
}

uint32_t peer::priorityForHostAddress(const Address& addr)
{
	return localPreference(addr) | TypePreference::Host;
}

uint32_t peer::priorityChangeTypePreference(uint32_t priority, TypePreference::E newType)
{
	return (priority & 0x00FFFFFF) | newType;
}
