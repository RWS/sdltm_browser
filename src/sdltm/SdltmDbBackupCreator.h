#pragma once
#include <QDateTime>
#include <QFileInfo>
#include <QThread>


class FuncThread;
class QString;
struct SdltmDbInfo;

class SdltmDbBackupCreator
{
public:
	SdltmDbBackupCreator();
	void CreateInitialBackup(const SdltmDbInfo & dbInfo);
	void CreateBackupNow(const SdltmDbInfo& dbInfo);
	bool IsBackingUpNow() const;
	void WaitForBackupToComplete() const;

	void OnAfterSuccessfulUpdate(const SdltmDbInfo& dbInfo);
	void OnLastSuccessfulUpdate(QDateTime lastUpdate, const SdltmDbInfo& dbInfo);

	void RestoreInitialDatabase(const SdltmDbInfo& dbInfo);

private:
	void CreateBackup(const QString& srcFile, const QString& destFile, const SdltmDbInfo& dbInfo);
	void CreateBackupImpl(const QString& srcFile, const QString& destFile, const SdltmDbInfo& dbInfo);

private:
	std::shared_ptr<FuncThread> _backupThread;
	bool _backupRunning = false;
	mutable  std::recursive_mutex _mutex;
	QDateTime _lastBackup;
	std::vector<QFileInfo> _existingBackups;
};

