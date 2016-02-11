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
#include "tiny/net/adapter.h"

#if TINY_PLATFORM_WINDOWS

#include <stdint.h>
#include <string.h>
#include <vector>
#include "tiny/net/address.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <IPHlpApi.h>

using namespace tiny;
using namespace tiny::net;

static_assert(sizeof(Address4) == sizeof( in_addr), "IPv4 address size mismatch");
static_assert(sizeof(Address6) == sizeof(in6_addr), "IPv6 address size mismatch");

uint32_t net::enumerateAdapters(Address* addresses, uint32_t naddresses)
{
	// enumerate local adapters
	static const ULONG flags = 0
		| GAA_FLAG_SKIP_ANYCAST
		| GAA_FLAG_SKIP_MULTICAST
		| GAA_FLAG_SKIP_DNS_SERVER
		| GAA_FLAG_SKIP_FRIENDLY_NAME
		;

	ULONG bufferSize = 0;
	if (ERROR_BUFFER_OVERFLOW != GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &bufferSize))
	{
		return 0;
	}

	std::vector<uint8_t> storage(bufferSize);
	IP_ADAPTER_ADDRESSES* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(storage.data());
	if (ERROR_SUCCESS != GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapters, &bufferSize))
	{
		return 0;
	}

	uint32_t numAdpaters = 0;
	for (IP_ADAPTER_ADDRESSES* adapter = adapters; adapter; adapter = adapter->Next)
	{
		if (adapter->OperStatus == IfOperStatusUp)
		{
			for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; address; address = address->Next)
			{
				Address addr;

				const sockaddr* sockaddr = address->Address.lpSockaddr;
				switch (sockaddr->sa_family)
				{
				case AF_INET:
					addr.family = AddressFamily::IPv4;
					memcpy(&addr.u.v4, &reinterpret_cast<const sockaddr_in*>(sockaddr)->sin_addr, sizeof(addr.u.v4));
					break;

				case AF_INET6:
					addr.family = AddressFamily::IPv6;
					memcpy(&addr.u.v6, &reinterpret_cast<const sockaddr_in6*>(sockaddr)->sin6_addr, sizeof(addr.u.v6));
					break;

				default:
					continue; // unsupported protocol
				}

				++numAdpaters;

				if (naddresses)
				{
					numAdpaters = naddresses;
					*addresses = addr;
					++addresses;
				}
			}
		}
	}

	return numAdpaters;
}

#endif // TINYPEER_ENABLE_
