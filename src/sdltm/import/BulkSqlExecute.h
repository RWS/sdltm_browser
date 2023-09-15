#pragma once
#include <QString>
#include <sqlite3.h>

#include "sqlitedb.h"



class BulkSqlExecute
{
public:
	explicit BulkSqlExecute(DBBrowserDB& db, int sqlCount = 64 * 1024);
	~BulkSqlExecute();

	void AddSql(const QString& sql);

private:
	void BeginTransaction();
	void CommitTransaction();
private:
	DBBrowserDB& _db;
	int _cacheSqlCount;
	int _sqlIndex = 0;
	bool _isTransactionInProcess = false;

	DBBrowserDB::db_pointer_type _dbLock ;

	int _error = 0;
	QString _errorMsg;
};

