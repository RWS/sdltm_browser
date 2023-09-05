#pragma once
#include <QDateTime>

#include "DbFieldValueService.h"
#include "SdltmFilter.h"
#include "sqlitedb.h"

struct CustomField
{
	friend bool operator==(const CustomField& lhs, const CustomField& rhs) {
		return lhs.ID == rhs.ID
			&& lhs.FieldName == rhs.FieldName
			&& lhs.FieldType == rhs.FieldType
			&& lhs.Values == rhs.Values
			&& lhs.ValueToID == rhs.ValueToID;
	}

	friend bool operator!=(const CustomField& lhs, const CustomField& rhs) {
		return !(lhs == rhs);
	}

	bool IsPresent() const { return ID > 0; }

	int ID = 0;
	QString FieldName;
	SdltmFieldMetaType FieldType = SdltmFieldMetaType::Text;

	int StringValueToID(const QString & val) const
	{
		auto it = std::find(Values.begin(), Values.end(), val);
		if (it != Values.end())
			return ValueToID[ it - Values.begin()];
		else
			return -1;
	}
	int StringValueToIndex(const QString& val) const
	{
		auto it = std::find(Values.begin(), Values.end(), val);
		if (it != Values.end())
			return it - Values.begin();
		else
			return -1;
	}

	bool IsEquivalent(const CustomField& other) const;

	// only when it's a list of strings 
	std::vector<QString> Values;
	// each picklist value has an ID
	std::vector<int> ValueToID;
};


struct CustomFieldValue {
	friend bool operator==(const CustomFieldValue& lhs, const CustomFieldValue& rhs) {
		return lhs._field == rhs._field
			&& lhs.Text == rhs.Text
			&& lhs.MultiText == rhs.MultiText
			&& lhs.Time == rhs.Time
			&& lhs.ComboIndex == rhs.ComboIndex
			&& lhs.CheckboxIndexes == rhs.CheckboxIndexes;
	}

	friend bool operator!=(const CustomFieldValue& lhs, const CustomFieldValue& rhs) {
		return !(lhs == rhs);
	}

	CustomFieldValue(const CustomField& cf);

	const CustomField& Field() const { return _field; }

	bool IsEmpty() const;
	QString FriendlyValue() const;

	QString Text = "";
	std::vector<QString> MultiText;
	QDateTime Time = QDateTime(QDate(1990,1,1));
	int ComboIndex = -1;
	std::vector<bool> CheckboxIndexes; // checklist

	int ComboId() const;
	std::vector<int> CheckboxIds() const;

private:
	CustomField _field;
	int IndexToId(int index) const;
};


class CustomFieldService
{
public:
	explicit CustomFieldService(DBBrowserDB& db);

	void Update();
	const std::vector<CustomField>& GetFields() const;

	int NextCustomFieldID() const;
	int NextCustomFieldListvalueID() const;

private:
	DBBrowserDB* _db;
	std::vector<CustomField> _fields;
	DbFieldValueService _fvService;
};

