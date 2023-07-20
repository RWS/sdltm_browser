#pragma once
#include <QString>
#include <sqlite3.h>
#include <vector>

#include "CustomFieldService.h"
#include "SdltmFilter.h"


struct FindAndReplaceFieldInfo;
struct FindAndReplaceTextInfo;
class DBBrowserDB;
void SdltmLog(const QString& s);

std::vector<SdltmFilter> LoadFilters(const QString& fileName);
void SaveFilters(const std::vector<SdltmFilter>& filters, const QString& file);
QString UserSettingsFile();

QString FiltersFile();
QString DefaultFiltersFile();
QString AppRoamingDir();
QString AppExecutableDir();

void InitLog();

void LoadSqliteRegexExtensions(DBBrowserDB& db);

void SdltmGetText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]);
void SdltmGetFriendlyText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]);
void SdltmReplaceText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]);
void SdltmRegexReplaceText(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]);

void SdltmDeleteTags(sqlite3_context* ctx, int num_arguments, sqlite3_value* arguments[]);


QString EscapeXml(const QString& str);
QString UnescapeXml(const QString& str);

QString EscapeSqlString(const QString& s);

QString ToRegexFindString(const QString& find, bool matchCase, bool wholeWord, bool useRegex);