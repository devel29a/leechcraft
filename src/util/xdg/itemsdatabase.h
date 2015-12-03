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

#include "itemsfinder.h"

class QFileSystemWatcher;

namespace LeechCraft
{
namespace Util
{
namespace XDG
{
	/** @brief An ItemsFinder automatically watching for changes in
	 * <code>.desktop</code> files.
	 *
	 * The only difference between this class and ItemsFinder is in this
	 * class automatically watching for changes in the directories
	 * matching the types passed to its constructor. The changes include
	 * both updates to the existing files as well as addition of new files
	 * and removal of already existing ones.
	 *
	 * Refer to the documentation for ItemsFinder for more information.
	 *
	 * @sa ItemsFinder
	 */
	class UTIL_XDG_API ItemsDatabase : public ItemsFinder
	{
		Q_OBJECT

		bool UpdateScheduled_ = false;
		QFileSystemWatcher * const Watcher_;
	public:
		/** @brief Creates the ItemsDatabase for the given \em types.
		 *
		 * @param[in] proxy The proxy to use to get the icons of the
		 * items that were found.
		 * @param[in] types The item types to watch for.
		 * @param[in] parent The parent object of this finder.
		 *
		 * @sa ItemsFinder::ItemsFinder
		 */
		ItemsDatabase (ICoreProxy_ptr proxy, const QList<Type>& types, QObject *parent = nullptr);
	private slots:
		void scheduleUpdate ();
	};
}
}
}
