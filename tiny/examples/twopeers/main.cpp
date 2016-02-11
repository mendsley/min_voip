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

#include <tiny/platform.h>
#include <tiny/peer/mesh.h>
#include <tiny/peer/message.h>
#include <tiny/crypto/rand.h>
#include <stdint.h>
#include <stdio.h>
#include <thread>
#include <future>
#include <vector>

using namespace tiny;
using namespace tiny::crypto;
using namespace tiny::peer;

static void meshThread(const uint8_t* key, uint32_t nkey, uint64_t id, std::future<std::vector<uint8_t>> incomingAddress, std::promise<std::vector<uint8_t>>& outgoingAddress)
{
	IMesh* m = meshCreateICE(7, id, 0);
	if (!m)
	{
		return;
	}

	m->setSessionKey(key, nkey);

	if (!m->startSession("stun.l.google.com", 19302))
	{
		return;
	}

	// wait for local address
	for (bool waiting = true; waiting;)
	{
		switch (m->update())
		{
		case MeshState::StartComplete:
			waiting = false;
			break;
		case MeshState::Invalid:
			m->destroy();
			return;
		}
	}

	// get local address
	uint32_t localAddrSize = m->localAddressSize();
	if (!localAddrSize)
	{
		m->destroy();
		return;
	}

	std::vector<uint8_t> localAddress(localAddrSize);
	m->serializeLocalAddress(localAddress.data());
	outgoingAddress.set_value(std::move(localAddress));

	std::vector<uint8_t> remoteAddr = incomingAddress.get();
	uint32_t peer = m->connectToPeer((id+1)%2, remoteAddr.data(), static_cast<uint32_t>(remoteAddr.size()));
	if (peer == InvalidMeshPeer)
	{
		m->destroy();
		return;
	}

	// wait for peer to connect
	for (bool waiting = true; waiting;)
	{
		if (MeshState::Invalid == m->update())
		{
			m->destroy();
			return;
		}

		switch (m->peerState(peer))
		{
		case PeerState::Connected:
			waiting = false;
			break;
		case PeerState::Invalid:
			m->destroy();
			return;
		}
	}

	uint32_t packetsReceived = 0;
	uint32_t packetsAcked = 0;
	while (m->peerState(peer) == PeerState::Connected)
	{
		if (MeshState::Invalid == m->update())
		{
			m->destroy();
			return;
		}

		char data[9] = "hello";
		memcpy(&data[5], &packetsReceived, sizeof(packetsReceived));

		m->sendUnreliableDataToPeer(peer, data, sizeof(data));

		Message** messages;
		uint32_t nmessages;
		if (m->receive(peer, &messages, &nmessages))
		{
			for (uint32_t ii = 0; ii < nmessages; ++ii)
			{
				uint32_t acked;
				memcpy(&acked, messages[ii]->data+messages[ii]->ndata-4, 4);
				if (acked > packetsAcked)
				{
					packetsAcked = acked;
				}
				printf("%p Receieved packet %d from 0x%08X: %.*s (%d)\n", m, packetsReceived+ii, peer, messages[ii]->ndata - 4, messages[ii]->data, acked);
			}

			packetsReceived += nmessages;
			if (packetsReceived >= 10 && packetsAcked >= 10)
			{
				break;
			}
		}
	}

	m->destroy();
}

int main()
{
	if (!platformStartup())
	{
		return -1;
	}
	
	uint8_t key[20];

	CryptoRandSource rand;
	crandInit(&rand);
	crandFill(&rand, key, sizeof(key));
	crandDestroy(&rand);
	

	std::promise<std::vector<uint8_t>> addr1;
	std::promise<std::vector<uint8_t>> addr2;
	std::thread thr([&key, &addr1, &addr2]() {
		meshThread(key, sizeof(key), 1, addr1.get_future(), addr2);
	});
	meshThread(key, sizeof(key), 0, addr2.get_future(), addr1);
	thr.join();

	platformShutdown();
}
