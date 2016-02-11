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

#ifndef TINYPEER_SRC_ICE__STUN_H
#define TINYPEER_SRC_ICE__STUN_H

#include <stdint.h>
#include "tiny/net/address.h"

namespace tiny
{
	namespace crypto { struct CryptoRandSource; }

	namespace peer
	{
		struct StunBindingRequest
		{
			const uint8_t* hmacKey;
			uint64_t incomingUsername;
			uint64_t targetUsername;
			uint64_t controllingTiebreaker;
			uint32_t nhmacKey;
			uint32_t priority;
			bool controlling;
			bool useCandidate;
		};

		struct StunBindingResult
		{
			const uint8_t* hmacKey;
			net::Address address;
			uint32_t nhmacKey;
			uint32_t priority;
			uint16_t bePort;
		};

		uint8_t* stunGenerateBindingRequest(crypto::CryptoRandSource& rand, uint8_t packet[20], uint16_t attributeLength);
		uint8_t* stunGenerateBindingResponse(uint8_t packet[20], uint16_t attributeLength, const uint8_t* request);
		uint8_t* stunAppendUsernameAttribute20(uint8_t* nextAttribute, uint64_t localUserId, uint64_t remoteUserId);
		uint8_t* stunAppendMessageIntegrityAttribute24(uint8_t* nextAttribute, const uint8_t* packetStart, const uint8_t* key, uint32_t nkey);
		uint8_t* stunAppendXorMappedAddress12(uint8_t* nextAttribute, uint16_t bePort, const net::Address4& addr, const uint8_t* packet);
		uint8_t* stunAppendXorMappedAddress24(uint8_t* nextAttribute, uint16_t bePort, const net::Address6& addr, const uint8_t* packet);
		uint8_t* stunAppendFingerprint8(uint8_t* nextAttribute, const uint8_t* packetStart);

		void stunGenerateNewTransactionId(crypto::CryptoRandSource& rand, uint8_t packet[20]);
		bool stunMatchesTransactionId(const uint8_t a[20], const uint8_t b[20]);

		bool stunIsBindingRequest(const uint8_t* packet, int npacket);
		bool stunIsBindingResponse(const uint8_t* packet, int npacket);

		bool stunProcessBindingRequest(StunBindingRequest* req, const uint8_t* packet, uint32_t npacket);
		bool stunProcessBindingResult(net::Address* address, uint16_t* bePort, const uint8_t* packet, uint32_t npacket, const uint8_t* request);
		bool stunProcessBindingResult(StunBindingResult* res, const uint8_t* packet, uint32_t npacket);

		// ICE extensions
		uint8_t* stunAppendICEPriorityAttribute8(uint8_t* nextAttribute, uint32_t priority);
		uint8_t* stunAppendICEControlAttribute12(uint8_t* nextAttribute, bool controlling, uint64_t tieBreaker);
		uint8_t* stunAppendICEUseCandidateAttribute4(uint8_t* nextAttribute);
	}
}

#endif // TINYPEER_SRC_ICE__STUN_H
