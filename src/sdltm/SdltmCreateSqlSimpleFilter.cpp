#include "SdltmCreateSqlSimpleFilter.h"

#include "SqlFilterBuilder.h"

SdltmCreateSqlSimpleFilter::SdltmCreateSqlSimpleFilter(const SdltmFilter& filter, const std::vector<CustomField>& customFields)
	: _filter(filter), _customFields(customFields)
{
}

namespace 
{
	bool CanHaveCaseInsensitive(const SdltmFieldMetaType & metaType)
	{
		switch (metaType)
		{

		case SdltmFieldMetaType::Text: 
		case SdltmFieldMetaType::MultiText: return true;

		case SdltmFieldMetaType::Int: break;
		case SdltmFieldMetaType::Double: break;
		case SdltmFieldMetaType::Number: break;
		case SdltmFieldMetaType::List: break;
		case SdltmFieldMetaType::CheckboxList: break;
		case SdltmFieldMetaType::DateTime: break;
		default: assert(false); break;
		}
		return false;
	}

	QString SqlFieldName(QString tableName, const SdltmFilterItem & fi, const CustomField & customField = CustomField())
	{
		QString fieldName;
		if (customField.IsPresent())
			fieldName = "value";
		else 
			switch (fi.FieldType)
			{
			case SdltmFieldType::LastModifiedOn: fieldName = "change_date"; break;
			case SdltmFieldType::LastModifiedBy: fieldName ="change_user"; break;
			case SdltmFieldType::LastUsedOn: fieldName ="last_used_date"; break;
			case SdltmFieldType::LastUsedBy: fieldName ="last_used_user"; break;
			case SdltmFieldType::UseageCount: fieldName ="usage_counter"; break;
			case SdltmFieldType::CreatedOn: fieldName ="creation_date"; break;
			case SdltmFieldType::CreatedBy: fieldName ="creation_user"; break;
			case SdltmFieldType::SourceSegment: fieldName ="source_segment"; break;
			case SdltmFieldType::TargetSegment: fieldName ="target_segment"; break;

				// FIXME each of these needs separate handling
			case SdltmFieldType::SourceSegmentLength: fieldName ="source_segment"; break;
			case SdltmFieldType::TargetSegmentLength: fieldName ="target_segment"; break;
			case SdltmFieldType::NumberOfTagsInSourceSegment: fieldName ="source_tags"; break;
			case SdltmFieldType::NumberOfTagsInTargetSegment: fieldName ="target_tags"; break;

			case SdltmFieldType::CustomSqlExpression: 
				assert(false); fieldName =""; break;
			case SdltmFieldType::CustomField:
				// here, we should have a custom field (custom field should be present)
				assert(false);
				fieldName =""; break;
			default: ;
			}

		fieldName = tableName + "." + fieldName;
		if (CanHaveCaseInsensitive(fi.FieldMetaType) && !fi.CaseSensitive)
			fieldName = "lower(" + fieldName + ")";

		return fieldName;
	}

	QString EscapeSqlString(const QString & s)
	{
		QString copy = s;
		copy.replace("%", "%%").replace("'", "''");
		return copy;
	}

	QString SqlCompare(const SdltmFilterItem & fi)
	{
		// note: even this is a number, and it's case-insensitive (which would make no sense),
		// converting a number to lower is the equivalent of a no-op
		auto value = fi.FieldValue;
		if (CanHaveCaseInsensitive(fi.FieldMetaType) && !fi.CaseSensitive)
			value = value.toLower();

		// care if number or string
		switch (fi.FieldMetaType)
		{
			// number
		case SdltmFieldMetaType::Int:
		case SdltmFieldMetaType::Double:
		case SdltmFieldMetaType::Number:
			switch (fi.NumberComparison)
			{
			case NumberComparisonType::Equal: return " = " + value; 
			case NumberComparisonType::Less:  return " < " + value;
			case NumberComparisonType::LessOrEqual:  return " <=" + value;
			case NumberComparisonType::Bigger:  return " > " + value;
			case NumberComparisonType::BiggerOrEqual:  return " >= " + value;
			default: assert(false); 
			}
			

		case SdltmFieldMetaType::DateTime:
			

			// string
		case SdltmFieldMetaType::Text:
			switch (fi.StringComparison)
			{
			case StringComparisonType::Equal: return " = '" + EscapeSqlString(value) + "'";
			case StringComparisonType::Contains: return " LIKE '%" + EscapeSqlString(value) + "%'";
			case StringComparisonType::StartsWith: return " LIKE '" + EscapeSqlString(value) + "%'";
			case StringComparisonType::EndsWith: return " LIKE '%" + EscapeSqlString(value) + "'";
			default: assert(false); 
			}
			

			// multi-string
		case SdltmFieldMetaType::MultiText:
			switch (fi.MultiStringComparison)
			{
			case MultiStringComparisonType::AnyEqual: return " = '" + EscapeSqlString(value) + "'";
			case MultiStringComparisonType::AnyContains: return " LIKE '%" + EscapeSqlString(value) + "%'";
			case MultiStringComparisonType::AnyStartsWith: return " LIKE '" + EscapeSqlString(value) + "%'";
			case MultiStringComparisonType::AnyEndsWith: return " LIKE '%" + EscapeSqlString(value) + "'";
			default: assert(false); 
			}
			

			// multi-comparison (has-item) -- single value
		case SdltmFieldMetaType::List:
			assert(false);
			break;

			// multi-comparison (has-item) -- several values
		case SdltmFieldMetaType::CheckboxList:
			assert(false);
			break;

		default: 
			assert(false); 
			break;
		}

		return "";
	}

