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

	QString SqlFieldName(QString tableName, const SdltmFilterItem & fi, const CustomField & customField = CustomField())
	{
		QString fieldName;
		if (customField.IsPresent())
		{
			if (IsList(fi.FieldMetaType))
				fieldName = "picklist_value_id";
			else
				fieldName = "value";
		}
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

			// 23 = len(<Elements><Text>) + len(</Text>)
			case SdltmFieldType::SourceSegmentLength: return "(length(substr(t.source_segment,instr(t.source_segment,'<Elements>'),instr(t.source_segment,'</Elements>') - instr(t.source_segment,'<Elements>'))) - 23)"; 
				// 23 = len(<Elements><Text>) + len(</Text>)
			case SdltmFieldType::TargetSegmentLength: return "(length(substr(t.target_segment,instr(t.target_segment,'<Elements>'),instr(t.target_segment,'</Elements>') - instr(t.target_segment,'<Elements>'))) - 23)"; 
			case SdltmFieldType::NumberOfTagsInSourceSegment: return  "((length(t.source_segment) - length(replace(t.source_segment,'<Tag>',''))) / 5)"; 
			case SdltmFieldType::NumberOfTagsInTargetSegment: return  "((length(t.target_segment) - length(replace(t.target_segment,'<Tag>',''))) / 5)";

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

	bool IsSqlNumber(SdltmFieldMetaType metaType)
	{
		switch (metaType)
		{
		case SdltmFieldMetaType::Int:
		case SdltmFieldMetaType::Double:
		case SdltmFieldMetaType::Number: return true;

		case SdltmFieldMetaType::Text:
		case SdltmFieldMetaType::MultiText:
		case SdltmFieldMetaType::List:
		case SdltmFieldMetaType::CheckboxList:
		case SdltmFieldMetaType::DateTime: return false;

		default: assert(false); return false;
		}
	}


	void AppendSql(QString & sql, const QString & subQuery, bool isAnd, bool prependEnter = false)
	{
		if (subQuery.trimmed() == "")
			return;

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
		// custom field: present only if ID > 0
		assert(fi.FieldType != SdltmFieldType::CustomSqlExpression);

		if (IsSqlNumber(fi.FieldMetaType) && fi.FieldValue.trimmed() == "")
			// user hasn't filled the number
			return  "";

		if (!IsList(fi.FieldMetaType))
		{
			auto subQuery = SqlFieldName(tableName, fi, customField) + " " + SqlCompare(fi) + " ";
			if (fi.IsNegated)
				subQuery = "NOT (" + subQuery + ")";

			return subQuery;
		} else
		{
			// list or checklist
			auto fieldName = SqlFieldName(tableName, fi, customField);
			auto isList = fi.FieldMetaType == SdltmFieldMetaType::List;
			QString subQuery;
			if (isList)
			{
				// list
				auto id = customField.StringValueToID(fi.FieldValue);
				if (id < 0)
					// field not found in our database
					return "";
				subQuery = fieldName + " = " + QString::number(id) ;
			} else
			{
				// checklist
				for(const auto & value : fi.FieldValues)
				{
					auto id = customField.StringValueToID(value);
					if (id < 0)
						// field not found in our database
						continue;
					if (subQuery != "")
						subQuery += " OR ";
					subQuery += fieldName + " = " + QString::number(id);
				}
			}

			if (fi.IsNegated)
				subQuery = "NOT (" + subQuery + ")";

			return subQuery;
		}

	}

	struct CustomFieldQuery
	{
		// this is always ANDed with the rest
		QString QueryPrefix;
		SqlFilterBuilder Query;

		QString QueryString() const
		{
			if (QueryPrefix == "")
				return Query.Get();
			auto fieldSql ="(" + QueryPrefix + ") AND (" + Query.Get() + ")";
			return fieldSql;
		}
	};

	CustomField FindCustomField(const QString & fieldName, const std::vector<CustomField> & allCustomFields)
	{
		auto found = std::find_if(allCustomFields.begin(), allCustomFields.end(), [fieldName](const CustomField& f) { return f.FieldName == fieldName; });
		return found != allCustomFields.end() ? *found : CustomField();
	}

	// distinct suffix -- we want distinct queries, for instance, for Text and Multi-text, or List and checklist
	QString FieldValueQuery(const std::vector<SdltmFilterItem> & filterItems, const QString & joinTableName, 
							const QString & joinFieldName,
							const QString& distinctSuffix,
							const std::vector<CustomField> & allCustomFields, 
							QString & globalWhere, bool globalIsAnd)
	{
		if (filterItems.empty())
			return "";// no such fields

		// IMPORTANT:
		// dealing with checkbox list is quite far from trivial. At this time, I assume the user is looking for a single such field. If you'll create
		// a query for several checkboxlist fields, things might not work as expected. Especially for "Has all of" query.
		//
		// the reason for the extra complication is to check for "Has all of", i will count the number of "|" inside the SELECT group_concat,
		// so for instance, for 1|3|5, I know there are 3 values (because there are 2 separators)
		//
		// To check for "Has all of", I will count the number of separator and make sure they are 3. If you have several such fields,
		// we can end up with more than 3 values, and thus valid values won't be shown in the results

		QString distinctTableName = joinTableName + "_d" + distinctSuffix;
		auto isCheckboxList = filterItems[0].FieldMetaType == SdltmFieldMetaType::CheckboxList;

		QString separator = "|";
		QString valueFieldName = IsList(filterItems[0].FieldMetaType) ? "picklist_value_id" : "value";
		QString sql = "\r\n    , (SELECT group_concat(" + distinctTableName + ", '" + separator + "') FROM (\r\n"
			+ "       SELECT DISTINCT " + joinTableName + ".translation_unit_id, " +joinTableName + "." + valueFieldName + " as "
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
			auto queryPrefix = IsList(fi.FieldMetaType) ? "" : joinTableName + "." + joinFieldName + " = " + QString::number(customField.ID);
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
			AppendSql(subQuery, cf.second.QueryString(), isAnd, prependEnter);
		}

		auto distinctFieldName = "distinct_" + joinTableName + distinctSuffix;
		sql += " AND (" + subQuery + ")\r\n       ) AS " + distinctFieldName + ") AS " + distinctFieldName + " ";

		auto fieldCount = customFields.size();
		auto globalSql = distinctFieldName + " is not NULL";
		auto isCheckboxListAll = isCheckboxList && filterItems[0].ChecklistComparison == ChecklistComparisonType::HasAllOf;
		if (isCheckboxListAll)
			fieldCount = filterItems[0].FieldValues.size();
		if ((fieldCount > 1 && isAnd) || isCheckboxListAll)
		{
			// count number of separators
			auto lenOfSeparators = "length(" + distinctFieldName + ") - length(replace(" + distinctFieldName + ",'" + separator + "','')) = " + QString::number(fieldCount - 1);
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
	const int MAX_LEVEL = 100000;
	auto lowestIndent = std::min_element(_filter.FilterItems.begin(), _filter.FilterItems.end(), [MAX_LEVEL](const SdltmFilterItem& a, const SdltmFilterItem& b)
	{
		auto aLevel = a.IndentLevel;
		auto bLevel = b.IndentLevel;
		// ... ignore those items when user hasn't entered anything
		if (a.CustomFieldName == "")
			aLevel = MAX_LEVEL;
		if (b.CustomFieldName == "")
			bLevel = MAX_LEVEL;
		return aLevel < bLevel;
	});
	auto customFieldsIsAnd = lowestIndent != _filter.FilterItems.end() && lowestIndent->CustomFieldName != "" ? lowestIndent->IsAnd : true;

	QString sql = "SELECT t.id, t.source_segment, t.target_segment ";
	QString where;
	sql += FieldValueQuery(
				FilterCustomFields(_filter.FilterItems, SdltmFieldMetaType::Number), 
				"numeric_attributes", "attribute_id", "", _customFields, where, customFieldsIsAnd);
	sql += FieldValueQuery(
				FilterCustomFields(_filter.FilterItems, SdltmFieldMetaType::Text),
				"string_attributes", "attribute_id", "", _customFields, where, customFieldsIsAnd);
	sql += FieldValueQuery(
				FilterCustomFields(_filter.FilterItems, SdltmFieldMetaType::MultiText),
				"string_attributes", "attribute_id", "_mt", _customFields, where, customFieldsIsAnd);
	sql += FieldValueQuery(
				FilterCustomFields(_filter.FilterItems, SdltmFieldMetaType::DateTime),
				"date_attributes", "attribute_id", "", _customFields, where, customFieldsIsAnd);

	sql += FieldValueQuery(
		FilterCustomFields(_filter.FilterItems, SdltmFieldMetaType::List),
		"picklist_attributes", "picklist_value_id", "", _customFields, where, customFieldsIsAnd);
	sql += FieldValueQuery(
		FilterCustomFields(_filter.FilterItems, SdltmFieldMetaType::CheckboxList),
		"picklist_attributes", "picklist_value_id", "_pa", _customFields, where, customFieldsIsAnd);

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

	SqlFilterBuilder globalBuilder;
	if (where != "") {
		auto indentLevel = lowestIndent != _filter.FilterItems.end() ? lowestIndent->IndentLevel : MAX_LEVEL;
		if (customFieldsIsAnd)
			globalBuilder.AddAnd(where, indentLevel);
		else
			globalBuilder.AddOr(where, indentLevel);
	}

	QString limit;
	for (const auto & fi : filterItems)
	{
		if (fi.CustomFieldName != "")
			continue; // already processed
		if (fi.IsUserEditableArg)
			continue;

		QString condition;
		if (fi.FieldType != SdltmFieldType::CustomSqlExpression)
		{
			condition = SqlFieldName("t", fi) + " " + SqlCompare(fi) + " ";

			if (IsSqlNumber(fi.FieldMetaType) && fi.FieldValue.trimmed() == "")
				// user hasn't filled the number
				condition = "";

			if (fi.IsNegated && condition != "")
				condition = "NOT (" + condition + ")";
		}
		else
		{
			condition = CustomExpression(fi);
			if (condition == "")
				// custom expression with user-editable args, but user hasn't entered all values
				continue;
		}

		if (condition.startsWith("LIMIT "))
			condition = " " + condition; // so that we match LIMIT clause all the time
		auto limitIdx = condition.indexOf(" LIMIT ");
		if (limitIdx >= 0)
		{
			limit = condition.right(condition.size() - limitIdx);
			condition = condition.left(limitIdx);
		}

		if (fi.IsAnd)
			globalBuilder.AddAnd(condition, fi.IndentLevel);
		else
			globalBuilder.AddOr(condition, fi.IndentLevel);
	}

	sql += "FROM translation_units t ";
	if (where != "")
		sql += "\r\n            WHERE " + globalBuilder.Get();
	sql += "\r\n\r\n            ORDER BY t.id";
	if (limit != "")
		sql += " DESC " + limit;
	sql += "\r\n";
	return sql;
}

QString SdltmCreateSqlSimpleFilter::CustomExpression(const SdltmFilterItem& fi) const
{
	// see if i have user-editable args
	auto expression = fi.FieldValue;
	if (fi.IsCustomExpressionWithUserEditableArgs())
	{
		auto userEditableArgs = fi.ToUserEditableFilterItems();
		auto idxNext = 0;
		while (expression.indexOf('{', idxNext) >= 0)
		{
			auto start = expression.indexOf('{', idxNext);
			auto end = expression.indexOf('}', start);
			if (end >= 0)
			{
				auto name = expression.mid(start + 1, end - start - 1);
				auto typeIdx = name.indexOf(',');
				if (typeIdx >= 0)
					name = name.left(typeIdx);

				auto found = std::find_if(_filter.FilterItems.begin(), _filter.FilterItems.end(), 
					[name](const SdltmFilterItem& of) { return of.CustomFieldName == name; });
				if (found != _filter.FilterItems.end())
				{
					if (found->FieldValue != "")
						expression.replace(start, end + 1 - start, found->FieldValue);
					else
						//  user hasn't yet entered a value for <name>
						return fi.FieldValue;
				}
				else
					// the {<name>} was not found as a filter item
					return fi.FieldValue;
			}
			else
				// user is still editing - { without }
				return fi.FieldValue;
			idxNext = end;
		}
	}
	return expression;
}
