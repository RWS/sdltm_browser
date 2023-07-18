#include "SdltmFilter.h"

/*
SELECT     t.id,
	t.source_segment,
	t.target_segment,
	n.value AS numeric_value,
	(
		SELECT group_concat(distinct_pv, ',')
		FROM (
			SELECT DISTINCT p.translation_unit_id, pv.value AS distinct_pv
			FROM picklist_attributes p
			JOIN picklist_values pv ON p.picklist_value_id = pv.id
			WHERE t.id = p.translation_unit_id
		) AS distinct_picklist_values
	) AS picklist_values,
	(
		SELECT group_concat(distinct_sv, ',')
		FROM (
			SELECT DISTINCT s.translation_unit_id, s.value AS distinct_sv
			FROM string_attributes s
			WHERE t.id = s.translation_unit_id
		) AS distinct_string_values
	) AS string_values
FROM
	translation_units t
LEFT JOIN
	numeric_attributes n ON t.id = n.translation_unit_id
ORDER BY
	t.id; */


std::vector<SdltmFilterItem> SdltmFilterItem::ToUserEditableFilterItems() const
{
	assert(IsCustomExpressionWithUserEditableArgs());
	auto idxNext = 0;
	auto expr = FieldValue;
	std::vector<SdltmFilterItem> items;
	while (expr.indexOf('{', idxNext) >= 0)
	{
		auto start = expr.indexOf('{', idxNext);
		auto end = expr.indexOf('}', start);
		if (end >= 0)
		{
			++start;
			auto name = expr.mid(start, end - start);
			QString type = "s";
			auto typeIdx = name.indexOf(',');
			if (typeIdx >= 0)
			{
				type = name.right(name.size() - typeIdx - 1);
				name = name.left(typeIdx);
			}

			SdltmFilterItem arg;
			arg.CustomFieldName = name;
			arg.IsUserEditableArg = true;
			arg.FieldType = SdltmFieldType::CustomField;
			arg.NumberComparison = NumberComparisonType::Equal;
			arg.StringComparison = StringComparisonType::Equal;
			arg.MultiComparison = MultiComparisonType::HasItem;
			arg.MultiStringComparison = MultiStringComparisonType::AnyEqual;
			arg.ChecklistComparison = ChecklistComparisonType::HasAnyOf;

			if (type == "n") // number (int)
				arg.FieldMetaType = SdltmFieldMetaType::Int;
			else if (type == "d") // double
				arg.FieldMetaType = SdltmFieldMetaType::Double;
			else if (type == "s") // string
				arg.FieldMetaType = SdltmFieldMetaType::Text;
			else // default - string
				arg.FieldMetaType = SdltmFieldMetaType::Text;
			items.push_back(arg);
		}
		idxNext = end;
	}

	return items;
}

SdltmFieldMetaType SdltmFilterItem::PresetFieldMetaType(SdltmFieldType fieldType)
{
	switch (fieldType)
	{
	case SdltmFieldType::LastModifiedBy:
	case SdltmFieldType::LastUsedBy:
	case SdltmFieldType::CreatedBy:
	case SdltmFieldType::SourceSegment:
	case SdltmFieldType::TargetSegment:
	case SdltmFieldType::CustomSqlExpression:
		return SdltmFieldMetaType::Text;
		break;

	case SdltmFieldType::LastModifiedOn:
	case SdltmFieldType::LastUsedOn:
	case SdltmFieldType::CreatedOn:
		return SdltmFieldMetaType::DateTime;
		break;

	case SdltmFieldType::UseageCount:
	case SdltmFieldType::SourceSegmentLength:
	case SdltmFieldType::TargetSegmentLength:
	case SdltmFieldType::NumberOfTagsInSourceSegment:
	case SdltmFieldType::NumberOfTagsInTargetSegment:
		return SdltmFieldMetaType::Int;
		break;
	case SdltmFieldType::CustomField:
		assert(false);
		break;
	default: assert(false);
	}
	return SdltmFieldMetaType::Text;
}


