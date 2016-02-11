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

#include "peer/config.h"
#if TINY_PEER_ENABLE_NULL

#include <stdint.h>
#include "tiny/peer/mesh.h"

using namespace tiny;
using namespace tiny::peer;

Mesh* peer::meshCreate(uint32_t maxPeers, uint64_t localId, uint16_t port)
{
	return nullptr;
}

void peer::meshDestroy(Mesh* m)
{
}

MeshState::E peer::meshUpdate(Mesh* m)
{
	return MeshState::Invalid;
}

bool peer::meshStartSession(Mesh* m, const char* stunHost, uint16_t stunPort)
{
	return false;
}

void peer::meshSetSessionKey(Mesh* m, const uint8_t* key, int nkey)
{
}

void peer::meshEndSession(Mesh* m)
{
}

uint32_t peer::meshLocalAddressSize(Mesh* m)
{
	return 0;
}

void peer::meshSerializeLocalAddress(Mesh* m, uint8_t* out)
{
}

uint32_t peer::meshConnectToPeer(Mesh* m, uint64_t remoteId, const uint8_t* remoteAddress, uint32_t nremoteAddress)
{
	return InvalidMeshPeer;
}

void peer::meshDisconnectPeer(Mesh* m, uint32_t peer)
{
}

PeerState::E peer::meshPeerState(Mesh* m, uint32_t peer)
{
	return PeerState::Invalid;
}

void peer::meshSendUnreliableDataToPeer(Mesh* m, uint32_t peer, const void* p, uint32_t n)
{
}

bool peer::meshReceive(Mesh* m, uint32_t peer, Message*** messages, uint32_t* nmessages)
{
	return false;
}

#endif // TINY_PEER_ENABLE_NULL
