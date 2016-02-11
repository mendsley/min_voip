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

#if TINY_PEER_ENABLE_ICE

#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <vector>
#if TINY_PLATFORM_WINDOWS
#include <eh.h>
#endif
#include <thread>
#include "peer/ice/candidate.h"
#include "peer/ice/foundation.h"
#include "peer/ice/priority.h"
#include "peer/ice/stun.h"
#include "tiny/endian.h"
#include "tiny/crypto/hmac.h"
#include "tiny/crypto/rand.h"
#include "tiny/net/adapter.h"
#include "tiny/net/address.h"
#include "tiny/net/resolve.h"
#include "tiny/net/socket.h"
#include "tiny/peer/mesh.h"
#include "tiny/peer/message.h"
#include "tiny/time.h"

using namespace tiny;
using namespace tiny::peer;
using namespace tiny::crypto;
using namespace tiny::net;

namespace
{
	struct SortByPriority
	{
		template<typename T>
		bool operator()(const T& a, const T& b)
		{
			return a.priority > b.priority;
		}
	};

	struct LocalCandidate : Candidate
	{
		Socket s;
		uint8_t stunBindingRequest[20+8];
		uint64_t nextStunAttempt;
		uint8_t totalStunAttempts;
		bool waitingOnServerReflexive;
		bool hasServerReflexiveAddress;
	};

	struct RemoteCandidate : Candidate
	{
		PlatformSocketAddr sockaddr;
	};

	struct CheckState
	{
		enum E
		{
			InProgress,
			Succeeded,
			Failed,
		};
	};

	struct ConnectivityCheck
	{
		uint64_t foundation;
		uint64_t priority;
		uint64_t timeout;
		CheckState::E state;
		uint8_t totalAttempts;
		uint8_t localCandidate;
		uint8_t remoteCandidate;
		bool nominated;

		uint8_t stunRequest[20+76];
		int nstunRequest;
	};

	struct peerconn
	{
		PlatformSocketAddr sockaddr;

		uint64_t id;
		uint64_t timeout;
		uint64_t recvTimeout;
		std::vector<RemoteCandidate> remoteCandidates;
		std::vector<ConnectivityCheck> connectivityChecks;
		std::vector<Message*> incoming;
		PeerState::E state;
		uint32_t sequence;
		uint8_t localCandidate;
		bool controlling;

		uint8_t keepAlive[20+52];
	};

	struct peerBindingRequest
	{
		Address address;
		uint64_t id;
		uint32_t peerReflexivePriority;
		uint16_t bePort;
		uint8_t localCandidate;
		bool useCandidate;
	};
}

static const int c_stunRetryStartupMS = 250; // Time to wait before retyring a STUN request
static const int c_stunRetryConnectedMS = 15000; // Time to wait before retyring a STUN request
static const int c_stunMaxAttempts = 5; // Maxmimum times to retry a STUN request
// Time to wait for a remote connectivity attempt to revive an otherwise unreachable peer
static const int c_peerCloseWaitMS = 3000;
// Time to wait before assuming there is a gap in traffic, and to trigger a
// STUN keep alive
static const int c_peerTrafficAbsentMS = 1000;
// Time to wait before assuming the connectiong is dead
static const int c_peerReceiveTimeout = 3000;

namespace
{
	class MeshICE : public IMesh
	{
	public:

		~MeshICE()
		{
			// destroy sockets on another thread (closesocket is blocking in VR)
			std::thread([](std::vector<LocalCandidate> localCandidates) {
				// destroy sockets
				for (size_t ii = 0, nn = localCandidates.size(); ii != nn; ++ii)
				{
					if (localCandidates[ii].s != InvalidSocket)
					{
						socketClose(localCandidates[ii].s);
					}
				}
			}, std::move(localCandidates)).detach();

			// free remaining messages
			for (size_t ii = 0, nn = peers.size(); ii != nn; ++ii)
			{
				peerconn* p = &peers[ii];
				for (size_t jj = 0, nn1 = p->incoming.size(); jj != nn1; ++jj)
				{
					messageRelease(p->incoming[jj]);
				}
			}

			crandDestroy(&rand);
		}

