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

#include <boost/optional.hpp>
#include <QtPlugin>
#include <qwebpluginfactory.h>

namespace LeechCraft
{
namespace Poshuku
{
	/** Base class for plugins that want to be available from the
	 * Poshuku's Web Plugin Factory which is a subclass of
	 * QWebPluginFactory.
	 */
	class IWebPlugin
	{
	public:
		virtual ~IWebPlugin () {}

		/** Queries the plugin for the information.
		 *
		 * inPlugins is true if the plugin is being queried inside the
		 * QWebPluginFactory::plugins(), false otherwise. Plugins can
		 * relate on this parameter for them to not be exposed to the
		 * DOM.
		 *
		 * @param[in] inPlugins If queried inside the
		 * QWebPluginFactory::plugins().
		 */
		virtual boost::optional<QWebPluginFactory::Plugin> Plugin (bool inPlugins) const = 0;

		/** Askes the plugin to create its instance for the given mime,
		 * url, args and their params.
		 *
		 * See QWebPluginFactory::create() for more info.
		 */
		virtual QWidget* Create (const QString& mime,
				const QUrl& url,
				const QStringList& args,
				const QStringList& params) = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Poshuku::IWebPlugin,
		"org.LeechCraft.Poshuku.IWebPlugin/1.0")
