/*

Copyright (c) 2014-2017, 2019-2021, Arvid Norberg
Copyright (c) 2016-2017, 2020-2021, Alden Torres
Copyright (c) 2018, Steven Siloti
Copyright (c) 2020, Paul-Louis Ageneau
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/

#include "libtorrent/aux_/torrent_peer.hpp"
#include "libtorrent/assert.hpp"
#include "libtorrent/aux_/string_util.hpp"
#include "libtorrent/aux_/peer_connection.hpp"
#include "libtorrent/aux_/crc32c.hpp"
#include "libtorrent/aux_/ip_voter.hpp"
#include "libtorrent/aux_/io_bytes.hpp" // for write_uint16
#include "libtorrent/aux_/ip_helpers.hpp"

namespace libtorrent::aux {

	namespace {

		void apply_mask(std::uint8_t* b, std::uint8_t const* mask, int size)
		{
			for (int i = 0; i < size; ++i)
			{
				*b &= *mask;
				++b;
				++mask;
			}
		}
	}

	// 1. if the IP addresses are identical, hash the ports in 16 bit network-order
	//    binary representation, ordered lowest first.
	// 2. if the IPs are in the same /24, hash the IPs ordered, lowest first.
	// 3. if the IPs are in the ame /16, mask the IPs by 0xffffff55, hash them
	//    ordered, lowest first.
	// 4. if IPs are not in the same /16, mask the IPs by 0xffff5555, hash them
	//    ordered, lowest first.
	//
	// * for IPv6 addresses, the lower 48 bits are always unmasked
	//
	// * for IPv6 addresses, compare /48, /56, /64, /72 and so on.
	//
	// * the two IP addresses that are used to calculate the rank must
	//   always be of the same address family
	//
	// * all IP addresses are in network byte order when hashed
	// The full specification is here:
	// https://www.bittorrent.org/beps/bep_0040.html
	std::uint32_t peer_priority(tcp::endpoint e1, tcp::endpoint e2)
	{
		TORRENT_ASSERT(aux::is_v4(e1) == aux::is_v4(e2));

		using std::swap;

		std::uint32_t ret;
		if (e1.address() == e2.address())
		{
			if (e1.port() > e2.port())
				swap(e1, e2);
			std::array<char, 4> buf;
			auto ptr = buf.data();
			aux::write_uint16(e1.port(), ptr);
			aux::write_uint16(e2.port(), ptr);
			std::uint32_t p;
			std::memcpy(&p, buf.data(), 4);
			ret = crc32c_32(p);
		}
		else if (aux::is_v6(e1))
		{
			if (e1 > e2) swap(e1, e2);
			address_v6::bytes_type b1 = e1.address().to_v6().to_bytes();
			address_v6::bytes_type b2 = e2.address().to_v6().to_bytes();
			size_t offset = 0xff;
			for (size_t i = 0; i < b1.size(); ++i)
			{
				// we never mask the first 6 bytes, index 6 (the 7th byte)
				// is the earliest we start masking at. But if the prefix is
				// identical, we keep pushing out where we start masking
				if (offset == 0xff && b1[i] != b2[i])
					offset = std::max(i + 1, size_t(5));
				else if (i > offset)
				{
#if defined __GNUC__ && __GNUC__ >= 7
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
					b1[i] &= 0x55;
					b2[i] &= 0x55;
#if defined __GNUC__ && __GNUC__ >= 7
#pragma GCC diagnostic pop
#endif
				}
			}
			std::uint64_t addrbuf[4];
			std::memcpy(&addrbuf[0], b1.data(), 16);
			std::memcpy(&addrbuf[2], b2.data(), 16);
			ret = aux::crc32c(addrbuf, 4);
		}
		else
		{
			static const std::uint8_t v4mask[][4] = {
				{ 0xff, 0xff, 0x55, 0x55 },
				{ 0xff, 0xff, 0xff, 0x55 },
				{ 0xff, 0xff, 0xff, 0xff }
			};

			if (e1 > e2) swap(e1, e2);
			address_v4::bytes_type b1 = e1.address().to_v4().to_bytes();
			address_v4::bytes_type b2 = e2.address().to_v4().to_bytes();
			int mask = std::memcmp(&b1[0], &b2[0], 2) ? 0
				: std::memcmp(&b1[0], &b2[0], 3) ? 1 : 2;
			apply_mask(&b1[0], v4mask[mask], 4);
			apply_mask(&b2[0], v4mask[mask], 4);
			std::uint64_t addrbuf;
			std::memcpy(&addrbuf, &b1[0], 4);
			std::memcpy(reinterpret_cast<char*>(&addrbuf) + 4, &b2[0], 4);
			ret = aux::crc32c(&addrbuf, 1);
		}

		return ret;
	}

	torrent_peer::torrent_peer(std::uint16_t port_, bool conn
		, peer_source_flags_t const src)
		: connection(nullptr)
		, port(port_)
		, failcount(0)
		, connectable(conn)
		, optimistically_unchoked(false)
		, seed(false)
		, maybe_upload_only(false)
		, fast_reconnects(0)
		, trust_points(0)
		, source(static_cast<std::uint8_t>(src))
#if !defined TORRENT_DISABLE_ENCRYPTION
		// assume no support in order to
		// prefer opening non-encrypted
		// connections. If it fails, we'll
		// retry with encryption
		, pe_support(false)
#endif
		, is_v6_addr(false)
#if TORRENT_USE_I2P
		, is_i2p_addr(false)
#endif
#if TORRENT_USE_RTC
		, is_rtc_addr(false)
#endif
		, on_parole(false)
		, banned(false)
		, supports_utp(true) // assume peers support utp
		, confirmed_supports_utp(false)
		, supports_holepunch(false)
		, web_seed(false)
		, protocol_v2(false)
	{}

	std::uint32_t torrent_peer::rank(aux::external_ip const& external, int external_port) const
	{
		TORRENT_ASSERT(in_use);
		//TODO: how do we deal with our external address changing?
		if (peer_rank == 0)
			peer_rank = peer_priority(
				tcp::endpoint(external.external_address(this->address()), std::uint16_t(external_port))
				, tcp::endpoint(this->address(), this->port));
		return peer_rank;
	}

#ifndef TORRENT_DISABLE_LOGGING
	std::string torrent_peer::to_string() const
	{
		TORRENT_ASSERT(in_use);
#if TORRENT_USE_I2P
		if (is_i2p_addr) return std::string(dest());
#endif
		return address().to_string();
	}
#endif

	std::int64_t torrent_peer::total_download() const
	{
		TORRENT_ASSERT(in_use);
		if (connection != nullptr)
		{
			TORRENT_ASSERT(prev_amount_download == 0);
			return connection->statistics().total_payload_download();
		}
		else
		{
			return std::int64_t(prev_amount_download) * 1024;
		}
	}

	std::int64_t torrent_peer::total_upload() const
	{
		TORRENT_ASSERT(in_use);
		if (connection != nullptr)
		{
			TORRENT_ASSERT(prev_amount_upload == 0);
			return connection->statistics().total_payload_upload();
		}
		else
		{
			return std::int64_t(prev_amount_upload) * 1024;
		}
	}

	ipv4_peer::ipv4_peer(tcp::endpoint const& ep, bool c
		, peer_source_flags_t const src)
		: torrent_peer(ep.port(), c, src)
		, addr(ep.address().to_v4())
	{
		is_v6_addr = false;
#if TORRENT_USE_I2P
		is_i2p_addr = false;
#endif
#if TORRENT_USE_RTC
		is_rtc_addr = false;
#endif
	}

	ipv4_peer::ipv4_peer(ipv4_peer const&) = default;
	ipv4_peer& ipv4_peer::operator=(ipv4_peer const& p) & = default;

#if TORRENT_USE_I2P
	i2p_peer::i2p_peer(string_view dest, bool connectable_
		, peer_source_flags_t const src)
		: torrent_peer(0, connectable_, src)
		, destination(dest)
	{
		is_v6_addr = false;
		is_i2p_addr = true;
#if TORRENT_USE_RTC
        is_rtc_addr = false;
#endif
	}
#endif // TORRENT_USE_I2P

#if TORRENT_USE_RTC
	rtc_peer::rtc_peer(tcp::endpoint const& ep, peer_source_flags_t src)
		: torrent_peer(0, false, src)
		, endpoint(ep)
	{
		is_v6_addr = false;
#if TORRENT_USE_I2P
		is_i2p_addr = false;
#endif
		is_rtc_addr = true;
	}
#endif // TORRENT_USE_RTC

	ipv6_peer::ipv6_peer(tcp::endpoint const& ep, bool c
		, peer_source_flags_t const src)
		: torrent_peer(ep.port(), c, src)
		, addr(ep.address().to_v6().to_bytes())
	{
		is_v6_addr = true;
#if TORRENT_USE_I2P
		is_i2p_addr = false;
#endif
#if TORRENT_USE_RTC
		is_rtc_addr = false;
#endif
	}

	ipv6_peer::ipv6_peer(ipv6_peer const&) = default;

#if TORRENT_USE_I2P
	string_view torrent_peer::dest() const
	{
		if (is_i2p_addr)
			return *static_cast<i2p_peer const*>(this)->destination;
		else
			return "";
	}
#endif

	libtorrent::address torrent_peer::address() const
	{
		if (is_v6_addr)
			return libtorrent::address_v6(
				static_cast<ipv6_peer const*>(this)->addr);
		else
#if TORRENT_USE_I2P
		if (is_i2p_addr) return {};
		else
#endif
#if TORRENT_USE_RTC
		if (is_rtc_addr) return static_cast<rtc_peer const*>(this)->endpoint.address();
		else
#endif
		return static_cast<ipv4_peer const*>(this)->addr;
	}

}
