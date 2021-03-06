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

#include <functional>
#include <QObject>
#include <QSettings>
#include "todoitem.h"

namespace LeechCraft
{
namespace Otlozhu
{
	class TodoStorage : public QObject
	{
		Q_OBJECT

		const QString Context_;
		QList<TodoItem_ptr> Items_;

		QSettings Storage_;
	public:
		TodoStorage (const QString&, QObject* = 0);

		int GetNumItems () const;
		int FindItem (const QString&) const;

		void AddItem (TodoItem_ptr);
		TodoItem_ptr GetItemAt (int idx) const;
		TodoItem_ptr GetItemByID (const QString&) const;
		QList<TodoItem_ptr> GetAllItems () const;

		void AddDependency (const QString& itemId, const QString& depId);

		void HandleUpdated (TodoItem_ptr);
		void RemoveItem (const QString&);
	private:
		void HandleUpdated (TodoItem_ptr, const std::function<void ()>&);
		void Load ();
		void SaveAt (int);
		void SaveAt (const QList<int>&);
	signals:
		void itemAdded (int);
		void itemRemoved (int);
		void itemUpdated (int);
		void itemDiffGenerated (const QString&, const QVariantMap&);

		void itemDepAdded (int itemIdx, int depIdx);
		void itemDepRemoved (int itemIdx, int depIdx);
	};
}
}
