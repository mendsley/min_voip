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
#include "peer/ice/stun.h"
#include "tiny/hash/crc32.h"
#include "tiny/crypto/hmac.h"
#include "tiny/crypto/rand.h"
#include "tiny/endian.h"
#include "tiny/net/address.h"

using namespace tiny;
using namespace tiny::crypto;
using namespace tiny::hash;
using namespace tiny::net;
using namespace tiny::peer;

static const uint8_t c_stunMagic[4] = {0x21, 0x12, 0xA4, 0x42};

uint8_t* peer::stunGenerateBindingRequest(crypto::CryptoRandSource& rand, uint8_t packet[20], uint16_t attributeLength)
{
	attributeLength = endianToBig(attributeLength);

	packet[0] = 0x00; // method+class : uint16_t
	packet[1] = 0x01;
	memcpy(&packet[2], &attributeLength, sizeof(attributeLength));
	packet[4] = c_stunMagic[0]; // magic : uint32_t
	packet[5] = c_stunMagic[1];
	packet[6] = c_stunMagic[2];
	packet[7] = c_stunMagic[3];

	stunGenerateNewTransactionId(rand, packet);
	return packet+20;
}

uint8_t* peer::stunGenerateBindingResponse(uint8_t packet[20], uint16_t attributeLength, const uint8_t* request)
{
	attributeLength = endianToBig(attributeLength);

	packet[0] = 0x01; // method+class : uint16_t
	packet[1] = 0x01;
	memcpy(&packet[2], &attributeLength, sizeof(attributeLength));
	memcpy(&packet[4], &request[4], 16);

	return packet+20;
}

uint8_t* peer::stunAppendUsernameAttribute20(uint8_t* nextAttribute, uint64_t localUserId, uint64_t remoteUserId)
{
	nextAttribute[0] = 0; // type : uint16_t
	nextAttribute[1] = 0x06;
	nextAttribute[2] = 0; // length : uint16_t
	nextAttribute[3] = 0x10;
	localUserId = endianToBig(localUserId);
	remoteUserId = endianToBig(remoteUserId);
	memcpy(&nextAttribute[4], &localUserId, sizeof(localUserId));
	memcpy(&nextAttribute[12], &remoteUserId, sizeof(remoteUserId));
	return nextAttribute + 20;
}

uint8_t* peer::stunAppendMessageIntegrityAttribute24(uint8_t* nextAttribute, const uint8_t* packetStart, const uint8_t* key, uint32_t nkey)
{
	nextAttribute[0] = 0; // type : uint16_t
	nextAttribute[1] = 0x08;
	nextAttribute[2] = 0; // length : uint16_t
	nextAttribute[3] = 0x14;

	hmac_sha1_state st;
	hmac_sha1_begin(&st, key, nkey);
	hmac_sha1_add(&st, packetStart, static_cast<uint32_t>(nextAttribute-packetStart));
	hmac_sha1_end(&st, &nextAttribute[4]);

	return nextAttribute + 24;
}

uint8_t* peer::stunAppendXorMappedAddress12(uint8_t* nextAttribute, uint16_t bePort, const Address4& addr, const uint8_t* packet)
{
	nextAttribute[0] = 0; // type : uint16_t
	nextAttribute[1] = 0x20;
	nextAttribute[2] = 0; // length : uint16_t
	nextAttribute[3] = 0x08;

	nextAttribute[4] = 0;
	nextAttribute[5] = 0x01; // IPv4
	memcpy(&nextAttribute[6], &bePort, sizeof(bePort));
	memcpy(&nextAttribute[8], &addr, sizeof(addr));

	// mask data
	nextAttribute[6] ^= 0x21;
	nextAttribute[7] ^= 0x12;
	for (uint32_t ii = 8; ii < 12; ++ii)
		nextAttribute[ii] ^= packet[ii-4];

	return nextAttribute + 12;
}

uint8_t* peer::stunAppendXorMappedAddress24(uint8_t* nextAttribute, uint16_t bePort, const Address6& addr, const uint8_t* packet)
{
	nextAttribute[0] = 0; // type : uint16_t
	nextAttribute[1] = 0x20;
	nextAttribute[2] = 0; // length : uint16_t
	nextAttribute[3] = 0x14;

	nextAttribute[4] = 0;
	nextAttribute[5] = 0x02; // IPv6
	memcpy(&nextAttribute[6], &bePort, sizeof(bePort));
	memcpy(&nextAttribute[8], &addr, sizeof(addr));

	// mask data
	nextAttribute[6] ^= 0x21;
	nextAttribute[7] ^= 0x12;
	for (uint32_t ii = 8; ii < 24; ++ii)
		nextAttribute[ii] ^= packet[ii-4];

	return nextAttribute + 24;
}

