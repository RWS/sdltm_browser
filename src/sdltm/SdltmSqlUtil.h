#pragma once
#include <vector>

#include "BatchEdit.h"
#include "CustomFieldService.h"

struct SdltmFilter;
class DBBrowserDB;
class QString;
std::vector<int> RunQueryGetIDs(const QString& sql, DBBrowserDB& db);
int RunQueryGetCount(const QString& sql, DBBrowserDB& db);

bool TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const FindAndReplaceTextInfo& info, DBBrowserDB& db, int& replaceCount, int& error, QString& errorMsg);
bool TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const FindAndReplaceFieldInfo& info, DBBrowserDB& db, int& replaceCount, int& error, QString& errorMsg);

