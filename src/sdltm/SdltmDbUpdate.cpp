#include "SdltmDbUpdate.h"

#include "SdltmSqlUtil.h"

SdltmDbUpdate::SdltmDbUpdate() {
}

bool SdltmDbUpdate::TryDelete(const SdltmFilter& filter, const std::vector<CustomField>& customFields, DBBrowserDB& db) {
	auto ok = ::TryDelete(filter, customFields, db, _error, _errorMsg);
	if (ok)
		_lastSuccessfulUpdate = QDateTime::currentDateTime();
	return ok;
}

bool SdltmDbUpdate::TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const FindAndReplaceTextInfo& info, DBBrowserDB& db, int& replaceCount) {
	auto ok = :: TryFindAndReplace(filter, customFields, info, db, replaceCount, _error, _errorMsg);
	if (ok)
		_lastSuccessfulUpdate = QDateTime::currentDateTime();
	return ok;
}

bool SdltmDbUpdate::TryFindAndReplace(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const FindAndReplaceFieldInfo& info, DBBrowserDB& db, int& replaceCount) {
	auto ok = ::TryFindAndReplace(filter, customFields, info, db, replaceCount, _error, _errorMsg);
	if (ok)
		_lastSuccessfulUpdate = QDateTime::currentDateTime();
	return ok;
}

bool SdltmDbUpdate::TryFindAndReplaceDeleteField(const SdltmFilter& filter, const std::vector<CustomField>& customFields, const CustomField& info, DBBrowserDB& db, int& replaceCount) {
	auto ok = ::TryFindAndReplaceDeleteField(filter, customFields, info, db, replaceCount, _error,_errorMsg);
	if (ok)
		_lastSuccessfulUpdate = QDateTime::currentDateTime();
	return ok;
}

bool SdltmDbUpdate::TryFindAndReplaceDeleteTags(const SdltmFilter& filter, const std::vector<CustomField>& customFields, DBBrowserDB& db, int& replaceCount) {
	auto ok = ::TryFindAndReplaceDeleteTags(filter, customFields, db, replaceCount, _error, _errorMsg);
	if (ok)
		_lastSuccessfulUpdate = QDateTime::currentDateTime();
	return ok;
}

bool SdltmDbUpdate::TryUpdateSource(DBBrowserDB& db, int translationUnitId, const QString& xml) {
	auto ok = ::TryUpdateSource(db, translationUnitId, xml, _error, _errorMsg);
	if (ok)
		_lastSuccessfulUpdate = QDateTime::currentDateTime();
	return ok;
}

bool SdltmDbUpdate::TryUpdateTarget(DBBrowserDB& db, int translationUnitId, const QString& xml) {
	auto ok = ::TryUpdateTarget(db, translationUnitId, xml, _error, _errorMsg);
	if (ok)
		_lastSuccessfulUpdate = QDateTime::currentDateTime();
	return ok;
}