uint8_t* peer::stunAppendFingerprint8(uint8_t* nextAttribute, const uint8_t* packetStart)
{
	nextAttribute[0] = 0x80; // type : uint16_t
	nextAttribute[1] = 0x28;
	nextAttribute[2] = 0; // length : uint16_t
	nextAttribute[3] = 0x04;

	crc32_state st;
	crc32_begin(&st);
	crc32_add(&st, packetStart, static_cast<uint32_t>(nextAttribute-packetStart));
	const uint32_t hash = endianToBig(crc32_end(&st));
	memcpy(&nextAttribute[4], &hash, sizeof(hash));

	return nextAttribute+8;
}

void peer::stunGenerateNewTransactionId(crypto::CryptoRandSource& rand, uint8_t packet[20])
{
	crandFill(&rand, &packet[8], 12);
}

bool peer::stunMatchesTransactionId(const uint8_t a[20], const uint8_t b[20])
{
	return 0 == memcmp(&a[8], &b[8], 12);
}

bool peer::stunIsBindingRequest(const uint8_t* packet, int npacket)
{
	if (npacket < 20)
		return false;

	if (packet[0] != 0x00 || packet[1] != 0x01)
		return false;

	// correct magic
	if (packet[4] != c_stunMagic[0] || packet[5] != c_stunMagic[1]
		|| packet[6] != c_stunMagic[2] || packet[7] != c_stunMagic[3])
		return false;

	return true;
}

bool peer::stunIsBindingResponse(const uint8_t* packet, int npacket)
{
	if (npacket <= 20)
		return false;

	// correct method+class
	if (packet[0] != 0x01 || packet[1] != 0x01)
		return false;

	// correct magic
	if (packet[4] != c_stunMagic[0] || packet[5] != c_stunMagic[1]
		|| packet[6] != c_stunMagic[2] || packet[7] != c_stunMagic[3])
		return false;

	return true;
}

bool peer::stunProcessBindingRequest(StunBindingRequest* req, const uint8_t* packet, uint32_t npacket)
{
	uint16_t messageLength;
	memcpy(&messageLength, &packet[2], sizeof(messageLength));
	messageLength = endianFromBig(messageLength);
	if (npacket != 20u+messageLength)
		return false;

	req->incomingUsername = 0;
	req->targetUsername = 0;
	req->controllingTiebreaker = 0;
	req->priority = 0;
	req->controlling = false;
	req->useCandidate = false;

	bool foundMac = false;

	// process attributes
	const uint8_t* attribute = packet+20;
	const uint8_t* attributeEnd = attribute+messageLength;
	while (attributeEnd-attribute >= 4)
	{
		uint16_t attrType;
		uint16_t attrLength;
		memcpy(&attrType, &attribute[0], sizeof(attrType));
		memcpy(&attrLength, &attribute[2], sizeof(attrLength));
		attrType = endianFromBig(attrType);
		attrLength = endianFromBig(attrLength);
		attribute += sizeof(attrType)+sizeof(attrLength);

		switch (attrType)
		{
		case 0x0006: // USERNAME
			if (attrLength != 16)
				return false;

			memcpy(&req->incomingUsername, &attribute[0], sizeof(req->incomingUsername));
			memcpy(&req->targetUsername, &attribute[8], sizeof(req->targetUsername));
			req->incomingUsername = endianFromBig(req->incomingUsername);
			req->targetUsername = endianFromBig(req->targetUsername);
			break;

		case 0x0008: // MESSAGE-INTEGRITY
			{
				// must be last attribute
				if (attribute+attrLength != attributeEnd)
				{
					// or the next attribute is FINGERPRINT
					if (attribute[attrLength] != 0x80 || attribute[attrLength+1] != 0x28)
						return false;
				}

				if (attrLength != 20)
					return false;

				hmac_sha1_state st;
				hmac_sha1_begin(&st, req->hmacKey, req->nhmacKey);
				hmac_sha1_add(&st, packet, static_cast<uint32_t>(attribute-packet-4)); // -4 to exclude this attribute's header

				uint8_t digest[hmac_sha1_state::DIGEST_SIZE];
				hmac_sha1_end(&st, digest);
				if (!hmac_sha1_digest_equal(digest, hmac_sha1_state::DIGEST_SIZE, attribute, 20))
					return false;

				foundMac = true;
			} break;

		case 0x0024: // ICE-PRIORITY
			if (attrLength != 4)
				return false;

			memcpy(&req->priority, attribute, sizeof(req->priority));
			req->priority = endianFromBig(req->priority);
			break;

		case 0x0025: // ICE-USE-CANDIDATE
			if (attrLength != 0)
				return false;

			req->useCandidate = true;
			break;

		case 0x8028: // FINGERPRINT
			{
				if (attribute+attrLength != attributeEnd)
					return false; // must be last attribute
				if (attrLength != 4)
					return false;

				crc32_state st;
				crc32_begin(&st);
				crc32_add(&st, packet, static_cast<uint32_t>(attribute-packet-4)); // -4 to exclude this atrribute's header
				crc32_end(&st);
				const uint32_t hash = endianToBig(crc32_end(&st));
				if (0 != memcmp(&hash, &attribute[0], 4))
					return false;
			
			} break;

		case 0x8029: // ICE-CONTROLLED
			// fallthrough
		case 0x802A: // ICE-CONTROLLING
			if (attrLength != 8)
				return false;

			req->controlling = (attrType == 0x802A);
			memcpy(&req->controllingTiebreaker, attribute, 8);
			req->controllingTiebreaker = endianFromBig(req->controllingTiebreaker);
			break;
		}

		attribute += attrLength;
	}

	// if we had an HMAC key, but no MAC, fail
	if (req->nhmacKey != 0 && !foundMac)
		return false;

	return (attribute == attributeEnd);
}

