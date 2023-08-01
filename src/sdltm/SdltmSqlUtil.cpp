#include "SdltmSqlUtil.h"

#include "SdltmCreateSqlSimpleFilter.h"
#include "SdltmUtil.h"

std::vector<int> RunQueryGetIDs(const QString& sql, DBBrowserDB& db) {
    auto forceWait = true;
    auto pDb = db.get("run sql get IDs", forceWait);

    std::vector<int> result;

    sqlite3_stmt* stmt;
    int status = sqlite3_prepare_v2(pDb.get(), sql.toStdString().c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
    if (status == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            auto id = sqlite3_column_int(stmt, 0);
            result.push_back(id);
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

int RunQueryGetCount(const QString& sql, DBBrowserDB& db) {
    auto forceWait = true;
    auto pDb = db.get("run sql get count", forceWait);

    int count = 0;
    sqlite3_stmt* stmt;
    auto countSql = "SELECT count(*) FROM ( " + sql + " )";
    int status = sqlite3_prepare_v2(pDb.get(), countSql.toStdString().c_str(), static_cast<int>(countSql.size()), &stmt, nullptr);
    if (status == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

/*
 * We're updating the translation units table.
 *
 * We have 2 inputs:
 * - the original SELECT, which generates what records we need to update
 * - the UPDATE -> what are we trying to update in the translation_units table


    UPDATE translation_units
    SET <update>
    WHERE id in (<array>)

    Example: UPDATE translation_units SET source_hash=source_hash+1 WHERE id in (1,12,15,23)

    The <array> is an array of integers. However, the <array> can also be an SQL.
    So we can have:

        UPDATE translation_units
        SET <update>
        WHERE id in (<sql>)

    However, if the <sql> contains several columns, the above will generate an error, since the WHERE id in (...) expects a single column.
    Thus, we'll wrap our original <sql> into
    "SELECT id FROM (<sql>)"

    So our query will become:

        UPDATE translation_units
        SET <update>
        WHERE id in (SELECT id FROM (<sql>))


    Example:
        UPDATE translation_units
        SET source_hash=source_hash+1
        WHERE id in (SELECT id FROM (

        SELECT t.id, t.source_segment, t.target_segment
            , (SELECT group_concat(string_attributes_d, '|') FROM (
               SELECT DISTINCT string_attributes.translation_unit_id, string_attributes.value as string_attributes_d FROM string_attributes
                 WHERE t.id = string_attributes.translation_unit_id AND ((string_attributes.attribute_id = 3 AND (lower(string_attributes.value)  = 'gigi' ))
                     AND (string_attributes.attribute_id = 2 AND (lower(string_attributes.value)  LIKE '%ent%' )))
               ) AS distinct_string_attributes) AS distinct_string_attributes
            , (SELECT group_concat(string_attributes_d_mt, '|') FROM (
               SELECT DISTINCT string_attributes.translation_unit_id, string_attributes.value as string_attributes_d_mt FROM string_attributes
                 WHERE t.id = string_attributes.translation_unit_id AND ((string_attributes.attribute_id = 7 AND ((lower(string_attributes.value)  LIKE '%titi%' ) OR (lower(string_attributes.value)  LIKE '%gi%' ))))
               ) AS distinct_string_attributes_mt) AS distinct_string_attributes_mt FROM translation_units t
                    WHERE (distinct_string_attributes is not NULL AND (length(distinct_string_attributes) - length(replace(distinct_string_attributes,'|','')) = 1))
                     OR (distinct_string_attributes_mt is not NULL)
                    ORDER BY t.id))

 */
namespace {
    bool TryRunUpdateSourceOrTargetTextSql(const QString& selectSql, const QString& updateSql, DBBrowserDB& db, int& error, QString& errorMsg) {
        auto sql = "UPDATE translation_units SET " + updateSql + " WHERE id in (SELECT id FROM ( " + selectSql + " ))";

        auto forceWait = true;
        auto pDb = db.get("run sql update", forceWait);

        bool ok = false;
        sqlite3_stmt* stmt;
        int status = sqlite3_prepare_v2(pDb.get(), sql.toStdString().c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
        if (status == SQLITE_OK) {
            ok = sqlite3_step(stmt) == SQLITE_DONE;
            sqlite3_finalize(stmt);
        }

        if (!ok) {
            error = sqlite3_errcode(pDb.get());
            errorMsg = sqlite3_errmsg(pDb.get());
        }

        SdltmLog("update sql=" + sql);
        return ok;
    }
}


namespace {
    QString FieldTypeToAttributesTable(SdltmFieldMetaType fieldType) {
        switch (fieldType) {
        case SdltmFieldMetaType::Int: assert(false); break;
        case SdltmFieldMetaType::Double: assert(false); break;
        case SdltmFieldMetaType::Text:
            return "string_attributes";
        case SdltmFieldMetaType::MultiText:
            return "string_attributes";
        case SdltmFieldMetaType::Number:
            return "numeric_attributes";
        case SdltmFieldMetaType::List:
            return "picklist_attributes";
        case SdltmFieldMetaType::CheckboxList:
            return "picklist_attributes";
        case SdltmFieldMetaType::DateTime:
            return "date_attributes";
        default:;
        }

        assert(false);
        return "string_attributes";
    }
    QString FieldTypeToAttributesField(SdltmFieldMetaType fieldType) {
        switch (fieldType) {
        case SdltmFieldMetaType::Int: assert(false); break;
        case SdltmFieldMetaType::Double: assert(false); break;
        case SdltmFieldMetaType::Text:
        case SdltmFieldMetaType::MultiText:
        case SdltmFieldMetaType::Number:
        case SdltmFieldMetaType::DateTime:
            return "attribute_id";
        case SdltmFieldMetaType::List:
        case SdltmFieldMetaType::CheckboxList:
            return "picklist_value_id";
        default:;
        }

        assert(false);
        return "attribute_id";
    }
    QString FieldTypeToAttributesUpdateValueField(SdltmFieldMetaType fieldType) {
        switch (fieldType) {
        case SdltmFieldMetaType::Int: assert(false); break;
        case SdltmFieldMetaType::Double: assert(false); break;
        case SdltmFieldMetaType::Text:
        case SdltmFieldMetaType::MultiText:
        case SdltmFieldMetaType::Number:
        case SdltmFieldMetaType::DateTime:
            return "value";
        case SdltmFieldMetaType::List:
        case SdltmFieldMetaType::CheckboxList:
            return "picklist_value_id";
        default:;
        }

        assert(false);
        return "attribute_id";
    }

    // OBSOLETE:
    // now we first delete old value, and then insert new one
    // rationale: we might not have an old value (thus, an UPDATE would be a no-op)
    bool TryRunUpdateSourceChangeFieldSql(const QString& selectSql, const QString& updateSql, const FindAndReplaceFieldInfo& info, DBBrowserDB& db, int& error, QString& errorMsg) {
        auto fieldType = info.EditField.FieldType;
        auto attributeId = info.EditField.ID;
        auto field = FieldTypeToAttributesField(fieldType);
        QString wherePrefix = "";
        if (fieldType != SdltmFieldMetaType::List && fieldType != SdltmFieldMetaType::CheckboxList)
            wherePrefix = field + " = " + QString::number(attributeId) + " AND ";
        else if (fieldType == SdltmFieldMetaType::List) {
            wherePrefix = field + " = " + QString::number(info.OldComboId()) + " AND ";
        }
        auto sql = "UPDATE " + FieldTypeToAttributesTable(fieldType) + " SET " + updateSql
            + " WHERE  " + wherePrefix + " translation_unit_id in (SELECT id FROM ( " + selectSql + " ))";

        auto forceWait = true;
        auto pDb = db.get("run sql update change field", forceWait);

        bool ok = false;
        sqlite3_stmt* stmt;
        int status = sqlite3_prepare_v2(pDb.get(), sql.toStdString().c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
        if (status == SQLITE_OK) {
            ok = sqlite3_step(stmt) == SQLITE_DONE;
            sqlite3_finalize(stmt);
        }

        if (!ok) {
            error = sqlite3_errcode(pDb.get());
            errorMsg = sqlite3_errmsg(pDb.get());
        }

        SdltmLog("update sql=" + sql);
        return ok;
    }

    typedef DBBrowserDB::db_pointer_type db_pointer;

    bool RunExecuteQuery(const QString& sql, sqlite3* pDb, int& error, QString& errorMsg) {
        sqlite3_stmt* stmt;
		auto utf8 = sql.toUtf8();
		const char* data = utf8.constData();
        int status = sqlite3_prepare_v2(pDb, data, static_cast<int>(utf8.size()), &stmt, nullptr);
        bool ok = false;
        if (status == SQLITE_OK) {
            ok = sqlite3_step(stmt) == SQLITE_DONE;
            sqlite3_finalize(stmt);
        }

		error = 0;
        if (!ok) {
            error = sqlite3_errcode(pDb);
            errorMsg = sqlite3_errmsg(pDb);
        }
        return ok;
    }

    QString ListToSqlString(const std::vector<int>& ids) {
        assert(ids.size() > 0);
        QString sql;
        for (const auto& id : ids) {
            if (sql != "")
                sql += ",";
            sql += QString::number(id);
        }

        return "(" + sql + ")";
    }

    QString StringValueToSql(SdltmFieldMetaType fieldType, const QString& text, const QDateTime& date, int comboValue) {
        switch (fieldType) {
        case SdltmFieldMetaType::Int: assert(false); break;
        case SdltmFieldMetaType::Double: assert(false); break;
        case SdltmFieldMetaType::Text:
        case SdltmFieldMetaType::MultiText:
            return "'" + EscapeXmlAndSql(text) + "'";
        case SdltmFieldMetaType::Number:
            return text;

        case SdltmFieldMetaType::List:
            return QString::number(comboValue);
        case SdltmFieldMetaType::CheckboxList: break;

        case SdltmFieldMetaType::DateTime:
            return "datetime('" + date.toString(Qt::ISODate) + "')";
        default:;
        }
        assert(false);
        return "";
    }






    QString GetAttributeTableName(SdltmFieldMetaType fieldType) {
	    switch (fieldType) {
        case SdltmFieldMetaType::Int: assert(false);  break;
	    case SdltmFieldMetaType::Double:assert(false); break;

        case SdltmFieldMetaType::MultiText: 
        case SdltmFieldMetaType::Text: return "string_attributes";
        case SdltmFieldMetaType::Number: return "numeric_attributes";
        case SdltmFieldMetaType::CheckboxList: 
        case SdltmFieldMetaType::List: return "picklist_attributes";
        case SdltmFieldMetaType::DateTime: return "date_attributes";
	    default: ;
	    }
        return "";
    }

    bool DeleteAttributeQuery(const CustomField& info, const std::vector<int>& ids, sqlite3* pDb, int& error, QString& errorMsg) {
        auto tableName = GetAttributeTableName(info.FieldType);
        auto attributeId = FieldTypeToAttributesField(info.FieldType);
        QString sql;
        if (info.FieldType != SdltmFieldMetaType::List)
            sql = "DELETE FROM " + tableName + " WHERE translation_unit_id in " + ListToSqlString(ids) + " and attribute_id = " + QString::number(info.ID);
        else
            // list
            sql = "DELETE FROM " + tableName + " WHERE translation_unit_id in " + ListToSqlString(ids) + " and picklist_value_id in " + ListToSqlString(info.ValueToID);
        return RunExecuteQuery(sql, pDb, error, errorMsg);
    }


    QString InsertAttributeQuery(const FindAndReplaceFieldInfo& info, int translationUnitId, sqlite3* pDb, int& error, QString& errorMsg) {
        auto tableName = GetAttributeTableName(info.EditField.FieldType);
        QString fieldValue = StringValueToSql(info.EditField.FieldType, info.NewText, info.NewDate, info.NewComboId());
        if (info.EditField.FieldType == SdltmFieldMetaType::List)
			return "INSERT INTO " + tableName + "(translation_unit_id,picklist_value_id) VALUES(" + QString::number(translationUnitId) + "," + fieldValue + ");";

        QString sql = "INSERT INTO " + tableName + "(translation_unit_id,attribute_id,value) VALUES(" + QString::number(translationUnitId) + "," + QString::number(info.EditField.ID) + "," + fieldValue + ");";
        return sql;
    }

    bool TryRunUpdateSourceChangeFieldAttributeSql(const QString& selectSql, const FindAndReplaceFieldInfo& info, DBBrowserDB& db, int& error, QString& errorMsg) {
        const int BLOCK_SIZE = 128;
        auto ids = RunQueryGetIDs(selectSql, db);

        auto forceWait = true;
        auto pDbScopedPtr = db.get("change field value - attribute", forceWait);
        sqlite3* pDb = pDbScopedPtr.get();

        if (!RunExecuteQuery("BEGIN TRANSACTION", pDb, error, errorMsg))
            return false;

        // first, delete old entries
        for (int i = 0; i < ids.size(); i += BLOCK_SIZE) {
            auto count = i + BLOCK_SIZE <= ids.size() ? BLOCK_SIZE : ids.size() - i;
            std::vector<int> subIds;
            std::copy(ids.begin() + i, ids.begin() + i + count, std::back_inserter(subIds));
            if (!DeleteAttributeQuery(info.EditField, subIds, pDb, error, errorMsg))
                return false;
        }
        // now, reinsert, one by one
        for (int i = 0; i < ids.size(); i ++) {
            auto sql = InsertAttributeQuery(info, ids[i], pDb, error, errorMsg);
            if (!RunExecuteQuery(sql, pDb, error, errorMsg))
                return false;
        }

        if (!RunExecuteQuery("END TRANSACTION", pDb, error, errorMsg))
            return false;
        return true;
    }

    bool TryRunUpdateSourceDelFieldAttributeSql(const QString& selectSql, const CustomField& info, DBBrowserDB& db, int& error, QString& errorMsg) {
        const int BLOCK_SIZE = 128;
        auto ids = RunQueryGetIDs(selectSql, db);

        auto forceWait = true;
        auto pDbScopedPtr = db.get("del field value - attribute", forceWait);
        sqlite3* pDb = pDbScopedPtr.get();

        if (!RunExecuteQuery("BEGIN TRANSACTION", pDb, error, errorMsg))
            return false;

        // first, delete old entries
        for (int i = 0; i < ids.size(); i += BLOCK_SIZE) {
            auto count = i + BLOCK_SIZE <= ids.size() ? BLOCK_SIZE : ids.size() - i;
            std::vector<int> subIds;
            std::copy(ids.begin() + i, ids.begin() + i + count, std::back_inserter(subIds));
            if (!DeleteAttributeQuery(info, subIds, pDb, error, errorMsg))
                return false;
        }

        if (!RunExecuteQuery("END TRANSACTION", pDb, error, errorMsg))
            return false;
        return true;
    }














    bool DeleteMultiTextQuery(const CustomField& info, const std::vector<int>& ids, sqlite3* pDb, int& error, QString& errorMsg) {
        auto sql = "DELETE FROM string_attributes WHERE translation_unit_id in " + ListToSqlString(ids) + " and attribute_id = " + QString::number(info.ID);
        return RunExecuteQuery(sql, pDb, error, errorMsg);
    }


    std::vector< QString> InsertMultiTextQuery(const FindAndReplaceFieldInfo& info, int translationUnitId, sqlite3* pDb, int& error, QString& errorMsg) {
        std::vector< QString> sqls;
        for (const auto& text : info.NewMultiText) {
            sqls.push_back("INSERT INTO string_attributes(translation_unit_id,attribute_id,value) VALUES(" + QString::number(translationUnitId) + "," + QString::number(info.EditField.ID) + ",'" + EscapeXmlAndSql(text) + "');");
        }
        return sqls;
    }

    bool TryRunUpdateSourceChangeFieldMultiTextSql(const QString& selectSql, const FindAndReplaceFieldInfo& info, DBBrowserDB& db, int& error, QString& errorMsg) {
        const int BLOCK_SIZE = 128;
        auto ids = RunQueryGetIDs(selectSql, db);

        auto forceWait = true;
        auto pDbScopedPtr = db.get("change field value - multitext", forceWait);
        sqlite3* pDb = pDbScopedPtr.get();

        if (!RunExecuteQuery("BEGIN TRANSACTION", pDb, error, errorMsg))
            return false;

        // first, delete old entries
        for (int i = 0; i < ids.size(); i += BLOCK_SIZE) {
            auto count = i + BLOCK_SIZE <= ids.size() ? BLOCK_SIZE : ids.size() - i;
            std::vector<int> subIds;
            std::copy(ids.begin() + i, ids.begin() + i + count, std::back_inserter(subIds));
            if (!DeleteMultiTextQuery(info.EditField, subIds, pDb, error, errorMsg))
                return false;
        }
        // now, reinsert, one by one
        for (int i = 0; i < ids.size(); i ++) {
            auto sqls = InsertMultiTextQuery(info, ids[i], pDb, error, errorMsg);
            for (const auto& sql : sqls)
                if (!RunExecuteQuery(sql, pDb, error, errorMsg))
                    return false;
        }

        if (!RunExecuteQuery("END TRANSACTION", pDb, error, errorMsg))
            return false;
        return true;
    }

    bool TryRunUpdateSourceDelFieldMultiTextSql(const QString& selectSql, const CustomField& info, DBBrowserDB& db, int& error, QString& errorMsg) {
        const int BLOCK_SIZE = 128;
        auto ids = RunQueryGetIDs(selectSql, db);

        auto forceWait = true;
        auto pDbScopedPtr = db.get("change field value - multitext", forceWait);
        sqlite3* pDb = pDbScopedPtr.get();

        if (!RunExecuteQuery("BEGIN TRANSACTION", pDb, error, errorMsg))
            return false;

        // first, delete old entries
        for (int i = 0; i < ids.size(); i += BLOCK_SIZE) {
            auto count = i + BLOCK_SIZE <= ids.size() ? BLOCK_SIZE : ids.size() - i;
            std::vector<int> subIds;
            std::copy(ids.begin() + i, ids.begin() + i + count, std::back_inserter(subIds));
            if (!DeleteMultiTextQuery(info, subIds, pDb, error, errorMsg))
                return false;
        }

        if (!RunExecuteQuery("END TRANSACTION", pDb, error, errorMsg))
            return false;
        return true;
    }


    bool DeleteCheckListQuery(const CustomField& info, const std::vector<int>& ids, sqlite3* pDb, int& error, QString& errorMsg) {
        auto sql = "DELETE FROM picklist_attributes WHERE translation_unit_id in " + ListToSqlString(ids) + " and picklist_value_id in " + ListToSqlString(info.ValueToID);
        return RunExecuteQuery(sql, pDb, error, errorMsg);
    }


    std::vector< QString> InsertCheckListQuery(const FindAndReplaceFieldInfo& info, int translationUnitId, sqlite3* pDb, int& error, QString& errorMsg) {
        std::vector< QString> sqls;
        auto attributeIds = info.NewComboIds();
        for (const auto& attributeId : attributeIds) {
            sqls.push_back("INSERT INTO picklist_attributes(translation_unit_id,picklist_value_id) VALUES(" + QString::number(translationUnitId) + "," + QString::number(attributeId) + ");");
        }
        return sqls;
    }

    bool TryRunUpdateSourceChangeFieldCheckListSql(const QString& selectSql, const FindAndReplaceFieldInfo& info, DBBrowserDB& db, int& error, QString& errorMsg) {
        const int BLOCK_SIZE = 128;
        auto ids = RunQueryGetIDs(selectSql, db);

        auto forceWait = true;
        auto pDbScopedPtr = db.get("change field value - checklist", forceWait);
        sqlite3* pDb = pDbScopedPtr.get();

        if (!RunExecuteQuery("BEGIN TRANSACTION", pDb, error, errorMsg))
            return false;

        // first, delete old entries
        for (int i = 0; i < ids.size(); i += BLOCK_SIZE) {
            auto count = i + BLOCK_SIZE <= ids.size() ? BLOCK_SIZE : ids.size() - i;
            std::vector<int> subIds;
            std::copy(ids.begin() + i, ids.begin() + i + count, std::back_inserter(subIds));
            if (!DeleteCheckListQuery(info.EditField, subIds, pDb, error, errorMsg))
                return false;
        }
        // now, reinsert, one by one
        for (int i = 0; i < ids.size(); i ++) {
            auto sqls = InsertCheckListQuery(info, ids[i], pDb, error, errorMsg);
            for (const auto& sql : sqls)
                if (!RunExecuteQuery(sql, pDb, error, errorMsg))
                    return false;
        }

        if (!RunExecuteQuery("END TRANSACTION", pDb, error, errorMsg))
            return false;

        return true;
    }
    bool TryRunUpdateSourceDelFieldCheckListSql(const QString& selectSql, const CustomField& info, DBBrowserDB& db, int& error, QString& errorMsg) {
        const int BLOCK_SIZE = 128;
        auto ids = RunQueryGetIDs(selectSql, db);

        auto forceWait = true;
        auto pDbScopedPtr = db.get("del field value - checklist", forceWait);
        sqlite3* pDb = pDbScopedPtr.get();

        if (!RunExecuteQuery("BEGIN TRANSACTION", pDb, error, errorMsg))
            return false;

        // first, delete old entries
        for (int i = 0; i < ids.size(); i += BLOCK_SIZE) {
            auto count = i + BLOCK_SIZE <= ids.size() ? BLOCK_SIZE : ids.size() - i;
            std::vector<int> subIds;
            std::copy(ids.begin() + i, ids.begin() + i + count, std::back_inserter(subIds));
            if (!DeleteCheckListQuery(info, subIds, pDb, error, errorMsg))
                return false;
        }

        if (!RunExecuteQuery("END TRANSACTION", pDb, error, errorMsg))
            return false;

        return true;
    }

}




bool TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const FindAndReplaceTextInfo& info, DBBrowserDB& db, int& replaceCount, int& error, QString& errorMsg) {
    if (info.Find == "")
        return false;

    SdltmFilter findAndReplace = filter;
    for (auto& item : findAndReplace.FilterItems)
        item.IndentLevel += 2;

    // the idea: I want to AND them, the find-and-replace to the existing filter
    //           but, for "both", I actually have an OR
    SdltmFilterItem dummy(SdltmFieldType::CustomSqlExpression);
    dummy.IndentLevel = 0;
    dummy.FieldValue = "1=1";
    dummy.IsAnd = true;
    findAndReplace.FilterItems.push_back(dummy);
    auto searchSource = info.Type == FindAndReplaceTextInfo::SearchType::Source || info.Type == FindAndReplaceTextInfo::SearchType::Both;
    auto searchTarget = info.Type == FindAndReplaceTextInfo::SearchType::Target || info.Type == FindAndReplaceTextInfo::SearchType::Both;
    if (searchSource) {
        SdltmFilterItem source(SdltmFieldType::SourceSegment);
        source.IndentLevel = 1;
        source.IsAnd = false;
        source.StringComparison = StringComparisonType::Contains;
        source.FieldValue = info.Find;
        source.CaseSensitive = info.MatchCase;
        findAndReplace.FilterItems.push_back(source);
    }

    if (searchTarget) {
        SdltmFilterItem target(SdltmFieldType::TargetSegment);
        target.IndentLevel = 1;
        target.IsAnd = false;
        target.StringComparison = StringComparisonType::Contains;
        target.FieldValue = info.Find;
        target.CaseSensitive = info.MatchCase;
        findAndReplace.FilterItems.push_back(target);
    }

    auto selectSql = SdltmCreateSqlSimpleFilter(findAndReplace, customFields).ToSqlFilter();
    SdltmLog("Find and replace (select): " + selectSql);
    replaceCount = RunQueryGetCount(selectSql, db);
    if (replaceCount == 0)
        return true;

    QString replaceSql;
    QString replaceSource, replaceTarget;
    // whole-word only is implemented with regex
    auto needsRegex = info.UseRegex || info.WholeWordOnly;
    if (needsRegex) {
        if (searchSource)
            replaceSource = "source_segment = sdltm_regex_replace(source_segment,'"
            + ToRegexFindString(info.Find, info.MatchCase, info.WholeWordOnly, info.UseRegex) + "', '" + EscapeXmlAndSql(info.Replace) + "') ";

        if (searchTarget)
            replaceTarget = "target_segment = sdltm_regex_replace(target_segment,'"
            + ToRegexFindString(info.Find, info.MatchCase, info.WholeWordOnly, info.UseRegex) + "', '" + EscapeXmlAndSql(info.Replace) + "') ";
    }
    else {
        if (searchSource)
            replaceSource = "source_segment = sdltm_replace(source_segment,'"
            + EscapeXmlAndSql(info.Find) + "', '" + EscapeXmlAndSql(info.Replace) + "'," + QString::number(info.MatchCase ? 1 : 0) + ") ";

        if (searchTarget)
            replaceTarget = "target_segment = sdltm_replace(target_segment,'"
            + EscapeXmlAndSql(info.Find) + "', '" + EscapeXmlAndSql(info.Replace) + "'," + QString::number(info.MatchCase ? 1 : 0) + ") ";
    }

    if (replaceSource != "" && replaceTarget != "")
        replaceSql = replaceSource + ", " + replaceTarget;
    else if (replaceSource != "")
        replaceSql = replaceSource;
    else
        replaceSql = replaceTarget;

    return TryRunUpdateSourceOrTargetTextSql(selectSql, replaceSql, db, error, errorMsg);
}

namespace {

}
bool TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields,
		const FindAndReplaceFieldInfo& info, DBBrowserDB& db, int& replaceCount, int& error, QString& errorMsg) {
    if (!info.EditField.IsPresent())
        return false;

    SdltmFilter findAndReplace = filter;
    for (auto& item : findAndReplace.FilterItems)
        item.IndentLevel += 1;

    if (info.HasOldValue()) {
        SdltmFilterItem dummy(info.EditField.FieldName, info.EditField.FieldType);
        dummy.IndentLevel = 0;
        dummy.IsAnd = true;
        dummy.CaseSensitive = true;
        dummy.MultiStringComparison = MultiStringComparisonType::Equals;
        dummy.ChecklistComparison = ChecklistComparisonType::Equals;
        if (info.EditField.FieldType != SdltmFieldMetaType::CheckboxList)
            dummy.FieldValue = info.OldValue();
        if (info.EditField.FieldType == SdltmFieldMetaType::MultiText)
            dummy.FieldValues = info.OldMultiText;
        else
            dummy.FieldValues = info.OldComboNames();
        findAndReplace.FilterItems.push_back(dummy);
    }

    auto selectSql = SdltmCreateSqlSimpleFilter(findAndReplace, customFields).ToSqlFilter();
    SdltmLog("Find and replace (select): " + selectSql);
    replaceCount = RunQueryGetCount(selectSql, db);
    if (replaceCount == 0)
        return true;

    switch (info.EditField.FieldType) {
    case SdltmFieldMetaType::MultiText:
        return TryRunUpdateSourceChangeFieldMultiTextSql(selectSql, info, db, error, errorMsg);

    case SdltmFieldMetaType::CheckboxList:
        return TryRunUpdateSourceChangeFieldCheckListSql(selectSql, info, db, error, errorMsg);

    case SdltmFieldMetaType::Int: assert(false); break;
    case SdltmFieldMetaType::Double: assert(false); break;

        // normal UPDATE handling
    case SdltmFieldMetaType::Text:
    case SdltmFieldMetaType::Number:
    case SdltmFieldMetaType::List:
    case SdltmFieldMetaType::DateTime:
        return TryRunUpdateSourceChangeFieldAttributeSql(selectSql, info, db, error, errorMsg);
    default:
        break;
    }
    assert(false);
    return false;
}

bool TryFindAndReplaceDeleteField(const SdltmFilter& filter, const std::vector<CustomField>& customFields,
		const CustomField& info, DBBrowserDB& db, int& replaceCount, int& error, QString& errorMsg) {
    if (!info.IsPresent())
        return false;

    auto selectSql = SdltmCreateSqlSimpleFilter(filter, customFields).ToSqlFilter();
    SdltmLog("Find and replace delete (select): " + selectSql);
    replaceCount = RunQueryGetCount(selectSql, db);
    if (replaceCount == 0)
        return true;

    switch (info.FieldType) {
    case SdltmFieldMetaType::MultiText:
        return TryRunUpdateSourceDelFieldAttributeSql(selectSql, info, db, error, errorMsg);

    case SdltmFieldMetaType::CheckboxList:
        return TryRunUpdateSourceDelFieldCheckListSql(selectSql, info, db, error, errorMsg);

    case SdltmFieldMetaType::Int: assert(false); break;
    case SdltmFieldMetaType::Double: assert(false); break;

        // normal UPDATE handling
    case SdltmFieldMetaType::Text:
    case SdltmFieldMetaType::Number:
    case SdltmFieldMetaType::List:
    case SdltmFieldMetaType::DateTime:
        return TryRunUpdateSourceDelFieldAttributeSql(selectSql, info, db, error, errorMsg);
    default:
        break;
    }
    assert(false);
    return false;
}

bool TryFindAndReplaceDeleteTags(const SdltmFilter& filter, const std::vector<CustomField>& customFields,
		DBBrowserDB& db, int& replaceCount, int& error, QString& errorMsg) {

    auto selectSql = SdltmCreateSqlSimpleFilter(filter, customFields).ToSqlFilter();
    SdltmLog("Find and replace delete tags (select): " + selectSql);
    replaceCount = RunQueryGetCount(selectSql, db);
    if (replaceCount == 0)
        return true;
    QString replaceSql = "source_segment = sdltm_delete_tags(source_segment), target_segment = sdltm_delete_tags(target_segment)";
    return TryRunUpdateSourceOrTargetTextSql(selectSql, replaceSql, db, error, errorMsg);
}

bool TryUpdateSource(DBBrowserDB& db, int translationUnitId, const QString& xml, int& error, QString& errorMsg) {
	auto forceWait = true;
	auto pDbScopedPtr = db.get("update source", forceWait);
	sqlite3* pDb = pDbScopedPtr.get();

	auto copy = xml;
	auto sql = "UPDATE translation_units set source_segment='" + copy.replace("'", "''") + "' where id=" + QString::number(translationUnitId);
	SdltmLog("update query= " + sql);
	return RunExecuteQuery(sql, pDb, error, errorMsg);
}

bool TryUpdateTarget(DBBrowserDB& db, int translationUnitId, const QString& xml, int& error, QString& errorMsg) {
	auto forceWait = true;
	auto pDbScopedPtr = db.get("update target", forceWait);
	sqlite3* pDb = pDbScopedPtr.get();

	auto copy = xml;
	auto sql = "UPDATE translation_units set target_segment='" + copy.replace("'", "''") + "' where id=" + QString::number(translationUnitId);
	SdltmLog("update query= " + sql);
	return RunExecuteQuery(sql, pDb, error, errorMsg);
}

