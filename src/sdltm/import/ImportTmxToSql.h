#pragma once
#include <QString>

#include "BackgroundProgressDialog.h"
#include "CustomFieldService.h"
#include "sqlitedb.h"
#include "export/CustomFieldValueService.h"


class ImportTmxToSql
{
public:
	explicit ImportTmxToSql(const QString& fileName, DBBrowserDB& db, CustomFieldService& cfs);
	void Import(BackgroundProgressDialog::CallbackFunc callback);
private:
	DBBrowserDB& _db;
	std::vector<CustomField> _fields;
	CustomFieldService& _customFieldService;

	QString _importFileName;

	bool _headerParsed = false;
	bool _headerEndReached = false;
	QString _sourceLanguage;
	QDateTime _creationDate;
	QString _creationUser;

	int _importIndex = 0;
};

