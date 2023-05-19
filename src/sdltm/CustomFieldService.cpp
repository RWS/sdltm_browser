#include "CustomFieldService.h"

#include <QTextCodec>

CustomFieldService::CustomFieldService(DBBrowserDB& db)
	: _db(&db)
	, _fvService(db)
{
}

// the idea: when opening a new database, update everything
void CustomFieldService::Update()
{
	_fields.clear();

	auto attributesCallback = [this](int, const std::vector<QByteArray> &values, const std::vector<QByteArray> & )
	{
		// false => don't abort
		auto id = values[0].toInt();
		auto name = _fvService.GetString(values[1]) ;
		auto type = values[2].toInt();

		bool fieldOk = true;
		if (name == "StructureContext")
			fieldOk = false;

		CustomField cf;
		cf.ID = id;
		cf.FieldName = name;
		switch (type)
		{
		case 1: cf.FieldType = SdltmFieldMetaType::Text; break;
		case 2: cf.FieldType = SdltmFieldMetaType::MultiText; break;
		case 3: cf.FieldType = SdltmFieldMetaType::DateTime; break;
		case 4: cf.FieldType = SdltmFieldMetaType::List; break;
		case 5: cf.FieldType = SdltmFieldMetaType::CheckboxList; break;
		case 6: cf.FieldType = SdltmFieldMetaType::Number; break;
		default:
			fieldOk = false; break;
		}

		if (fieldOk)
			_fields.push_back(cf);

		return false;
	};

	// Attributes table
	_db->executeSQL("select id,name,type from attributes order by id", false, false, attributesCallback);

	auto picklistCallback = [this](int, const std::vector<QByteArray>& values, const std::vector<QByteArray>&)
	{
		auto id = values[0].toInt();
		auto name = _fvService.GetString(values[2]);
		auto attributeId = values[1].toInt();

		auto field = std::find_if(_fields.begin(), _fields.end(), [this, attributeId](const CustomField& cf) { return cf.ID == attributeId; });
		if (field != _fields.end())
		{
			field->Values.push_back(name);
			field->ValueToID.push_back(id);
		}
		return false;
	};

	// picklist values
	_db->executeSQL("select id,attribute_id,value from picklist_values order by id", false, false, picklistCallback);
}

const std::vector<CustomField>& CustomFieldService::GetFields() const
{
	return _fields;
}
