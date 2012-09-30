/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2012  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "formats.h"
#include <QtDebug>
#include "transcodingparams.h"

namespace LeechCraft
{
namespace LMP
{
	void Format::StandardQualityAppend (QStringList& result, const TranscodingParams& params) const
	{
		const auto& num = GetBitrateLabels (params.BitrateType_).value (params.Quality_);
		switch (params.BitrateType_)
		{
		case Format::BitrateType::CBR:
			result << "-ab"
					<< (QString::number (num) + "k");
			break;
		case Format::BitrateType::VBR:
			result << "-aq"
					<< QString::number (num);
			break;
		}
	}

	class OggFormat : public Format
	{
	public:
		QString GetFormatID () const
		{
			return "ogg";
		}

		QString GetFormatName () const
		{
			return "OGG Vorbis";
		}

		QList<BitrateType> GetSupportedBitrates() const
		{
			return { BitrateType::VBR, BitrateType::CBR };
		}

		QList<int> GetBitrateLabels (BitrateType type) const
		{
			switch (type)
			{
			case BitrateType::CBR:
				return { 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 500 };
			case BitrateType::VBR:
				return { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
			}

			qWarning () << Q_FUNC_INFO
					<< "unknown bitrate type";
			return QList<int> ();
		}

		QStringList ToFFmpeg (const TranscodingParams& params) const
		{
			QStringList result;
			result << "-acodec" << "libvorbis";
			StandardQualityAppend (result, params);
			return result;
		}
	};

	class MP3Format : public Format
	{
	public:
		QString GetFormatID () const
		{
			return "mp3";
		}

		QString GetFormatName () const
		{
			return "MP3";
		}

		QList<BitrateType> GetSupportedBitrates () const
		{
			return { BitrateType::CBR, BitrateType::VBR };
		}

		QList<int> GetBitrateLabels (BitrateType type) const
		{
			switch (type)
			{
			case BitrateType::CBR:
				return { 64, 96, 128, 144, 160, 192, 224, 256, 320 };
			case BitrateType::VBR:
				return { -9, -8, -7, -6, -5, -4, -3, -2, -1 };
			}

			qWarning () << Q_FUNC_INFO
					<< "unknown bitrate type";
			return QList<int> ();
		}

		QStringList ToFFmpeg (const TranscodingParams& params) const
		{
			QStringList result;
			result << "-acodec" << "libmp3lame";
			StandardQualityAppend (result, params);
			return result;
		}
	};

	Formats::Formats ()
	{
		Formats_ << Format_ptr (new OggFormat);
		Formats_ << Format_ptr (new MP3Format);
	}

	QList<Format_ptr> Formats::GetFormats () const
	{
		return Formats_;
	}

	Format_ptr Formats::GetFormat (const QString& id) const
	{
		const auto pos = std::find_if (Formats_.begin (), Formats_.end (),
				[&id] (const Format_ptr format) { return format->GetFormatID () == id; });
		return pos == Formats_.end () ?
				Format_ptr () :
				*pos;
	}
}
}
