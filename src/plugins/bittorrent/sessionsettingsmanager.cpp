/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "sessionsettingsmanager.h"
#include <QMessageBox>
#include <QMainWindow>
#include <QTimer>
#include <libtorrent/session.hpp>
#include <libtorrent/extensions/metadata_transfer.hpp>
#include <libtorrent/extensions/ut_metadata.hpp>
#include <libtorrent/extensions/ut_pex.hpp>
#include <libtorrent/extensions/smart_ban.hpp>
#include <util/xpc/util.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/core/irootwindowsmanager.h>
#include "xmlsettingsmanager.h"
#include "core.h"

namespace LeechCraft
{
namespace BitTorrent
{
	SessionSettingsManager::SessionSettingsManager (libtorrent::session *session, const ICoreProxy_ptr& proxy, QObject *parent)
	: QObject { parent }
	, Session_ { session }
	, Proxy_ { proxy }
	, ScrapeTimer_ { new QTimer { this } }
	, SettingsSaveTimer_ { new QTimer { this } }
	{
		setLoggingSettings ();
		sslPortChanged ();
		tcpPortRangeChanged ();

#if LIBTORRENT_VERSION_NUM < 010000
		if (XmlSettingsManager::Instance ()->
				property ("EnableMetadata").toBool ())
			Session_->add_extension (&libtorrent::create_metadata_plugin);
#endif
		if (XmlSettingsManager::Instance ()->
				property ("EnablePEX").toBool ())
			Session_->add_extension (&libtorrent::create_ut_pex_plugin);
		if (XmlSettingsManager::Instance ()->
				property ("EnableUTMetadata").toBool ())
			Session_->add_extension (&libtorrent::create_ut_metadata_plugin);
		if (XmlSettingsManager::Instance ()->
				property ("EnableSmartBan").toBool ())
			Session_->add_extension (&libtorrent::create_smart_ban_plugin);

		maxUploadsChanged ();
		maxConnectionsChanged ();
		setProxySettings ();
		setGeneralSettings ();
		setScrapeInterval ();
		setDHTSettings ();

		setScrapeInterval ();
		autosaveIntervalChanged ();

		connect (ScrapeTimer_,
				SIGNAL (timeout ()),
				this,
				SIGNAL (scrapeRequested ()));
		connect (SettingsSaveTimer_,
				SIGNAL (timeout ()),
				this,
				SIGNAL (saveSettingsRequested ()));
	}

