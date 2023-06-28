#include "SdltmUtil.h"

#include <sqlite3.h>

#include "BatchEdit.h"
#include "SdltmCreateSqlSimpleFilter.h"
#include "sqlitedb.h"
#include "ui_PlotDock.h"

void DebugWriteLine(const QString& s)
{
	OutputDebugString((s + "\r\n").toStdString().c_str());
}

namespace 
{
    std::vector<QString> JsonToStringArray(const QJsonArray & json)
    {
        std::vector<QString> strings;
        for (int i = 0; i < json.count(); ++i)
            strings.push_back(json[i].toString());
        return strings;
    }
    SdltmFilterItem JsonToFilterItem(const QJsonObject & json)
    {
        SdltmFilterItem item;
        item.IsNegated = json["IsNegated"].toBool();
        item.FieldMetaType = static_cast<SdltmFieldMetaType>(json["FieldMetaType"].toInt());
        item.FieldType = static_cast<SdltmFieldType>(json["FieldType"].toInt());
        item.CustomFieldName = json["CustomFieldName"].toString();
        item.CustomValues = JsonToStringArray(json["CustomValues"].toArray());
        item.NumberComparison = static_cast<NumberComparisonType>(json["NumberComparison"].toInt());
        item.StringComparison = static_cast<StringComparisonType>(json["StringComparison"].toInt());
        item.MultiStringComparison = static_cast<MultiStringComparisonType>(json["MultiStringComparison"].toInt());
        item.MultiComparison = static_cast<MultiComparisonType>(json["MultiComparison"].toInt());
        item.ChecklistComparison = static_cast<ChecklistComparisonType>(json["ChecklistComparison"].toInt());
        item.FieldValue = json["FieldValue"].toString();
        item.FieldValues = JsonToStringArray(json["FieldValues"].toArray());
        item.IndentLevel = json["IndentLevel"].toInt();
        item.IsAnd = json["IsAnd"].toBool();
        item.CaseSensitive = json["CaseSensitive"].toBool();
        item.IsVisible = json["IsVisible"].toBool();
        item.IsUserEditableArg = json["IsUserEditableArg"].toBool();
        return item;
    }

	SdltmFilter JsonToFilter(const QJsonObject & json)
	{
        SdltmFilter filter;
        filter.Name = json["Name"].toString();
        filter.QuickSearch = json["QuickSearch"].toString();
        filter.QuickSearchTarget = json["QuickSearchTarget"].toString();
        filter.QuickSearchSearchSourceAndTarget = json["QuickSearchSearchSourceAndTarget"].toBool();
        filter.AdvancedSql = json["AdvancedSql"].toString();
        auto items = json["FilterItems"].toArray();
        for (int i = 0; i < items.count(); ++i)
            filter.FilterItems.push_back(JsonToFilterItem(items[i].toObject()));
        return filter;
	}

    QJsonArray StringArrayToJson(const std::vector<QString> & array)
    {
        QJsonArray json;
        for (const auto& s : array)
            json.push_back(s);
        return json;
    }

    QJsonObject FilterItemToJson(const SdltmFilterItem & item)
    {
        QJsonObject json;
        json["IsNegated"] = item.IsNegated;
        json["FieldMetaType"] = static_cast<int>(item.FieldMetaType);
        json["FieldType"] = static_cast<int>(item.FieldType);
        json["CustomFieldName"] = item.CustomFieldName;
        json["CustomValues"] = StringArrayToJson(item.CustomValues) ;
        json["NumberComparison"] = static_cast<int>(item.NumberComparison);
        json["StringComparison"] = static_cast<int>(item.StringComparison);
        json["MultiStringComparison"] = static_cast<int>(item.MultiStringComparison);
        json["MultiComparison"] = static_cast<int>(item.MultiComparison);
        json["ChecklistComparison"] = static_cast<int>(item.ChecklistComparison);
        json["FieldValue"] = item.FieldValue;
        json["FieldValues"] = StringArrayToJson(item.FieldValues) ;
        json["IndentLevel"] = item.IndentLevel;
        json["IsAnd"] = item.IsAnd;
        json["CaseSensitive"] = item.CaseSensitive;
        json["IsVisible"] = item.IsVisible;
        json["IsUserEditableArg"] = item.IsUserEditableArg;
        return json;
    }

    QJsonObject FilterToJson(const SdltmFilter & filter)
    {
        QJsonObject json;
        json["Name"] = filter.Name;
        json["QuickSearch"] = filter.QuickSearch;
        json["QuickSearchTarget"] = filter.QuickSearchTarget;
        json["QuickSearchSearchSourceAndTarget"] = filter.QuickSearchSearchSourceAndTarget;
        json["AdvancedSql"] = filter.AdvancedSql;

        QJsonArray items;
        for (const auto& item : filter.FilterItems)
            items.push_back(FilterItemToJson(item));
        json["FilterItems"] = items;
        return json;
    }
}

