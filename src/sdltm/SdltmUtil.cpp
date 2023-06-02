#include "SdltmUtil.h"

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
        item.MultiComparisson = static_cast<MultiComparisonType>(json["MultiComparisson"].toInt());
        item.FieldValue = json["FieldValue"].toString();
        item.FieldValues = JsonToStringArray(json["FieldValues"].toArray());
        item.IndentLevel = json["IndentLevel"].toInt();
        item.IsAnd = json["IsAnd"].toBool();
        item.CaseSensitive = json["CaseSensitive"].toBool();
        item.IsVisible = json["IsVisible"].toBool();
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
        filter.IsLocked = json["IsLocked"].toBool();
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
        json["MultiComparisson"] = static_cast<int>(item.MultiComparisson);
        json["FieldValue"] = item.FieldValue;
        json["FieldValues"] = StringArrayToJson(item.FieldValues) ;
        json["IndentLevel"] = item.IndentLevel;
        json["IsAnd"] = item.IsAnd;
        json["CaseSensitive"] = item.CaseSensitive;
        json["IsVisible"] = item.IsVisible;
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
        json["IsLocked"] = filter.IsLocked;

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

// FIXME this need to be a resource
QString DefaultFiltersFile()
{
    auto locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    if (locations.count() > 0)
        return locations[0] + "/default_filter_settings.sdlfilters";
    return "";
}

QString AppDir()
{
    auto locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    if (locations.count() > 0)
        return locations[0];
    return "";
}