		bool create(uint32_t maxPeers, uint64_t localId, uint16_t port)
		{
			std::vector<Address> addresses;
			uint32_t nadapters = enumerateAdapters(nullptr, 0);
			if (0 == nadapters)
			{
				return false;
			}
			addresses.resize(nadapters);
			if (nadapters != enumerateAdapters(addresses.data(), nadapters))
			{
				return false;
			}

			if (maxPeers > 0xff)
			{
				return false;
			}

			this->localId = localId;
			this->peers.resize(maxPeers);
			for (size_t ii = 0, nn = maxPeers; ii != nn; ++ii)
			{
				this->peers[ii].state = PeerState::Invalid;
			}

			// build local candidates, along with sockets (establish port numbers)
			this->localCandidates.reserve(addresses.size());
			for (size_t ii = 0, nn = addresses.size(); ii != nn; ++ii)
			{
				if (!candidateShouldUseHostAddress(addresses[ii]))
					continue;

				uint16_t candidatePort = endianToBig(port);
				Socket s;
				if (!socketCreateUDP(&s, addresses[ii], &candidatePort))
				{
					return false;
				}

				LocalCandidate cand;
				cand.s = s;
				cand.priority = priorityForHostAddress(addresses[ii]);
				cand.foundation = foundationForHostAddress(addresses[ii]);
				cand.address = addresses[ii];
				cand.port = candidatePort;
				cand.waitingOnServerReflexive = false;
				cand.hasServerReflexiveAddress = false;
				this->localCandidates.push_back(cand);
			}

			this->localCandidates.shrink_to_fit();
			std::sort(this->localCandidates.begin(), this->localCandidates.end(), SortByPriority());
	
			this->state = MeshState::Created;
			this->timeFreqMS = timestampFrequency()/1000;
			this->peerSequence = 1;
			crandInit(&this->rand);
			return true;
		}

		virtual void destroy()
		{
			delete this;
		}

		virtual bool startSession(const char* stunHost, uint16_t stunPort)
		{
			if (state != MeshState::Created)
			{
				return false;
			}

			memset(&stunAddr4, 0, sizeof(stunAddr4));
			memset(&stunAddr6, 0, sizeof(stunAddr6));

			// resolve stun address
			Address stun4;
			Address stun6;
			if (!stunHost || !resolveHost(&stun4.u.v4, &stun6.u.v6, stunHost))
			{
				state = MeshState::Starting;
				return true;
			}

			const uint16_t bePort = endianToBig(stunPort);
			stun4.family = AddressFamily::IPv4;
			stun6.family = AddressFamily::IPv6;
			addressFrom(&stunAddr4, stun4, bePort);
			addressFrom(&stunAddr6, stun6, bePort);

			// generate and send initial STUN binding requests
			const uint64_t now = timestampCurrent();
			for (size_t ii = 0, nn = localCandidates.size(); ii != nn; ++ii)
			{
				uint8_t* attr = stunGenerateBindingRequest(rand, localCandidates[ii].stunBindingRequest, 8);
				stunAppendFingerprint8(attr, localCandidates[ii].stunBindingRequest);
				if (sendServerReflexiveBindingRequest(localCandidates[ii], now, c_stunRetryStartupMS))
				{
					localCandidates[ii].waitingOnServerReflexive = true;
					localCandidates[ii].totalStunAttempts = 1;
				}
			}

			state = MeshState::Starting;
			return true;
		}

		virtual void setSessionKey(const uint8_t* key, int nkey)
		{
			sessionKey.assign(key, key+nkey);
		}

		virtual void endSession()
		{
			if (state != MeshState::Invalid)
			{
				state = MeshState::Created;
			}
		}

		virtual uint32_t localAddressSize()
		{
			if (state != MeshState::Running)
			{
				return 0;
			}

			uint32_t sizeRequired = 1; // number of candidates
			for (size_t ii = 0, nn = localCandidates.size(); ii != nn; ++ii)
			{
				sizeRequired += candidatesEncodeLength(localCandidates[ii]);
			}
			for (size_t ii = 0, nn = remoteCandidates.size(); ii != nn; ++ii)
			{
				sizeRequired += candidatesEncodeLength(remoteCandidates[ii]);
			}

			return sizeRequired;
		}

		virtual void serializeLocalAddress(uint8_t* out)
		{
			out[0] = static_cast<uint8_t>(localCandidates.size() + remoteCandidates.size());
			++out;
			for (size_t ii = 0, nn = localCandidates.size(); ii != nn; ++ii)
			{
				out = candidatesEncode(out, localCandidates[ii]);
			}
			for (size_t ii = 0, nn = remoteCandidates.size(); ii != nn; ++ii)
			{
				out = candidatesEncode(out, remoteCandidates[ii]);
			}
		}

