#pragma once
#include "CustomFieldService.h"


class CustomFieldsTmxLoader
{
public:
	explicit CustomFieldsTmxLoader(const QString& fileName, DBBrowserDB& db, CustomFieldService& cfs);

	void SaveToDb();

	const CustomField* TryGetCustomField(const QString& name) const;

private:
	void LoadFromFile();
	void SaveCustomFieldToDb(const CustomField& customField);
private:
	DBBrowserDB& _db;
	CustomFieldService& _customFieldService;

	std::vector<CustomField> _initialFields;
	std::vector<CustomField> _importedFields;

	QString _importFileName;

	// this contains what we saved to DB
	std::map<QString, CustomField> _dbFields;
};

