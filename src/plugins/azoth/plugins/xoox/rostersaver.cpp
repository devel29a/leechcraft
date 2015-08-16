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

#include "rostersaver.h"
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QtDebug>
#include <util/sys/paths.h>
#include <interfaces/azoth/iproxyobject.h>
#include "glooxprotocol.h"
#include "glooxaccount.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	RosterSaver::RosterSaver (GlooxProtocol *proto, IProxyObject *proxy, QObject *parent)
	: QObject { parent }
	, Proto_ { proto }
	, Proxy_ { proxy }
	{
		LoadRoster ();

		for (const auto account : Proto_->GetRegisteredAccounts ())
		{
			connect (account,
					SIGNAL (gotCLItems (QList<QObject*>)),
					this,
					SLOT (handleItemsAdded (QList<QObject*>)));
			connect (account,
					SIGNAL (rosterSaveRequested ()),
					this,
					SLOT (scheduleSaveRoster ()));
		}
	}

	void RosterSaver::LoadRoster ()
	{
		QFile rosterFile { Util::CreateIfNotExists ("azoth/xoox").absoluteFilePath ("roster.xml") };
		if (!rosterFile.exists ())
			return;
		if (!rosterFile.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open roster file"
					<< rosterFile.fileName ()
					<< rosterFile.errorString ();
			return;
		}

		QDomDocument doc;
		QString errStr;
		int errLine = 0;
		int errCol = 0;
		if (!doc.setContent (&rosterFile, &errStr, &errLine, &errCol))
		{
			qWarning () << Q_FUNC_INFO
					<< errStr
					<< errLine
					<< ":"
					<< errCol;
			return;
		}

		QDomElement root = doc.firstChildElement ("roster");
		if (root.attribute ("formatversion") != "1")
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown format version"
					<< root.attribute ("formatversion");
			return;
		}

		QMap<QByteArray, GlooxAccount*> id2account;
		for (const auto accObj : Proto_->GetRegisteredAccounts ())
		{
			const auto acc = qobject_cast<GlooxAccount*> (accObj);
			id2account [acc->GetAccountID ()] = acc;
		}

		auto account = root.firstChildElement ("account");
		while (!account.isNull ())
		{
			const auto& id = account.firstChildElement ("id").text ().toUtf8 ();
			if (id.isEmpty ())
			{
				qWarning () << Q_FUNC_INFO
						<< "empty ID";
				continue;
			}

			if (!id2account.contains (id))
			{
				account = account.nextSiblingElement ("account");
				continue;
			}

			auto entry = account
					.firstChildElement ("entries")
					.firstChildElement ("entry");
			while (!entry.isNull ())
			{
				const auto& entryID = QByteArray::fromPercentEncoding (entry
								.firstChildElement ("id").text ().toLatin1 ());

				if (entryID.isEmpty ())
					qWarning () << Q_FUNC_INFO
							<< "entry ID is empty";
				else
				{
					const auto ods = std::make_shared<OfflineDataSource> ();
					Load (ods, entry, Proxy_);

					id2account [id]->CreateFromODS (ods);
				}
				entry = entry.nextSiblingElement ("entry");
			}

			account = account.nextSiblingElement ("account");
		}
	}

	void RosterSaver::scheduleSaveRoster (int hint)
	{
		if (SaveRosterScheduled_)
			return;

		SaveRosterScheduled_ = true;
		QTimer::singleShot (hint,
				this,
				SLOT (saveRoster ()));
	}

	void RosterSaver::saveRoster ()
	{
		SaveRosterScheduled_ = false;
		QFile rosterFile (Util::CreateIfNotExists ("azoth/xoox")
					.absoluteFilePath ("roster.xml"));
		if (!rosterFile.open (QIODevice::WriteOnly | QIODevice::Truncate))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open file"
					<< rosterFile.fileName ()
					<< rosterFile.errorString ();
			return;
		}

		QXmlStreamWriter w (&rosterFile);
		w.setAutoFormatting (true);
		w.setAutoFormattingIndent (2);
		w.writeStartDocument ();
		w.writeStartElement ("roster");
		w.writeAttribute ("formatversion", "1");
		for (auto accObj : Proto_->GetRegisteredAccounts ())
		{
			auto acc = qobject_cast<IAccount*> (accObj);
			w.writeStartElement ("account");
				w.writeTextElement ("id", acc->GetAccountID ());
				w.writeStartElement ("entries");
				for (auto entryObj : acc->GetCLEntries ())
				{
					const auto entry = qobject_cast<GlooxCLEntry*> (entryObj);
					if (!entry ||
							(entry->GetEntryFeatures () & ICLEntry::FMaskLongetivity) != ICLEntry::FPermanentEntry)
						continue;

					Save (entry->ToOfflineDataSource (), &w, Proxy_);
				}
				w.writeEndElement ();
			w.writeEndElement ();
		}
		w.writeEndElement ();
		w.writeEndDocument ();
	}

	void RosterSaver::handleItemsAdded (const QList<QObject*>& items)
	{
		if (std::any_of (items.begin (), items.end (), [] (QObject *clEntry)
				{
					if (const auto entry = qobject_cast<GlooxCLEntry*> (clEntry))
					{
						const auto lng = entry->GetEntryFeatures () & ICLEntry::FMaskLongetivity;
						return lng == ICLEntry::FPermanentEntry;
					}
					return false;
				}))
			scheduleSaveRoster (5000);
	}
}
}
}