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

#include "core.h"
#include <interfaces/iplugin2.h>
#include <interfaces/core/ientitymanager.h>
#include "collectionsmanager.h"
#include "localfileresolver.h"
#include "localcollection.h"
#include "xmlsettingsmanager.h"
#include "playlistmanager.h"
#include "sync/syncmanager.h"
#include "sync/syncunmountablemanager.h"
#include "sync/clouduploadmanager.h"
#include "interfaces/lmp/ilmpplugin.h"
#include "interfaces/lmp/icloudstorageplugin.h"
#include "lmpproxy.h"
#include "player.h"
#include "previewhandler.h"
#include "progressmanager.h"
#include "radiomanager.h"
#include "rganalysismanager.h"
#include "hookinterconnector.h"

namespace LeechCraft
{
namespace LMP
{
	Core::Core ()
	: Resolver_ (new LocalFileResolver)
	, HookInterconnector_ (new HookInterconnector)
	, Collection_ (new LocalCollection)
	, CollectionsManager_ (new CollectionsManager)
	, PLManager_ (new PlaylistManager)
	, SyncManager_ (new SyncManager)
	, SyncUnmountableManager_ (new SyncUnmountableManager)
	, CloudUpMgr_ (new CloudUploadManager)
	, ProgressManager_ (new ProgressManager)
	, RadioManager_ (new RadioManager)
	, LmpProxy_ (new LMPProxy (Collection_.get (), Resolver_.get ()))
	{
		ProgressManager_->AddSyncManager (SyncManager_.get ());
		ProgressManager_->AddSyncManager (SyncUnmountableManager_.get ());
		ProgressManager_->AddSyncManager (CloudUpMgr_.get ());

		new RgAnalysisManager (Collection_, this);

		CollectionsManager_->Add (Collection_->GetCollectionModel ());
	}

	Core& Core::Instance ()
	{
		static Core c;
		return c;
	}

	void Core::SetProxy (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
	}

	ICoreProxy_ptr Core::GetProxy ()
	{
		return Proxy_;
	}

	void Core::SendEntity (const Entity& e)
	{
		Proxy_->GetEntityManager ()->HandleEntity (e);
	}

	void Core::PostInit ()
	{
		Collection_->FinalizeInit ();

		Player_ = std::make_shared<Player> ();
		PreviewMgr_ = std::make_shared<PreviewHandler> (Player_.get ());
	}

	void Core::InitWithOtherPlugins ()
	{
		Player_->InitWithOtherPlugins ();
		RadioManager_->InitProviders ();
	}

	const std::shared_ptr<LMPProxy>& Core::GetLmpProxy () const
	{
		return LmpProxy_;
	}

	void Core::AddPlugin (QObject *pluginObj)
	{
		auto ip2 = qobject_cast<IPlugin2*> (pluginObj);
		auto ilmpPlug = qobject_cast<ILMPPlugin*> (pluginObj);

		if (!ilmpPlug)
		{
			qWarning () << Q_FUNC_INFO
					<< pluginObj
					<< "doesn't implement ILMPPlugin";
			return;
		}

		ilmpPlug->SetLMPProxy (LmpProxy_);

		const auto& classes = ip2->GetPluginClasses ();
		if (classes.contains ("org.LeechCraft.LMP.CollectionSync") &&
			qobject_cast<ISyncPlugin*> (pluginObj))
			SyncPlugins_ << pluginObj;

		if (classes.contains ("org.LeechCraft.LMP.CloudStorage") &&
			qobject_cast<ICloudStoragePlugin*> (pluginObj))
		{
			CloudPlugins_ << pluginObj;
			emit cloudStoragePluginsChanged ();
		}

		if (classes.contains ("org.LeechCraft.LMP.PlaylistProvider") &&
			qobject_cast<IPlaylistProvider*> (pluginObj))
			PLManager_->AddProvider (pluginObj);

		HookInterconnector_->AddPlugin (pluginObj);
	}

	QObjectList Core::GetSyncPlugins () const
	{
		return SyncPlugins_;
	}

	QObjectList Core::GetCloudStoragePlugins() const
	{
		return CloudPlugins_;
	}

	void Core::RequestArtistBrowser (const QString& artist)
	{
		emit artistBrowseRequested (artist);
	}

	HookInterconnector* Core::GetHookInterconnector () const
	{
		return HookInterconnector_.get ();
	}

	LocalFileResolver* Core::GetLocalFileResolver () const
	{
		return Resolver_.get ();
	}

	LocalCollection* Core::GetLocalCollection () const
	{
		return Collection_.get ();
	}

	CollectionsManager* Core::GetCollectionsManager () const
	{
		return CollectionsManager_.get ();
	}

	PlaylistManager* Core::GetPlaylistManager () const
	{
		return PLManager_.get ();
	}

	SyncManager* Core::GetSyncManager () const
	{
		return SyncManager_.get ();
	}

	SyncUnmountableManager* Core::GetSyncUnmountableManager () const
	{
		return SyncUnmountableManager_.get ();
	}

	CloudUploadManager* Core::GetCloudUploadManager () const
	{
		return CloudUpMgr_.get ();
	}

	ProgressManager* Core::GetProgressManager () const
	{
		return ProgressManager_.get ();
	}

	RadioManager* Core::GetRadioManager () const
	{
		return RadioManager_.get ();
	}

	Player* Core::GetPlayer () const
	{
		return Player_.get ();
	}

	PreviewHandler* Core::GetPreviewHandler () const
	{
		return PreviewMgr_.get ();
	}

	boost::optional<MediaInfo> Core::TryURLResolve (const QUrl& url) const
	{
		return PLManager_->TryResolveMediaInfo (url);
	}

	void Core::rescan ()
	{
		Collection_->Rescan ();
	}
}
}
