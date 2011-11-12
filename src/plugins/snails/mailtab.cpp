/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2011  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "mailtab.h"
#include <QToolBar>
#include <QStandardItemModel>
#include <QTextDocument>
#include <QSortFilterProxyModel>
#include <util/util.h>
#include "core.h"
#include "storage.h"
#include "mailtreedelegate.h"

namespace LeechCraft
{
namespace Snails
{
	MailTab::MailTab (const TabClassInfo& tc, QObject *pmt, QWidget *parent)
	: QWidget (parent)
	, TabToolbar_ (new QToolBar)
	, TabClass_ (tc)
	, PMT_ (pmt)
	, MailModel_ (new QStandardItemModel (this))
	, MailSortFilterModel_ (new QSortFilterProxyModel (this))
	{
		Ui_.setupUi (this);

		Ui_.AccountsTree_->setModel (Core::Instance ().GetAccountsModel ());

		MailSortFilterModel_->setDynamicSortFilter (true);
		MailSortFilterModel_->setSortRole (Roles::Sort);
		MailSortFilterModel_->setSourceModel (MailModel_);
		MailSortFilterModel_->sort (2, Qt::DescendingOrder);
		Ui_.MailTree_->setItemDelegate (new MailTreeDelegate (this));
		Ui_.MailTree_->setModel (MailSortFilterModel_);

		connect (Ui_.AccountsTree_->selectionModel (),
				SIGNAL (currentChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (handleCurrentAccountChanged (QModelIndex)));
		connect (Ui_.MailTree_->selectionModel (),
				SIGNAL (currentChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (handleMailSelected (QModelIndex)));

		QAction *fetch = new QAction (tr ("Fetch new mail"), this);
		fetch->setProperty ("ActionIcon", "fetchall");
		TabToolbar_->addAction (fetch);
		connect (fetch,
				SIGNAL (triggered ()),
				this,
				SLOT (handleFetchNewMail ()));
	}

	TabClassInfo MailTab::GetTabClassInfo () const
	{
		return TabClass_;
	}

	QObject* MailTab::ParentMultiTabs ()
	{
		return PMT_;
	}

	void MailTab::Remove ()
	{
		emit removeTab (this);
		deleteLater ();
	}

	QToolBar* MailTab::GetToolBar () const
	{
		return TabToolbar_;
	}

	void MailTab::handleCurrentAccountChanged (const QModelIndex& idx)
	{
		MailModel_->clear ();
		MailID2Item_.clear ();
		if (CurrAcc_)
			disconnect (CurrAcc_.get (),
					0,
					this,
					0);

		CurrAcc_ = Core::Instance ().GetAccount (idx);
		if (!CurrAcc_)
			return;

		connect (CurrAcc_.get (),
				SIGNAL (gotNewMessages (QList<Message_ptr>)),
				this,
				SLOT (handleGotNewMessages (QList<Message_ptr>)));
		connect (CurrAcc_.get (),
				SIGNAL (messageBodyFetched (Message_ptr)),
				this,
				SLOT (handleMessageBodyFetched (Message_ptr)));

		QStringList headers;
		headers << tr ("From")
				<< tr ("Subject")
				<< tr ("Date")
				<< tr ("Size");
		MailModel_->setHorizontalHeaderLabels (headers);

		handleGotNewMessages (Core::Instance ().GetStorage ()->
					LoadMessages (CurrAcc_.get ()));
	}

	namespace
	{
		QString GetFrom (Message_ptr message)
		{
			const QString& fromName = message->GetFrom ();
			return fromName.isEmpty () ?
					message->GetFromEmail () :
					fromName + " <" + message->GetFromEmail () + ">";
		}
	}

	void MailTab::handleMailSelected (const QModelIndex& sidx)
	{
		if (!CurrAcc_)
		{
			Ui_.MailView_->setHtml (QString ());
			return;
		}

		const QModelIndex& idx = MailSortFilterModel_->mapToSource (sidx);
		const QByteArray& id = idx.sibling (idx.row (), 0)
				.data (Roles::ID).toByteArray ();

		Message_ptr msg;
		try
		{
			msg = Core::Instance ().GetStorage ()->
					LoadMessage (CurrAcc_.get (), id);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to load message"
					<< CurrAcc_->GetID ().toHex ()
					<< id.toHex ()
					<< e.what ();

			const QString& html = tr ("<h2>Unable to load mail</h2><em>%1</em>").arg (e.what ());
			Ui_.MailView_->setHtml (html);
			return;
		}

		msg->SetRead (true);
		Core::Instance ().GetStorage ()->SaveMessages (CurrAcc_.get (), { msg });

		if (!msg->IsFullyFetched ())
			CurrAcc_->FetchWholeMessage (msg);

		QString html;
		html += "<em>Subject</em>: %1<br />";
		html += "<em>From</em>: %2<br />";
		html += "<em>On</em>: %3<hr />";
		html += "%4";

		const QString& htmlBody = msg->IsFullyFetched () ?
				msg->GetHTMLBody () :
				"<em>" + tr ("Fetching the message...") + "</em>";

		Ui_.MailView_->setHtml (html
				.arg (msg->GetSubject ())
				.arg (Qt::escape (GetFrom (msg)))
				.arg (msg->GetDate ().toString ())
				.arg (htmlBody.isEmpty () ?
						"<pre>" + Qt::escape (msg->GetBody ()) + "</pre> " :
						htmlBody));
	}

	void MailTab::handleFetchNewMail ()
	{
		Q_FOREACH (auto acc, Core::Instance ().GetAccounts ())
			acc->FetchNewHeaders (Account::FetchNew);
	}

	void MailTab::handleMessageBodyFetched (Message_ptr msg)
	{
		const QModelIndex& cur = Ui_.MailTree_->currentIndex ();
		if (cur.data (Roles::ID).toByteArray () != msg->GetID ())
			return;

		handleMailSelected (cur);
	}

	void MailTab::handleGotNewMessages (QList<Message_ptr> messages)
	{
		Q_FOREACH (Message_ptr message, messages)
		{
			if (MailID2Item_.contains (message->GetID ()))
				MailModel_->removeRow (MailID2Item_ [message->GetID ()]->row ());

			QList<QStandardItem*> row;
			row << new QStandardItem (GetFrom (message));
			row << new QStandardItem (message->GetSubject ());
			row << new QStandardItem (message->GetDate ().toString ());
			row << new QStandardItem (Util::MakePrettySize (message->GetSize ()));
			MailModel_->appendRow (row);

			row [Columns::From]->setData (row [Columns::From]->text (), Roles::Sort);
			row [Columns::Subj]->setData (row [Columns::Subj]->text (), Roles::Sort);
			row [Columns::Date]->setData (message->GetDate (), Roles::Sort);
			row [Columns::Size]->setData (message->GetSize (), Roles::Sort);

			Q_FOREACH (auto item, row)
				item->setData (message->GetID (), Roles::ID);
			MailID2Item_ [message->GetID ()] = row.first ();

			updateReadStatus (message->GetID (), message->IsRead ());

			connect (message.get (),
					SIGNAL (readStatusChanged (const QByteArray&, bool)),
					this,
					SLOT (updateReadStatus (const QByteArray&, bool)),
					Qt::QueuedConnection);
		}
	}

	void MailTab::updateReadStatus (const QByteArray& id, bool isRead)
	{
		qDebug () << Q_FUNC_INFO << id << isRead << MailID2Item_.contains (id);
		if (!MailID2Item_.contains (id))
			return;

		QStandardItem *item = MailID2Item_ [id];
		const QModelIndex& sIdx = item->index ();
		for (int i = 0; i < Columns::Max; ++i)
		{
			QStandardItem *other = MailModel_->
					itemFromIndex (sIdx.sibling (sIdx.row (), i));
			other->setData (isRead, Roles::ReadStatus);
			other->setText (other->text ());
		}
	}
}
}