std::vector<SdltmFilter> LoadFilters(const QString& fileName)
{
    std::vector<SdltmFilter> filters;
    DebugWriteLine("loading filters from " + fileName);

    QFile file(fileName);
    if (file.exists() && file.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&file);

        auto json = QJsonDocument::fromJson(stream.readAll().toUtf8()).object();
        auto array = json.value("Filters").toArray();
        for (int i = 0; i < array.count(); ++i)
            filters.push_back( JsonToFilter(array[i].toObject()));
    }

    return filters;
}

void SaveFilters(const std::vector<SdltmFilter>& filters, const QString& fileName)
{
    DebugWriteLine("saving filters to " + fileName);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly))
    {
        QJsonArray jsonArray;
        for (const auto& filter : filters)
            jsonArray.push_back(FilterToJson(filter));
        QJsonObject json;
        json["Filters"] = jsonArray;
        QJsonDocument jsonDoc;
        jsonDoc.setObject(json);
        file.write(jsonDoc.toJson());
    }
}

QString UserSettingsFile()
{
    auto locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    if (locations.count() > 0)
        return locations[0] + "/trados_sdltm_settings.txt";
    return "";
}

QString FiltersFile()
{
    auto locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    if (locations.count() > 0)
        return locations[0] + "/filter_settings.sdlfilters";
    return "";
}

QString DefaultFiltersFile()
{
    auto locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    for (const auto &location: locations)
    {
        auto possible = location + "/default_filter_settings.sdlfilters";
        if (QFile::exists(possible))
            return possible;
    }
    // in this case, we don't have it -- the only time when this can happen is in Debug mode, since
    // I manually copy this file during installation process
    return "";
}

QString AppRoamingDir()
{
    auto locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    if (locations.count() > 0)
        return locations[0];
    return "";
}

QString AppExecutableDir() {
    auto locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    for (const auto& location : locations)
    {
        auto possible = location + "/TM Fusion for Trados.exe";
        if (QFile::exists(possible))
            return location;
    }
    // perhaps the .exe app name has changed
    assert(false);
    return "";

}


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

int RunQueryGetCount(const QString &sql, DBBrowserDB &db) {
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
bool TryRunUpdateSql(const QString & selectSql, const QString & updateSql, DBBrowserDB &db, int& error, QString& errorMsg) {
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

    DebugWriteLine("update sql=" + sql);
    return ok;
}

bool TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const FindAndReplaceInfo& info, DBBrowserDB& db, int& replaceCount, int& error, QString& errorMsg) {
    if (info.Find == "")
        return false;

    SdltmFilter findAndReplace = filter;
    for (auto& item : findAndReplace.FilterItems)
        item.IndentLevel += 2;

    // the idea: I want to AND the find-and-replace to the existing filter
    //           but, for "both", I actually have an OR
    SdltmFilterItem dummy(SdltmFieldType::CustomSqlExpression);
    dummy.IndentLevel = 0;
    dummy.FieldValue = "1=1";
    dummy.IsAnd = true;
    findAndReplace.FilterItems.push_back(dummy);
    auto searchSource = info.Type == FindAndReplaceInfo::SearchType::Source || info.Type == FindAndReplaceInfo::SearchType::Both;
    auto searchTarget = info.Type == FindAndReplaceInfo::SearchType::Target || info.Type == FindAndReplaceInfo::SearchType::Both;
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
    DebugWriteLine("Find and replace (select): " + selectSql);
    replaceCount = RunQueryGetCount(selectSql, db);
    if (replaceCount == 0) 
        return true;

    QString replaceSql ;
    QString replaceSource, replaceTarget;
    if (searchSource) 
        replaceSource = "source_segment = sdltm_replace(source_segment,'"
    	+ EscapeXml(info.Find) + "', '" + EscapeXml(info.Replace) + "'," + QString::number(info.MatchCase ? 1 : 0) + ") ";
    
    if (searchTarget) 
        replaceTarget = "target_segment = sdltm_replace(target_segment,'"
        + EscapeXml(info.Find) + "', '" + EscapeXml(info.Replace) + "'," + QString::number(info.MatchCase ? 1 : 0) + ") ";

    if (replaceSource != "" && replaceTarget != "")
        replaceSql = replaceSource + ", " + replaceTarget;
    else if (replaceSource != "")
        replaceSql = replaceSource;
    else
        replaceSql = replaceTarget;

    return TryRunUpdateSql(selectSql, replaceSql, db, error, errorMsg);
}

void LoadSqliteRegexExtensions(DBBrowserDB& db) {
    auto root = AppExecutableDir();
    db.loadExtension(root + "/sqlean.dll");
}

namespace {
    QString StartTag = QString::fromWCharArray(L"\u23E9");
    QString EndTag = QString::fromWCharArray(L"\u23EA");
    QString ConvertXmlToSimpleTags(const QString & txt) {
        QString simple;
        auto idxStart = 0;
        while(true) {
            auto idxNextStartTag = txt.indexOf("<Type>Start</Type>", idxStart);
            auto idxNextEndTag = txt.indexOf("<Type>End</Type>", idxStart);
            if (idxNextStartTag < 0 && idxNextEndTag < 0)
                break;
            if (idxNextStartTag < 0)
                idxNextStartTag = INT_MAX;
            if (idxNextEndTag < 0)
                idxNextEndTag = INT_MAX;
            auto minIsStart = idxNextStartTag < idxNextEndTag;
            simple += minIsStart ? StartTag : EndTag;
            idxStart = (minIsStart ? idxNextStartTag : idxNextEndTag) + 1;
        }
        return simple;
    }