		virtual uint32_t connectToPeer(uint64_t remoteId, const uint8_t* remoteAddress, uint32_t nremoteAddress)
		{
			if (MeshState::Running != state || nremoteAddress == 0)
				return InvalidMeshPeer;

			if (localId == remoteId)
				return InvalidMeshPeer;

			// find an available beer
			uint32_t index = InvalidMeshPeer;
			for (size_t ii = 0, nn = peers.size(); ii != nn; ++ii)
			{
				if (peers[ii].state == PeerState::Invalid && index == InvalidMeshPeer)
				{
					index = static_cast<uint8_t>(ii);
				}
				else if (peers[ii].id == remoteId)
				{
					return InvalidMeshPeer;
				}
			}
			if (index == InvalidMeshPeer)
				return InvalidMeshPeer;

			peerconn* p = &peers[index];
			p->id = remoteId;

			// parse the remote address list
			uint8_t numCandidates = remoteAddress[0];
			p->remoteCandidates.resize(numCandidates);
			++remoteAddress;
			--nremoteAddress;
			for (uint8_t ii = 0; ii < numCandidates; ++ii)
			{
				uint32_t read = candidateDecode(&p->remoteCandidates[ii], remoteAddress, nremoteAddress);
				if (read == 0)
				{
					return InvalidMeshPeer;
				}

				remoteAddress += read;
				nremoteAddress -= read;
			}
			if (nremoteAddress != 0)
			{
				return InvalidMeshPeer;
			}

			// build socket addresses
			for (uint8_t ii = 0; ii < numCandidates; ++ii)
			{
				RemoteCandidate& c = p->remoteCandidates[ii];
				addressFrom(&c.sockaddr, c.address, c.port);
			}

			// are we the controlling entity for this peer
			p->controlling = localId > remoteId;

			// count candidate pairs
			uint32_t numCandidatePairs = 0;
			for (uint8_t ii = 0, nn0 = static_cast<uint8_t>(localCandidates.size()); ii != nn0; ++ii)
			{
				for (uint8_t jj = 0, nn1 = static_cast<uint8_t>(p->remoteCandidates.size()); jj != nn1; ++jj)
				{
					const LocalCandidate& local = localCandidates[ii];
					const Candidate& remote = p->remoteCandidates[jj];

					// compatible?
					if (local.address.family != remote.address.family)
						continue;

					++numCandidatePairs;
				}
			}

			// can't connect to a peer for which we don't have a valid candidate pair
			if (0 == numCandidatePairs)
			{
				return InvalidMeshPeer;
			}

			// generate candidate pairs
			p->connectivityChecks.reserve(numCandidatePairs);
			for (uint8_t ii = 0, nn0 = static_cast<uint8_t>(localCandidates.size()); ii != nn0; ++ii)
			{
				for (uint8_t jj = 0, nn1 = static_cast<uint8_t>(p->remoteCandidates.size()); jj != nn1; ++jj)
				{
					const LocalCandidate& local = localCandidates[ii];
					const Candidate& remote = p->remoteCandidates[jj];

					// compatible?
					if (local.address.family != remote.address.family)
						continue;

					p->connectivityChecks.resize(p->connectivityChecks.size() + 1);
					ConnectivityCheck* check = &p->connectivityChecks.back();
					initializeConnectivityCheck(check, p, ii, jj);
				}
			}

			// limit candidate pairs
			std::sort(p->connectivityChecks.begin(), p->connectivityChecks.end(), SortByPriority());
			if (p->connectivityChecks.size() > 50)
			{
				p->connectivityChecks.resize(50);
				p->connectivityChecks.shrink_to_fit();
			}

			const uint64_t now = timestampCurrent();

			// generate STUN request packets
			for (size_t ii = 0, nn = p->connectivityChecks.size(); ii != nn; ++ii)
			{
				ConnectivityCheck* check = &p->connectivityChecks[ii];
				uint8_t* attr = stunGenerateBindingRequest(rand, check->stunRequest, 72);
				attr = stunAppendUsernameAttribute20(attr, localId, remoteId);
				attr = stunAppendICEControlAttribute12(attr, p->controlling, localId);
				attr = stunAppendICEPriorityAttribute8(attr, priorityChangeTypePreference(localCandidates[check->localCandidate].priority, TypePreference::PeerReflexive));
				attr = stunAppendMessageIntegrityAttribute24(attr, check->stunRequest, sessionKey.data(), static_cast<uint32_t>(sessionKey.size()));
				attr = stunAppendFingerprint8(attr, check->stunRequest);
				check->nstunRequest = static_cast<int>(attr-check->stunRequest);
			}

			p->state = PeerState::Negotiating;
			p->timeout = 0xFFFFFFFFFFFFFFFF;
			p->sockaddr.size = 0;
			p->sequence = index|(peerSequence << 8);
			++peerSequence;

			// process any pending binding requests for this peer
			for (int32_t ii = static_cast<int32_t>(pendingPeerRequests.size()) - 1; ii >= 0; --ii)
			{
				const peerBindingRequest& request = pendingPeerRequests[ii];
				if (request.id == remoteId)
				{
					processPeerStunRequest(p, request, now);
					pendingPeerRequests.erase(pendingPeerRequests.begin() + ii);
				}
			}

			updatePeerNegotiation(p, now);
			return p->sequence;
		}

		virtual void disconnectPeer(uint32_t peerId)
		{
			const uint8_t index = static_cast<uint8_t>(peerId & 0xFF);
			if (index >= peers.size())
				return;

			struct peerconn* p = &peers[index];
			if (p->sequence == peerId)
				p->state = PeerState::Invalid;
		}

