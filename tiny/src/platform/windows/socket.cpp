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
#include "tiny/net/socket.h"

using namespace tiny;
using namespace tiny::net;

#if TINY_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "tiny/net/address.h"

static_assert(sizeof(PlatformSocketAddr) >= sizeof(sockaddr_in ), "PlatformSocketAddr not large enough for IPv4 socket address");
static_assert(sizeof(PlatformSocketAddr) >= sizeof(sockaddr_in6), "PlatformSocketAddr not large enough for IPv6 socket address");
static_assert(sizeof(Address4) == sizeof( in_addr), "IPv4 address not correct size");
static_assert(sizeof(Address6) == sizeof(in6_addr), "IPv4 address not correct size");
static_assert(sizeof(WSABUF) == sizeof(ConstBuffer), "ConstBuffer does not match WSABUF");
static_assert(sizeof(reinterpret_cast<WSABUF*>(0)->len) == sizeof(reinterpret_cast<ConstBuffer*>(0)->len), "ConstBuffer::len is the wrong size");
static_assert(sizeof(reinterpret_cast<WSABUF*>(0)->buf) == sizeof(reinterpret_cast<ConstBuffer*>(0)->p), "ConstBuffer::b is the wrong size");
static_assert(static_cast<void*>(&reinterpret_cast<WSABUF*>(0)->len) == static_cast<void*>(&reinterpret_cast<ConstBuffer*>(0)->len), "ConstBuffer::len is at the wrong offset");
static_assert(static_cast<void*>(&reinterpret_cast<WSABUF*>(0)->buf) == static_cast<void*>(&reinterpret_cast<ConstBuffer*>(0)->p), "ConstBuffer::b is at the wrong offset");

// allocate a socket
bool net::socketCreateUDP(Socket* out, const Address& addr, uint16_t* bePort)
{
	int addressFamily;
	switch (addr.family)
	{
	case AddressFamily::IPv4:
		addressFamily = AF_INET;
		break;

	case AddressFamily::IPv6:
		addressFamily = AF_INET6;
		break;

	default:
		return false;
	}

	SOCKET s = socket(addressFamily, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET)
		return false;

	u_long nonBlocking = 1;
	if (0 != ioctlsocket(s, FIONBIO, &nonBlocking))
	{
		closesocket(s);
		return false;
	}

	const int exclusive = 1;
	if (0 != setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof(exclusive)))
	{
		closesocket(s);
		return false;
	}

	PlatformSocketAddr saddr;
	addressFrom(&saddr, addr, *bePort);
	if (0 != bind(s, reinterpret_cast<const sockaddr*>(&saddr.storage), saddr.size))
	{
		closesocket(s);
		return false;
	}

	int addrLen = sizeof(saddr.storage);
	if (0 != getsockname(s, reinterpret_cast<sockaddr*>(&saddr.storage), &addrLen))
	{
		return false;
	}

	switch (reinterpret_cast<sockaddr*>(&saddr.storage)->sa_family)
	{
	case AF_INET:
		*bePort = reinterpret_cast<const sockaddr_in*>(&saddr.storage)->sin_port;
		break;

	case AF_INET6:
		*bePort = reinterpret_cast<const sockaddr_in6*>(&saddr.storage)->sin6_port;
		break;

	default:
		closesocket(s);
		return false;
	}

	*out = s;
	return true;
}

void net::socketClose(Socket s)
{
	closesocket(s);
}

bool net::socketSendTo(Socket s, const ConstBuffer* buffers, uint32_t nbuffers, const PlatformSocketAddr& addr)
{
	WSABUF* wsab = const_cast<WSABUF*>(reinterpret_cast<const WSABUF*>(buffers));
	
	DWORD sent;
	if (0 != WSASendTo(s, wsab, nbuffers, &sent, 0, reinterpret_cast<const sockaddr*>(&addr.storage), addr.size, NULL, NULL))
		return false;

	uint32_t total = 0;
	for (uint32_t ii = 0; ii < nbuffers; ++ii)
		total += buffers[ii].len;

	return sent == total;
}

int32_t net::socketRecvFrom(Socket s, uint8_t* buffer, int32_t nbuffer, PlatformSocketAddr* addr)
{
	int addrLen = sizeof(addr->storage);
	int result = recvfrom(s, reinterpret_cast<char*>(buffer), nbuffer, 0, reinterpret_cast<sockaddr*>(&addr->storage), &addrLen);
	addr->size = static_cast<uint32_t>(addrLen);
	return result;
}

bool net::socketOperationWouldHaveBlocked()
{
	return WSAGetLastError() == WSAEWOULDBLOCK;
}

void net::addressFrom(PlatformSocketAddr* out, const Address& addr, uint16_t bePort)
{
	memset(&out->storage, 0, sizeof(out->storage));

	switch (addr.family)
	{
	case AddressFamily::IPv4:
		{
			sockaddr_in* sa = reinterpret_cast<sockaddr_in*>(&out->storage);
			sa->sin_family = AF_INET;
			memcpy(&sa->sin_addr, &addr.u.v4, sizeof(sa->sin_addr));
			sa->sin_port = bePort;
			out->size = sizeof(*sa);
		} break;

	case AddressFamily::IPv6:
		{
			sockaddr_in6* sa = reinterpret_cast<sockaddr_in6*>(&out->storage);
			sa->sin6_family = AF_INET6;
			memcpy(&sa->sin6_addr, &addr.u.v6, sizeof(sa->sin6_addr));
			sa->sin6_port = bePort;
			out->size = sizeof(*sa);
		} break;

	default:
		out->size = 0;
		break;
	}
}

void net::addressTo(Address* out, uint16_t* outBePort, const PlatformSocketAddr& addr)
{
	switch (addr.size)
	{
	case sizeof(sockaddr_in):
		{
			const sockaddr_in* sa = reinterpret_cast<const sockaddr_in*>(&addr.storage);
			if (sa->sin_family == AF_INET)
			{
				out->family = AddressFamily::IPv4;
				memcpy(&out->u.v4, &sa->sin_addr, sizeof(sa->sin_addr));
				*outBePort = sa->sin_port;
				return;
			}
		}

	case sizeof(sockaddr_in6):
		{
			const sockaddr_in6* sa = reinterpret_cast<const sockaddr_in6*>(&addr.storage);
			if (sa->sin6_family == AF_INET6)
			{
				out->family = AddressFamily::IPv6;
				memcpy(&out->u.v6, &sa->sin6_addr, sizeof(sa->sin6_addr));
				*outBePort = sa->sin6_port;
				return;
			}
		}
	}

	out->family = AddressFamily::Unspecified;
}

//AddressFamily::E net::socketAddressFamilty(const PlatformSocketAddr& addr)
//{
//	switch (addr.size)
//	{
//	case sizeof(sockaddr_in):
//		return AddressFamily::IPv4;
//
//	case sizeof(sockaddr_in6):
//		return AddressFamily::IPv6;
//
//	default:
//		return AddressFamily::Unspecified;
//	}
//}

#endif // TINY_PLATFORM_WINDOWS
