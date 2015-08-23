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

#include "vcardstorageondisk.h"
#include <QDir>
#include <QSqlError>
#include <util/db/dblock.h>
#include <util/db/util.h>
#include <util/db/oral.h>
#include <util/sys/paths.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	struct VCardStorageOnDisk::Record
	{
		Util::oral::PKey<QString, Util::oral::NoAutogen> JID_;
		QString VCardIq_;

		static QString ClassName ()
		{
			return "VCards";
		}

		static QString FieldNameMorpher (const QString& str)
		{
			return str.left (str.size () - 1);
		}
	};
}
}
}

using VCardRecord = LeechCraft::Azoth::Xoox::VCardStorageOnDisk::Record;

BOOST_FUSION_ADAPT_STRUCT (VCardRecord,
		(decltype (VCardRecord::JID_), JID_)
		(decltype (VCardRecord::VCardIq_), VCardIq_))

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	namespace sph = Util::oral::sph;

	VCardStorageOnDisk::VCardStorageOnDisk (QObject *parent)
	: QObject { parent }
	{
		DB_.setDatabaseName (Util::CreateIfNotExists ("azoth/xoox").filePath ("vcards.db"));
		if (!DB_.open ())
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot open the database";
			Util::DBLock::DumpError (DB_.lastError ());
			throw std::runtime_error { "Cannot create database" };
		}

		Util::RunTextQuery (DB_, "PRAGMA synchronous = NORMAL;");
		Util::RunTextQuery (DB_, "PRAGMA journal_mode = WAL;");

		AdaptedRecord_ = Util::oral::AdaptPtr<Record> (DB_);

	}

	void VCardStorageOnDisk::SetVCard (const QString& jid, const QString& vcard)
	{
		AdaptedRecord_->DoInsert_ ({ jid, vcard });
	}

	boost::optional<QString> VCardStorageOnDisk::GetVCard (const QString& jid) const
	{
		const auto& result = AdaptedRecord_->DoSelectByFields_ (sph::_0 == jid);
		if (result.isEmpty ())
			return {};

		return result.front ().VCardIq_;
	}
}
}
}
