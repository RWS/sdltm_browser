#pragma once
#include <vector>

#include "CustomFieldService.h"

class DBBrowserDB;

namespace SqlCustomFieldValueService {
	struct Numeric {
		int CustomFieldID;
		int Value;
	};
	struct Date {
		int CustomFieldID;
		QDateTime Value;
	};
	struct SingleString {
		int CustomFieldID;
		QString Text;
	};
	struct MultiString {
		int CustomFieldID;
		std::vector<QString> Texts;
	};
	struct Combo {
		int CustomFieldID;
		QString Value;
	};
	struct Checklist {
		int CustomFieldID;
		std::vector<QString> Values;
	};

	struct CustomFieldValues {
		bool IsEmpty() const {
			return Numbers.empty() && Dates.empty() && Texts.empty() && MultiTexts.empty() && Combos.empty() && Checklists.empty();
		}

		std::vector<Numeric> Numbers;
		std::vector<Date> Dates;
		std::vector<SingleString> Texts;
		std::vector<MultiString> MultiTexts;
		std::vector<Combo> Combos;
		std::vector<Checklist> Checklists;
	};
}

class CustomFieldValueService
{
public:
	explicit CustomFieldValueService(DBBrowserDB& db, CustomFieldService& cfs);

private:
	void Load();
public:
	const SqlCustomFieldValueService::CustomFieldValues * const TryGetCustomValues(int tuID) const;
	const CustomField& GetCustomField(int customFieldID) const;
private:
	DBBrowserDB* _db;
	std::vector<CustomField> _fields;

	int _error = 0;
	QString _errorMsg;

	// Tranlation Unit ID -> its fields
	typedef std::map<int, SqlCustomFieldValueService::CustomFieldValues> map;
	map _customFieldValues;
};

