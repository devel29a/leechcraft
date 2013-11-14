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

#include <memory>
#include <QObject>
#include <QSet>
#include <interfaces/blogique/iaccount.h>
#include "profiletypes.h"
#include "ljfriendentry.h"
#include "entryoptions.h"

namespace LeechCraft
{
namespace Blogique
{
namespace Metida
{
	class LJFriendEntry;
	class LJAccountConfigurationWidget;
	class LJBloggingPlatform;
	class LJXmlRPC;
	class LJProfile;

	struct LJEventProperties
	{
		QString CurrentLocation_;
		QString CurrentMood_;
		int CurrentMoodId_;
		QString CurrentMusic_;
		bool ShowInFriendsPage_;
		bool AutoFormat_;
		AdultContent AdultContent_;
		CommentsManagement CommentsManagement_;
		CommentsManagement ScreeningComments_;
		QString PostAvatar_;
		bool EntryVisibility_;
		bool UsedRTE_;
		bool NotifyByEmail_;
		QStringList LikeButtons_;
		QUrl RepostUrl_;
		bool IsRepost_;

		LJEventProperties ()
		: CurrentMoodId_ (-1)
		, ShowInFriendsPage_ (true)
		, AutoFormat_ (true)
		, AdultContent_ (AdultContent::WithoutAdultContent)
		, CommentsManagement_ (CommentsManagement::EnableComments)
		, ScreeningComments_ (CommentsManagement::Default)
		, EntryVisibility_ (true)
		, UsedRTE_ (true)
		, NotifyByEmail_ (true)
		, IsRepost_ (false)
		{
		}
	};

	struct LJEvent
	{
		//for posting
		QString Event_;
		QString Subject_;
		Access Security_;
		quint32 AllowMask_;
		QDateTime DateTime_;
		QStringList Tags_;
		QString TimeZone_;
		QString UseJournal_;
		LJEventProperties Props_;

		// for getting
		qlonglong ItemID_;
		qlonglong DItemID_;
		uint ANum_;
		QUrl Url_;

		LJEvent ()
		: Security_ (Access::Public)
		, AllowMask_ (0)
		, ItemID_ (-1)
		, DItemID_ (-1)
		, ANum_ (0)
		{
		}
	};

	class LJAccount : public QObject
					, public IAccount
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Blogique::IAccount)

		LJBloggingPlatform *ParentBloggingPlatform_;
		LJXmlRPC *LJXmlRpc_;
		QString Name_;
		QString Login_;
		bool IsValid_;
		std::shared_ptr<LJProfile> LJProfile_;

		QAction *LoadLastEvents_;
		QAction *LoadChangedEvents_;

		enum class LastUpdateType
		{
			NoType,
			LastEntries,
			ChangedEntries
		};
		LastUpdateType LastUpdateType_;

	public:
		LJAccount (const QString& name, QObject *parent = 0);

		QObject* GetQObject ();
		QObject* GetParentBloggingPlatform () const;
		QString GetAccountName () const;
		QString GetOurLogin () const;
		void RenameAccount (const QString& name);
		QByteArray GetAccountID () const;
		void OpenConfigurationDialog ();
		bool IsValid () const;

		QString GetPassword () const;

		QObject* GetProfile ();

		void GetEntriesByDate (const QDate& date);
		void GetEntriesWithFilter (const Filter& filter);
		void RemoveEntry (const Entry& entry);
		void UpdateEntry (const Entry& entry);

		void RequestLastEntries (int count);
		void RequestStatistics ();
		void RequestTags ();

		void RequestInbox ();

		void RequestRecentComments ();
		void AddComment (const CommentEntry& comment);
		void DeleteComment (qint64 id, bool deleteThread = false);

		QList<QAction*> GetUpdateActions () const;

		void FillSettings (LJAccountConfigurationWidget *widget);

		QByteArray Serialize () const;
		static LJAccount* Deserialize (const QByteArray& data, QObject *parent);

		void Validate ();
		void Init ();

		void AddFriends (const QList<LJFriendEntry_ptr>& friends);
		void AddNewFriend (const QString& username,
				const QString& bgcolor, const QString& fgcolor, uint groupMask);
		void DeleteFriend (const QString& username);

		void AddGroup (const QString& name, bool isPublic, int id);
		void DeleteGroup (int id);

		void SetMessagesAsRead (const QList<int>& ids);
		void SendMessage (const QStringList& addresses, const QString& subject,
				const QString& text);
	private:
		void CallLastUpdateMethod ();

	public slots:
		void handleValidatingFinished (bool success);
		void updateProfile ();

		void submit (const Entry& event);
		void preview (const Entry& event);

		void handleEventPosted (const QList<LJEvent>& entries);
		void handleEventUpdated (const QList<LJEvent>& entries);
		void handleEventRemoved (int id);

		void handleGotFilteredEvents (const QList<LJEvent>& events);
		void handleGettingFilteredEventsFinished ();
		void handleGotEvents (const QList<LJEvent>& events);

		void handleLoadLastEvents ();
		void handleLoadChangedEvents ();

		void handleUnreadMessagesIds (const QList<int>& ids);
		void handleMessagesRead ();
		void handleMessageSent ();

		void handleGotRecentComments (const QList<LJCommentEntry>& comments);

	signals:
		void accountRenamed (const QString& newName);
		void accountSettingsChanged ();
		void accountValidated (bool validated);

		void requestEntriesBegin ();

		void entryPosted (const QList<Entry>& entries);
		void entryUpdated (const QList<Entry>& entries);
		void entryRemoved (int itemId);

		void gotError(int errorCode, const QString& errorString,
				const QString& localizedErrorString = QString ());

		void gotFilteredEntries (const QList<Entry>& entries);
		void gettingFilteredEntriesFinished ();

		void gotEntries (const QList<Entry>& entries);

		void gotBlogStatistics (const QMap<QDate, int>& statistics);
		void tagsUpdated (const QHash<QString, int>& tags);

		void gotRecentComments (const QList<CommentEntry>& comments);
	};
}
}
}