	bool IsSqlString(SdltmFieldMetaType metaType)
	{
		switch (metaType)
		{
		case SdltmFieldMetaType::Int: 
		case SdltmFieldMetaType::Double: 
		case SdltmFieldMetaType::Number: return false;

		case SdltmFieldMetaType::Text: 
		case SdltmFieldMetaType::MultiText: 
		case SdltmFieldMetaType::List: 
		case SdltmFieldMetaType::CheckboxList: 
		case SdltmFieldMetaType::DateTime: return true;

		default: assert(false); return false;
		}
	}

	bool IsList(SdltmFieldMetaType metaType)
	{
		switch (metaType)
		{
		case SdltmFieldMetaType::List:
		case SdltmFieldMetaType::CheckboxList: return true;

		case SdltmFieldMetaType::Int: 
		case SdltmFieldMetaType::Double: 
		case SdltmFieldMetaType::Text: 
		case SdltmFieldMetaType::MultiText: 
		case SdltmFieldMetaType::Number: 
		case SdltmFieldMetaType::DateTime: return false;

		default: assert(false); return false;
		}
	}

	void AppendSql(QString & sql, const QString & subQuery, bool isAnd, bool prependEnter = false)
	{
		if (sql != "")
		{
			if (prependEnter)
				sql += "\r\n            ";
			sql += isAnd ? " AND " : " OR ";
		}
		sql += "(" + subQuery + ")";
	}

	// IMPORTANT: I don't use fi.isAnd, because I can override that for custom fields
	// the custom fields, in the FieldValueQuery, they are ALWAYS ORed
	QString GetSubSql( const SdltmFilterItem &fi, 
					const QString & tableName, 
					const CustomField & customField = CustomField(), const QString& customFieldSqlFieldName = "")
	{
		if (IsList(fi.FieldMetaType))
			return ""; // ignore list fields for now

		// custom field: present only if ID > 0
		assert(fi.FieldType != SdltmFieldType::CustomSqlExpression);
		auto subQuery = SqlFieldName(tableName, fi, customField) + " " + SqlCompare(fi) + " ";
		if (fi.IsNegated)
			subQuery = "NOT (" + subQuery + ")";

		return subQuery;
	}

	struct CustomFieldQuery
	{
		// this is always ANDed with the rest
		QString QueryPrefix;
		SqlFilterBuilder Query;
	};

	CustomField FindCustomField(const QString & fieldName, const std::vector<CustomField> & allCustomFields)
	{
		auto found = std::find_if(allCustomFields.begin(), allCustomFields.end(), [fieldName](const CustomField& f) { return f.FieldName == fieldName; });
		return found != allCustomFields.end() ? *found : CustomField();
	}

	QString FieldValueQuery(const std::vector<SdltmFilterItem> & filterItems, const QString & joinTableName, 
							const QString & joinFieldName, 
							const std::vector<CustomField> & allCustomFields, 
							QString & globalWhere, bool globalIsAnd)
	{
		if (filterItems.empty())
			return "";// no such fields

		QString distinctTableName = joinTableName + "_d";
		QString separator = "|";
		QString sql = "\r\n    , (SELECT group_concat(" + distinctTableName + ", '" + separator + "') FROM (\r\n"
			+ "       SELECT DISTINCT " + joinTableName + ".translation_unit_id, " +joinTableName + ".value as "
			+ distinctTableName + " FROM " + joinTableName + "\r\n         WHERE t.id = " + joinTableName+ ".translation_unit_id";
		// find field value -> custom field
		std::map<QString, CustomFieldQuery> customFields;
		for (const auto & fi : filterItems)
		{
			auto customField = FindCustomField(fi.CustomFieldName, allCustomFields);
			if (!customField.IsPresent())
				continue; // field not found

			auto subSql = GetSubSql(fi, joinTableName, customField, joinFieldName);
			if (subSql == "")
				continue;

			auto& subField = customFields[fi.CustomFieldName];
			auto queryPrefix = joinTableName + "." + joinFieldName + " = " + QString::number(customField.ID) ;
			subField.QueryPrefix = queryPrefix;

			if (fi.IsAnd)
				subField.Query.AddAnd(subSql, fi.IndentLevel);
			else 
				subField.Query.AddOr(subSql, fi.IndentLevel);
		}

		if (customFields.empty())
			return ""; // no custom fields present

		// at this point, I want to find out whether this is an AND or an OR
		// look at the first item with the lowest Indent
		auto lowestIndent = std::min_element(filterItems.begin(), filterItems.end(), [](const SdltmFilterItem& a, const SdltmFilterItem & b) { return a.IndentLevel < b.IndentLevel; });
		auto isAnd = lowestIndent->IsAnd;

		// custom fields here are always ORed, since they are different SQL records
		// (if we were to AND anything, we'd always end up with an empty query)
		QString subQuery;
		auto prependEnter = true;
		for (const auto & cf : customFields)
		{
			auto fieldSql = cf.second.QueryPrefix;
			fieldSql += " AND (" + cf.second.Query.Get() + ")";
			AppendSql(subQuery, fieldSql, isAnd, prependEnter);
		}

		auto distinctFieldName = "distinct_" + joinTableName;
		sql += " AND (" + subQuery + ")\r\n       ) AS " + distinctFieldName + ") AS " + distinctFieldName + " ";

		auto fieldCount = customFields.size();
		auto globalSql = distinctFieldName + " is not NULL";
		if (fieldCount > 1 && isAnd)
		{
			// count number of separators
			auto lenOfSeparators = "len(" + distinctFieldName + ") - len(replace(" + distinctFieldName + ",'" + separator + "','')) = " + QString::number(fieldCount);
			globalSql += " AND (" + lenOfSeparators + ")";
		}
		AppendSql(globalWhere, globalSql, globalIsAnd, prependEnter);

		return sql;
	}

