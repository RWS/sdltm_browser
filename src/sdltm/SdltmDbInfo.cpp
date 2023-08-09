#include "SdltmDbInfo.h"

#include <QCryptographicHash>

#include "FileDialog.h"
#include "SdltmUtil.h"


SdltmDbInfo SdltmDbInfo::FromDbFile(const QString& fileName) {
	SdltmDbInfo dbInfo;
	QFileInfo file(fileName);
	dbInfo.DatabaseFile = fileName;

	if (QFile::exists(fileName)) {
		dbInfo.DatabaseFile = file.absoluteFilePath();
		dbInfo.SizeMB = (double)file.size() / (1024. * 1024.);
		// only for very small databases
		dbInfo.BackupOnEachUpdate = dbInfo.SizeMB < 50;

		QString md5 = QString(QCryptographicHash::hash(dbInfo.DatabaseFile.toUtf8(), QCryptographicHash::Md5).toHex());
		// use the basename, since if I will manually look through the backup folder, I will easily know what to look for,
		// since obviously, the hash is cryptic
		dbInfo.BackupFolder = AppRoamingDir() + "/backup/" + file.baseName() + "-" + md5 + "/";
		QDir dir;
		dir.mkpath(dbInfo.BackupFolder);
	}

	return dbInfo;
}
