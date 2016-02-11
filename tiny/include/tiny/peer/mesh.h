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

#ifndef TINY_PEER__MESH_H
#define TINY_PEER__MESH_H

#include <stdint.h>

namespace tiny
{
	namespace peer
	{
		struct Message;

		struct MeshState
		{
			enum E
			{
				Created,
				Starting,
				// same as Running, but only on initial edge trigger of state.
				StartComplete,
				Running,
				Invalid,
			};
		};

		struct PeerState
		{
			enum E
			{
				Negotiating,
				Connected,
				Invalid,
			};
		};

		static const uint32_t InvalidMeshPeer = 0xFFFFFFFF;

		// peer-to-peer mesh interface
		class IMesh
		{
		public:
			// destroys an existing peer mesh
			virtual void destroy() = 0;

			// update peer mesh state. dispatches data to peers that may be retreived
			// with `receive'
			virtual MeshState::E update() = 0;

			// starts processing a session with the supplied session key. If
			// `stunHost' is not `nullptr' then the mesh will use
			// stun:`stunHost':`stunPort' to obtain server reflexive candidates
			// to aid in NAT traversal.
			virtual bool startSession(const char* stunHost, uint16_t stunPort) = 0;

			// sets the session key for the peer-to-peer session. any client
			// connecting to this mesh will need to have the same key set
			// before `connectToPeer' is called. key distribution must
			// use a separate signaling mechanism; tinypeer does not provide
			// this signaling. this function may be called at any time before
			// the local peer attempts to connect to remote peers, and before
			// remote peers attempt to connect
			virtual void setSessionKey(const uint8_t* key, int nkey) = 0;

			// end a running peer-to-peer session
			virtual void endSession() = 0;

			// acquires the number of bytes needed to serialize the local
			// address of the peer mesh. this address is platform specific
			// and may map to multiple host or reflexive addresses. this
			// function will return 0 until `update' returns a state
			// of `MeshState::StartComplete' or `MeshState::Running'
			virtual uint32_t localAddressSize() = 0;

			// serialize the local address of the peer mesh to a binary stream.
			// this will write exactly `localAddressSize` to the pointer
			// `out'
			virtual void serializeLocalAddress(uint8_t* out) = 0;

			// start the process of connecting to a peer. returns an index
			// to the connection
			virtual uint32_t connectToPeer(uint64_t remoteId
				, const uint8_t* remoteAddress, uint32_t nremoteAddress) = 0;

			// disconnect from a peer. does not signal the peer, simply
			// stops processing incoming packets
			virtual void disconnectPeer(uint32_t peer) = 0;

			// get the connection status of a peer
			virtual PeerState::E peerState(uint32_t peer) = 0;

			// send unreliable data to a connection
			virtual void sendUnreliableDataToPeer(uint32_t peer
				, const void* p, uint32_t n) = 0;

			// receive data from a peer connection. fills `messages', and
			// `nmessages' and returns `true' if data is available. otherwise returns
			// `false'. the mesh itself will release all messages internally on the next
			// update.
			virtual bool receive(uint32_t peer , Message*** messages
				, uint32_t* nmessages) = 0;

		protected:
			virtual ~IMesh() = 0;
		};

		// create a new peer-to-peer mesh that supports up to `maxPeers'
		// remote connections and runs on the port `port'. If the platform
		// supports it, setting `port' to 0 allows the platform to operate
		// on an arbitrary port.
		IMesh* meshCreateICE(uint32_t maxPeers, uint64_t localId, uint16_t port);
	}
}

#endif // TINY_PEER__MESH_H
