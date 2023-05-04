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

QString SdltmFilterItem::MetaFieldValue() const
{
	assert(IsMetaFieldValue());
	// ignore $
	auto meta = FieldValue.mid(1);
	auto idxComma = meta.indexOf(',');
	if (idxComma >= 0)
		meta = meta.left(idxComma);
	return meta;
}

SdltmFilterItem SdltmFilterItem::ToUserEditableFilterItem() const
{
	assert(IsMetaFieldValue());
	SdltmFilterItem editable;
	editable.CustomFieldName = MetaFieldValue();
	editable.FieldType = FieldType;
	editable.FieldMetaType = FieldMetaType;
	editable.NumberComparison = NumberComparison;
	editable.StringComparison = StringComparison;
	editable.MultiComparisson = MultiComparisson;
	editable.MultiStringComparison = MultiStringComparison;
	return editable;
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
		friendly += " Has Any Of ";
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

	return friendly;
}