	void SessionSettingsManager::SetPreset (Preset sp)
	{
		switch (sp)
		{
		case Preset::MinMemoryUsage:
			// TODO file_checks_delay_per_block = 15
			// max_paused_peerlist_size = 50
			// recv_socket_buffer_size = 16 * 1024
			// send_socket_buffer_size = 16 * 1024
			// optimize_hashing_for_speed = false
			// coalesce_reads = false
			// coalesce_writes = false
			XmlSettingsManager::Instance ()->setProperty ("WholePiecesThreshold", 2);
			XmlSettingsManager::Instance ()->setProperty ("UseParoleMode", false);
			XmlSettingsManager::Instance ()->setProperty ("PrioritizePartialPieces", true);
			XmlSettingsManager::Instance ()->setProperty ("FilePoolSize", 4);
			XmlSettingsManager::Instance ()->setProperty ("AllowMultipleConnectionsPerIP", false);
			XmlSettingsManager::Instance ()->setProperty ("MaxFailcount", 2);
			XmlSettingsManager::Instance ()->setProperty ("InactivityTimeout", 120);
			XmlSettingsManager::Instance ()->setProperty ("MaxOutstandingDiskBytesPerConnection", 1);
			XmlSettingsManager::Instance ()->setProperty ("UPNPIgnoreNonrouters", true);
			XmlSettingsManager::Instance ()->setProperty ("SendBufferWatermark", 9);
			XmlSettingsManager::Instance ()->setProperty ("CacheSize", 0);
			XmlSettingsManager::Instance ()->setProperty ("CacheBufferChunkSize", 1);
			XmlSettingsManager::Instance ()->setProperty ("UseReadCache", false);
			XmlSettingsManager::Instance ()->setProperty ("CloseRedundantConnections", true);
			XmlSettingsManager::Instance ()->setProperty ("MaxPeerListSize", 500);
			XmlSettingsManager::Instance ()->setProperty ("PreferUDPTrackers", true);
			XmlSettingsManager::Instance ()->setProperty ("MaxRejects", 10);
			break;
		case Preset::HighPerfSeed:
			// TODO read_cache_line_size = 512
			// write_cache_line_size = 512
			// optimize_hashing_for_speed = true
			XmlSettingsManager::Instance ()->setProperty ("FilePoolSize", 500);
			XmlSettingsManager::Instance ()->setProperty ("AllowMultipleConnectionsPerIP", true);
			XmlSettingsManager::Instance ()->setProperty ("CacheSize", 512);
			XmlSettingsManager::Instance ()->setProperty ("UseReadCache", true);
			XmlSettingsManager::Instance ()->setProperty ("CacheBufferChunkSize", 128);
			XmlSettingsManager::Instance ()->setProperty ("CacheExpiry", 60 * 60);
			XmlSettingsManager::Instance ()->setProperty ("CloseRedundantConnections", true);
			XmlSettingsManager::Instance ()->setProperty ("MaxRejects", 10);
			XmlSettingsManager::Instance ()->setProperty ("RequestTimeout", 10);
			XmlSettingsManager::Instance ()->setProperty ("PeerTimeout", 20);
			XmlSettingsManager::Instance ()->setProperty ("InactivityTimeout", 20);
			XmlSettingsManager::Instance ()->setProperty ("AutoUploadSlots", false);
			XmlSettingsManager::Instance ()->setProperty ("MaxFailcount", 1);
			break;
		default:
			break;
		}

		setGeneralSettings ();
	}

	void SessionSettingsManager::SetOverallDownloadRate (int val)
	{
#if LIBTORRENT_VERSION_NUM >= 1600
		auto settings = Session_->settings ();
		settings.download_rate_limit = val == 0 ? -1 : val * 1024;
		Session_->set_settings (settings);
#else
		Session_->set_download_rate_limit (val == 0 ? -1 : val * 1024);
#endif
		XmlSettingsManager::Instance ()->setProperty ("DownloadRateLimit", val);
	}

	void SessionSettingsManager::SetOverallUploadRate (int val)
	{
#if LIBTORRENT_VERSION_NUM >= 1600
		auto settings = Session_->settings ();
		settings.upload_rate_limit = val == 0 ? -1 : val * 1024;
		Session_->set_settings (settings);
#else
		Session_->set_upload_rate_limit (val == 0 ? -1 : val * 1024);
#endif
		XmlSettingsManager::Instance ()->setProperty ("UploadRateLimit", val);
	}

	void SessionSettingsManager::SetMaxDownloadingTorrents (int val)
	{
		XmlSettingsManager::Instance ()->setProperty ("MaxDownloadingTorrents", val);
		libtorrent::session_settings settings = Session_->settings ();
		settings.active_downloads = val;
		Session_->set_settings (settings);
	}

	void SessionSettingsManager::SetMaxUploadingTorrents (int val)
	{
		XmlSettingsManager::Instance ()->setProperty ("MaxUploadingTorrents", val);
		libtorrent::session_settings settings = Session_->settings ();
		settings.active_seeds = val;
		Session_->set_settings (settings);
	}

	int SessionSettingsManager::GetOverallDownloadRate () const
	{
		return XmlSettingsManager::Instance ()->property ("DownloadRateLimit").toInt ();
	}

	int SessionSettingsManager::GetOverallUploadRate () const
	{
		return XmlSettingsManager::Instance ()->property ("UploadRateLimit").toInt ();
	}

	int SessionSettingsManager::GetMaxDownloadingTorrents () const
	{
		return XmlSettingsManager::Instance ()->Property ("MaxDownloadingTorrents", -1).toInt ();
	}

