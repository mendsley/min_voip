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

#include "tiny/net/resolve.h"
#include "tiny/platform.h"

using namespace tiny;
using namespace tiny::net;

#if TINY_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "tiny/net/address.h"

static_assert(sizeof(Address4) == sizeof( in_addr), "IPv4 address size mismatch");
static_assert(sizeof(Address6) == sizeof(in6_addr), "IPv6 address size mismatch");

bool net::resolveHost(Address4* out4, Address6* out6, const char* hostname)
{
	addrinfo* results;
	if (0 != getaddrinfo(hostname, nullptr, nullptr, &results))
		return false;

	for (addrinfo* it = results; it; it = it->ai_next)
	{
		switch (it->ai_family)
		{
		case AF_INET:
			if (out4)
				memcpy(out4, &reinterpret_cast<const sockaddr_in*>(it->ai_addr)->sin_addr, sizeof(*out4));
			break;

		case AF_INET6:
			if (out6)
				memcpy(out6, &reinterpret_cast<const sockaddr_in6*>(it->ai_addr)->sin6_addr, sizeof(*out6));
			break;

		default:
			break;
		}
	}

	freeaddrinfo(results);
	return true;
}

#endif // TINY_PLATFORM_WINDOWS
