#pragma once
#include <QDateTime>
#include <QString>
#include <vector>

struct FindAndReplaceTextInfo;
struct FindAndReplaceFieldInfo;
struct SdltmFilter;
struct CustomField;
class DBBrowserDB;
class QString;

class SdltmDbUpdate
{
public:
	SdltmDbUpdate();
	bool TryDelete(const SdltmFilter& filter, const std::vector<CustomField>& customFields, DBBrowserDB& db);

	bool TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const FindAndReplaceTextInfo& info, DBBrowserDB& db, int& replaceCount);
	bool TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const FindAndReplaceFieldInfo& info, DBBrowserDB& db, int& replaceCount);

	bool TryFindAndReplaceDeleteField(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const CustomField& info, DBBrowserDB& db, int& replaceCount);
	bool TryFindAndReplaceDeleteTags(const SdltmFilter& filter, const std::vector<CustomField>& customFields, DBBrowserDB& db, int& replaceCount);

	bool TryUpdateSource(DBBrowserDB& db, int translationUnitId, const QString& xml);
	bool TryUpdateTarget(DBBrowserDB& db, int translationUnitId, const QString& xml);

	int Error() const { return _error; }
	const QString& ErrorMsg() const { return _errorMsg; }
	QDateTime LastSuccessfulUpdate() const { return _lastSuccessfulUpdate; }
	const QString& ResultSql() const { return _resultSql; }
private:
	QString _resultSql;

	int _error;
	QString _errorMsg;

	QDateTime _lastSuccessfulUpdate;
};