		virtual PeerState::E peerState(uint32_t peerId)
		{
			const uint8_t index = static_cast<uint8_t>(peerId & 0xFF);
			if (index >= peers.size() || peers[index].sequence != peerId)
				return PeerState::Invalid;

			return peers[index].state;
		}

		virtual void sendUnreliableDataToPeer(uint32_t peerId, const void* p, uint32_t n)
		{
			const uint8_t index = static_cast<uint8_t>(peerId & 0xFF);
			if (index >= peers.size())
				return;

			peerconn* peer = &peers[index];

			// if we're not connected, drop the packet.
			if (peer->sequence != peerId || peer->state != PeerState::Connected)
				return;

			uint8_t mac[hmac_sha1_state::DIGEST_SIZE];
			hmac_sha1_state st;
			hmac_sha1_begin(&st, sessionKey.data(), static_cast<uint32_t>(sessionKey.size()));
			hmac_sha1_add(&st, &localId, sizeof(localId));
			hmac_sha1_add(&st, p, n);
			hmac_sha1_end(&st, mac);

			const uint8_t packetPrefix = 0xC0;

			ConstBuffer b[3];
			b[0].p = &packetPrefix;
			b[0].len = 1;
			b[1].p = static_cast<const uint8_t*>(p);
			b[1].len = n;
			b[2].p = mac;
			b[2].len = sizeof(mac);

			socketSendTo(localCandidates[peer->localCandidate].s, b, 3, peer->sockaddr);
			peer->timeout = timestampCurrent() + c_peerTrafficAbsentMS*timeFreqMS;
		}

		virtual bool receive(uint32_t peer, Message*** messages, uint32_t* nmessages)
		{
			const uint8_t index = static_cast<uint8_t>(peer & 0xFF);
			if (index >= peers.size())
				return false;

			peerconn* p = &peers[index];
			if (p->sequence != peer)
				return false;
			if (p->state != PeerState::Connected)
				return false;
			if (p->incoming.empty())
				return false;

			*messages = p->incoming.data();
			*nmessages = static_cast<uint32_t>(p->incoming.size());
			return true;
		}

		virtual MeshState::E update()
		{
			MeshState::E currentState = state;
			switch (currentState)
			{
			case MeshState::Starting:
				return updateStarting();

			case MeshState::Running:
				updateRunning();
				return currentState;
			}

			return currentState;
		}

	private:

		MeshState::E updateStarting()
		{
			PlatformSocketAddr addr;
			uint8_t incoming[128];

			const uint64_t now = timestampCurrent();

			// update STUN requests
			bool stillWaiting = false;
			for (size_t ii = 0, nn = localCandidates.size(); ii != nn; ++ii)
			{
				LocalCandidate& c = localCandidates[ii];
				if (c.waitingOnServerReflexive)
				{
					// is data available for this request
					int32_t read = socketRecvFrom(c.s, incoming, sizeof(incoming), &addr);
					if (read >= 20)
					{
						// ensure response is from STUN host
						bool validAddress = false;
						if (addr.size == stunAddr4.size)
						{
							validAddress = (0 == memcmp(&addr.storage, stunAddr4.storage, addr.size));
						}
						else if (addr.size == stunAddr6.size)
						{
							validAddress = (0 == memcmp(&addr.storage, stunAddr6.storage, addr.size));
						}

						if (validAddress)
						{
							if (stunIsBindingResponse(incoming, read))// && stunProcessBindingResult(&serverReflexive.address, &serverReflexive.port, incoming, read, c.stunBindingRequest))
							{
								StunBindingResult res;
								res.nhmacKey = 0;
								if (stunProcessBindingResult(&res, incoming, read))
								{
									c.waitingOnServerReflexive = false;
									c.hasServerReflexiveAddress = true;
									c.nextStunAttempt = now + 500*timeFreqMS;

									Candidate serverReflexive;
									serverReflexive.address = res.address;
									serverReflexive.port = res.bePort;
									serverReflexive.foundation = foundationForServerReflexiveAddress(c.foundation, serverReflexive.address);
									serverReflexive.priority = priorityChangeTypePreference(c.priority, TypePreference::ServerReflexive);
									remoteCandidates.push_back(serverReflexive);
								}
							}
						}
					}
					else if (read < 0 && !socketOperationWouldHaveBlocked())
					{
						c.waitingOnServerReflexive = false;
					}

					// if we're still waiting, see if we need to resend the request
					if (c.waitingOnServerReflexive)
					{
						if (now > c.nextStunAttempt)
						{
							++c.totalStunAttempts;
							if (c.totalStunAttempts > c_stunMaxAttempts || !sendServerReflexiveBindingRequest(c, now, c_stunRetryStartupMS))
							{
								c.waitingOnServerReflexive = false;
							}
						}

						// if we're still waiting, then we need to stay in this state
						if (c.waitingOnServerReflexive)
						{
							stillWaiting = true;
						}
					}
				}
			}

			if (!stillWaiting)
			{
				std::sort(remoteCandidates.begin(), remoteCandidates.end(), SortByPriority());
				state = MeshState::Running;
				return MeshState::StartComplete;
			}
	
			return MeshState::Starting;
		}

