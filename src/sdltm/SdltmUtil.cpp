#include "SdltmUtil.h"

#include <fstream>
#include <sqlite3.h>

#include "BatchEdit.h"
#include "SdltmCreateSqlSimpleFilter.h"
#include "sqlitedb.h"
#include "ui_PlotDock.h"

namespace {
    std::unique_ptr<QFile> _log;
}

void SdltmLog(const QString& s)
{
    //auto str = (s + "\r\n").toStdString();
	auto str = (s + "\r").toStdString();
	OutputDebugString(str.c_str());
    if (_log) {
        _log->write(str.c_str());
        _log->flush();
    }
}

void InitLog() {
    auto dir = AppRoamingDir();
    auto logFile = dir + "\\trados_fusion.log";
    auto suffixIdx = 20;
    QFile::remove(logFile + QString::number(suffixIdx + 1));
    for (int i = suffixIdx; i >= 0; --i) {
        auto curName = logFile + (i > 0 ? "." + QString::number(i) : "");
        auto renameAs = logFile + "." + QString::number(i + 1);
        if (QFile::exists(curName))
            QFile::rename(curName, renameAs);
    }
    _log = std::make_unique<QFile>(logFile);
    _log->open(QIODevice::ReadWrite | QIODevice::Text);
    SdltmLog("Application started");
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
        item.WholeWordOnly= json["WholeWordOnly"].toBool();
        item.UseRegex = json["UseRegex"].toBool();
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

        filter.QuickSearchCaseSensitive = json["QuickSearchCaseSensitive"].toBool();
        filter.QuickSearchWholeWordOnly = json["QuickSearchWholeWordOnly"].toBool();
        filter.QuickSearchUseRegex = json["QuickSearchUseRegex"].toBool();


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
        json["WholeWordOnly"] = item.WholeWordOnly;
        json["UseRegex"] = item.UseRegex;
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

        json["QuickSearchCaseSensitive"] = filter.QuickSearchCaseSensitive;
        json["QuickSearchWholeWordOnly"] = filter.QuickSearchWholeWordOnly;
        json["QuickSearchUseRegex"] = filter.QuickSearchUseRegex;

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
    SdltmLog("loading filters from " + fileName);

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
    SdltmLog("saving filters to " + fileName);
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
    QString GetSdltmRegexReplacedText(const QString& inputTextXml, const QString& regexSearch, const QString& replace) {
        auto findResult = GetNonXmlTextDetails(inputTextXml);
        QString outputText;

        for (int i = 0; i < findResult.size(); ++i) {
            auto prevEndIdx = i > 0 ? findResult[i - 1].FindPosition + findResult[i - 1].FindLength : 0;
            outputText += inputTextXml.mid(prevEndIdx, findResult[i].FindPosition - prevEndIdx);
            outputText += findResult[i].FindText.replace(QRegularExpression(regexSearch), replace);
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

// 1 arg - the source (xml) text
void SdltmDeleteTags(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]) {
    auto inputText = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[0])));
    bool addTags = false;
    auto outputText = GetNonXmlText(inputText, " ", addTags);
    // note: we're preserving the language
    auto idxStart = inputText.indexOf("<Elements>");
    auto idxEnd = inputText.lastIndexOf("</Elements>");
    if (idxStart >= 0)
        outputText = inputText.left(idxStart) + "<Elements><Text><Value>" + outputText;
    if (idxEnd >= 0)
        outputText += "</Value></Text>" + inputText.right(inputText.size() - idxEnd);

    auto asUtf8 = outputText.toUtf8();
    sqlite3_result_text(ctx, asUtf8.begin(), asUtf8.size(), SQLITE_TRANSIENT);
}


// 4 args
// - the source (xml) text,
// - what to search
// - what to replace with
// - case sensitive
void SdltmReplaceText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]) {
    auto inputText = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[0])));
    auto search = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[1])));
    auto replace = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[2])));
	auto caseSensitive = sqlite3_value_int(arguments[3]) != 0;

    auto outputText = GetSdltmReplacedText(inputText, search, replace, caseSensitive);
    auto asUtf8 = outputText.toUtf8();
    sqlite3_result_text(ctx, asUtf8.begin(), asUtf8.size(), SQLITE_TRANSIENT);
}

// 3 args
// - the source (xml) text,
// - what to search
// - what to replace with
void SdltmRegexReplaceText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]) {
    auto inputText = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[0])));
    auto search = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[1])));
    auto replace = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_value_text(arguments[2])));

    auto outputText = GetSdltmRegexReplacedText(inputText, search, replace);
    auto asUtf8 = outputText.toUtf8();
    sqlite3_result_text(ctx, asUtf8.begin(), asUtf8.size(), SQLITE_TRANSIENT);
}


QString EscapeXml(const QString& str) {
    auto copy = str;
    copy.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;");
    return copy;
}

QString EscapeXmlAndSql(const QString& str) {
	auto copy = str;
	// extra besides EscapeXml - escape quote (') as double-quote
	copy.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;").replace("'", "''");
	return copy;
}

QString UnescapeXml(const QString& str) {
    auto copy = str;
    copy.replace("&gt;", ">").replace("&lt;", "<").replace("&amp;", "&").replace("&nbsp;", " ").replace("&quot;", "\"");
    return copy;
}

QString EscapeSqlString(const QString& s)
{
    QString copy = s;
    copy.replace("%", "%%");
    return copy;
}


QString ToRegexFindString(const QString& find, bool matchCase, bool wholeWord, bool useRegex) {
    auto text = EscapeXmlAndSql(find);
    if (useRegex)
        // this overrides case-sensive, whole-world
        return text;

    text.replace("\\", "\\\\");
    text.replace("(", "\\(");

    if (!matchCase) {
        auto words = text.split(QRegularExpression("\\s"));
        QString includeBothCases = "";
        for (const auto& word : words) {
            if (includeBothCases.size() > 0)
                includeBothCases += " ";
            if (word.size() > 0 && word[0].isLetter()) {
                QString lower = word[0].toLower();
                QString higher = word[0].toUpper();
                includeBothCases += "(" + lower + "|" + higher + ")" + word.right(word.size() - 1);
            }
            else
                includeBothCases += word;
        }
        text = includeBothCases;
    }

    if (wholeWord)
        text = "\\b" + text + "\\b";

    return text;
}

QString SdmtmXmlToFriendlyText(const QString& inputText) {
	auto addTags = true;
	auto outputText = UnescapeXml(GetNonXmlText(inputText, " ", addTags));
	return outputText;
}


