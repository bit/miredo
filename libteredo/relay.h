/*
 * relay.h - Teredo relay peers list declaration
 * $Id$
 *
 * See "Teredo: Tunneling IPv6 over UDP through NATs"
 * for more information
 */

/***********************************************************************
 *  Copyright (C) 2004 Remi Denis-Courmont.                            *
 *  This program is free software; you can redistribute and/or modify  *
 *  it under the terms of the GNU General Public License as published  *
 *  by the Free Software Foundation; version 2 of the license.         *
 *                                                                     *
 *  This program is distributed in the hope that it will be useful,    *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
 *  See the GNU General Public License for more details.               *
 *                                                                     *
 *  You should have received a copy of the GNU General Public License  *
 *  along with this program; if not, you can get it from:              *
 *  http://www.gnu.org/copyleft/gpl.html                               *
 ***********************************************************************/

#ifndef LIBTEREDO_RELAY_H
# define LIBTEREDO_RELAY_H

# include <sys/time.h> // struct timeval

# include <libteredo/relay-udp.h>

struct ip6_hdr;
struct in6_addr;
union teredo_addr;



// big TODO: make all functions re-entrant safe
//           make all functions thread-safe
class TeredoRelay
{
	private:
		class peer;
		class OutQueue;
		friend class OutQueue;
		class InQueue;
		friend class InQueue;

		/*** Internal stuff ***/
		union teredo_addr addr;
		struct
		{
			struct timeval next, serv;
			uint8_t nonce[8];
			unsigned state:2;
			unsigned count:3;
		} probe;
		uint32_t server_ip2;

		class peer *head;

		TeredoRelayUDP sock;

		peer *AllocatePeer (void);
		peer *FindPeer (const struct in6_addr *addr);

		int SendUnreach (int code, const void *in, size_t inlen);

		/*** Callbacks ***/
		/*
		 * Sends an IPv6 packet from Teredo toward the IPv6 Internet.
		 *
		 * Returns 0 on success, -1 on error.
		 */
		virtual int SendIPv6Packet (const void *packet,
						size_t length) = 0;

		/*
		 * Tries to define the Teredo client IPv6 address. This is an
		 * indication that the Teredo tunneling interface is ready.
		 * The default implementation in base class TeredoRelay does
		 * nothing.
		 *
		 * Returns 0 on success, -1 on error.
		 * TODO: handle error in calling function.
		 */
		virtual int NotifyUp (const struct in6_addr *addr);

		/*
		 * Indicates that the Teredo tunneling interface is no longer
		 * ready to process packets.
		 * Any packet sent when the relay/client is down will be
		 * ignored.
		 */
		virtual int NotifyDown (void);

	protected:
		/*
		 * Creates a Teredo relay manually (ie. one that does not
		 * qualify with a Teredo server and has no Teredo IPv6
		 * address). The prefix must therefore be specified.
		 *
		 * If port is nul, the OS will choose an available UDP port
		 * for communication. This is NOT a good idea if you are
		 * behind a fascist firewall, as the port might be blocked.
		 */
		TeredoRelay (uint32_t pref, uint16_t port /*= 0*/,
				uint32_t ipv4 /* = 0 */, bool cone /*= true*/);

		/*
		 * Creates a Teredo client/relay automatically. The client
		 * will try to qualify and get a Teredo IPv6 address from the
		 * server.
		 *
		 * TODO: support for secure qualification
		 */
		TeredoRelay (uint32_t server_ip, uint16_t port = 0,
				uint32_t ipv4 = 0);

	public:
		virtual ~TeredoRelay ();

		int operator! (void) const
		{
			return !sock;
		}

		/*
		 * Transmits a packet from IPv6 Internet via Teredo,
		 * i.e. performs "Packet transmission".
		 * This function will not block because normal IPv4 stacks do
		 * not block when sending UDP packets.
		 * Not thread-safe yet.
		 */
		int SendPacket (const void *packet, size_t len);

		/*
		 * Receives a packet from Teredo to IPv6 Internet, i.e.
		 * performs "Packet reception". This function will block until
		 * a Teredo packet is received.
		 * Not thread-safe yet.
		 */
		int ReceivePacket (const fd_set *reaset);

		/*
		 * Sends pending queued UDP packets (Teredo bubbles,
		 * Teredo pings, Teredo router solicitation) if any.
		 *
		 * Call this function as frequently as possible.
		 * Not thread-safe yet.
		 */
		int Process (void);

		/*
		 * Returns true if the relay/client is behind a cone NAT.
		 * The result is not meaningful if the client is not fully
		 * qualified.
		 */
		uint32_t GetPrefix (void) const
		{
			return IN6_TEREDO_PREFIX (&addr);
		}

		uint32_t GetServerIP (void) const
		{
			return IN6_TEREDO_SERVER (&addr);
		}

		uint32_t GetServerIP2 (void) const
		{
			return server_ip2;
		}

		bool IsCone (void) const
		{
			return IN6_IS_TEREDO_ADDR_CONE (&addr);
		}

		uint16_t GetMappedPort (void) const
		{
			return IN6_TEREDO_PORT (&addr);
		}

		uint32_t GetMappedIP (void) const
		{
			return IN6_TEREDO_IPV4 (&addr);
		}

		bool IsClient (void) const
		{
			return GetServerIP () != 0;
		}

		bool IsRelay (void) const
		{
			return GetServerIP () == 0;
		}

		bool IsRunning (void) const
		{
			return is_valid_teredo_prefix (GetPrefix ())
				&& (probe.state == 0);
		}


		int RegisterReadSet (fd_set *rs) const
		{
			return sock.RegisterReadSet (rs);
		}

		static unsigned QualificationRetries;
		static unsigned QualificationTimeOut;
		static unsigned RestartDelay;
};

#endif /* ifndef MIREDO_RELAY_H */