	int SessionSettingsManager::GetMaxUploadingTorrents () const
	{
		return XmlSettingsManager::Instance ()->Property ("MaxUploadingTorrents", -1).toInt ();
	}

	void SessionSettingsManager::ManipulateSettings ()
	{
		SetOverallDownloadRate (XmlSettingsManager::Instance ()->
				Property ("DownloadRateLimit", 5000).toInt ());
		SetOverallUploadRate (XmlSettingsManager::Instance ()->
				Property ("UploadRateLimit", 5000).toInt ());
		SetMaxDownloadingTorrents (XmlSettingsManager::Instance ()->
				Property ("MaxDownloadingTorrents", -1).toInt ());
		SetMaxUploadingTorrents (XmlSettingsManager::Instance ()->
				Property ("MaxUploadingTorrents", -1).toInt ());

		XmlSettingsManager::Instance ()->RegisterObject ("TCPPortRange",
				this, "tcpPortRangeChanged");
		// TODO remove this
		XmlSettingsManager::Instance ()->RegisterObject ("DHTEnabled",
				this, "dhtStateChanged");
		XmlSettingsManager::Instance ()->RegisterObject ("AutosaveInterval",
				this, "autosaveIntervalChanged");
		XmlSettingsManager::Instance ()->RegisterObject ("MaxUploads",
				this, "maxUploadsChanged");
		XmlSettingsManager::Instance ()->RegisterObject ("MaxConnections",
				this, "maxConnectionsChanged");
		XmlSettingsManager::Instance ()->RegisterObject ({ "SSLPort", "EnableSSLPort" },
				this, "sslPortChanged");

		QList<QByteArray> proxySettings;
		proxySettings << "TrackerProxyEnabled"
			<< "TrackerProxyHost"
			<< "TrackerProxyPort"
			<< "TrackerProxyAuth"
			<< "PeerProxyEnabled"
			<< "PeerProxyHost"
			<< "PeerProxyPort"
			<< "PeerProxyAuth";
		XmlSettingsManager::Instance ()->RegisterObject (proxySettings,
				this, "setProxySettings");

		QList<QByteArray> generalSettings;
		generalSettings << "TrackerCompletionTimeout"
			<< "TrackerReceiveTimeout"
			<< "StopTrackerTimeout"
			<< "TrackerMaximumResponseLength"
			<< "PieceTimeout"
			<< "RequestQueueTime"
			<< "MaxAllowedInRequestQueue"
			<< "MaxOutRequestQueue"
			<< "WholePiecesThreshold"
			<< "PeerTimeout"
			<< "UrlSeedTimeout"
			<< "UrlSeedPipelineSize"
			<< "SeedingPieceQuota"
			<< "UrlSeedWaitRetry"
			<< "FilePoolSize"
			<< "AllowMultipleConnectionsPerIP"
			<< "MaxFailcount"
			<< "MinReconnectTime"
			<< "PeerConnectTimeout"
			<< "IgnoreLimitsOnLocalNetwork"
			<< "ConnectionSpeed"
			<< "SendRedundantHave"
			<< "LazyBitfields"
			<< "InactivityTimeout"
			<< "UnchokeInterval"
			<< "OptimisticUnchokeMultiplier"
			<< "AnnounceIP"
			<< "NumWant"
			<< "InitialPickerThreshold"
			<< "AllowedFastSetSize"
			<< "MaxOutstandingDiskBytesPerConnection"
			<< "HandshakeTimeout"
			<< "UseDHTAsFallback"
			<< "FreeTorrentHashes"
			<< "UPNPIgnoreNonrouters"
			<< "SendBufferWatermark"
			<< "AutoUploadSlots"
			<< "UseParoleMode"
			<< "CacheSize"
			<< "CacheExpiry"
			<< "OutgoingPorts"
			<< "PeerTOS"
			<< "DontCountSlowTorrents"
			<< "AutoManageInterval"
			<< "ShareRatioLimit"
			<< "SeedTimeRatioLimit"
			<< "SeedTimeLimit"
			<< "CloseRedundantConnections"
			<< "AutoScrapeInterval"
			<< "AutoScrapeMinInterval"
			<< "MaxPeerListSize"
			<< "MinAnnounceInterval"
			<< "PrioritizePartialPieces"
			<< "AnnounceToAllTrackers"
			<< "PreferUDPTrackers"
			<< "StrictSuperSeeding";
		XmlSettingsManager::Instance ()->RegisterObject (generalSettings,
				this, "setGeneralSettings");

		QList<QByteArray> dhtSettings;
		dhtSettings << "MaxPeersReply"
			<< "SearchBranching"
			<< "ServicePort"
			<< "MaxDHTFailcount"
			<< "DHTEnabled"
			<< "EnableLSD"
			<< "EnableUPNP"
			<< "EnableNATPMP";
		XmlSettingsManager::Instance ()->RegisterObject (dhtSettings,
				this, "setDHTSettings");

		// TODO unite
		XmlSettingsManager::Instance ()->RegisterObject ("ScrapeInterval",
				this, "setScrapeInterval");
		XmlSettingsManager::Instance ()->RegisterObject ("ScrapeEnabled",
				this, "setScrapeInterval");

		const QList<QByteArray> loggingSettings
		{
			"PerformanceWarning",
			"NotificationError",
			"NotificationPeer",
			"NotificationPortMapping",
			"NotificationStorage",
			"NotificationTracker",
			"NotificationStatus",
			"NotificationProgress",
			"NotificationIPBlock",
			"NotificationDHT"
		};
		XmlSettingsManager::Instance ()->RegisterObject (loggingSettings,
				this, "setLoggingSettings");
	}

