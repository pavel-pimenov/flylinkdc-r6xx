/*

Copyright (c) 2015, 2017, 2020, Arvid Norberg
Copyright (c) 2016, Alden Torres
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/

#ifndef TORRENT_AUX_TIME_HPP
#define TORRENT_AUX_TIME_HPP

#include "libtorrent/config.hpp"
#include "libtorrent/time.hpp"

#include <ctime> // for std::time_t

namespace libtorrent { namespace aux {

	// returns the current time, as represented by time_point. The
	// resolution of this timer is about 100 ms.
	TORRENT_EXTRA_EXPORT time_point time_now();
	TORRENT_EXTRA_EXPORT time_point32 time_now32();

	TORRENT_EXTRA_EXPORT std::time_t to_time_t(time_point32 tp);
	TORRENT_EXTRA_EXPORT time_point32 from_time_t(std::time_t t);

	// returns the current posix time (UTC)
	TORRENT_EXTRA_EXPORT time_t posix_time();
} }

#endif
