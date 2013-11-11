/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2012  Oleg Linkin
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

#include <QObject>
#include <QSet>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/structures.h>
#include "interfaces/blogique/iaccount.h"

class QTimer;

namespace LeechCraft
{
namespace Blogique
{
	struct Entry;
	class BlogiqueWidget;
	class StorageManager;
	class IAccount;
	class IBloggingPlatform;
	class PluginProxy;

	class Core : public QObject
	{
		Q_OBJECT

		QByteArray UniqueID_;
		ICoreProxy_ptr Proxy_;
		QObjectList BlogPlatformPlugins_;
		std::shared_ptr<PluginProxy> PluginProxy_;
		StorageManager *StorageManager_;

		QTimer *AutoSaveTimer_;
		QTimer *CommentsCheckingTimer_;

		Core ();
		Q_DISABLE_COPY (Core)

	public:
		static Core& Instance ();

		QByteArray GetUniqueID () const;
		QIcon GetIcon () const;

		void SetCoreProxy (ICoreProxy_ptr proxy);
		ICoreProxy_ptr GetCoreProxy ();

		QSet<QByteArray> GetExpectedPluginClasses () const;
		void AddPlugin (QObject *plugin);

		QList<IBloggingPlatform*> GetBloggingPlatforms () const;
		QList<IAccount*> GetAccounts () const;

		void SendEntity (const Entity& e);
		void DelayedProfilesUpdate ();

		StorageManager* GetStorageManager () const;

		BlogiqueWidget* CreateBlogiqueWidget ();
	private:
		void AddBlogPlatformPlugin (QObject *plugin);

	private slots:
		void handleNewBloggingPlatforms (const QObjectList& platforms);
		void addAccount (QObject *accObj);
		void handleAccountRemoved (QObject *accObj);
		void handleAccountValidated (QObject *accObj, bool validated);
		void updateProfiles ();
		void handleEntryPosted (const QList<Entry>& entries);
		void handleEntryRemoved (int itemId);
		void handleEntryUpdated (const QList<Entry>& entries);

		void handleAutoSaveIntervalChanged ();
		void handleCommentsCheckingChanged ();
		void handleCommentsCheckingTimerChanged ();
		void handleGotRecentComments (const QList<RecentComment>& comments);

		void exportBlog ();
		
		void checkForComments ();

	signals:
		void accountAdded (QObject *account);
		void accountRemoved (QObject *account);
		void accountValidated (QObject *account, bool validated);

		void gotEntity (LeechCraft::Entity e);
		void delegateEntity (LeechCraft::Entity e, int *id, QObject **obj);

		void addNewTab (const QString& name, QWidget *tab);
		void removeTab (QWidget *tab);
		void changeTabName (QWidget *content, const QString& name);

		void checkAutoSave ();

		void requestEntriesBegin ();

		void entryPosted ();
		void entryRemoved ();

		void tagsUpdated (const QHash<QString, int>& tags);

		void insertTag (const QString& tag);
		
		void gotRecentComments (const QByteArray& accountId,
				const QList<RecentComment>& comments);

		void gotError (int errorCode, const QString& errorString,
				const QString& localizedErrorString);
	};
}
}
