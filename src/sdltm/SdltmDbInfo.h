#pragma once
#include <QString>

struct SdltmDbInfo
{
	double SizeMB = 0;
	// if true, we're backing this database after each update (basically, only on small databases)
	bool BackupOnEachUpdate = false;

	int KeepLastBackupCount = 10;

	QString DatabaseFile;

	QString BackupFolder;

	// "inactivity" here means: no update operations
	// so for instance, if there's one or more updates, after 1 minute of "no updates", do a backup
	int BackupAfterSecondsOfInactivity = 60;

	static SdltmDbInfo FromDbFile(const QString& fileName);
};

