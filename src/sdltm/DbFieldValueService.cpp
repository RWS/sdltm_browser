#include "DbFieldValueService.h"

#include <QTextCodec>

DbFieldValueService::DbFieldValueService(DBBrowserDB& db)
	: _db(&db)
{
}

QString DbFieldValueService::GetString(const QByteArray& ba) const
{
	if (_encoding.isEmpty())
		return QString::fromUtf8(ba);
	else
		return QTextCodec::codecForName(_encoding.toUtf8())->toUnicode(ba);
}
