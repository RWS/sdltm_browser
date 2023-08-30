#include "CustomFieldValueService.h"

#include <QVariant>
#include <sqlite3.h>

namespace  {
	struct SqlCustomFieldValue {
		int AttributeId;
		int PicklistValueId;
		QVariant Value;
	};
	// how to know which type it is
	std::vector<SqlCustomFieldValue> RunQueryGetValues(const QString& sql, sqlite3* pDb, SdltmFieldMetaType fieldType) {

		std::vector<SqlCustomFieldValue> result;

		sqlite3_stmt* stmt;
		int status = sqlite3_prepare_v2(pDb, sql.toStdString().c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
		if (status == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				SqlCustomFieldValue fv;
				auto id = sqlite3_column_int(stmt, 0);

				switch (fieldType) {
				case SdltmFieldMetaType::Int:
				case SdltmFieldMetaType::Double:
					assert(false);
					break;
				case SdltmFieldMetaType::Number: {
					auto n = sqlite3_column_int64(stmt, 1);
					fv.AttributeId = id;
					fv.Value = (int)n;
				}
											   break;
				case SdltmFieldMetaType::Text:
				case SdltmFieldMetaType::MultiText: {
					QByteArray blob(reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 1)), sqlite3_column_bytes(stmt, 1));
					QString content = QString::fromUtf8(blob);
					fv.AttributeId = id;
					fv.Value = content;
				}
												  break;
				case SdltmFieldMetaType::List:
				case SdltmFieldMetaType::CheckboxList:
					fv.PicklistValueId = id;
					break;
				case SdltmFieldMetaType::DateTime: {
					QByteArray blob(reinterpret_cast<const char*>(sqlite3_column_blob(stmt, 1)), sqlite3_column_bytes(stmt, 1));
					QString content = QString::fromUtf8(blob);
					auto time = QDateTime::fromString(content, "yyyy-MM-dd hh:mm:ss");
					fv.AttributeId = id;
					fv.Value = time;
				}
												 break;
				default: assert(false); break;
				}
				result.push_back(fv);
			}
			sqlite3_finalize(stmt);
		}

		return result;
	}

	SqlCustomFieldValueService::CustomFieldValues GetCustomFieldValues(DBBrowserDB& db, const std::vector<CustomField>& customFields, int translationUnitId, int& error, QString& errorMsg) {
		auto forceWait = true;
		auto pDb = db.get("get custom field values", forceWait);

		// load each of them
		QString sql;
		auto idStr = QString::number(translationUnitId);
		sql = "select attribute_id, value from numeric_attributes where translation_unit_id = " + idStr + " order by attribute_id";
		auto numbers = RunQueryGetValues(sql, pDb.get(), SdltmFieldMetaType::Number);
		sql = "select attribute_id, value from string_attributes where translation_unit_id = " + idStr + " order by attribute_id";
		auto strings = RunQueryGetValues(sql, pDb.get(), SdltmFieldMetaType::Text);

		sql = "select attribute_id, datetime(value) from date_attributes where translation_unit_id = " + idStr + " order by attribute_id";
		auto times = RunQueryGetValues(sql, pDb.get(), SdltmFieldMetaType::DateTime);
		sql = "select picklist_value_id from picklist_attributes where translation_unit_id = " + idStr + " order by picklist_value_id";
		auto picks = RunQueryGetValues(sql, pDb.get(), SdltmFieldMetaType::List);

		// now, figure out which is which
		// special cases Text & Multitext, List & Checklist
		SqlCustomFieldValueService::CustomFieldValues fieldValues;
		for (const auto& number : numbers) {
			auto customField = std::find_if(customFields.begin(), customFields.end(), [number](const CustomField& cf) { return cf.ID == number.AttributeId; });
			if (customField != customFields.end()) {
				fieldValues.Numbers. push_back( { number.AttributeId, number.Value.toInt() });
			}
		}

		for (const auto& time : times) {
			auto customField = std::find_if(customFields.begin(), customFields.end(), [time](const CustomField& cf) { return cf.ID == time.AttributeId; });
			if (customField != customFields.end()) {
				fieldValues.Dates.push_back( { time.AttributeId, time.Value.toDateTime() });
			}
		}

		for (const auto& str : strings) {
			auto customField = std::find_if(customFields.begin(), customFields.end(), [str](const CustomField& cf) { return cf.ID == str.AttributeId; });
			if (customField != customFields.end()) {
				if (customField->FieldType == SdltmFieldMetaType::MultiText) {
					auto existing = std::find_if(fieldValues.MultiTexts.begin(), fieldValues.MultiTexts.end(), 
											     [customField](const SqlCustomFieldValueService::MultiString& fv) { return fv.CustomFieldID == customField->ID; });
					if (existing != fieldValues.MultiTexts.end())
						existing->Texts.push_back(str.Value.toString());
					else {
						SqlCustomFieldValueService::MultiString ms;
						ms.CustomFieldID = str.AttributeId;
						ms.Texts.push_back(str.Value.toString());
						fieldValues.MultiTexts.push_back(ms);
					}
				}
				else {
					fieldValues.Texts.push_back({ str.AttributeId, str.Value.toString() });
				}
			}
		}

		for (const auto& pick : picks) {
			auto customField = std::find_if(customFields.begin(), customFields.end(), [pick](const CustomField& cf)
				{
					return (cf.FieldType == SdltmFieldMetaType::List || cf.FieldType == SdltmFieldMetaType::CheckboxList)
						&& std::find(cf.ValueToID.begin(), cf.ValueToID.end(), pick.PicklistValueId) != cf.ValueToID.end();
				});

			if (customField->FieldType == SdltmFieldMetaType::CheckboxList) {
				auto index = std::find(customField->ValueToID.begin(), customField->ValueToID.end(), pick.PicklistValueId) - customField->ValueToID.begin();
				auto existing = std::find_if(fieldValues.Checklists.begin(), fieldValues.Checklists.end(), 
											 [customField](const SqlCustomFieldValueService::Checklist& fv) { return fv.CustomFieldID == customField->ID; });
				if (existing != fieldValues.Checklists.end()) {
					existing->Values.push_back(customField->Values[index]);
				}
				else {
					SqlCustomFieldValueService::Checklist cl;
					cl.CustomFieldID = customField->ID;
					cl.Values.push_back(customField->Values[index]);
					fieldValues.Checklists.push_back(cl);
				}
			}
			else {
				CustomFieldValue cf(*customField);
				auto index = std::find(customField->ValueToID.begin(), customField->ValueToID.end(), pick.PicklistValueId) - customField->ValueToID.begin();
				SqlCustomFieldValueService::Combo c;
				c.CustomFieldID = customField->ID;
				c.Value = customField->Values[index];
			}
		}

		return fieldValues;
	}

	// note: this is obviously not as efficient as possible (the most efficient way would have been to load everything at once)
	// however, in practice, we seem not to have that many custom field values, so this is fine
	//
	// but yeah, we can improve on this later
	std::vector<int> GetTUIdsWithCustomFieldValues(DBBrowserDB& db, const std::vector<CustomField>& customFields, int& error, QString& errorMsg) {
		auto forceWait = true;
		auto pDb = db.get("run sql get IDs", forceWait);

		std::set<int> idsSet;

		QString sql;
		sql = "select distinct translation_unit_id from numeric_attributes ";
		sqlite3_stmt* stmt;
		int status = sqlite3_prepare_v2(pDb.get(), sql.toStdString().c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
		if (status == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				auto id = sqlite3_column_int(stmt, 0);
				idsSet.insert(id);
			}
			sqlite3_finalize(stmt);
		}

		sql = "select distinct translation_unit_id from string_attributes ";
		status = sqlite3_prepare_v2(pDb.get(), sql.toStdString().c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
		if (status == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				auto id = sqlite3_column_int(stmt, 0);
				idsSet.insert(id);
			}
			sqlite3_finalize(stmt);
		}
		sql = "select distinct translation_unit_id from date_attributes ";
		status = sqlite3_prepare_v2(pDb.get(), sql.toStdString().c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
		if (status == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				auto id = sqlite3_column_int(stmt, 0);
				idsSet.insert(id);
			}
			sqlite3_finalize(stmt);
		}
		sql = "select distinct translation_unit_id from picklist_attributes ";
		status = sqlite3_prepare_v2(pDb.get(), sql.toStdString().c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
		if (status == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				auto id = sqlite3_column_int(stmt, 0);
				idsSet.insert(id);
			}
			sqlite3_finalize(stmt);
		}

		std::vector<int> ids;
		std::copy(idsSet.begin(), idsSet.end(), std::back_inserter(ids));
		return ids;
	}
}

CustomFieldValueService::CustomFieldValueService(DBBrowserDB& db, CustomFieldService & cfs)
		: _db(&db){
	_fields = cfs.GetFields();
	Load();
}

void CustomFieldValueService::Load() {
	auto tuIDs = GetTUIdsWithCustomFieldValues(*_db, _fields, _error, _errorMsg);
	for (auto id : tuIDs) {
		auto values = GetCustomFieldValues(*_db, _fields, id, _error, _errorMsg);
		if (!values.IsEmpty())
			_customFieldValues[id] = values;
	}
}

const SqlCustomFieldValueService::CustomFieldValues* const CustomFieldValueService::TryGetCustomValues(int tuID) const {
	auto found = _customFieldValues.find(tuID);
	if (found != _customFieldValues.end())
		return &(found->second);
	else
		return nullptr;
}

const CustomField& CustomFieldValueService::GetCustomField(int customFieldID) const {
	auto found = std::find_if(_fields.begin(), _fields.end(), [this, customFieldID](const CustomField& cf) { return cf.ID == customFieldID; });
	assert(found != _fields.end());
	return *found;
}
