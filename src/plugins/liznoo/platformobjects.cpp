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

#include "platformobjects.h"
#include <vector>
#include <memory>
#include <boost/optional.hpp>
#include <QtDebug>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <util/threads/futures.h>
#include <util/sll/delayedexecutor.h>
#include <util/sll/either.h>
#include "availabilitychecker.h"
#include "platform/screen/screenplatform.h"
#include "platform/battery/batteryplatform.h"

#if defined(Q_OS_LINUX)
	#include "platform/battery/upowerplatform.h"
	#include "platform/events/platformupowerlike.h"

	#ifdef USE_PMUTILS
		#include "platform/poweractions/pmutils.h"
	#else
		#include "platform/poweractions/upower.h"
	#endif

	#include "platform/screen/freedesktop.h"
	#include "platform/common/dbusthread.h"
	#include "platform/upower/upowerconnector.h"
	#include "platform/logind/logindconnector.h"
#elif defined(Q_OS_WIN32)
	#include "platform/battery/winapiplatform.h"
	#include "platform/events/platformwinapi.h"
	#include "platform/winapi/fakeqwidgetwinapi.h"
#elif defined(Q_OS_FREEBSD)
	#include "platform/battery/freebsdplatform.h"
	#include "platform/events/platformfreebsd.h"
	#include "platform/poweractions/freebsd.h"
	#include "platform/screen/freedesktop.h"
#elif defined(Q_OS_MAC)
	#include "platform/battery/macplatform.h"
	#include "platform/events/platformmac.h"
#else
	#pragma message ("Unsupported system")
#endif

namespace LeechCraft
{
namespace Liznoo
{
	namespace
	{
		class LogindEventsChecker final : public IChecker<Events::PlatformLayer>
		{
			const ICoreProxy_ptr Proxy_;

			std::shared_ptr<DBusThread<Logind::LogindConnector>> Thread_;
		public:
			LogindEventsChecker (const ICoreProxy_ptr& proxy)
			: Proxy_ { proxy }
			{
			}

			QFuture<bool> Check () override
			{
				qDebug () << Q_FUNC_INFO;
				Thread_ = std::make_shared<DBusThread<Logind::LogindConnector>> ();
				Thread_->start (QThread::LowestPriority);
				return Thread_->ScheduleImpl (&Logind::LogindConnector::ArePowerEventsAvailable);
			}

			std::shared_ptr<Events::PlatformLayer> Make () override
			{
				return Events::MakeUPowerLike (Thread_, Proxy_);
			}
		};
	}

	PlatformObjects::PlatformObjects (const ICoreProxy_ptr& proxy, QObject *parent)
	: QObject { parent }
	, Proxy_ { proxy }
	{
#if defined(Q_OS_LINUX)
		const auto upowerThread = std::make_shared<DBusThread<UPower::UPowerConnector>> ();

		const auto eventsChecker = new AvailabilityChecker<Events::PlatformLayer>
		{
			MakePureChecker<Events::PlatformLayer> ([upowerThread]
					{
						return upowerThread->
								ScheduleImpl (&UPower::UPowerConnector::ArePowerEventsAvailable);
					},
					[upowerThread, this] { return Events::MakeUPowerLike (upowerThread, Proxy_); }),
			std::make_unique<LogindEventsChecker> (proxy)
		};
		Util::Sequence (this, eventsChecker->GetResult ()) >>
				[this] (const auto& maybeEvents)
				{
					if (maybeEvents)
						EventsPlatform_ = *maybeEvents;
					else
						qWarning () << Q_FUNC_INFO
								<< "no events platform";
				};

		ScreenPlatform_ = new Screen::Freedesktop (this);
		BatteryPlatform_ = std::make_shared<Battery::UPowerPlatform> (upowerThread);

	#ifdef USE_PMUTILS
		PowerActPlatform_ = std::make_shared<PowerActions::PMUtils> ();
	#else
		PowerActPlatform_ = std::make_shared<PowerActions::UPower> ();
	#endif

		upowerThread->start (QThread::LowestPriority);
#elif defined(Q_OS_WIN32)
		const auto widget = std::make_shared<WinAPI::FakeQWidgetWinAPI> ();

		EventsPlatform_ = std::make_shared<Events::PlatformWinAPI> (widget, Proxy_);
		BatteryPlatform_ = std::make_shared<Battery::WinAPIPlatform> (widget);
#elif defined(Q_OS_FREEBSD)
		EventsPlatform_ = std::make_shared<Events::PlatformFreeBSD> (Proxy_);
		PowerActPlatform_ = std::make_shared<PowerActions::FreeBSD> ();
		BatteryPlatform_ = std::make_shared<Battery::FreeBSDPlatform> ();
		ScreenPlatform_ = new Screen::Freedesktop (this);
#elif defined(Q_OS_MAC)
		BatteryPlatform_ = std::make_shared<Battery::MacPlatform> ();
		EventsPlatform_ = std::make_shared<Events::PlatformMac> (Proxy_);
#endif

		if (BatteryPlatform_)
			connect (BatteryPlatform_.get (),
					SIGNAL (batteryInfoUpdated (Liznoo::BatteryInfo)),
					this,
					SIGNAL (batteryInfoUpdated (Liznoo::BatteryInfo)));
		else
			qWarning () << Q_FUNC_INFO
					<< "battery backend is not available";

	}

	QFuture<PlatformObjects::ChangeStateResult_t> PlatformObjects::ChangeState (PowerActions::Platform::State state)
	{
		if (!PowerActPlatform_)
			return Util::MakeReadyFuture (ChangeStateResult_t::Left ({
						ChangeStateFailed::Reason::Unavailable,
						{}
					}));

		return Util::Sequence (this, PowerActPlatform_->CanChangeState (state)) >>
				[state, this] (const PowerActions::Platform::QueryChangeStateResult& res)
				{
					if (res.CanChangeState_)
					{
						PowerActPlatform_->ChangeState (state);
						return Util::MakeReadyFuture (ChangeStateResult_t::Right ({}));
					}
					else
						return Util::MakeReadyFuture (ChangeStateResult_t::Left ({
										ChangeStateFailed::Reason::PlatformFailure,
										res.Reason_
								}));
				};
	}

	void PlatformObjects::ProhibitScreensaver (bool enable, const QString& id)
	{
		if (!ScreenPlatform_)
		{
			qWarning () << Q_FUNC_INFO
					<< "screen platform layer unavailable, screensaver prohibiton won't work";
			return;
		}

		ScreenPlatform_->ProhibitScreensaver (enable, id);
	}

	bool PlatformObjects::EmitTestSleep ()
	{
		if (!EventsPlatform_)
		{
			qWarning () << Q_FUNC_INFO
					<< "platform backend unavailable";
			return false;
		}

		EventsPlatform_->emitGonnaSleep (1000);
		return true;
	}

	bool PlatformObjects::EmitTestWakeup ()
	{
		if (!EventsPlatform_)
		{
			qWarning () << Q_FUNC_INFO
					<< "platform backend unavailable";
			return false;
		}

		EventsPlatform_->emitWokeUp ();
		return true;
	}
}
}
