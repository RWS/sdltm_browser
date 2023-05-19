#pragma once
#include "DbFieldValueService.h"
#include "SdltmFilter.h"
#include "sqlitedb.h"

struct CustomField
{
	bool IsPresent() const { return ID > 0; }
	int ID = 0;
	QString FieldName;
	SdltmFieldMetaType FieldType = SdltmFieldMetaType::Text;
	// only when it's a list of strings 
	std::vector<QString> Values;
	// each picklist value has an ID
	std::vector<int> ValueToID;
};


class CustomFieldService
{
public:
	explicit CustomFieldService(DBBrowserDB& db);

	void Update();
	const std::vector<CustomField>& GetFields() const;

private:
	DBBrowserDB* _db;
	std::vector<CustomField> _fields;
	DbFieldValueService _fvService;
};

