#pragma once
#include <QString>
#include <vector>


enum class SdltmFieldMetaType
{
	Int, Double,
	Text,
	// user can actually enter multiple texts
	MultiText,
	Number,
	// user can pick one value from a list
	List,

	// user can pick zero-to-many from a list
	CheckboxList,

	DateTime,
};

// IMPORTANT:
// must match _SdltmFieldTypes from EditSdltmFilter.cpp !!!
enum class SdltmFieldType
{
	// presets
	LastModifiedOn,
	LastModifiedBy,
	LastUsedOn,
	LastUsedBy,
	UseageCount,
	CreatedOn,
	CreatedBy,
	SourceSegment,
	SourceSegmentLength,
	TargetSegment,
	TargetSegmentLength,
	NumberOfTagsInSourceSegment,
	NumberOfTagsInTargetSegment,

	// in this case, we treat the value as an SQL query
	CustomSqlExpression,

	// custom attributes, the types can be set by the user
	CustomField, 
};

enum class NumberComparisonType
{
	// there's no "Not Equal", since user can simply click "Negate"
	Equal, Less, LessOrEqual, Bigger, BiggerOrEqual,
};

enum class StringComparisonType
{
	Equal, Contains, StartsWith, EndsWith,
};

enum class MultiStringComparisonType
{
	AnyEqual, AnyContains, AnyStartsWith, AnyEndsWith,
};

enum class MultiComparisonType
{
	HasItem,	
};




struct SdltmFilterItem
{
	bool IsNegated = false;

	SdltmFilterItem()
	{}
	// helper
	SdltmFilterItem(QString customFieldName, SdltmFieldMetaType metaType)
	{
		FieldMetaType = metaType;
		CustomFieldName = customFieldName;
		FieldType = SdltmFieldType::CustomField;
	}
	// helper
	SdltmFilterItem(SdltmFieldType type)
	{
		FieldType = type;
		FieldMetaType = PresetFieldMetaType(type);
	}

	SdltmFieldMetaType FieldMetaType;
	SdltmFieldType FieldType;
	// only used when the field meta type is "CustomField"
	QString CustomFieldName = "";
	// only used when the field meta type is "CustomField" and it's a list 
	std::vector<QString> CustomValues;
	NumberComparisonType NumberComparison = NumberComparisonType::Equal;
	StringComparisonType StringComparison = StringComparisonType::Equal;
	MultiStringComparisonType MultiStringComparison = MultiStringComparisonType::AnyEqual;
	MultiComparisonType MultiComparisson = MultiComparisonType::HasItem;

	// if FieldValue starts with $, it's a meta value
	// we can create common scenarios, where the user only needs to enter one or two values, and the rest is pre-computed
	//
	// Example: "Last X Days" - I want to allow the user to specify the number of days
	QString FieldValue;
	// in case user selects several items from a list
	std::vector<QString> FieldValues;

	bool IsMetaFieldValue() const
	{
		return FieldValue.startsWith("$");
	}
	QString MetaFieldValue() const;
	SdltmFilterItem ToUserEditableFilterItem() const;

	static SdltmFieldMetaType PresetFieldMetaType(SdltmFieldType fieldType);

	int IndentLevel = 0;
	bool IsAnd = true;

	bool CaseSensitive = false;

	// can be hidden when this is not available for the current database (for instance, it's using a custom field not available here)
	bool IsVisible = true;

	QString FriendlyString() const;
};



struct SdltmFilter
{
	std::vector<SdltmFilterItem> FilterItems;

	bool HasQuickSearch() const
	{
		if (QuickSearch != "")
			return true;

		if (QuickSearchTarget != "" && !QuickSearchSearchSourceAndTarget)
			return true;

		return false;
	}

	// note: the quick search is implemented as CONTAINS
	QString QuickSearch;
	QString QuickSearchTarget;
	bool QuickSearchCaseSensitive = false;
	// if true, search both source + target for the same text
	bool QuickSearchSearchSourceAndTarget = true;

	// if user goes to "advanced" tab and modifies anything
	QString AdvancedSql;

	// we'll be using this when creating presets (common scenarios)
	// in this case, I don't want the user to add/insert/delete , just allow it to set one or two values and hit "Run"
	bool IsLocked = false;
};

