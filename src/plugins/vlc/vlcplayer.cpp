/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Vladislav Tyulbashev
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

#include <vlc/vlc.h>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTime>
#include <QMouseEvent>
#include <QWidget>
#include <QTime>
#include <QTimer>
#include <QSizePolicy>
#include <QEventLoop>
#include <QUrl>
#include "vlcplayer.h"

namespace 
{
	QTime convertTime (libvlc_time_t t) 
	{
		return QTime (t / 1000 / 60 / 60, t / 1000 / 60 % 60, t / 1000 % 60, t % 1000);
	}
	
	void sleep (int ms)
	{
			QEventLoop loop;
			QTimer::singleShot (ms, &loop, SLOT (quit ()));
			loop.exec();
	}
	
	bool IsDVD (const char *s)
	{
		int i;
		for (i = 0; (i < 5) && (s[i] != 0); i++)
			if (s[i] != "dvd:/"[i])
				return false;
			
		return i == 5;
	}
}

namespace LeechCraft
{
namespace vlc
{
	VlcPlayer::VlcPlayer (QWidget *parent)
	: QObject (parent)
	, M_ (nullptr)
	, Parent_ (parent)
	, DVD_ (false)
	{
		const char * const vlc_args[] = 
		{
			"-I", "dummy", /* Don't use any interface */
			"--ignore-config", /* Don't use VLC's config */
			"--extraintf=logger", //log anything
			"--verbose=0"
		};

		VlcInstance_ = std::shared_ptr<libvlc_instance_t> (libvlc_new (5, vlc_args), libvlc_release);
		Mp_ = std::shared_ptr<libvlc_media_player_t> (libvlc_media_player_new (VlcInstance_.get ()), libvlc_media_player_release);
		libvlc_media_player_set_xwindow (Mp_.get (), parent->winId ());
	}
	
	void VlcPlayer::addUrl (const QUrl &url) 
	{
		libvlc_media_player_stop (Mp_.get ());
		
		DVD_ = IsDVD (url.toEncoded ().constData ());
		M_.reset(libvlc_media_new_location (VlcInstance_.get (), url.toEncoded ()), libvlc_media_release);
		
		libvlc_media_player_set_media (Mp_.get (), M_.get ());
		libvlc_media_player_play (Mp_.get ());
	}
	
	void VlcPlayer::ClearAll () 
	{	
	}
	
	bool VlcPlayer::NowPlaying () const
	{
		return libvlc_media_player_is_playing (Mp_.get ());
	}
	
	double VlcPlayer::GetPosition () const
	{
		return libvlc_media_player_get_position (Mp_.get ());
	}
	
	void VlcPlayer::togglePlay () 
	{
		if (NowPlaying ())
			libvlc_media_player_pause (Mp_.get ());
		else
			libvlc_media_player_play (Mp_.get ());
	}
	
	void VlcPlayer::stop () 
	{
		libvlc_media_player_stop (Mp_.get ());
	}
	
	void VlcPlayer::changePosition (double pos)
	{
		if (libvlc_media_player_get_media (Mp_.get ()))
			libvlc_media_player_set_position (Mp_.get (), pos);
	}	
	
	QTime VlcPlayer::GetFullTime () const 
	{
		if (libvlc_media_player_get_media (Mp_.get ()))
			return convertTime (libvlc_media_player_get_length (Mp_.get ()));
		else
			return convertTime (0);
	}
	
	QTime VlcPlayer::GetCurrentTime () const
	{
		if (libvlc_media_player_get_media (Mp_.get ()))
			return convertTime (libvlc_media_player_get_time (Mp_.get ()));
		else
			return convertTime (0);
	}
	
	void VlcPlayer::switchWidget (QWidget *widget) 
	{
		libvlc_time_t cur;
		int audio;
		int subtitle;
		bool playingMedia = libvlc_media_player_get_media (Mp_.get ());
		if (playingMedia) 
		{
			cur = libvlc_media_player_get_time (Mp_.get ());
			audio = GetCurrentAudioTrack ();
			subtitle = GetCurrentSubtitle ();
		}
		
		bool isPlaying = libvlc_media_player_is_playing (Mp_.get ()); 
		bool dvd = DVD_ && libvlc_media_player_get_length (Mp_.get ()) > 600000; // = 10 min
		
		libvlc_media_player_stop (Mp_.get ());
		libvlc_media_player_set_xwindow (Mp_.get (), widget->winId ());
		libvlc_media_player_play (Mp_.get ());
		
		WaitForPlaying ();
		if (dvd)
		{
			libvlc_media_player_navigate (Mp_.get (), libvlc_navigate_activate);
			sleep (150);
		}
		
		if (playingMedia && (!DVD_ || dvd))
		{
			libvlc_media_player_set_time (Mp_.get (), cur);
			setAudioTrack (audio);
			setSubtitle (subtitle);
		}
		
		if (!isPlaying)
			libvlc_media_player_pause (Mp_.get ());
	}
	
	QWidget* VlcPlayer::GetParent () const
	{
		return Parent_;
	}
	
	std::shared_ptr<libvlc_media_player_t> VlcPlayer::GetPlayer () const
	{
		return Mp_;
	}
	
	int VlcPlayer::GetAudioTracksNumber () const
	{
		return libvlc_audio_get_track_count (Mp_.get ());
	}
	
	int VlcPlayer::GetCurrentAudioTrack () const
	{
		return libvlc_audio_get_track (Mp_.get ());
	}
	
	void VlcPlayer::setAudioTrack (int track)
	{
		libvlc_audio_set_track (Mp_.get (), track);
	}
	
	QString VlcPlayer::GetAudioTrackDescription (int track) const
	{
		libvlc_track_description_t *t = GetTrack (libvlc_audio_get_track_description (Mp_.get ()), track);
		return QString (t->psz_name);
	}
	
	int VlcPlayer::GetAudioTrackId(int track) const
	{
		libvlc_track_description_t *t = GetTrack (libvlc_audio_get_track_description (Mp_.get ()), track);
		return t->i_id;
	}
	
	int VlcPlayer::GetSubtitlesNumber () const
	{
		return libvlc_video_get_spu_count (Mp_.get ());
	}
	
	void VlcPlayer::AddSubtitles (QString file)
	{
		libvlc_video_set_subtitle_file (Mp_.get (), file.toUtf8 ());
	}

	int VlcPlayer::GetCurrentSubtitle () const
	{
		return libvlc_video_get_spu (Mp_.get ());
	}
	
	QString VlcPlayer::GetSubtitleDescription (int track) const
	{
		libvlc_track_description_t *t = GetTrack (libvlc_video_get_spu_description (Mp_.get ()), track);	
		return QString (t->psz_name);
	}
	
	int VlcPlayer::GetSubtitleId(int track) const
	{
		libvlc_track_description_t *t = GetTrack (libvlc_video_get_spu_description (Mp_.get ()), track);
		return t->i_id;
	}

	void VlcPlayer::setSubtitle (int track)
	{
		libvlc_video_set_spu (Mp_.get (), track);
	}
	
	libvlc_track_description_t* VlcPlayer::GetTrack(libvlc_track_description_t *t, int track) const
	{
		for (int i = 0; i < track; i++)
			t = t->p_next;
		
		return t;
	}

	void VlcPlayer::DVDNavigate (unsigned nav)
	{
		libvlc_media_player_navigate (Mp_.get (), nav);
	}

	void VlcPlayer::WaitForPlaying () const
	{
		while (!NowPlaying ())
			sleep (1);
	}
}
}
