#pragma once
#include <QString>
#include <sqlite3.h>

#include "CustomFieldService.h"
#include "CustomFieldValueService.h"


class QFile;

class ExportSqlToTmx
{
public:
	explicit ExportSqlToTmx(const QString & fileName, DBBrowserDB& db, CustomFieldService& cfs);

	void Export(const QString & sql);
private:
	void WriteHeader(QFile & file);
	void WriteBody(QFile& file, const QString& sql);


	void WriteTuHeader(QFile& file, sqlite3_stmt* db);
	void WriteSegment(QFile& file, const QString& segment);
	void WriteTuEnd(QFile& file);


private:
	DBBrowserDB* _db;
	std::vector<CustomField> _fields;
	CustomFieldValueService _fieldValues;

	QString _exportFileName;

	int _error = 0;
	QString _errorMsg;
};