bool peer::stunProcessBindingResult(Address* address, uint16_t* bePort, const uint8_t* packet, uint32_t npacket, const uint8_t* request)
{
	// correct transaction id
	if (0 != memcmp(&packet[8], &request[8], 12))
		return false;

	uint16_t messageLength;
	memcpy(&messageLength, &packet[2], sizeof(messageLength));
	messageLength = endianFromBig(messageLength);
	if (npacket != 20u+messageLength)
		return false;

	// process attributes
	const uint8_t* attribute = packet + 20;
	const uint8_t* attributeEnd = attribute + messageLength;
	while  (attributeEnd-attribute >= 4)
	{
		uint16_t attrType;
		uint16_t attrLength;
		memcpy(&attrType, &attribute[0], sizeof(attrType));
		memcpy(&attrLength, &attribute[2], sizeof(attrLength));
		attrType = endianFromBig(attrType);
		attrLength = endianFromBig(attrLength);
		attribute += sizeof(attrType)+sizeof(attrLength);

		switch (attrType)
		{
		case 0x0020: // XOR-MAPPED-ADDRESS
		case 0x0001: // MAPPED-ADDRESS
			if (attrLength >= 8)
			{
				// client MUUST ignore data[0]
				uint8_t family = attribute[1];
				memcpy(bePort, &attribute[2], sizeof(*bePort));
				if (attrType == 0x0020)
				{
					*bePort ^= 0x1221;
				}

				switch (family)
				{
				case 0x01: // IPv4
					if (attrLength == 8)
					{
						address->family = AddressFamily::IPv4;
						memcpy(&address->u.v4, &attribute[4], 4);
						for (uint32_t ii = 0; attrType == 0x0020 && ii < 4; ++ii)
						{
							reinterpret_cast<uint8_t*>(&address->u.v4)[ii] ^= request[4+ii];
						}
						return true;
					}
					break;

				case 0x02: // IPv6
					if (attrLength == 20)
					{
						address->family = AddressFamily::IPv6;
						memcpy(&address->u.v6, &attribute[4], 16);
						for (uint32_t ii = 0; attrType == 0x0020 && ii < 16; ++ii)
						{
							reinterpret_cast<uint8_t*>(&address->u.v4)[ii] ^= request[4+ii];
						}

						return true;
					}
					break;
				}
			}
		}

		attribute += attrLength;
	}

	return false;
}