QString SdltmFilterItem::FriendlyString() const
{
	const int INDENT_SIZE = 4;
	QString friendly = "";
	// care about indent
	friendly += QString(IndentLevel * INDENT_SIZE, ' ');
	if (IsNegated)
		friendly += " NOT ";

	switch (FieldType)
	{
	case SdltmFieldType::LastModifiedOn: friendly += "Last Modified On"; break;
	case SdltmFieldType::LastModifiedBy: friendly += "Last Modified By"; break;
	case SdltmFieldType::LastUsedOn: friendly += "Last Used On"; break;
	case SdltmFieldType::LastUsedBy: friendly += "Last Used By"; break;
	case SdltmFieldType::UseageCount: friendly += "Usage Count"; break;
	case SdltmFieldType::CreatedOn: friendly += "Created On"; break;
	case SdltmFieldType::CreatedBy: friendly += "Created By"; break;
	case SdltmFieldType::SourceSegment: friendly += "Source Segment"; break;
	case SdltmFieldType::SourceSegmentLength: friendly += "Source Segment Length"; break;
	case SdltmFieldType::TargetSegment: friendly += "Target Segment"; break;
	case SdltmFieldType::TargetSegmentLength: friendly += "Target Segment Length"; break;
	case SdltmFieldType::NumberOfTagsInSourceSegment: friendly += "Number of Tags in Source"; break;
	case SdltmFieldType::NumberOfTagsInTargetSegment: friendly += "Number of Tags in Target"; break;

	case SdltmFieldType::CustomSqlExpression: 
		friendly += FieldValue;
		return friendly;

	case SdltmFieldType::CustomField: friendly += CustomFieldName; break;
	default: assert(false); break;
	}

	auto hasCaseSensitive = false;
	switch (FieldMetaType)
	{
		// number
	case SdltmFieldMetaType::Int: 
	case SdltmFieldMetaType::Double: 
	case SdltmFieldMetaType::Number: 
	case SdltmFieldMetaType::DateTime:
		switch(NumberComparison)
		{
		case NumberComparisonType::Equal: friendly += " = "; break;
		case NumberComparisonType::Less:  friendly += " < "; break;
		case NumberComparisonType::LessOrEqual:  friendly += " <="; break;
		case NumberComparisonType::Bigger:  friendly += " > "; break;
		case NumberComparisonType::BiggerOrEqual:  friendly += " >= "; break;
		default: assert(false); break;
		}
		break;

		// string
	case SdltmFieldMetaType::Text:
		hasCaseSensitive = true;
		switch(StringComparison)
		{
		case StringComparisonType::Equal: friendly += " = "; break;
		case StringComparisonType::Contains: friendly += " Contains "; break;
		case StringComparisonType::StartsWith: friendly += "Starts With "; break;
		case StringComparisonType::EndsWith: friendly += "Ends With "; break;
		default: assert(false); break;
		}
		break;

		// multi-string
	case SdltmFieldMetaType::MultiText:
		hasCaseSensitive = true;
		switch(MultiStringComparison)
		{
		case MultiStringComparisonType::AnyEqual: friendly += " Any Equals "; break;
		case MultiStringComparisonType::AnyContains: friendly += " Any Contains "; break;
		case MultiStringComparisonType::AnyStartsWith: friendly += "Any Starts With "; break;
		case MultiStringComparisonType::AnyEndsWith: friendly += " Any Ends With "; break;
		default: assert(false); break;
		}
		break;

		// multi-comparison (has-item) -- single value
	case SdltmFieldMetaType::List: 
		friendly += " Has Value ";
		break;

		// multi-comparison (has-item) -- several values
	case SdltmFieldMetaType::CheckboxList: 
		switch (ChecklistComparison) {
			case ChecklistComparisonType::HasAnyOf: friendly += " Has Any Of "; break;
			case ChecklistComparisonType::HasAllOf: friendly += " Has All Of "; break;
			case ChecklistComparisonType::Equals: friendly += " Equals "; break;
			default: ;
		}
		break;
	default: assert(false); break;
	}

	auto isChecklist = FieldMetaType == SdltmFieldMetaType::CheckboxList;
	if (!isChecklist)
		friendly += FieldValue;
	else
	{
		friendly += " [";
		auto prependComma = false;
		for (const auto & fv : FieldValues)
		{
			if (prependComma)
				friendly += ", ";
			friendly += fv;
			prependComma = true;
		}
		friendly += "]";
	}

	if (hasCaseSensitive && CustomFieldName == "" && !CaseSensitive)
		friendly += " (Case-Insensitive)";

	return friendly;
}