		bool sendServerReflexiveBindingRequest(LocalCandidate& c, uint64_t now, uint64_t nextRequestMS)
		{
			const PlatformSocketAddr* addr;
			switch (c.address.family)
			{
			case AddressFamily::IPv4:
				addr = &stunAddr4;
				break;

			case AddressFamily::IPv6:
				addr = &stunAddr6;
				break;

			default:
				return false;
			}

			ConstBuffer b;
			b.len = sizeof(c.stunBindingRequest);
			b.p = c.stunBindingRequest;
			socketSendTo(c.s, &b, 1, *addr);

			c.nextStunAttempt = now + nextRequestMS*timeFreqMS;
			return true;
		}

		void updatePeerNegotiation(peerconn* p, uint64_t now)
		{
			bool complete = true;
			bool pendingRequests = false;

			// (re)send binding requests
			for (size_t ii = 0, nn = p->connectivityChecks.size(); ii != nn; ++ii)
			{
				ConnectivityCheck& check = p->connectivityChecks[ii];
				if (check.state == CheckState::InProgress)
				{
					complete = false;

					if (check.timeout < now)
					{
						++check.totalAttempts;
						if (check.totalAttempts > c_stunMaxAttempts)
						{
							check.state = CheckState::Failed;
							continue;
						}

						ConstBuffer buf;
						buf.p = check.stunRequest;
						buf.len = check.nstunRequest;

						const RemoteCandidate& remote = p->remoteCandidates[check.remoteCandidate];
						if (!socketSendTo(localCandidates[check.localCandidate].s
							, &buf, 1, remote.sockaddr))
						{
							if (!socketOperationWouldHaveBlocked())
							{
								// failed
								check.state = CheckState::Failed;
								continue;
							}
						}

						check.timeout = now + c_stunRetryStartupMS*timeFreqMS;
					}
				}

				if (check.state != CheckState::Failed)
				{
					pendingRequests = true;
				}
			}

			// we've completed our checklist
			if (complete)
			{
				// we've failed our check list, schedule the peer to be marked as invalid.
				// an incoming check can revive the peer it is behind a symmetric NAT
				if (!pendingRequests && p->timeout == 0xFFFFFFFFFFFFFFFF)
				{
					p->timeout = now + c_peerCloseWaitMS*timeFreqMS;
				}
				else if (p->controlling)
				{
					// find the highest priority candidate that successfully completed and nominate it
					for (size_t ii = 0, nn = p->connectivityChecks.size(); ii != nn; ++ii)
					{
						ConnectivityCheck* check = &p->connectivityChecks[ii];
						if (check->state == CheckState::Succeeded)
						{
							// remove all other checks
							p->connectivityChecks.erase(p->connectivityChecks.begin(), p->connectivityChecks.begin() + ii);
							p->connectivityChecks.erase(p->connectivityChecks.begin() + ii + 1, p->connectivityChecks.end());

							// rebuild stun request
							check = &p->connectivityChecks[0];
							check->nominated = true;
							check->state = CheckState::InProgress;
							check->totalAttempts = 0;
							check->timeout = 0;

							uint8_t* attr = stunGenerateBindingRequest(rand, check->stunRequest, 76);
							attr = stunAppendUsernameAttribute20(attr, localId, p->id);
							attr = stunAppendICEControlAttribute12(attr, p->controlling, localId);
							attr = stunAppendICEPriorityAttribute8(attr, priorityChangeTypePreference(localCandidates[check->localCandidate].priority, TypePreference::PeerReflexive));
							attr = stunAppendICEUseCandidateAttribute4(attr);
							attr = stunAppendMessageIntegrityAttribute24(attr, check->stunRequest, sessionKey.data(), static_cast<uint32_t>(sessionKey.size()));
							attr = stunAppendFingerprint8(attr, check->stunRequest);
							check->nstunRequest = static_cast<int>(attr-check->stunRequest);
							break;
						}
					}
				}
			}
		}

		void initializeConnectivityCheck(ConnectivityCheck* out, peerconn* p, uint8_t localIndex, uint8_t remoteIndex)
		{
			const Candidate& local = localCandidates[localIndex];
			const Candidate& remote = p->remoteCandidates[remoteIndex];

			out->foundation = (static_cast<uint64_t>(local.foundation) << 32) | static_cast<uint64_t>(remote.foundation);
			uint32_t priorityG = local.priority;
			uint32_t priorityD = remote.priority;
			if (!p->controlling)
			{
				priorityG = remote.priority;
				priorityD = local.priority;
			}

			out->priority = (static_cast<uint64_t>(std::min(priorityG, priorityD)) << 32)
				+ (static_cast<uint64_t>(std::min(priorityG, priorityD)) << 1)
				+ (priorityG > priorityD)
				;

			out->localCandidate = localIndex;
			out->remoteCandidate = remoteIndex;
			out->timeout = 0;
			out->totalAttempts = 0;
			out->nominated = false;
			out->state = CheckState::InProgress;
		}