	std::vector<SdltmFilterItem> FilterCustomFields(const std::vector<SdltmFilterItem> & fields, SdltmFieldMetaType metaType)
	{
		std::vector<SdltmFilterItem> customFields;
		for (const auto & field : fields)
		{
			if (field.CustomFieldName != "" && field.FieldMetaType == metaType)
				customFields.push_back(field);
		}
		return  customFields;
	}

}

QString SdltmCreateSqlSimpleFilter::ToSqlFilter() const
{
	auto lowestIndent = std::min_element(_filter.FilterItems.begin(), _filter.FilterItems.end(), [](const SdltmFilterItem& a, const SdltmFilterItem& b)
	{
		auto aLevel = a.IndentLevel;
		auto bLevel = b.IndentLevel;
		if (a.CustomFieldName == "")
			aLevel = 100000;
		if (b.CustomFieldName == "")
			bLevel = 100000;
		return aLevel < bLevel;
	});
	auto customFieldsIsAnd = lowestIndent != _filter.FilterItems.end() && lowestIndent->CustomFieldName != "" ? lowestIndent->IsAnd : true;

	QString sql = "SELECT t.id, t.source_segment, t.target_segment ";
	QString where;
	sql += FieldValueQuery(
				FilterCustomFields(_filter.FilterItems, SdltmFieldMetaType::Number), 
				"numeric_attributes", "attribute_id", _customFields, where, customFieldsIsAnd);
	sql += FieldValueQuery(
				FilterCustomFields(_filter.FilterItems, SdltmFieldMetaType::Text),
				"string_attributes", "attribute_id", _customFields, where, customFieldsIsAnd);
	sql += FieldValueQuery(
				FilterCustomFields(_filter.FilterItems, SdltmFieldMetaType::MultiText),
				"string_attributes", "attribute_id", _customFields, where, customFieldsIsAnd);
	sql += FieldValueQuery(
				FilterCustomFields(_filter.FilterItems, SdltmFieldMetaType::DateTime),
				"date_attributes", "attribute_id", _customFields, where, customFieldsIsAnd);
	// FIXME lists/picklists/ multi-text ?

	auto filterItems = _filter.FilterItems;
	// quick source/target - if present
	if (_filter.HasQuickSearch())
	{
		auto source = _filter.QuickSearch;
		auto target = _filter.QuickSearchSearchSourceAndTarget ? _filter.QuickSearch : _filter.QuickSearchTarget;
		// case sensitive
		auto isOr = source != "" && target != "";
		if (source != "")
		{
			SdltmFilterItem fi(SdltmFieldType::SourceSegment);
			fi.FieldValue = source;
			fi.CaseSensitive = _filter.QuickSearchCaseSensitive;
			filterItems.push_back(fi);
		}
		if (target != "")
		{
			SdltmFilterItem fi(SdltmFieldType::TargetSegment);
			fi.FieldValue = target;
			fi.CaseSensitive = _filter.QuickSearchCaseSensitive;
			fi.IsAnd = !isOr;
			filterItems.push_back(fi);
		}
	}

	for (const auto & fi : filterItems)
	{
		if (fi.CustomFieldName != "")
			continue;

		QString condition =  SqlFieldName("t", fi) + " " + SqlCompare(fi) + " ";
		if (fi.FieldType == SdltmFieldType::CustomSqlExpression)
			condition = fi.FieldValue;

		if (fi.IsNegated)
			condition = "NOT (" + condition + ")";
		auto prependEnter = true;
		AppendSql(where, condition, fi.IsAnd, prependEnter);
	}

	sql += "FROM translation_units t ";
	if (where != "")
		sql += "\r\n            WHERE " + where;
	sql += "\r\n\r\n            ORDER BY t.id\r\n";
	return sql;
}
