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

#ifndef TINY_NET__SOCKET_H
#define TINY_NET__SOCKET_H

#include <stddef.h>
#include <stdint.h>
#include "tiny/net/address.h"

namespace tiny
{
	namespace net
	{
		typedef uintptr_t Socket;
		static const Socket InvalidSocket = ~static_cast<Socket>(0);

		struct PlatformSocketAddr
		{
			uint32_t size;
			uint8_t storage[28];
		};

		struct ConstBuffer
		{
#if defined(_MSC_VER)
			uint32_t len;
			const uint8_t* p;
#else
			const uint8_t* p;
			size_t len;
#endif
		};

		// allocate a socket
		bool socketCreateUDP(Socket* out, const Address& addr, uint16_t* bePort);
		void socketClose(Socket s);

		bool socketSendTo(Socket s, const ConstBuffer* buffers, uint32_t nbuffers, const PlatformSocketAddr& addr);
		int32_t socketRecvFrom(Socket s, uint8_t* buffer, int32_t nbuffer, PlatformSocketAddr* addr);
		bool socketOperationWouldHaveBlocked();

		// create a platform socket address from an IP address and port
		void addressFrom(PlatformSocketAddr* out, const Address& addr, uint16_t bePort);
		void addressTo(Address* out, uint16_t* outBePort, const PlatformSocketAddr& addr);
	}
}

#endif // TINY_NET__ADDRESS_H