		void processPeerStunRequest(peerconn* p, const peerBindingRequest& request, uint64_t now)
		{
			if (p->state != PeerState::Negotiating)
			{
				return;
			}

			// find the remote candidate
			uint8_t remoteIndex = 0xff;
			for (size_t ii = 0, nn = p->remoteCandidates.size(); ii != nn; ++ii)
			{
				const RemoteCandidate& c = p->remoteCandidates[ii];
				if (addressIsEqual(c.address, request.address))
				{
					if (c.port == request.bePort)
					{
						remoteIndex = static_cast<uint8_t>(ii);
						break;
					}
				}
			}

			// if this is this a new remote candidate, create a peer reflexive candidate
			// and use that
			if (remoteIndex == 0xff)
			{
				remoteIndex = static_cast<uint8_t>(p->remoteCandidates.size());
				p->remoteCandidates.resize(remoteIndex + 1);
				RemoteCandidate& c = p->remoteCandidates.back();

				c.priority = request.peerReflexivePriority;
				c.foundation = foundationForPeerReflexiveAddress(request.address);
				c.address = request.address;
				c.port = request.bePort;

				addressFrom(&c.sockaddr, request.address, request.bePort);
			}

			// check for the pair on the connectivity list
			ConnectivityCheck* check = nullptr;
			for (size_t ii = 0, nn = p->connectivityChecks.size(); ii != nn; ++ii)
			{
				ConnectivityCheck* candidate = &p->connectivityChecks[ii];
				if (candidate->localCandidate == request.localCandidate && candidate->remoteCandidate == remoteIndex)
				{
					check = candidate;
					break;
				}
			}

			if (check)
			{
				switch (check->state)
				{
				case CheckState::Failed:
					check->state = CheckState::InProgress;
					p->timeout = 0xFFFFFFFFFFFFFFFF;

					// fallthrough
				case CheckState::InProgress:
					check->timeout = 0;
					check->totalAttempts = 0;
					break;
				}
			}
			else
			{
				// if the pair is not on the check list, insert it and start a new check
				ConnectivityCheck newCheck;
				initializeConnectivityCheck(&newCheck, p, request.localCandidate, remoteIndex);
				std::vector<ConnectivityCheck>::iterator it = std::lower_bound(p->connectivityChecks.begin(), p->connectivityChecks.end(), newCheck, SortByPriority()); 
				check = &(*p->connectivityChecks.insert(it, newCheck));

				// generate STUN request packet
				uint8_t* attr = stunGenerateBindingRequest(rand, check->stunRequest, 72);
				attr = stunAppendUsernameAttribute20(attr, localId, p->id);
				attr = stunAppendICEControlAttribute12(attr, p->controlling, localId);
				attr = stunAppendICEPriorityAttribute8(attr, priorityChangeTypePreference(localCandidates[check->localCandidate].priority, TypePreference::PeerReflexive));
				attr = stunAppendMessageIntegrityAttribute24(attr, check->stunRequest, sessionKey.data(), static_cast<uint32_t>(sessionKey.size()));
				attr = stunAppendFingerprint8(attr, check->stunRequest);
				check->nstunRequest = static_cast<int>(attr-check->stunRequest);
			}

			if (!p->controlling && request.useCandidate)
			{
				check->nominated = true;
				const RemoteCandidate& candidate = p->remoteCandidates[check->remoteCandidate];
				addressFrom(&p->sockaddr, candidate.address, candidate.port);

				// build keep-alive packet
				uint8_t* attr = stunGenerateBindingRequest(rand, p->keepAlive, 52);
				attr = stunAppendUsernameAttribute20(attr, localId, p->id);
				attr = stunAppendMessageIntegrityAttribute24(attr, p->keepAlive, sessionKey.data(), static_cast<uint32_t>(sessionKey.size()));
				attr = stunAppendFingerprint8(attr, p->keepAlive);

				p->localCandidate = check->localCandidate;
				p->state = PeerState::Connected;
				p->connectivityChecks.clear();
				p->connectivityChecks.shrink_to_fit();
			}
		}

