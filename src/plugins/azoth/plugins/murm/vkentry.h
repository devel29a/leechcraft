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
#include <QObject>
#include <QImage>
#include <QPointer>
#include <QStringList>
#include <util/sll/util.h>
#include <interfaces/azoth/imetainfoentry.h>
#include <interfaces/azoth/ihaveavatars.h>
#include "structures.h"
#include "entrybase.h"

class QTimer;

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	class VkAccount;
	class VkMessage;
	class PhotoStorage;
	class VCardDialog;
	class VkChatEntry;

	class VkEntry : public EntryBase
				  , public IMetaInfoEntry
				  , public IHaveAvatars
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Azoth::IMetaInfoEntry LeechCraft::Azoth::IHaveAvatars)

		UserInfo Info_;

		QTimer *RemoteTypingTimer_;
		QTimer *LocalTypingTimer_;

		bool IsSelf_ = false;
		bool IsNonRoster_ = false;
		bool HasUnread_ = false;

		QImage AppImage_;

		QPointer<VCardDialog> VCardDialog_;

		QStringList Groups_;

		QList<VkChatEntry*> Chats_;
	public:
		VkEntry (const UserInfo&, VkAccount*);

		void UpdateInfo (const UserInfo&, bool spontaneous = true);
		const UserInfo& GetInfo () const;

		void UpdateAppInfo (const AppInfo&, const QImage&);

		void Send (VkMessage*) override;

		void SetSelf ();
		void SetNonRoster ();
		bool IsNonRoster () const;

		Util::DefaultScopeGuard RegisterIn (VkChatEntry*);
		void ReemitGroups ();

		VkMessage* FindMessage (qulonglong) const;
		void HandleMessage (MessageInfo, const FullMessageInfo&);

		void HandleTypingNotification ();

		Features GetEntryFeatures () const override;
		EntryType GetEntryType () const override;
		QString GetEntryName () const override;
		void SetEntryName (const QString& name) override;
		QString GetEntryID () const override;
		QString GetHumanReadableID () const override;
		QStringList Groups () const override;
		void SetGroups (const QStringList& groups) override;
		QStringList Variants () const override;
		void SetChatPartState (ChatPartState state, const QString& variant) override;
		EntryStatus GetStatus (const QString& variant = QString ()) const override;
		QImage GetAvatar () const override;
		void ShowInfo () override;
		QList<QAction*> GetActions () const override;
		QMap<QString, QVariant> GetClientInfo (const QString&) const override;
		void MarkMsgsRead () override;
		void ChatTabClosed () override;

		QVariant GetMetaInfo (DataField) const override;
		QList<QPair<QString, QVariant>> GetVCardRepresentation () const override;

		QFuture<QImage> RefreshAvatar (Size) override;
		bool HasAvatar () const override;
		bool SupportsSize (Size) const override;
	private slots:
		void handleTypingTimeout ();
		void sendTyping ();

		void handleEntryNameFormat ();
	protected:
		using EntryBase::avatarChanged;
	signals:
		void vcardUpdated () override;
		void avatarChanged (QObject*) override;
	};
}
}
}