bool peer::stunProcessBindingResult(StunBindingResult* res, const uint8_t* packet, uint32_t npacket)
{
	uint16_t messageLength;
	memcpy(&messageLength, &packet[2], sizeof(messageLength));
	messageLength = endianFromBig(messageLength);
	if (npacket != 20u+messageLength)
		return false;

	memset(&res->address, 0, sizeof(res->address));
	res->priority = 0;
	res->bePort = 0;

	bool foundMac = false;

	// process attributes
	const uint8_t* attribute = packet+20;
	const uint8_t* attributeEnd = attribute+messageLength;
	while (attributeEnd-attribute >= 4)
	{
		uint16_t attrType;
		uint16_t attrLength;
		memcpy(&attrType, &attribute[0], sizeof(attrType));
		memcpy(&attrLength, &attribute[2], sizeof(attrLength));
		attrType = endianFromBig(attrType);
		attrLength = endianFromBig(attrLength);
		attribute += sizeof(attrType)+sizeof(attrLength);

		switch (attrType)
		{
		case 0x0008: // MESSAGE-INTEGRITY
			{
				// must be last attribute
				if (attribute+attrLength != attributeEnd)
				{
					// or the next attribute is FINGERPRINT
					if (attribute[attrLength] != 0x80 || attribute[attrLength+1] != 0x28)
						return false;
				}

				if (attrLength != 20)
					return false;

				hmac_sha1_state st;
				hmac_sha1_begin(&st, res->hmacKey, res->nhmacKey);
				hmac_sha1_add(&st, packet, static_cast<uint32_t>(attribute-packet-4)); // -4 to exclude this attribute's header

				uint8_t digest[hmac_sha1_state::DIGEST_SIZE];
				hmac_sha1_end(&st, digest);
				if (!hmac_sha1_digest_equal(digest, hmac_sha1_state::DIGEST_SIZE, attribute, 20))
					return false;

				foundMac = true;
			} break;

		case 0x0024: // ICE-PRIORITY
			if (attrLength != 4)
				return false;

			memcpy(&res->priority, attribute, sizeof(res->priority));
			res->priority = endianFromBig(res->priority);
			break;

		case 0x0020: // XOR-MAPPED-ADDRESS
		case 0x0001: // MAPPED-ADDRESS
			if (attrLength >= 8)
			{
				// client MUUST ignore data[0]
				uint8_t family = attribute[1];
				memcpy(&res->bePort, &attribute[2], sizeof(res->bePort));
				if (attrType == 0x0020)
				{
					res->bePort ^= 0x1221;
				}

				switch (family)
				{
				case 0x01: // IPv4
					if (attrLength == 8)
					{
						res->address.family = AddressFamily::IPv4;
						memcpy(&res->address.u.v4, &attribute[4], 4);
						for (uint32_t ii = 0; attrType == 0x0020 && ii < 4; ++ii)
						{
							reinterpret_cast<uint8_t*>(&res->address.u.v4)[ii] ^= packet[4+ii];
						}
					}
					break;

				case 0x02: // IPv6
					if (attrLength == 20)
					{
						res->address.family = AddressFamily::IPv6;
						memcpy(&res->address.u.v6, &attribute[4], 16);
						for (uint32_t ii = 0; attrType == 0x0020 && ii < 16; ++ii)
						{
							reinterpret_cast<uint8_t*>(&res->address.u.v4)[ii] ^= packet[4+ii];
						}
					}
					break;
				}
			}
			break;

		case 0x8028: // FINGERPRINT
			{
				if (attribute+attrLength != attributeEnd)
					return false; // must be last attribute
				if (attrLength != 4)
					return false;

				crc32_state st;
				crc32_begin(&st);
				crc32_add(&st, packet, static_cast<uint32_t>(attribute-packet-4)); // -4 to exclude this atrribute's header
				crc32_end(&st);
				const uint32_t hash = endianToBig(crc32_end(&st));
				if (0 != memcmp(&hash, &attribute[0], 4))
					return false;
			
			} break;
		}

		attribute += attrLength;
	}

	// if we had an HMAC key, but no MAC, fail
	if (res->nhmacKey != 0 && !foundMac)
		return false;

	// must have gotten a mapped address
	if (res->bePort == 0)
		return false;

	return (attribute == attributeEnd);
}

uint8_t* peer::stunAppendICEPriorityAttribute8(uint8_t* nextAttribute, uint32_t priority)
{
	nextAttribute[0] = 0; // type : uint16_t
	nextAttribute[1] = 0x24;
	nextAttribute[2] = 0; // length : uint16_t
	nextAttribute[3] = 0x04;
	priority = endianToBig(priority);
	memcpy(&nextAttribute[4], &priority, sizeof(priority));
	return nextAttribute + 8;
}

uint8_t* peer::stunAppendICEControlAttribute12(uint8_t* nextAttribute, bool controlling, uint64_t tieBreaker)
{
	nextAttribute[0] = 0x80; // type : uint16_t
	nextAttribute[1] = controlling ? 0x2A : 0x29;
	nextAttribute[2] = 0; // length : uint16_t
	nextAttribute[3] = 0x08;
	tieBreaker = endianToBig(tieBreaker);
	memcpy(&nextAttribute[4], &tieBreaker, sizeof(tieBreaker));
	return nextAttribute + 12;
}

uint8_t* peer::stunAppendICEUseCandidateAttribute4(uint8_t* nextAttribute)
{
	nextAttribute[0] = 0; // type : uint16_t
	nextAttribute[1] = 0x25;
	nextAttribute[2] = 0; // length : uint16_t
	nextAttribute[3] = 0;
	return nextAttribute + 4;
}
