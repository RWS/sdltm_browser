#include "CustomFieldService.h"

#include <QTextCodec>

CustomFieldValue::CustomFieldValue(const CustomField& cf) {
	_field = cf;
	if (_field.FieldType == SdltmFieldMetaType::CheckboxList)
		CheckboxIndexes.resize(_field.Values.size());
}

bool CustomFieldValue::IsEmpty() const {
	switch (_field.FieldType) {
	case SdltmFieldMetaType::Int:
	case SdltmFieldMetaType::Double:
	case SdltmFieldMetaType::Number:
	case SdltmFieldMetaType::Text:
		return Text.trimmed() == "";

	case SdltmFieldMetaType::MultiText:
		return MultiText.empty();
	case SdltmFieldMetaType::List:
		return ComboIndex == -1;
	case SdltmFieldMetaType::CheckboxList:
		return CheckboxIndexes.empty();
	case SdltmFieldMetaType::DateTime:
		return Time == QDateTime(QDate(1990, 1, 1));
	default: assert(false); break;
	}
	return true;
}

QString CustomFieldValue::FriendlyValue() const {
	switch(_field.FieldType) {
	case SdltmFieldMetaType::Int:
	case SdltmFieldMetaType::Double:
	case SdltmFieldMetaType::Number:
	case SdltmFieldMetaType::Text:
		return Text;
	case SdltmFieldMetaType::MultiText: {
		QString multi;
		for (const auto & line : MultiText) {
			if (multi != "")
				multi += ", ";
			multi += line;
		}
		return multi;
	}

	case SdltmFieldMetaType::List: {
		auto id = ComboId();
		if (ComboIndex < 0)
			return  "";
		return _field.Values[ComboIndex];
	}

	case SdltmFieldMetaType::CheckboxList: {
		QString checkList;
		for (int i = 0; i < CheckboxIndexes.size(); ++i) {
			if (!CheckboxIndexes[i])
				continue;

			if (checkList != "")
				checkList += ", ";
			checkList += _field.Values[i];
		}
		return checkList;
	}
	case SdltmFieldMetaType::DateTime:
		return Time.date().year() > 1990 ? Time.toString() : "";

	default: assert(false); break;
	}

	return "";
}

int CustomFieldValue::ComboId() const {
	return IndexToId(ComboIndex);
}

std::vector<int> CustomFieldValue::CheckboxIds() const {
	std::vector<int> ids;
	for (int i = 0; i < CheckboxIndexes.size(); ++i) {
		if (!CheckboxIndexes[i])
			continue;

		auto id = IndexToId(i);
		if (id >= 0)
			ids.push_back(id);
	}
	return ids;
}

int CustomFieldValue::IndexToId(int idx) const {
	if (idx >= 0 && idx < _field.Values.size())
		return _field.ValueToID[idx];
	return -1;
}



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
