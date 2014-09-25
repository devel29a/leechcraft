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

#include "findnotificationwk.h"
#include <QWebView>

namespace LeechCraft
{
namespace Util
{
	FindNotificationWk::FindNotificationWk (ICoreProxy_ptr proxy, QWebView* near)
	: FindNotification { proxy, near }
	, WebView_ { near }
	{
	}

	QWebPage::FindFlags FindNotificationWk::ToPageFlags (FindFlags flags)
	{
		QWebPage::FindFlags pageFlags;
		auto check = [&pageFlags, flags] (Util::FindNotification::FindFlag ourFlag, QWebPage::FindFlag pageFlag)
		{
			if (flags & ourFlag)
				pageFlags |= pageFlag;
		};
		check (Util::FindNotification::FindCaseSensitively, QWebPage::FindCaseSensitively);
		check (Util::FindNotification::FindBackwards, QWebPage::FindBackward);
		check (Util::FindNotification::FindWrapsAround, QWebPage::FindWrapsAroundDocument);
		return pageFlags;
	}


	void FindNotificationWk::handleNext (const QString& text, FindNotification::FindFlags findFlags)
	{
		const auto flags = ToPageFlags (findFlags);

		if (PreviousFindText_ != text)
		{
			auto nflags = flags | QWebPage::HighlightAllOccurrences;
			WebView_->page ()->findText (QString (), nflags);
			WebView_->page ()->findText (text, nflags);
			PreviousFindText_ = text;
		}

		const auto found = WebView_->page ()->findText (text, flags);
		SetSuccessful (found);
	}

	void FindNotificationWk::reject ()
	{
		FindNotification::reject ();
		WebView_->page ()->findText ("", QWebPage::HighlightAllOccurrences);
	}
}
}
