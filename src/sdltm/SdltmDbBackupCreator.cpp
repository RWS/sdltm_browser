#include "SdltmDbBackupCreator.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QThread>

#include "FuncThread.h"
#include "SdltmDbInfo.h"
#include "SdltmUtil.h"

namespace  {
	const QString INITIAL_BACKUP_NAME = "initial.sdltm";

}

SdltmDbBackupCreator::SdltmDbBackupCreator() {
}

void SdltmDbBackupCreator::CreateInitialBackup(const SdltmDbInfo& dbInfo) {
	// look at the database -> find out if "modified date" < last backup -> if so, rename last backup as "initial backup"
	auto initialBackup = dbInfo.BackupFolder + INITIAL_BACKUP_NAME;

	QDir dir(dbInfo.BackupFolder);
	QStringList filters;
	filters << "*.sdltm";
	auto backups = dir.entryInfoList(filters);
	auto lastBackup = std::max_element(backups.begin(), backups.end(), [](const QFileInfo& a, const QFileInfo& b) { return a.lastModified() < b.lastModified(); });
	bool alreadyHasInitialBackup = false;
	if (lastBackup != backups.end()) {
		QFileInfo original(dbInfo.DatabaseFile);
		if (lastBackup->lastModified() >= original.lastModified()) {
			// this backup is valid, no need for another backup at this time
			if (lastBackup->fileName() != INITIAL_BACKUP_NAME) {
				// rename this back as the "initial.backup"
				QFile::remove(initialBackup);
				QFile::rename(lastBackup->absoluteFilePath(), initialBackup);
			}
			alreadyHasInitialBackup = true;
		}
	}

	backups = dir.entryInfoList(filters);
	_existingBackups.clear();
	std::copy(backups.begin(), backups.end(), std::back_inserter(_existingBackups));
	auto initial = std::find_if(_existingBackups.begin(), _existingBackups.end(), [](const QFileInfo& f) { return f.fileName() == INITIAL_BACKUP_NAME; });
	if (initial != _existingBackups.end())
		_existingBackups.erase(initial);
	std::sort(_existingBackups.begin(), _existingBackups.end(), [](const auto& a, const auto& b) { return a.lastModified() < b.lastModified();  });

	// here, we need to do the backup
	if (!alreadyHasInitialBackup) {
		_lastBackup = QDateTime::currentDateTime();
		CreateBackup(dbInfo.DatabaseFile, initialBackup, dbInfo);
	}
}

void SdltmDbBackupCreator::CreateBackupNow(const SdltmDbInfo& dbInfo) {
	if (dbInfo.DatabaseFile == "")
		return;
	auto backupFile = dbInfo.BackupFolder + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".sdltm" ;
	CreateBackup(dbInfo.DatabaseFile, backupFile, dbInfo);
}

bool SdltmDbBackupCreator::IsBackingUpNow() const {
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	return !!_backupThread;
}

// in case there's a backup operation happening, waits for it to complete
// otherwise, it's a no-op
void SdltmDbBackupCreator::WaitForBackupToComplete() const {
	std::shared_ptr<FuncThread> t;
	// create the thread
	{ std::lock_guard<std::recursive_mutex> lock(_mutex);
	if (!_backupThread || !_backupRunning)
		return; // no backup running
	t = _backupThread;
	}
	t->wait();
}

void SdltmDbBackupCreator::OnAfterSuccessfulUpdate(const SdltmDbInfo& dbInfo) {
	if (dbInfo.BackupOnEachUpdate) {
		_lastBackup = QDateTime::currentDateTime();
		CreateBackupNow(dbInfo);
	}
}

void SdltmDbBackupCreator::OnLastSuccessfulUpdate(QDateTime lastUpdate, const SdltmDbInfo& dbInfo) {
	auto now = QDateTime::currentDateTime();
	if (_lastBackup < lastUpdate && (now.toSecsSinceEpoch() - lastUpdate.toSecsSinceEpoch()) >= dbInfo.BackupAfterSecondsOfInactivity ) {
		_lastBackup = now;
		CreateBackupNow(dbInfo);
	}
}

void SdltmDbBackupCreator::RestoreInitialDatabase(const SdltmDbInfo& dbInfo) {
	auto initial = dbInfo.BackupFolder + INITIAL_BACKUP_NAME;
	if (!QFile::exists(initial))
		// the idea -- if we don't have the initial file to restore, no point in going ahead with the process
		return;

	auto backup = dbInfo.DatabaseFile + ".bak";
	if (QFile::exists(backup))
		QFile::remove(backup);
	QFile::rename(dbInfo.DatabaseFile, backup);
	QFile::copy(initial, dbInfo.DatabaseFile);
}

void SdltmDbBackupCreator::CreateBackup(const QString& srcFile, const QString& destFile, const SdltmDbInfo& dbInfo) {
	WaitForBackupToComplete();
	// create the thread

	std::shared_ptr<FuncThread> f = std::make_shared<FuncThread>();
	f->Func = [this, srcFile, destFile, dbInfo]()
		{
			CreateBackupImpl(srcFile, destFile, dbInfo);
		};
	{ std::lock_guard<std::recursive_mutex> lock(_mutex);
	_backupThread = f;
	_backupRunning = true;
	}

	f->start();
}

void SdltmDbBackupCreator::CreateBackupImpl(const QString& srcFile, const QString& destFile, const SdltmDbInfo& dbInfo) {
	_lastBackup = QDateTime::currentDateTime();
	SdltmLog("backup started -> " + destFile);
	QFile::copy(srcFile, destFile);
	SdltmLog("backup complete -> " + destFile);

	QString toRemove;
	{ std::lock_guard<std::recursive_mutex> lock(_mutex);
	    _backupRunning = false;

		if (!destFile.endsWith(INITIAL_BACKUP_NAME)) {
			_existingBackups.push_back(destFile);
			if (_existingBackups.size() > dbInfo.KeepLastBackupCount) {
				toRemove = _existingBackups[0].absoluteFilePath();
				_existingBackups.erase(_existingBackups.begin());
			}
		}
	}

	if (toRemove != "")
		QFile::remove(toRemove);
}

