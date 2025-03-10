/*

Copyright (c) 2007-2009, 2012-2013, 2016-2020, 2022, Arvid Norberg
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/

#ifndef TORRENT_MAGNET_URI_HPP_INCLUDED
#define TORRENT_MAGNET_URI_HPP_INCLUDED

#include <string>
#include "libtorrent/config.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/add_torrent_params.hpp"
#include "libtorrent/string_view.hpp"

namespace libtorrent {

	struct torrent_handle;
	struct session;

	// Generates a magnet URI from the specified torrent.
	//
	// Several fields from the add_torrent_params objects are recorded in the
	// magnet link. In order to not include them, they have to be cleared before
	// calling make_magnet_uri(). These fields are used:
	//
	//  ``ti``, ``info_hashes``, ``url_seeds``, ``dht_nodes``,
	//  ``file_priorities``, ``trackers``, ``name``, ``peers``.
	//
	// Depending on what the use case for the resulting magnet link is, clearing
	// ``peers`` and ``dht_nodes`` is probably a good idea if the add_torrent_params
	// came from a running torrent. Those lists may be long and be ephemeral.
	//
	// If none of the ``info_hashes`` or ``ti`` fields are set, there is not
	// info-hash available, and a magnet link cannot be created. In this case
	// make_magnet_uri() returns an empty string.
	//
	// The recommended way to generate a magnet link from a torrent_handle is to
	// call save_resume_data(), which will post a save_resume_data_alert
	// containing an add_torrent_params object. This can then be passed to
	// make_magnet_uri().
	//
	// For more information about magnet links, see magnet-links_.
	TORRENT_EXPORT std::string make_magnet_uri(add_torrent_params const& atp);

#if TORRENT_ABI_VERSION < 4
	// The overload that takes a torrent_handle will make blocking calls to
	// query information about the torrent. If the torrent handle is invalid,
	// an empty string is returned.
	TORRENT_DEPRECATED_EXPORT
	std::string make_magnet_uri(torrent_handle const& handle);
	TORRENT_DEPRECATED_EXPORT
	std::string make_magnet_uri(torrent_info const& info);
#endif

#if TORRENT_ABI_VERSION == 1
#ifndef BOOST_NO_EXCEPTIONS
	// deprecated in 0.14
	TORRENT_DEPRECATED_EXPORT
	torrent_handle add_magnet_uri(session& ses, std::string const& uri
		, std::string const& save_path
		, storage_mode_t storage_mode = storage_mode_sparse
		, bool paused = false
		, void* userdata = nullptr);

	// deprecated in 0.16. Instead, pass in the magnet link as add_torrent_params::url
	TORRENT_DEPRECATED_EXPORT
	torrent_handle add_magnet_uri(session& ses, std::string const& uri
		, add_torrent_params const& p);
#endif

	// deprecated in 0.16. Instead, pass in the magnet link as add_torrent_params::url
	TORRENT_DEPRECATED_EXPORT
	torrent_handle add_magnet_uri(session& ses, std::string const& uri
		, add_torrent_params const& p, error_code& ec);
#endif // TORRENT_ABI_VERSION


	// This function parses out information from the magnet link and populates the
	// add_torrent_params object. The overload that does not take an
	// ``error_code`` reference will throw a system_error on error
	// The overload taking an ``add_torrent_params`` reference will fill in the
	// fields specified in the magnet URI.
	TORRENT_EXPORT add_torrent_params parse_magnet_uri(string_view uri, error_code& ec);
	TORRENT_EXPORT add_torrent_params parse_magnet_uri(string_view uri);
	TORRENT_EXPORT void parse_magnet_uri(string_view uri, add_torrent_params& p, error_code& ec);
}

#endif
