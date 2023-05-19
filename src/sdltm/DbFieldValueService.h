#pragma once
#include "sqlitedb.h"

// allows converting byte-array values from the DB into their type-based values
class DbFieldValueService
{
public:
	explicit DbFieldValueService(DBBrowserDB& db);

	QString GetString(const QByteArray& ba) const;
private:
	DBBrowserDB* _db;
	// if empty, assume no encoding
	QString _encoding;
};

