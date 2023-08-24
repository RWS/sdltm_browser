#pragma once
#include <vector>

// IMPORTANT: SQL UPDATE/DELETE functions are isolated in this file

#include "BatchEdit.h"
#include "CustomFieldService.h"

struct SdltmFilter;
class DBBrowserDB;
class QString;
std::vector<int> RunQueryGetIDs(const QString& sql, DBBrowserDB& db);
int RunQueryGetCount(const QString& sql, DBBrowserDB& db);

bool TryDelete(const SdltmFilter& filter, const std::vector<CustomField>& customFields, DBBrowserDB& db, QString & resultSql, int& error, QString& errorMsg);

bool TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const FindAndReplaceTextInfo& info, DBBrowserDB& db, int& replaceCount, QString& resultSql, int& error, QString& errorMsg);
bool TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const FindAndReplaceFieldInfo& info, DBBrowserDB& db, int& replaceCount, QString& resultSql, int& error, QString& errorMsg);

bool TryFindAndReplaceDeleteField(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const CustomField& info, DBBrowserDB& db, int& replaceCount, QString& resultSql, int& error, QString& errorMsg);
bool TryFindAndReplaceDeleteTags(const SdltmFilter& filter, const std::vector<CustomField>& customFields, DBBrowserDB& db, int& replaceCount, QString& resultSql, int& error, QString& errorMsg);

bool TryUpdateSource(DBBrowserDB& db, int translationUnitId, const QString& xml, QString& resultSql, int& error, QString& errorMsg);
bool TryUpdateTarget(DBBrowserDB& db, int translationUnitId, const QString& xml, QString& resultSql, int& error, QString& errorMsg);

bool TryUpdateCustomField(DBBrowserDB& db, int translationUnitId, const CustomFieldValue& oldValue, const CustomFieldValue& newValue, int& error, QString& errorMsg);

std::vector<CustomFieldValue> GetCustomFieldValues(DBBrowserDB& db, const std::vector<CustomField>& customFields, int translationUnitId, int& error, QString& errorMsg);