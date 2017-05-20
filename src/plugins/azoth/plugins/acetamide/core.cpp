/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011 Oleg Linkin
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
#include <QStandardItemModel>
#include <QRegExp>
#include <interfaces/azoth/iaccount.h>
#include <interfaces/azoth/iproxyobject.h>
#include "ircprotocol.h"
#include "nickservidentifywidget.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Acetamide
{
	Core::Core ()
	: IrcProtocol_ { std::make_shared<IrcProtocol> () }
	, Model_ { new QStandardItemModel { this } }
	, NickServIdentifyWidget_ { new NickServIdentifyWidget { Model_ } }
	{
	}

	Core& Core::Instance ()
	{
		static Core c;
		return c;
	}

	void Core::SecondInit ()
	{
		IrcProtocol_->SetProxyObject (PluginProxy_);
		IrcProtocol_->Prepare ();
	}

	void Core::Release ()
	{
		IrcProtocol_.reset ();
	}

	QList<QObject*> Core::GetProtocols () const
	{
		QList<QObject*> result;
		result << IrcProtocol_.get ();
		return result;
	}

	void Core::SetPluginProxy (QObject *proxy)
	{
		PluginProxy_ = proxy;
	}

	IProxyObject* Core::GetPluginProxy () const
	{
		return qobject_cast<IProxyObject*> (PluginProxy_);
	}

	void Core::SetProxy (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
	}

	ICoreProxy_ptr Core::GetProxy () const
	{
		return Proxy_;
	}

	void Core::SendEntity (const LeechCraft::Entity& e)
	{
		emit gotEntity (e);
	}

	void Core::handleItemsAdded (const QList<QObject*>&)
	{
	}

	NickServIdentifyWidget* Core::GetNickServIdentifyWidget () const
	{
		return NickServIdentifyWidget_;
	}

	QStandardItemModel* Core::GetNickServIdentifyModel () const
	{
		return Model_;
	}

	void Core::AddNickServIdentify (const NickServIdentify& nsi)
	{
		if (NickServIdentifyList_.contains (nsi))
			return;

		NickServIdentifyList_ << nsi;
	}

	QList<NickServIdentify> Core::GetAllNickServIdentify () const
	{
		return NickServIdentifyList_;
	}

	QList<NickServIdentify> Core::GetNickServIdentifyWithNick (const QString& nick) const
	{
		QList<NickServIdentify> list;
		for (const auto& nsi : NickServIdentifyList_)
			if (nsi.Nick_ == nick)
				list << nsi;
		return list;
	}

	QList<NickServIdentify> Core::GetNickServIdentifyWithNickServ (const QString& nickserv) const
	{
		QList<NickServIdentify> list;
		for (const auto& nsi : NickServIdentifyList_)
		if (nsi.NickServNick_ == nickserv)
			list << nsi;
		return list;
	}

	QList<NickServIdentify> Core::GetNickServIdentifyWithServ (const QString& server) const
	{
		QList<NickServIdentify> list;
		for (const auto& nsi : NickServIdentifyList_)
		if (nsi.Server_ == server)
			list << nsi;
		return list;
	}

	QList<NickServIdentify> Core::GetNickServIdentifyWithMainParams (const QString& server,
			const QString& nick, const QString& nickserv) const
	{
		QList<NickServIdentify> list;
		for (const auto& nsi : NickServIdentifyList_)
		{
			QRegExp nickMask (nsi.NickServNick_,
					Qt::CaseInsensitive,
					QRegExp::Wildcard);
			if ((nsi.Server_ == server) &&
					(nsi.Nick_ == nick) &&
					(nickMask.indexIn (nickserv) == 0))
				list << nsi;
		}
		return list;
	}
}
}
}
