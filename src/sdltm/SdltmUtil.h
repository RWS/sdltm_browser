#pragma once
#include <QString>
#include <sqlite3.h>
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

void SdltmGetText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]);
void SdltmGetFriendlyText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]);
void SdltmReplaceText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]);
void SdltmRegexReplaceText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]);

QString EscapeXml(const QString& str);
QString UnescapeXml(const QString& str);

QString EscapeSqlString(const QString& s);

QString ToRegexFindString(const QString& find, bool matchCase, bool wholeWord, bool useRegex);