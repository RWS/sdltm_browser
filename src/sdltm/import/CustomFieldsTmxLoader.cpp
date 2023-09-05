#include "CustomFieldsTmxLoader.h"

#include <QIODevice>
#include <QTextStream>

#include "FileDialog.h"
#include "SdltmSqlUtil.h"
#include "SdltmUtil.h"
#include "SimpleXml.h"

CustomFieldsTmxLoader::CustomFieldsTmxLoader(const QString& fileName, DBBrowserDB& db, CustomFieldService& cfs)
	: _db(db)
	, _customFieldService(cfs)
	, _importFileName(fileName)
	, _initialFields(cfs.GetFields()) {
	LoadFromFile();
}

// when we're doing the actual save, we see which are duplicates with different types,
// and for those, we're building a 1-to-1 correspondence, so that when importing something from TMX,
// we'll know what to correspond to
void CustomFieldsTmxLoader::SaveToDb() {
	// after we insert everything -> reload all custom fields, and then we'll figure out the IDs of each field

	int nextCustomFieldId = _customFieldService.NextCustomFieldID();
	int nextCustomPicklistId = _customFieldService.NextCustomFieldListvalueID();

	for (const auto & customField : _importedFields) {
		// here, I already have a field in the db with the same name
		// if equivalent, reuse it. Otherwise, create another one
		auto copy = customField;
		for (int idxSuffix = 0; ; ++idxSuffix) {
			copy.FieldName = idxSuffix == 0 ? customField.FieldName : customField.FieldName + "-" + QString::number(idxSuffix + 1);
			auto byName = std::find_if(_initialFields.begin(), _initialFields.end(), [=](const CustomField& cf) { return cf.FieldName == copy.FieldName; });
			if ( byName == _initialFields.end()) {
				// in this case -> this field name is not yet present in the db
				copy.ID = ++nextCustomFieldId;
				for (const auto& value : copy.Values)
					copy.ValueToID.push_back(++nextCustomPicklistId);
				SaveCustomFieldToDb(copy);
				// I will later see that the custom field's ID is -1, and properly reload it from DB
				_dbFields[customField.FieldName] = copy;
				break;
			}
			else if (copy.IsEquivalent(*byName)) {
				// in this case, I already have an equivalent custom field in the db
				_dbFields[customField.FieldName] = *byName;
				break;
			}
		}
	}

	_customFieldService.Update();
}

const CustomField* CustomFieldsTmxLoader::TryGetCustomField(const QString& name) const {
	if (_dbFields.find(name) != _dbFields.end())
		return &(_dbFields.find(name)->second);
	else
		return nullptr;
}

namespace  {

	CustomField LineToCustomField(const QString & line) {
		CustomField cf;
		auto xml = SimpleXmlNode::Parse(line);
		auto type = xml.Attributes["type"];
		type = type.right(type.size() - 2);// ignore "x-"
		auto idxColon = type.indexOf(':');
		auto metaType = type.right(type.size() - idxColon - 1);
		type = type.left(idxColon);

		cf.ID = -1;
		cf.FieldName = type;
		if (metaType == "Integer") {
			cf.FieldType = SdltmFieldMetaType::Number;
		} else if (metaType == "SingleString") {
			cf.FieldType = SdltmFieldMetaType::Text;
		}
		else if (metaType == "MultipleString") {
			cf.FieldType = SdltmFieldMetaType::MultiText;
		}
		else if (metaType == "DateTime") {
			cf.FieldType = SdltmFieldMetaType::DateTime;
		}
		else if (metaType == "SinglePicklist") {
			cf.FieldType = SdltmFieldMetaType::List;
			auto values = xml.Value.split(',');
			for (const auto & value : values) {
				cf.Values.push_back(value);
			}
		}
		else if (metaType == "MultiplePicklist") {
			cf.FieldType = SdltmFieldMetaType::CheckboxList;
			auto values = xml.Value.split(',');
			for (const auto& value : values) {
				cf.Values.push_back(value);
			}
		}
		else
			assert(false);

		return cf;
	}
}

void CustomFieldsTmxLoader::LoadFromFile() {
	// Parse all csv data
	QFile file(_importFileName);
	file.open(QIODevice::ReadOnly);

	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	QString typeSuffixes[] = { ":SingleString", ":Integer", ":MultiplePicklist" , ":SinglePicklist" , ":MultipleString" , ":DateTime" , };
	while (!stream.atEnd()) {
		auto line = stream.readLine();
		if (line.contains("</header"))
			break;

		auto isCustomField = false;
		for (const auto & suffix : typeSuffixes)
			if (line.contains(suffix)) {
				isCustomField = true;
				break;
			}
		if (isCustomField)
			_importedFields.push_back(LineToCustomField(line));
	}
}

void CustomFieldsTmxLoader::SaveCustomFieldToDb(const CustomField& customField) {
	int error;
	QString errorMsg;
	if (!TryAddCustomField(_db, customField, error, errorMsg))
		SdltmLog("FATAL: can't save custom field " + customField.FieldName);
}
