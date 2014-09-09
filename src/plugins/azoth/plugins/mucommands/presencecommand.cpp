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

#include "presencecommand.h"
#include <functional>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <QList>
#include <interfaces/azoth/iprovidecommands.h>
#include <interfaces/azoth/iaccount.h>
#include <interfaces/azoth/iproxyobject.h>
#include <interfaces/azoth/ihavedirectedstatus.h>

namespace LeechCraft
{
namespace Azoth
{
namespace MuCommands
{
	namespace
	{
		State String2State (const QString& str)
		{
			if (str.startsWith ("avail"))
				return SOnline;
			else if (str == "away")
				return SAway;
			else if (str == "xa")
				return SXA;
			else if (str == "dnd")
				return SDND;
			else if (str == "chat")
				return SChat;

			throw CommandException { QObject::tr ("Unknown status %1.")
						.arg ("<em>" + str + "</em>") };
		}

		std::function<EntryStatus (EntryStatus)> GetStatusMangler (IProxyObject *proxy, const QString& text)
		{
			const auto& action = text.section ('\n', 1, 1).trimmed ();
			const auto& message = text.section ('\n', 2).trimmed ();
			if (action == "clear")
				return [] (const EntryStatus& status)
						{ return EntryStatus { status.State_, {} }; };
			else if (action == "message")
				return [message] (const EntryStatus& status)
						{ return EntryStatus { status.State_, message }; };
			else if (const auto status = proxy->FindCustomStatus (action))
				return [status, message] (const EntryStatus&)
						{ return EntryStatus { status->State_, status->Text_}; };
			else
			{
				const auto state = String2State (action);
				return [state, message] (const EntryStatus&)
						{ return EntryStatus { state, message }; };
			}
		}
		struct AllAccounts {};

		struct CurrentAccount {};

		typedef boost::variant<AllAccounts, std::string, CurrentAccount> AccName_t;

		struct ClearStatus {};
		typedef boost::variant<State, std::string> State_t;
		typedef std::pair<State, std::string> FullState_t;
		typedef boost::variant<FullState_t, State_t, ClearStatus, std::string> Status_t;

		struct PresenceParams
		{
			AccName_t AccName_;
			Status_t Status_;
		};
	}
}
}
}

BOOST_FUSION_ADAPT_STRUCT (LeechCraft::Azoth::MuCommands::PresenceParams,
		(LeechCraft::Azoth::MuCommands::AccName_t, AccName_)
		(LeechCraft::Azoth::MuCommands::Status_t, Status_))

namespace LeechCraft
{
namespace Azoth
{
namespace MuCommands
{
	namespace Parse
	{
		enum Option
		{
			NoOptions = 0x00,
			NoAccount = 0x01
		};

		Q_DECLARE_FLAGS (Options, Option)
	}

	namespace
	{
		namespace ascii = boost::spirit::ascii;
		namespace qi = boost::spirit::qi;
		namespace phoenix = boost::phoenix;

		template<typename Iter>
		struct Parser : qi::grammar<Iter, PresenceParams ()>
		{
			qi::rule<Iter, PresenceParams ()> Start_;

			qi::rule<Iter, AllAccounts ()> AllAccounts_;
			qi::rule<Iter, CurrentAccount ()> CurAcc_;
			qi::rule<Iter, AccName_t ()> AccName_;

			qi::rule<Iter, State_t ()> State_;
			qi::rule<Iter, FullState_t ()> FullState_;
			qi::rule<Iter, ClearStatus ()> ClearStatus_;
			qi::rule<Iter, std::string ()> StateMessageOnly_;
			qi::rule<Iter, Status_t ()> Status_;

			const struct StateSymbs : qi::symbols<char, State>
			{
				StateSymbs ()
				{
					add ("avail", SOnline)
						("available", SOnline)
						("online", SOnline)
						("away", SAway)
						("xa", SXA)
						("dnd", SDND)
						("chat", SChat)
						("off", SOffline)
						("offline", SOffline);
				}
			} PredefinedState_;

			const struct CustomStatuses : qi::symbols<char, std::string>
			{
				CustomStatuses (const std::vector<std::string>& customStatuses)
				{
					for (const auto& str : customStatuses)
						add (str, str);
				}
			} CustomStatus_;

			Parser (const std::vector<std::string>& customStatuses, Parse::Options options)
			: Parser::base_type { Start_ }
			, CustomStatus_ { customStatuses }
			{
				AllAccounts_ = qi::lit ('*') > qi::attr (AllAccounts ());
				CurAcc_ = qi::eps > qi::attr (CurrentAccount ());
				AccName_ = AllAccounts_ | +(qi::char_ - '\n') | CurAcc_;

				FullState_ = PredefinedState_ >> '\n' >> +qi::char_;

				ClearStatus_ = (qi::lit ("clear") >> qi::eoi) > qi::attr (ClearStatus ());
				State_ = PredefinedState_ | CustomStatus_;
				StateMessageOnly_ = -qi::lit ('\n') >> +qi::char_;
				Status_ = FullState_ | State_ | ClearStatus_ | StateMessageOnly_;

				if (options & Parse::NoAccount)
					Start_ = qi::attr (CurrentAccount ()) >> -qi::lit ('\n') >> Status_;
				else
					Start_ = AccName_ >> '\n' >> Status_;
			}
		};

		template<typename Iter>
		PresenceParams ParseCommand (Iter begin, Iter end,
				const std::vector<std::string>& statuses = {},
				Parse::Options opts = Parse::NoOptions)
		{
			PresenceParams res;
			qi::parse (begin, end, Parser<Iter> { statuses, opts }, res);
			return res;
		}

