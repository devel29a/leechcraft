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

#include <QWidget>
#include <interfaces/ihavetabs.h>
#include "ui_composemessagetab.h"
#include "account.h"

class QSignalMapper;
class IEditorWidget;

namespace LeechCraft
{
namespace Snails
{
	class ComposeMessageTab : public QWidget
							, public ITabWidget
	{
		Q_OBJECT
		Q_INTERFACES (ITabWidget)

		static QObject *S_ParentPlugin_;
		static TabClassInfo S_TabClassInfo_;

		Ui::ComposeMessageTab Ui_;

		QToolBar *Toolbar_;
		QMenu *AccountsMenu_;
		QMenu *AttachmentsMenu_;
		QMenu *EditorsMenu_;

		QSignalMapper *EditorsMapper_;

		QList<QWidget*> MsgEditWidgets_;
		QList<IEditorWidget*> MsgEdits_;

		Message_ptr ReplyMessage_;
	public:
		static void SetParentPlugin (QObject*);
		static void SetTabClassInfo (const TabClassInfo&);

		ComposeMessageTab (QWidget* = 0);

		TabClassInfo GetTabClassInfo () const;
		QObject* ParentMultiTabs();
		void Remove();
		QToolBar* GetToolBar() const;

		void SelectAccount (const Account_ptr&);
		void PrepareReply (const Message_ptr&);
	private:
		void PrepareReplyEditor (const Message_ptr&);
		void PrepareReplyBody (const Message_ptr&);

		void SetupToolbar ();
		void SetupEditors ();

		void SelectPlainEditor ();
		void SelectHtmlEditor ();

		IEditorWidget* GetCurrentEditor () const;

		void SetMessageReferences (const Message_ptr&) const;
	private slots:
		void handleMessageSent ();

		void handleSend ();
		void handleAddAttachment ();
		void handleRemoveAttachment ();

		void handleEditorSelected (int);
	signals:
		void removeTab (QWidget*);
	};
}
}
