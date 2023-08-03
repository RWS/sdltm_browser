#pragma once
#include <qobjectdefs.h>
#include <QString>
#include <QWidget>

#include "SdltmUpdateCache.h"
#include "sqlitedb.h"


struct FindAndReplaceTextInfo;
class SdltmSqlViewStyledItemDelegate;
class ExtendedTableWidget;
class SqliteTableModel;

namespace Ui {
	class SdltmSqlView;
}

class SdltmSqlView : public QWidget
{
Q_OBJECT

public:
	explicit SdltmSqlView(QWidget* parent = nullptr);
	~SdltmSqlView() override;

	void SetDb(DBBrowserDB& db);
	void SetSql(const QString& sql);
	void Log(const QString& msg, bool ok);

	SqliteTableModel* getModel() { return _model; }
	ExtendedTableWidget* getTableResult();
	void OnRunQueryStarted();
	void OnRunQueryFinished();
	bool IsRunning() const { return _isRunning; }

	void OnActivated();

	void SetHightlightText(const FindAndReplaceTextInfo& highlight);
	void Refresh();
	void SetUpdateCache(const SdltmUpdateCache& updateCache);
private:
	void OnVerticalScrollPosChanged();
	void ResizeVisibleRows();

private slots:
	void OnFetchedData();

	void OnNavigatePrevious();
	void OnNavigateNext();
	void OnNavigateBegin();
	void OnNavigateEnd();
	void OnNavigateGoto();
	void OnTickResizeRows();

private:
	DBBrowserDB& db() { return *_db; }
	void selectTableLine(int idx);
private:
	DBBrowserDB *_db;
	SqliteTableModel* _model = nullptr;
	SdltmSqlViewStyledItemDelegate* _itemPainter = nullptr;
	const SdltmUpdateCache *_updateCache;

	QTimer* _resizeRowTimer;
	bool _columnsResized;
	bool _isError;
	bool _isRunning;

	QString _sql;
	QString _lastValidSql;
	Ui::SdltmSqlView* ui;

	std::set<int> _rowsResizedToContents;
	int _ignoreUpdate = 0;
};