		PresenceParams ParseCommand (QString cmd,
				const QString& marker,
				const QStringList& statuses = {},
				Parse::Options opts = Parse::NoOptions)
		{
			cmd = cmd.trimmed ();
			cmd = cmd.mid (marker.size ());
			if (cmd.startsWith (' '))
				cmd = cmd.mid (1);
			const auto& unicode = cmd.toUtf8 ();

			std::vector<std::string> convertedStatuses;
			for (const auto& status : statuses)
				convertedStatuses.push_back (status.toUtf8 ().constData ());

			return ParseCommand (unicode.begin (), unicode.end (), convertedStatuses, opts);
		}

		PresenceParams ParsePresenceCommand (QString cmd,
				const QStringList& statuses = {})
		{
			return ParseCommand (cmd, "/presence", statuses, Parse::NoOptions);
		}

		struct AccountsVisitor : boost::static_visitor<QList<IAccount*>>
		{
			const IProxyObject * const Proxy_;
			const ICLEntry * const Entry_;

			AccountsVisitor (const IProxyObject *proxy, const ICLEntry *entry)
			: Proxy_ { proxy }
			, Entry_ { entry }
			{
			}

			QList<IAccount*> operator() (const AllAccounts&) const
			{
				QList<IAccount*> allAccs;
				for (const auto accObj : Proxy_->GetAllAccounts ())
					allAccs << qobject_cast<IAccount*> (accObj);
				return allAccs;
			}

			QList<IAccount*> operator() (const CurrentAccount&) const
			{
				return { qobject_cast<IAccount*> (Entry_->GetParentAccount ()) };
			}

			QList<IAccount*> operator() (const std::string& accName) const
			{
				const auto& accNameStr = QString::fromUtf8 (accName.c_str ());
				for (const auto acc : (*this) (AllAccounts {}))
					if (acc->GetAccountName () == accNameStr)
						return { acc };

				throw CommandException { QObject::tr ("Unable to find account %1.")
							.arg ("<em>" + accNameStr + "</em>") };
			}
		};

		typedef std::function<EntryStatus (EntryStatus)> StatusMangler_f;

		struct StatusManglerVisitor : boost::static_visitor<StatusMangler_f>
		{
			const IProxyObject * const Proxy_;

			StatusManglerVisitor (const IProxyObject *proxy)
			: Proxy_ { proxy }
			{
			}

			StatusMangler_f operator() (const FullState_t& fullState) const
			{
				const auto& statusStr = QString::fromUtf8 (fullState.second.c_str ());
				const EntryStatus status { fullState.first, statusStr };
				return [status] (const EntryStatus&) { return status; };
			}

			StatusMangler_f operator() (const State_t& state) const
			{
				struct StateVisitor : boost::static_visitor<StatusMangler_f>
				{
					const IProxyObject * const Proxy_;

					StateVisitor (const IProxyObject *proxy)
					: Proxy_ { proxy }
					{
					}

					StatusMangler_f operator() (State state) const
					{
						return [state] (const EntryStatus& status) { return EntryStatus { state, status.StatusString_ }; };
					}

					StatusMangler_f operator() (const std::string& stateName) const
					{
						const auto& stateNameStr = QString::fromUtf8 (stateName.c_str ());
						const auto& customStatus = Proxy_->FindCustomStatus (stateNameStr);
						if (!customStatus)
							throw CommandException { QObject::tr ("Cannot find custom status %1.")
										.arg ("<em>" + stateNameStr + "</em>") };

						const EntryStatus status { customStatus->State_, customStatus->Text_ };
						return [status] (const EntryStatus&) { return status; };
					}
				};
				return boost::apply_visitor (StateVisitor { Proxy_ }, state);
			}

			StatusMangler_f operator() (const ClearStatus&) const
			{
				return [] (const EntryStatus& status) { return EntryStatus { status.State_, {} }; };
			}

			StatusMangler_f operator() (const std::string& msg) const
			{
				const auto& msgStr = QString::fromUtf8 (msg.c_str ());
				return [msgStr] (const EntryStatus& status) { return EntryStatus { status.State_, msgStr }; };
			}
		};
	}

	bool SetPresence (IProxyObject *proxy, ICLEntry *entry, const QString& text)
	{
		const auto& params = ParsePresenceCommand (text, proxy->GetCustomStatusNames ());
		const auto& accs = boost::apply_visitor (AccountsVisitor { proxy, entry }, params.AccName_);
		const auto& status = boost::apply_visitor (StatusManglerVisitor { proxy }, params.Status_);
		for (const auto acc : accs)
			acc->ChangeState (status (acc->GetState ()));

		return true;
	}

	bool SetDirectedPresence (IProxyObject *proxy, ICLEntry *entry, const QString& text)
	{
		const auto ihds = qobject_cast<IHaveDirectedStatus*> (entry->GetQObject ());
		if (!ihds)
			throw CommandException { QObject::tr ("%1 doesn't support directed presence.")
						.arg (entry->GetEntryName ()) };

		const auto& variant = text.section ('\n', 0, 0).section (' ', 1).trimmed ();

		const auto acc = qobject_cast<IAccount*> (entry->GetParentAccount ());
		const auto& newStatus = GetStatusMangler (proxy, text) (acc->GetState ());

		if (!variant.isEmpty ())
			ihds->SendDirectedStatus (newStatus, variant);
		else
			for (const auto& var : entry->Variants ())
				ihds->SendDirectedStatus (newStatus, var);

		return true;
	}
}
}
}
