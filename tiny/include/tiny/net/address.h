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

#ifndef TINY_NET__ADDRESS_H
#define TINY_NET__ADDRESS_H

#include <stdint.h>

namespace tiny
{
	namespace net
	{
		struct Address4
		{
			uint8_t addr[4];
		};

		struct Address6
		{
			uint8_t addr[16];
		};

		struct AddressFamily
		{
			enum E
			{
				Unspecified,
				IPv4,
				IPv6,
			};
		};

		struct Address
		{
			union
			{
				Address4 v4;
				Address6 v6;
			} u;

			uint8_t family;
		};

		static inline bool addressIsEqual(const Address4& a, const Address4& b)
		{
			return a.addr[0] == b.addr[0]
				&& a.addr[1] == b.addr[1]
				&& a.addr[2] == b.addr[2]
				&& a.addr[3] == b.addr[3]
				;
		}

		static inline bool addressIsEqual(const Address6& a, const Address6& b)
		{
			return a.addr[ 0] == b.addr[ 0]
				&& a.addr[ 1] == b.addr[ 1]
				&& a.addr[ 2] == b.addr[ 2]
				&& a.addr[ 3] == b.addr[ 3]
				&& a.addr[ 4] == b.addr[ 4]
				&& a.addr[ 5] == b.addr[ 5]
				&& a.addr[ 6] == b.addr[ 6]
				&& a.addr[ 7] == b.addr[ 7]
				&& a.addr[ 8] == b.addr[ 8]
				&& a.addr[ 9] == b.addr[ 9]
				&& a.addr[10] == b.addr[10]
				&& a.addr[11] == b.addr[11]
				&& a.addr[12] == b.addr[12]
				&& a.addr[13] == b.addr[13]
				&& a.addr[14] == b.addr[14]
				&& a.addr[15] == b.addr[15]
				;
		}

		static inline bool addressIsEqual(const Address& a, const Address& b)
		{
			if (a.family != b.family)
				return false;

			if (a.family == AddressFamily::IPv4)
				return addressIsEqual(a.u.v4, b.u.v4);

			return addressIsEqual(a.u.v6, b.u.v6);
		}
	
		static inline bool addressIsLoopback(const Address4& a)
		{
			return a.addr[0] == 0x7F
				&& a.addr[1] == 0x00
				&& a.addr[2] == 0x00
				&& a.addr[3] == 0x01
				;
		}

		static inline bool addressIsLoopback(const Address6& a)
		{
			return a.addr[ 0] == 0x00
				&& a.addr[ 1] == 0x00
				&& a.addr[ 2] == 0x00
				&& a.addr[ 3] == 0x00
				&& a.addr[ 4] == 0x00
				&& a.addr[ 5] == 0x00
				&& a.addr[ 6] == 0x00
				&& a.addr[ 7] == 0x00
				&& a.addr[ 8] == 0x00
				&& a.addr[ 9] == 0x00
				&& a.addr[10] == 0x00
				&& a.addr[11] == 0x00
				&& a.addr[12] == 0x00
				&& a.addr[13] == 0x00
				&& a.addr[14] == 0x00
				&& a.addr[15] == 0x01
				;
		}

		static inline bool addressIsLinkLocal(const Address6& a)
		{
			return a.addr[0] == 0xFE
				&& a.addr[1] == 0x80
				;
		}

		static inline bool addressIsSiteLocal(const Address6& a)
		{
			return a.addr[0] == 0xFE
				&& (a.addr[1] & 0xC0) == 0xC0
				;
		}

		static inline bool addressIsV4Compatible(const Address6& a)
		{
			return a.addr[ 0] == 0x00
				&& a.addr[ 1] == 0x00
				&& a.addr[ 2] == 0x00
				&& a.addr[ 3] == 0x00
				&& a.addr[ 4] == 0x00
				&& a.addr[ 5] == 0x00
				&& a.addr[ 6] == 0x00
				&& a.addr[ 7] == 0x00
				&& a.addr[ 8] == 0x00
				&& a.addr[ 9] == 0x00
				&& a.addr[10] == 0x00
				&& a.addr[11] == 0x00
				;
		}
	
		static inline bool addressIs6Bone(const Address6& a)
		{
			return a.addr[0] == 0x3F
				&& a.addr[1] == 0xFE
				;
		}

		static inline bool addressIsTeredo(const Address6& a)
		{
			return a.addr[0] == 0x20
				&& a.addr[1] == 0x01
				&& a.addr[2] == 0x00
				&& a.addr[3] == 0x00
				;
		}

		static inline bool addressIs6to4(const Address6& a)
		{
			return a.addr[0] == 0x20
				&& a.addr[1] == 0x02
				;
		}

		static inline bool addressIsV4Mapped(const Address6& a)
		{
			return a.addr[ 0] == 0x00
				&& a.addr[ 1] == 0x00
				&& a.addr[ 2] == 0x00
				&& a.addr[ 3] == 0x00
				&& a.addr[ 4] == 0x00
				&& a.addr[ 5] == 0x00
				&& a.addr[ 6] == 0x00
				&& a.addr[ 7] == 0x00
				&& a.addr[ 8] == 0x00
				&& a.addr[ 9] == 0x00
				&& a.addr[10] == 0xFF
				&& a.addr[11] == 0xFF
				;
		}

		static inline bool addressIsUniqueLocal(const Address6& a)
		{
			return (a.addr[0] & 0xFE) == 0xFC;
		}
	}
}

#endif // TINY_NET__ADDRESS_H
