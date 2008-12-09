#ifndef STORAGEBACKEND_H
#define STORAGEBACKEND_H
#include <vector>
#include <QObject>
#include "historymodel.h"
#include "favoritesmodel.h"

/** @brief Abstract base class for storage backends.
 *
 * Specifies interface for all storage backends. Includes functions for
 * storing the history, favorites, saved passwords etc.
 */
class StorageBackend : public QObject
{
	Q_OBJECT
public:
	StorageBackend (QObject* = 0);
	virtual ~StorageBackend ();

	virtual void Prepare () = 0;
	virtual void LoadHistory (std::vector<HistoryModel::HistoryItem>& items) const = 0;
	virtual void AddToHistory (const HistoryModel::HistoryItem& item) = 0;
	virtual void LoadFavorites (std::vector<FavoritesModel::FavoritesItem>& items) const = 0;
	virtual void AddToFavorites (const FavoritesModel::FavoritesItem& item) = 0;
	virtual void RemoveFromFavorites (const FavoritesModel::FavoritesItem& item) = 0;
	virtual void UpdateFavorites (const FavoritesModel::FavoritesItem& item) = 0;
	virtual void GetAuth (const QString& realm,
			QString& login, QString& password) const = 0;
	virtual void SetAuth (const QString& realm,
			const QString& login, const QString& password) = 0;
signals:
	void added (const HistoryModel::HistoryItem&);
	void added (const FavoritesModel::FavoritesItem&);
	void updated (const FavoritesModel::FavoritesItem&);
	void removed (const FavoritesModel::FavoritesItem&);
};

#endif

