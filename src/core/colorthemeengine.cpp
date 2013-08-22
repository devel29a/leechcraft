/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "colorthemeengine.h"
#include <util/resourceloader.h>
#include <algorithm>
#include <map>
#include <QFile>
#include <QStringList>
#include <QApplication>
#include <QDir>
#include <QtDebug>
#include <QSettings>

namespace LeechCraft
{
	ColorThemeEngine::ColorThemeEngine ()
	: Loader_ (new Util::ResourceLoader ("themes/", this))
	{
		Loader_->AddLocalPrefix ();
		Loader_->AddGlobalPrefix ();
		Loader_->SetCacheParams (1, 0);
	}

	ColorThemeEngine& ColorThemeEngine::Instance ()
	{
		static ColorThemeEngine engine;
		return engine;
	}

	QColor ColorThemeEngine::GetQMLColor (const QString& section, const QString& key)
	{
		return QMLColors_ [section] [key];
	}

	QObject* ColorThemeEngine::GetQObject ()
	{
		return this;
	}

	namespace
	{
		QPalette::ColorRole ColorRoleFromStr (const QString& str)
		{
			static const std::map<QString, QPalette::ColorRole> map =
			{
				{ "Window", QPalette::Window },
				{ "WindowText", QPalette::WindowText },
				{ "BrightText", QPalette::BrightText },
				{ "Base", QPalette::Base },
				{ "AlternateBase", QPalette::AlternateBase },
				{ "Text", QPalette::Text },
				{ "ToolTipBase", QPalette::ToolTipBase },
				{ "ToolTipText", QPalette::ToolTipText },
				{ "Button", QPalette::Button },
				{ "ButtonText", QPalette::ButtonText }
			};
			auto pres = map.find (str);
			return pres == map.end () ? QPalette::Window : pres->second;
		}

		QColor ParseColor (const QVariant& var)
		{
			const auto& str = var.toString ();
			if (str.startsWith ("Palette."))
			{
				const auto role = ColorRoleFromStr (str.mid (QString ("Palette.").size ()));
				return QApplication::palette ().color (role);
			}

			const auto& elems = var.toStringList ();
			if (elems.size () != 3)
			{
				qWarning () << Q_FUNC_INFO
						<< "wrong color"
						<< var
						<< elems;
				return QColor ();
			}

			return QColor (elems.at (0).toInt (),
					elems.at (1).toInt (),
					elems.at (2).toInt ());
		}

		void UpdateColor (QPalette& palette, QSettings& settings,
				QPalette::ColorGroup group, QPalette::ColorRole role,
				const QString& settingsGroup, const QString& key)
		{
			settings.beginGroup ("Colors:" + settingsGroup);
			palette.setColor (group, role, ParseColor (settings.value (key)));
			settings.endGroup ();
		}

		QPalette UpdatePalette (QPalette palette, QSettings& settings)
		{
			auto updateColor = [&palette, &settings] (QPalette::ColorRole role,
					QPalette::ColorGroup group, const QString& sg, const QString& key)
			{
				UpdateColor (palette, settings, group, role, sg, key);
			};
			auto updateAll = [updateColor] (QPalette::ColorRole role,
					const QString& sg, const QString& key)
			{
				updateColor (role, QPalette::Active, sg, key);
				updateColor (role, QPalette::Inactive, sg, key);
				updateColor (role, QPalette::Disabled, sg, key);
			};

			updateAll (QPalette::Window, "Window", "BackgroundNormal");
			updateAll (QPalette::WindowText, "Window", "ForegroundNormal");
			updateAll (QPalette::BrightText, "Window", "ForegroundPositive");
			updateAll (QPalette::Link, "Window", "ForegroundLink");
			updateAll (QPalette::LinkVisited, "Window", "ForegroundVisited");

			updateAll (QPalette::Base, "View", "BackgroundNormal");
			updateAll (QPalette::AlternateBase, "View", "BackgroundAlternate");
			updateAll (QPalette::Text, "View", "ForegroundNormal");

			updateAll (QPalette::Button, "Button", "BackgroundNormal");
			updateAll (QPalette::ButtonText, "Button", "ForegroundNormal");

			updateAll (QPalette::ToolTipBase, "Tooltip", "BackgroundNormal");
			updateAll (QPalette::ToolTipText, "Tooltip", "ForegroundNormal");

			return palette;
		}
	}

	QAbstractItemModel* ColorThemeEngine::GetThemesModel () const
	{
		return Loader_->GetSubElemModel ();
	}

	void ColorThemeEngine::SetTheme (const QString& themeName)
	{
		const auto& themePath = Loader_->GetPath (QStringList (themeName));

		if (themePath.isEmpty ())
		{
			qWarning () << Q_FUNC_INFO
					<< "no theme"
					<< themeName;
			return;
		}

		if (QFile::exists (themePath + "/colors.rc"))
		{
			QSettings settings (themePath + "/colors.rc", QSettings::IniFormat);
			if (settings.childGroups ().isEmpty ())
			{
				qWarning () << Q_FUNC_INFO
						<< "error opening colors file for"
						<< themeName;
				return;
			}

			auto palette = UpdatePalette (StartupPalette_, settings);
			QApplication::setPalette (palette);
		}
		else
			QApplication::setPalette (StartupPalette_);

		QSettings qmlSettings (themePath + "/qml.rc", QSettings::IniFormat);
		FillQML (qmlSettings);

		emit themeChanged ();
	}

	void ColorThemeEngine::FillQML (QSettings& settings)
	{
		QMLColors_.clear ();

		for (const auto& group : settings.childGroups ())
		{
			settings.beginGroup (group);
			auto& hash = QMLColors_ [group];
			for (const auto& key : settings.childKeys ())
				hash [key] = ParseColor (settings.value (key));
			settings.endGroup ();
		}

		auto fixup = [this, &settings] (const QString& section,
				const QString& name, const QString& fallback) -> void
		{
			auto& sec = QMLColors_ [section];
			if (sec.contains (name))
				return;

			qWarning () << Q_FUNC_INFO
					<< settings.fileName ()
					<< "lacks"
					<< (section + "_" + name)
					<< "; falling back to"
					<< fallback;
			sec [name] = sec [fallback];
		};

		fixup ("ToolButton", "HoveredTopColor", "SelectedTopColor");
		fixup ("ToolButton", "HoveredBottomColor", "SelectedBottomColor");
	}
}
