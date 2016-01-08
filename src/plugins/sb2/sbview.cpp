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

#include "sbview.h"
#include <QLayout>

#if QT_VERSION >= 0x050000
#include <QQmlEngine>
#include <QQuickItem>
#endif

#include <util/sll/delayedexecutor.h>

namespace LeechCraft
{
namespace SB2
{
	SBView::SBView (QWidget *parent)
#if QT_VERSION < 0x050000
	: QDeclarativeView (parent)
#else
	: QQuickWidget (parent)
#endif
	{
		setResizeMode (SizeRootObjectToView);
		setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Expanding);
	}

	void SBView::SetDimensions (int dim)
	{
		Dim_ = dim;
		if (!parentWidget ())
			return;

		if (auto lay = parentWidget ()->layout ())
			lay->update ();
	}

	QSize SBView::minimumSizeHint () const
	{
		return { Dim_, Dim_ };
	}

	QSize SBView::sizeHint () const
	{
		return { Dim_, Dim_ };
	}

#if QT_VERSION >= 0x050000
	void SBView::enterEvent (QEvent *lev)
	{
		for (const auto item : UnhoverItems_)
		{
			if (!item.Item_)
				continue;

			QHoverEvent ev { QEvent::HoverEnter, item.OldPos_, { -1, -1 } };
			static_cast<QObject*> (item.Item_)->event (&ev);
		}

		UnhoverItems_.clear ();

		QQuickWidget::enterEvent (lev);
	}

	namespace
	{
		QList<QQuickItem*> GetLogicalChildrenRepeater (QQuickItem *item)
		{
			QList<QQuickItem*> result;

			const auto count = item->property ("count").toInt ();
			for (int i = 0; i < count; ++i)
			{
				QQuickItem *childItem = nullptr;
				QMetaObject::invokeMethod (item,
						"itemAt",
						Q_RETURN_ARG (QQuickItem*, childItem),
						Q_ARG (int, i));
				if (childItem)
					result << childItem << childItem->findChildren<QQuickItem*> ();
			}

			return result;
		}

		QList<QQuickItem*> GetLogicalChildrenListView (QQuickItem *item)
		{
			QList<QQuickItem*> result;

			for (const auto child : item->children ())
			{
				QQmlListReference ref { child, "children" };
				if (ref.count () <= 1)
					continue;

				for (int i = 0; i < ref.count (); ++i)
				{
					if (const auto item = qobject_cast<QQuickItem*> (ref.at (i)))
						result << item << item->findChildren<QQuickItem*> ();
				}
			}

			return result;
		}

		QList<QQuickItem*> GetLogicalChildren (QQuickItem *item, const QByteArray& className)
		{
			if (className == "QQuickRepeater")
				return GetLogicalChildrenRepeater (item);
			if (className.startsWith ("QQuickListView"))
				return GetLogicalChildrenListView (item);

			return {};
		}
	}

	void SBView::leaveEvent (QEvent *lev)
	{
		UnhoverItems_.clear ();

		auto items = rootObject ()->findChildren<QQuickItem*> ();

		while (!items.isEmpty ())
		{
			auto item = items.takeFirst ();

			const QByteArray className { item->metaObject ()->className () };
			items += GetLogicalChildren (item, className);

			if (!className.startsWith ("QQuickMouseArea"))
				continue;

			if (!item->property ("containsMouse").toBool ())
				continue;

			const QPointF oldPos
			{
				item->property ("mouseX").toDouble (),
				item->property ("mouseY").toDouble ()
			};
			UnhoverItems_.append ({ item, oldPos });

			QHoverEvent ev { QEvent::HoverLeave, { -1, -1 }, oldPos };
			static_cast<QObject*> (item)->event (&ev);
		}

		QQuickWidget::leaveEvent (lev);
	}
#endif
}
}