		void updateRunning()
		{
			const uint64_t now = timestampCurrent();

			uint8_t incoming[2048];
			PlatformSocketAddr sockaddr;

			// keep STUN binding requests alive
			for (size_t ii = 0, nn = localCandidates.size(); ii != nn; ++ii)
			{
				LocalCandidate& c = localCandidates[ii];
				if (c.hasServerReflexiveAddress && now > c.nextStunAttempt)
				{
					sendServerReflexiveBindingRequest(c, now, c_stunRetryConnectedMS);
				}
			}

			// clear incoming arrays on all peers
			for (size_t ii = 0, nn = peers.size(); ii != nn; ++ii)
			{
				peerconn* p = &peers[ii];
				for (size_t jj = 0, nn1 = p->incoming.size(); jj != nn1; ++jj)
				{
					messageRelease(p->incoming[jj]);
				}

				p->incoming.clear();
			}

			// process incoming messages
			for (size_t ii = 0, nn = localCandidates.size(); ii != nn; ++ii)
			{
				LocalCandidate& c = localCandidates[ii];
				if (c.s == InvalidSocket)
					continue;

				for (uint32_t readAttempt = 0; readAttempt < 10; ++readAttempt)
				{
					const int32_t read = socketRecvFrom(c.s, incoming, sizeof(incoming), &sockaddr);
					if (read < 0)
						break;

					// if this data is coming from our STUN server, simply ignore it for now
					if (sockaddr.size == stunAddr4.size && 0 == memcmp(&sockaddr.storage, &stunAddr4.storage, sockaddr.size))
						continue;
					else if (sockaddr.size == stunAddr6.size && 0 == memcmp(&sockaddr.storage, &stunAddr6.storage, sockaddr.size))
						continue;

					// is this a STUN packet?
					if (stunIsBindingRequest(incoming, read))
					{
						StunBindingRequest req;
						req.hmacKey = sessionKey.data();
						req.nhmacKey = static_cast<uint32_t>(sessionKey.size());
						if (stunProcessBindingRequest(&req, incoming, read))
						{
							// if this request wasn't directed at us, discard it
							if (req.targetUsername != localId)
								continue;

							peerBindingRequest bindingRequest;
							bindingRequest.id = req.incomingUsername;
							bindingRequest.peerReflexivePriority = req.priority;
							bindingRequest.localCandidate = static_cast<uint8_t>(ii);
							bindingRequest.useCandidate = req.useCandidate;
							addressTo(&bindingRequest.address, &bindingRequest.bePort, sockaddr);

							// send a result to the requesting party
							uint8_t* attr = nullptr;
							int npacket;
							switch (bindingRequest.address.family)
							{
							case AddressFamily::IPv4:
								npacket = 20+56;
								attr = stunGenerateBindingResponse(stunResponse, 44, incoming);
								attr = stunAppendXorMappedAddress12(attr, bindingRequest.bePort, bindingRequest.address.u.v4, stunResponse);
								break;
							case AddressFamily::IPv6:
								npacket = 20+68;
								attr = stunGenerateBindingResponse(stunResponse, 56, incoming);
								attr = stunAppendXorMappedAddress24(attr, bindingRequest.bePort, bindingRequest.address.u.v6, stunResponse);
								break;
							}
							if (attr)
							{
								attr = stunAppendMessageIntegrityAttribute24(attr, stunResponse, sessionKey.data(), static_cast<uint32_t>(sessionKey.size()));
								attr = stunAppendFingerprint8(attr, stunResponse);

								ConstBuffer b;
								b.p = stunResponse;
								b.len = static_cast<uint32_t>(attr-stunResponse);
								if (socketSendTo(c.s, &b, 1, sockaddr))
								{ 
									// find the peer for this request
									peerconn* p = nullptr;
									for (size_t ii = 0, nn = peers.size(); ii != nn; ++ii)
									{
										peerconn* candidate = &peers[ii];
										if (candidate->state != PeerState::Invalid && candidate->id == bindingRequest.id)
										{
											p = candidate;
											break;
										}
									}
						
									if (p)
									{
										p->recvTimeout = now + c_peerReceiveTimeout*timeFreqMS;
										processPeerStunRequest(p, bindingRequest, now);
									}
									else
									{
										// got a request for a peer we don't know about yet, queue it for later
										pendingPeerRequests.push_back(bindingRequest);
									}
								}
							}
						}
					}
					else if (stunIsBindingResponse(incoming, read))
					{
						StunBindingResult result;
						result.hmacKey = sessionKey.data();
						result.nhmacKey = static_cast<uint32_t>(sessionKey.size());
						if (stunProcessBindingResult(&result, incoming, read))
						{
							// find the peer that generated this request
							peerconn* p = nullptr;
							uint8_t remoteIndex = 0xff;
							for (size_t ii = 0, nn1 = peers.size(); p == nullptr && ii != nn1; ++ii)
							{
								peerconn* candidate = &peers[ii];
								for (uint8_t jj = 0, nn2 = static_cast<uint8_t>(candidate->connectivityChecks.size()); jj != nn2; ++jj)
								{
									if (stunMatchesTransactionId(incoming, candidate->connectivityChecks[jj].stunRequest))
									{
										p = candidate;
										remoteIndex = jj;
										break;
									}
								}
							}
					
							if (p)
							{
								p->recvTimeout = now + c_peerReceiveTimeout*timeFreqMS;
								if (p->state == PeerState::Negotiating)
								{
									// TODO: downgrade open/restrited/moderate NAT here

									ConnectivityCheck* check = &p->connectivityChecks[remoteIndex];
									check->state = CheckState::Succeeded;
									if (p->controlling && check->nominated)
									{
										const RemoteCandidate& candidate = p->remoteCandidates[check->remoteCandidate];
										addressFrom(&p->sockaddr, candidate.address, candidate.port);

										// build keep-alive packet
										uint8_t* attr = stunGenerateBindingRequest(rand, p->keepAlive, 52);
										attr = stunAppendUsernameAttribute20(attr, localId, p->id);
										attr = stunAppendMessageIntegrityAttribute24(attr, p->keepAlive, sessionKey.data(), static_cast<uint32_t>(sessionKey.size()));
										attr = stunAppendFingerprint8(attr, p->keepAlive);

										p->localCandidate = check->localCandidate;
										p->state = PeerState::Connected;
										p->connectivityChecks.clear();
										p->connectivityChecks.shrink_to_fit();
									}
								}
								else
								{
								}
							}
						}
					}
					// media packet
					else if (read > 21 && (incoming[0] & 0xC0) == 0xC0)
					{
						// locate peer
						peerconn* p = nullptr;
						for (size_t ii = 0, nn = peers.size(); ii != nn; ++ii)
						{
							peerconn* candidate = &peers[ii];
							if (candidate->state != PeerState::Invalid && candidate->sockaddr.size == sockaddr.size)
							{
								if (0 == memcmp(&sockaddr.storage, &candidate->sockaddr.storage, sockaddr.size))
								{
									p = candidate;
									break;
								}
							}
						}

						if (p != nullptr)
						{
							// verify hmac
							hmac_sha1_state st;
							hmac_sha1_begin(&st, sessionKey.data(), static_cast<uint32_t>(sessionKey.size()));
							hmac_sha1_add(&st, &p->id, sizeof(p->id));
							hmac_sha1_add(&st, &incoming[1], read - 21);

							uint8_t mac[hmac_sha1_state::DIGEST_SIZE];
							hmac_sha1_end(&st, mac);
							if (hmac_sha1_digest_equal(mac, hmac_sha1_state::DIGEST_SIZE, &incoming[read-20], 20))
							{
								// valid packet incoming[1, read-20)
								Message* msg = messageAlloc(read-21);
								memcpy(msg->data, &incoming[1], read-21);
								p->incoming.push_back(msg);

								p->recvTimeout = now + c_peerReceiveTimeout*timeFreqMS;
							}
						}
					}
				}
			}

			// update peers
			for (size_t ii = 0, nn = peers.size(); ii != nn; ++ii)
			{
				peerconn* p = &peers[ii];
				switch (p->state)
				{
				case PeerState::Negotiating:
					{
						updatePeerNegotiation(p, now);
						if (now > p->timeout)
						{
							p->state = PeerState::Invalid;
						}
					} break;

				case PeerState::Connected:
					// need to send keep-alive?
					if (now > p->timeout)
					{
						ConstBuffer b;
						b.p = p->keepAlive;
						b.len = sizeof(p->keepAlive);
						socketSendTo(localCandidates[p->localCandidate].s, &b, 1, p->sockaddr);
						p->timeout = now + c_peerTrafficAbsentMS*timeFreqMS;
					}
					if (now > p->recvTimeout)
					{
						p->state = PeerState::Invalid;
					}
					break;
				}
			}
		}

		uint64_t localId;
		uint64_t timeFreqMS;
		MeshState::E state;

		PlatformSocketAddr stunAddr4;
		PlatformSocketAddr stunAddr6;

		uint32_t peerSequence;
		uint8_t stunResponse[20+56];

		CryptoRandSource rand;
		std::vector<peerconn> peers;
		std::vector<uint8_t> sessionKey;
		std::vector<LocalCandidate> localCandidates;
		std::vector<Candidate> remoteCandidates;
		std::vector<peerBindingRequest> pendingPeerRequests;
	};
}

IMesh* tiny::peer::meshCreateICE(uint32_t maxPeers, uint64_t localId, uint16_t port)
{
	MeshICE* m = new MeshICE;
	if (!m->create(maxPeers, localId, port))
	{
		m->destroy();
		return nullptr;
	}

	return m;
}

#else
IMesh* tiny::peer::meshCreateICE(uint32_t /*maxPeers*/, uint64_t /*localId*/, uint16_t /*port*/)
{
	return nullptr;
}
#endif // TINY_PEER_ENABLE_ICE