	void SessionSettingsManager::setLoggingSettings ()
	{
		boost::uint32_t mask = 0;

		if (XmlSettingsManager::Instance ()->property ("NotificationDHT").toBool ())
			mask |= libtorrent::alert::dht_notification;
		if (XmlSettingsManager::Instance ()->property ("PerformanceWarning").toBool ())
			mask |= libtorrent::alert::performance_warning;
		if (XmlSettingsManager::Instance ()->property ("NotificationError").toBool ())
			mask |= libtorrent::alert::error_notification;
		if (XmlSettingsManager::Instance ()->property ("NotificationPeer").toBool ())
			mask |= libtorrent::alert::peer_notification;
		if (XmlSettingsManager::Instance ()->property ("NotificationPortMapping").toBool ())
			mask |= libtorrent::alert::port_mapping_notification;

		if (XmlSettingsManager::Instance ()->property ("NotificationStorage").toBool ())
			mask |= libtorrent::alert::storage_notification;
		else
		{
			auto rootWM = Proxy_->GetRootWindowsManager ();
			if (QMessageBox::question (rootWM->GetPreferredWindow (),
						"LeechCraft BitTorrent",
						tr ("Storage notifications are disabled. Live streaming "
							"definitely won't work without them, so if you are "
							"experiencing troubles, re-enable storage notifications "
							"in \"Notifications\" section of BitTorrent settings. "
							"Do you want to enable them now?"),
						QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				XmlSettingsManager::Instance ()->setProperty ("NotificationStorage", true);
				mask |= libtorrent::alert::storage_notification;
			}
		}

		if (XmlSettingsManager::Instance ()->property ("NotificationTracker").toBool ())
			mask |= libtorrent::alert::tracker_notification;
		if (XmlSettingsManager::Instance ()->property ("NotificationStatus").toBool ())
			mask |= libtorrent::alert::status_notification;
		if (XmlSettingsManager::Instance ()->property ("NotificationProgress").toBool ())
			mask |= libtorrent::alert::progress_notification;
		if (XmlSettingsManager::Instance ()->property ("NotificationIPBlock").toBool ())
			mask |= libtorrent::alert::ip_block_notification;

		Session_->set_alert_mask (mask);
	}

	void SessionSettingsManager::tcpPortRangeChanged ()
	{
		const auto& ports = XmlSettingsManager::Instance ()->property ("TCPPortRange").toList ();
#if LIBTORRENT_VERSION_NUM >= 1600
		boost::system::error_code ec;
		Session_->listen_on ({ ports.at (0).toInt (), ports.at (1).toInt () }, ec);
		if (ec)
		{
			qWarning () << Q_FUNC_INFO
					<< "error listening on"
					<< ports.at (0).toInt ()
					<< ports.at (1).toInt ()
					<< ec.message ().c_str ();

			const auto& text = tr ("Error listening on ports %1-%2: %3")
					.arg (ports.at (0).toInt ())
					.arg (ports.at (1).toInt ())
					.arg (QString::fromUtf8 (ec.message ().c_str ()));
			const auto& e = Util::MakeNotification ("BitTorrent", text, PCritical_);
			Proxy_->GetEntityManager ()->HandleEntity (e);
		}
#else
		Session_->listen_on ({ ports.at (0).toInt (), ports.at (1).toInt () });
#endif
	}

	void SessionSettingsManager::sslPortChanged ()
	{
		const bool enable = XmlSettingsManager::Instance ()->property ("EnableSSLPort").toBool ();
		const auto sslPort = enable ?
				XmlSettingsManager::Instance ()->property ("SSLPort").toInt () :
				0;

		auto settings = Session_->settings ();
		settings.ssl_listen = sslPort;
		Session_->set_settings (settings);
	}

	void SessionSettingsManager::maxUploadsChanged ()
	{
		const int maxUps = XmlSettingsManager::Instance ()->property ("MaxUploads").toInt ();
#if LIBTORRENT_VERSION_NUM >= 1603
		auto settings = Session_->settings ();
		settings.unchoke_slots_limit = maxUps;
		Session_->set_settings (settings);
#else
		Session_->set_max_uploads (maxUps);
#endif
	}

	void SessionSettingsManager::maxConnectionsChanged ()
	{
		const int maxConn = XmlSettingsManager::Instance ()->property ("MaxConnections").toInt ();
#if LIBTORRENT_VERSION_NUM >= 1603
		auto settings = Session_->settings ();
		settings.connections_limit = maxConn;
		Session_->set_settings (settings);
#else
		Session_->set_max_connections (maxConn);
#endif
	}

	void SessionSettingsManager::setProxySettings ()
	{
		libtorrent::proxy_settings peerProxySettings;
		if (XmlSettingsManager::Instance ()->property ("PeerProxyEnabled").toBool ())
		{
			peerProxySettings.hostname = XmlSettingsManager::Instance ()->
				property ("PeerProxyAddress").toString ().toStdString ();
			peerProxySettings.port = XmlSettingsManager::Instance ()->
				property ("PeerProxyPort").toInt ();
			QStringList auth = XmlSettingsManager::Instance ()->
				property ("PeerProxyAuth").toString ().split ('@');
			if (auth.size ())
				peerProxySettings.username = auth.at (0).toStdString ();
			if (auth.size () > 1)
				peerProxySettings.password = auth.at (1).toStdString ();
			bool passworded = peerProxySettings.username.size ();
			QString pt = XmlSettingsManager::Instance ()->property ("PeerProxyType").toString ();
			if (pt == "http")
				peerProxySettings.type = passworded ?
					libtorrent::proxy_settings::http_pw :
					libtorrent::proxy_settings::http;
			else if (pt == "socks4")
				peerProxySettings.type = libtorrent::proxy_settings::socks4;
			else if (pt == "socks5")
				peerProxySettings.type = passworded ?
					libtorrent::proxy_settings::socks5_pw :
					libtorrent::proxy_settings::socks5;
			else
				peerProxySettings.type = libtorrent::proxy_settings::none;
		}
		else
			peerProxySettings.type = libtorrent::proxy_settings::none;
#if LIBTORRENT_VERSION_NUM >= 1504
		Session_->set_proxy (peerProxySettings);
#else
		Session_->set_peer_proxy (peerProxySettings);
		Session_->set_tracker_proxy (peerProxySettings);
		Session_->set_web_seed_proxy (peerProxySettings);
#endif
	}

	void SessionSettingsManager::setGeneralSettings ()
	{
		libtorrent::session_settings settings = Session_->settings ();

		settings.user_agent = std::string ("LeechCraft BitTorrent/") +
				Proxy_->GetVersion ().toStdString ();
		settings.tracker_completion_timeout = XmlSettingsManager::Instance ()->
			property ("TrackerCompletionTimeout").toInt ();
		settings.tracker_receive_timeout = XmlSettingsManager::Instance ()->
			property ("TrackerReceiveTimeout").toInt ();
		settings.stop_tracker_timeout = XmlSettingsManager::Instance ()->
			property ("StopTrackerTimeout").toInt ();
		settings.tracker_maximum_response_length = XmlSettingsManager::Instance ()->
			property ("TrackerMaximumResponseLength").toInt () * 1024;
		settings.piece_timeout = XmlSettingsManager::Instance ()->
			property ("PieceTimeout").toInt ();
		settings.request_timeout = XmlSettingsManager::Instance ()->
			property ("RequestTimeout").toInt ();
		settings.request_queue_time = XmlSettingsManager::Instance ()->
			property ("RequestQueueTime").toInt ();
		settings.max_allowed_in_request_queue = XmlSettingsManager::Instance ()->
			property ("MaxAllowedInRequestQueue").toInt ();
		settings.max_out_request_queue = XmlSettingsManager::Instance ()->
			property ("MaxOutRequestQueue").toInt ();
		settings.whole_pieces_threshold = XmlSettingsManager::Instance ()->
			property ("WholePiecesThreshold").toInt ();
		settings.peer_timeout = XmlSettingsManager::Instance ()->
			property ("PeerTimeout").toInt ();
		settings.urlseed_timeout = XmlSettingsManager::Instance ()->
			property ("UrlSeedTimeout").toInt ();
		settings.urlseed_pipeline_size = XmlSettingsManager::Instance ()->
			property ("UrlSeedPipelineSize").toInt ();
		settings.urlseed_wait_retry = XmlSettingsManager::Instance ()->
			property ("UrlSeedWaitRetry").toInt ();
		settings.file_pool_size = XmlSettingsManager::Instance ()->
			property ("FilePoolSize").toInt ();
		settings.allow_multiple_connections_per_ip = XmlSettingsManager::Instance ()->
			property ("AllowMultipleConnectionsPerIP").toBool ();
		settings.max_failcount = XmlSettingsManager::Instance ()->
			property ("MaxFailcount").toInt ();
		settings.min_reconnect_time = XmlSettingsManager::Instance ()->
			property ("MinReconnectTime").toInt ();
		settings.peer_connect_timeout = XmlSettingsManager::Instance ()->
			property ("PeerConnectTimeout").toInt ();
		settings.ignore_limits_on_local_network = XmlSettingsManager::Instance ()->
			property ("IgnoreLimitsOnLocalNetwork").toBool ();
		settings.connection_speed = XmlSettingsManager::Instance ()->
			property ("ConnectionSpeed").toInt ();
		settings.send_redundant_have = XmlSettingsManager::Instance ()->
			property ("SendRedundantHave").toBool ();
		settings.lazy_bitfields = XmlSettingsManager::Instance ()->
			property ("LazyBitfields").toBool ();
		settings.inactivity_timeout = XmlSettingsManager::Instance ()->
			property ("InactivityTimeout").toInt ();
		settings.unchoke_interval = XmlSettingsManager::Instance ()->
			property ("UnchokeInterval").toInt ();
		settings.optimistic_unchoke_interval = XmlSettingsManager::Instance ()->
			property ("OptimisticUnchokeMultiplier").toInt ();

		const auto& announceIP = XmlSettingsManager::Instance ()->
				property ("AnnounceIP").toString ();
		try
		{
#if LIBTORRENT_VERSION_NUM >= 1600
			settings.announce_ip = announceIP.toStdString ();
#else
			if (announceIP.isEmpty ())
				settings.announce_ip = boost::asio::ip::address ();
			else
				settings.announce_ip = boost::asio::ip::address::from_string (announceIP.toStdString ());
#endif
		}
		catch (...)
		{
			const auto& e = Util::MakeNotification ("BitTorrent",
					tr ("Wrong announce address %1")
						.arg (announceIP),
					PCritical_);
			Proxy_->GetEntityManager ()->HandleEntity (e);
		}

		settings.num_want = XmlSettingsManager::Instance ()->
			property ("NumWant").toInt ();
		settings.initial_picker_threshold = XmlSettingsManager::Instance ()->
			property ("InitialPickerThreshold").toInt ();
		settings.allowed_fast_set_size = XmlSettingsManager::Instance ()->
			property ("AllowedFastSetSize").toInt ();
		settings.max_queued_disk_bytes = XmlSettingsManager::Instance ()->
			property ("MaxOutstandingDiskBytesPerConnection").toInt () * 1024;
		settings.handshake_timeout = XmlSettingsManager::Instance ()->
			property ("HandshakeTimeout").toInt ();
		settings.use_dht_as_fallback = XmlSettingsManager::Instance ()->
			property ("UseDHTAsFallback").toBool ();
		settings.free_torrent_hashes = XmlSettingsManager::Instance ()->
			property ("FreeTorrentHashes").toBool ();
		settings.upnp_ignore_nonrouters = XmlSettingsManager::Instance ()->
			property ("UPNPIgnoreNonrouters").toBool ();
		settings.send_buffer_watermark = XmlSettingsManager::Instance ()->
			property ("SendBufferWatermark").toInt () * 1024;
		settings.auto_upload_slots = XmlSettingsManager::Instance ()->
			property ("AutoUploadSlots").toBool ();
		settings.use_parole_mode = XmlSettingsManager::Instance ()->
			property ("UseParoleMode").toBool ();
		settings.cache_size = 1048576 / 16384 * XmlSettingsManager::Instance ()->
			property ("CacheSize").value<long int> ();
		settings.cache_buffer_chunk_size = XmlSettingsManager::Instance ()->
			property ("CacheBufferChunkSize").toInt ();
		settings.cache_expiry = XmlSettingsManager::Instance ()->
			property ("CacheExpiry").toInt ();
		QList<QVariant> ports = XmlSettingsManager::Instance ()->
			property ("OutgoingPorts").toList ();
		if (ports.size () == 2)
			settings.outgoing_ports = std::make_pair (ports.at (0).toInt (),
					ports.at (1).toInt ());
		settings.use_read_cache = XmlSettingsManager::Instance ()->
			property ("UseReadCache").toBool ();
		settings.peer_tos = XmlSettingsManager::Instance ()->
			property ("PeerTOS").toInt ();
		settings.auto_manage_prefer_seeds = XmlSettingsManager::Instance ()->
			property ("AutoManagePreferSeeds").toBool ();
		settings.dont_count_slow_torrents = XmlSettingsManager::Instance ()->
			property ("DontCountSlowTorrents").toBool ();
		settings.auto_manage_interval = XmlSettingsManager::Instance ()->
			property ("AutoManageInterval").toInt ();
		settings.share_ratio_limit = XmlSettingsManager::Instance ()->
			property ("ShareRatioLimit").toDouble ();
		settings.seed_time_ratio_limit = XmlSettingsManager::Instance ()->
			property ("SeedTimeRatioLimit").toDouble ();
		settings.seed_time_limit = XmlSettingsManager::Instance ()->
			property ("SeedTimeLimit").toULongLong () * 60;
		settings.peer_turnover = XmlSettingsManager::Instance ()->
			property ("PeerTurnover").toDouble ();
		settings.close_redundant_connections = XmlSettingsManager::Instance ()->
			property ("CloseRedundantConnections").toBool ();
		settings.auto_scrape_interval = XmlSettingsManager::Instance ()->
			property ("AutoScrapeInterval").toInt () * 60;
		settings.auto_scrape_min_interval = XmlSettingsManager::Instance ()->
			property ("AutoScrapeMinInterval").toInt ();
		settings.max_peerlist_size = XmlSettingsManager::Instance ()->
			property ("MaxPeerListSize").toInt ();
		settings.min_announce_interval = XmlSettingsManager::Instance ()->
			property ("MinAnnounceInterval").toInt ();
		settings.prioritize_partial_pieces = XmlSettingsManager::Instance ()->
			property ("PrioritizePartialPieces").toBool ();
		settings.announce_to_all_trackers = XmlSettingsManager::Instance ()->
			property ("AnnounceToAllTrackers").toBool ();
		settings.announce_to_all_tiers = XmlSettingsManager::Instance ()->
			property ("AnnounceToAllTiers").toBool ();
		settings.prefer_udp_trackers = XmlSettingsManager::Instance ()->
			property ("PreferUDPTrackers").toBool ();
		settings.strict_super_seeding = XmlSettingsManager::Instance ()->
			property ("StrictSuperSeeding").toBool ();
		settings.seeding_piece_quota = XmlSettingsManager::Instance ()->
			property ("SeedingPieceQuota").toInt ();
		settings.auto_manage_startup = XmlSettingsManager::Instance ()->
			property ("AutoManageStartup").toInt ();
		settings.lock_disk_cache = XmlSettingsManager::Instance ()->
			property ("LockDiskCache").toBool ();
		settings.max_rejects = XmlSettingsManager::Instance ()->
			property ("MaxRejects").toInt ();

		settings.active_limit = 16384;

		Session_->set_settings (settings);
	}

	void SessionSettingsManager::setDHTSettings ()
	{
		if (XmlSettingsManager::Instance ()->property ("EnableLSD").toBool ())
			Session_->start_lsd ();
		else
			Session_->stop_lsd ();

		if (XmlSettingsManager::Instance ()->property ("EnableUPNP").toBool ())
			Session_->start_upnp ();
		else
			Session_->stop_upnp ();

		if (XmlSettingsManager::Instance ()->property ("EnableNATPMP").toBool ())
			Session_->start_natpmp ();
		else
			Session_->stop_natpmp ();

		if (XmlSettingsManager::Instance ()->property ("DHTEnabled").toBool ())
		{
			Session_->start_dht ();
			Session_->add_dht_router ({ "router.bittorrent.com", 6881 });
			Session_->add_dht_router ({ "router.utorrent.com", 6881 });
			Session_->add_dht_router ({ "dht.transmissionbt.com", 6881 });
			Session_->add_dht_router ({ "dht.aelitis.com", 6881 });
		}
		else
			Session_->stop_dht ();

		libtorrent::dht_settings settings;

		settings.max_peers_reply = XmlSettingsManager::Instance ()->property ("MaxPeersReply").toInt ();
		settings.search_branching = XmlSettingsManager::Instance ()->property ("SearchBranching").toInt ();
		settings.service_port = XmlSettingsManager::Instance ()->property ("ServicePort").toInt ();
		settings.max_fail_count = XmlSettingsManager::Instance ()->property ("MaxDHTFailcount").toInt ();

		Session_->set_dht_settings (settings);
	}

	void SessionSettingsManager::setScrapeInterval ()
	{
		bool scrapeEnabled = XmlSettingsManager::Instance ()->property ("ScrapeEnabled").toBool ();
		if (scrapeEnabled)
		{
			ScrapeTimer_->stop ();
			ScrapeTimer_->start (XmlSettingsManager::Instance ()->
					property ("ScrapeInterval").toInt () * 1000);
		}
		else
			ScrapeTimer_->stop ();
	}

	void SessionSettingsManager::autosaveIntervalChanged ()
	{
		SettingsSaveTimer_->stop ();
		SettingsSaveTimer_->start (XmlSettingsManager::Instance ()->
				property ("AutosaveInterval").toInt () * 1000);
	}
}
}