    QString TrimRight(const QString& str) {
        int n = str.size() - 1;
        for (; n >= 0; --n) {
            if (!str.at(n).isSpace()) {
                return str.left(n + 1);
            }
        }
        return "";
    }

    void AppendTags(QString & str, const QString & tags) {
	    if (tags.isEmpty())
            return;
        str = TrimRight(str);
        str += tags + " ";
    }

    // each sub-string will be on a separate line, to avoid matching text from different sub-tags
	QString GetNonXmlText(const QString & inputText, const char * appendAfterSubtext, bool addTagChars) {
        QString outputText;
        // ... most of the string is xml anyway
        outputText.reserve(inputText.size() / 2);
        auto idxStart = 0;
        while (true) {
            auto idxNext = inputText.indexOf("<Text><Value>", idxStart);
            if (idxNext < 0)
                break;
            idxNext += 13; // 13 = len(<Text><Value>)
            auto idxEnd = inputText.indexOf('<', idxNext); 
            if (idxEnd < 0)
                break;
            if (!outputText.isEmpty())
                outputText += appendAfterSubtext;
            if (addTagChars) 
                AppendTags(outputText, ConvertXmlToSimpleTags(inputText.mid(idxStart, idxNext - idxStart))) ;

            outputText += inputText.mid(idxNext, idxEnd - idxNext);
            idxStart = idxEnd + 1;
        }

        if (addTagChars)
            AppendTags( outputText, ConvertXmlToSimpleTags(inputText.mid(idxStart)));

        return outputText;
	}

    struct SubFindInfo {
        int FindPosition;
        int FindLength;
        QString FindText;
	};
    std::vector<SubFindInfo> GetNonXmlTextDetails(const QString & inputText) {
        std::vector<SubFindInfo> result;
        auto idxStart = 0;
        while (true) {
            auto idxNext = inputText.indexOf("<Text><Value>", idxStart);
            if (idxNext < 0)
                break;
            idxNext += 13; // 13 = len(<Text><Value>)
            auto idxEnd = inputText.indexOf('<', idxNext);
            if (idxEnd < 0)
                break;

            result.push_back(SubFindInfo{ idxNext, idxEnd - idxNext, inputText.mid(idxNext, idxEnd - idxNext) });
            idxStart = idxEnd + 1;
        }
        return result;
    }

    QString GetSdltmReplacedText(const QString & inputTextXml, const QString & search, const QString & replace, bool caseSensitive) {
        auto findResult = GetNonXmlTextDetails(inputTextXml);
        QString outputText;

        for (int i = 0; i < findResult.size(); ++i) {
            auto prevEndIdx = i > 0 ? findResult[i - 1].FindPosition + findResult[i - 1].FindLength : 0;
            outputText += inputTextXml.mid(prevEndIdx, findResult[i].FindPosition - prevEndIdx);
            outputText += findResult[i].FindText.replace(search, replace, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
        }

        if (findResult.size() > 0) {
            outputText += inputTextXml.mid(findResult.back().FindPosition + findResult.back().FindLength);
        }

        return outputText;
    }
}

// 1 arg - the source (xml) text
void SdltmGetFriendlyText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]) {
    auto inputText = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[0])));
    bool addTags = true;
    auto outputText = UnescapeXml(GetNonXmlText(inputText, " ", addTags) );
    auto asUtf8 = outputText.toUtf8();
    sqlite3_result_text(ctx, asUtf8.begin(), asUtf8.size(), SQLITE_TRANSIENT);
}

// 1 arg - the source (xml) text
void SdltmGetText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]) {
    auto inputText = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[0])));
    bool addTags = false;
    auto outputText = GetNonXmlText(inputText, "\r\n", addTags);
    auto asUtf8 = outputText.toUtf8();
    sqlite3_result_text(ctx, asUtf8.begin(), asUtf8.size(), SQLITE_TRANSIENT);
}

// 3 args
// - the source (xml) text,
// - what to search
// - what to replace with
void SdltmReplaceText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]) {
    auto inputText = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[0])));
    auto search = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[1])));
    auto replace = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[2])));
    bool caseSensitive = true;
    if (num_arguments == 4)
		caseSensitive = sqlite3_value_int(arguments[3]) != 0;

    auto outputText = GetSdltmReplacedText(inputText, search, replace, caseSensitive);
    auto asUtf8 = outputText.toUtf8();
    sqlite3_result_text(ctx, asUtf8.begin(), asUtf8.size(), SQLITE_TRANSIENT);
}

QString EscapeXml(const QString& str) {
    auto copy = str;
    copy.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("'", "''");
    return copy;
}

QString UnescapeXml(const QString& str) {
    auto copy = str;
    copy.replace("&gt;", ">").replace("&lt;", "<").replace("&amp;", "&").replace("&nbsp;", " ");
    return copy;
}


