#pragma once
#include <QString>
#include <vector>

#include "CustomFieldService.h"
#include "SdltmFilter.h"


struct FindAndReplaceInfo;
class DBBrowserDB;
void DebugWriteLine(const QString& s);

std::vector<SdltmFilter> LoadFilters(const QString& fileName);
void SaveFilters(const std::vector<SdltmFilter>& filters, const QString& file);
QString UserSettingsFile();

QString FiltersFile();
QString DefaultFiltersFile();
QString AppRoamingDir();
QString AppExecutableDir();

std::vector<int> RunQueryGetIDs(const QString& sql, DBBrowserDB &db);
int RunQueryGetCount(const QString& sql, DBBrowserDB& db);
bool TryRunUpdateSql(const QString& selectSql, const QString& updateSql, DBBrowserDB& db, int&error, QString& errorMsg);

bool TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const FindAndReplaceInfo& info, DBBrowserDB& db, int& replaceCount, int& error, QString& errorMsg);

void LoadSqliteRegexExtensions(DBBrowserDB& db);