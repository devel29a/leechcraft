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

#include "localcollectionwatcher.h"
#include <algorithm>
#include <QTimer>
#include "core.h"
#include "localcollection.h"
#include "recursivedirwatcher.h"

namespace LeechCraft
{
namespace LMP
{
	LocalCollectionWatcher::LocalCollectionWatcher (QObject *parent)
	: QObject (parent)
	, Watcher_ (new RecursiveDirWatcher (this))
	, ScanTimer_ (new QTimer (this))
	{
		connect (Watcher_,
				SIGNAL (directoryChanged (QString)),
				this,
				SLOT (handleDirectoryChanged (QString)));

		ScanTimer_->setSingleShot (true);
		connect (ScanTimer_,
				SIGNAL (timeout ()),
				this,
				SLOT (rescanQueue ()));
	}

	void LocalCollectionWatcher::AddPath (const QString& path)
	{
		Watcher_->AddRoot (path);
	}

	void LocalCollectionWatcher::RemovePath (const QString& path)
	{
		Watcher_->RemoveRoot (path);
	}

	void LocalCollectionWatcher::ScheduleDir (const QString& dir)
	{
		if (ScanTimer_->isActive ())
			ScanTimer_->stop ();
		ScanTimer_->start (2000);

		if (std::any_of (ScheduledDirs_.begin (), ScheduledDirs_.end (),
				[&dir] (const QString& other) { return dir.startsWith (other); }))
			return;

		ScheduledDirs_ << dir;
	}

	void LocalCollectionWatcher::handleDirectoryChanged (const QString& path)
	{
		ScheduleDir (path);
	}

	void LocalCollectionWatcher::rescanQueue ()
	{
		for (const auto& path : ScheduledDirs_)
			Core::Instance ().GetLocalCollection ()->Scan (path, false);

		ScheduledDirs_.clear ();
	}
}
}
