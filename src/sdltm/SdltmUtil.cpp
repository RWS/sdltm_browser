#include "SdltmUtil.h"

#include "ui_PlotDock.h"

void DebugWriteLine(const QString& s)
{
	OutputDebugString((s + "\r\n").toStdString().c_str());
}

std::vector<SdltmFilter> LoadFilters(const QString& file)
{
    std::vector<SdltmFilter> filters;
    DebugWriteLine("loading filters from " + file);

    if (file == DefaultFiltersFile())
    {
        // for testing
        SdltmFilter filter;
        filter.Name = "presety";
        filters.push_back(filter);
    }

    return filters;
}

void SaveFilters(const std::vector<SdltmFilter>& filters, const QString& file)
{
    DebugWriteLine("saving filters to " + file);
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
        return locations[0] + "/filter_settings.txt";
    return "";
}

// FIXME this need to be a resource
QString DefaultFiltersFile()
{
    auto locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    if (locations.count() > 0)
        return locations[0] + "/default_filter_settings.txt";
    return "";
}
