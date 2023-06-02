#pragma once
#include <QString>
#include <vector>

#include "SdltmFilter.h"


void DebugWriteLine(const QString& s);

std::vector<SdltmFilter> LoadFilters(const QString& fileName);
void SaveFilters(const std::vector<SdltmFilter>& filters, const QString& file);
QString UserSettingsFile();

QString FiltersFile();
QString DefaultFiltersFile();
QString AppDir();