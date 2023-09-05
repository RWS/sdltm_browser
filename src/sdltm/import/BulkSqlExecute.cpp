#include "BulkSqlExecute.h"

#include <cassert>
#include <QString>
#include <sqlite3.h>

#include "sqlitedb.h"

namespace {
	bool RunExecuteQuery(const QString& sql, sqlite3* pDb, int& error, QString& errorMsg) {
		sqlite3_stmt* stmt;
		auto utf8 = sql.toUtf8();
		const char* data = utf8.constData();
		int status = sqlite3_prepare_v2(pDb, data, static_cast<int>(utf8.size()), &stmt, nullptr);
		bool ok = false;
		if (status == SQLITE_OK) {
			ok = sqlite3_step(stmt) == SQLITE_DONE;
			sqlite3_finalize(stmt);
		}

		error = 0;
		if (!ok) {
			error = sqlite3_errcode(pDb);
			errorMsg = sqlite3_errmsg(pDb);
		}
		return ok;
	}

}

BulkSqlExecute::BulkSqlExecute(DBBrowserDB& db, int sqlCount)
	: _db(db), _cacheSqlCount(sqlCount) {
}

BulkSqlExecute::~BulkSqlExecute() {
	// commit last transaction, if any
	if (_isTransactionInProcess)
		CommitTransaction();
}

void BulkSqlExecute::AddSql(const QString& sql) {
	if (!_isTransactionInProcess)
		BeginTransaction();

	sqlite3* pDb = _dbLock.get();

	RunExecuteQuery(sql, pDb, _error, _errorMsg);

	if (++_sqlIndex >= _cacheSqlCount) {
		_sqlIndex = 0;
		CommitTransaction();
	}
}

void BulkSqlExecute::BeginTransaction() {
	if (_isTransactionInProcess) {
		assert(false);
		return;
	}
	_isTransactionInProcess = true;

	auto forceWait = true;
	_dbLock = _db.get("bulk execute", forceWait);
	sqlite3* pDb = _dbLock.get();

	RunExecuteQuery("BEGIN TRANSACTION", pDb, _error, _errorMsg);
}

void BulkSqlExecute::CommitTransaction() {

	_isTransactionInProcess = false;
	sqlite3* pDb = _dbLock.get();
	RunExecuteQuery("END TRANSACTION", pDb, _error, _errorMsg);
	_dbLock = nullptr;
}
