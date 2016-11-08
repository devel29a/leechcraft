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

#pragma once

#include <memory>
#include <QModelIndex>
#include <interfaces/iinfo.h>
#include <interfaces/idownload.h>
#include <interfaces/ijobholder.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/structures.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>

class QTabWidget;
class QToolBar;

namespace boost
{
	namespace logic
	{
		class tribool;
	}
}

namespace LeechCraft
{
namespace CSTP
{
	class Core;

	class CSTP : public QObject
				, public IInfo
				, public IDownload
				, public IJobHolder
				, public IHaveSettings
	{
		Q_OBJECT
		Q_INTERFACES (IInfo IDownload IJobHolder IHaveSettings)

		LC_PLUGIN_METADATA ("org.LeechCraft.CSTP")

		QMenu *Plugins_;
		Util::XmlSettingsDialog_ptr XmlSettingsDialog_;
		QToolBar *Toolbar_;
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		void Release ();
		QByteArray GetUniqueID () const;
		QString GetName () const;
		QString GetInfo () const;
		QStringList Provides () const;
		QIcon GetIcon () const;

		qint64 GetDownloadSpeed () const;
		qint64 GetUploadSpeed () const;
		void StartAll ();
		void StopAll ();
		EntityTestHandleResult CouldDownload (const LeechCraft::Entity&) const;
		int AddJob (LeechCraft::Entity);
		void KillTask (int);

		QAbstractItemModel* GetRepresentation () const;

		Util::XmlSettingsDialog_ptr GetSettingsDialog () const;
	private:
		void SetupToolbar ();
	private slots:
		void handleTasksTreeSelectionCurrentRowChanged (const QModelIndex&, const QModelIndex&);
		void handleFileExists (boost::logic::tribool*);
		void handleError (const QString&);
	signals:
		void jobFinished (int);
		void jobRemoved (int);
		void jobError (int, IDownload::Error);
		void gotEntity (const LeechCraft::Entity&);
	};
}
}
