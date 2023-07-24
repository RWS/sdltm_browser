#include "SdltmSqlView.h"

#include <QHeaderView>
#include <QScrollBar>

#include "ui_SdltmSqlView.h"
#include <qstandarditemmodel.h>
#include <QTimer>

#include "SdltmUtil.h"
#include "sqlitetablemodel.h"

SdltmSqlView::SdltmSqlView(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::SdltmSqlView)
	, _columnsResized(false)
	, _isError(false)
	, _isRunning(false)
{
	ui->setupUi(this);

    _resizeRowTimer = new QTimer(this);
    _resizeRowTimer->setInterval(10);
    connect(_resizeRowTimer, SIGNAL(timeout()), this, SLOT(OnTickResizeRows()));

    connect(ui->buttonNext, SIGNAL(clicked(bool)), this, SLOT(OnNavigateNext()));
    connect(ui->buttonPrevious, SIGNAL(clicked(bool)), this, SLOT(OnNavigatePrevious()));
    connect(ui->buttonBegin, SIGNAL(clicked(bool)), this, SLOT(OnNavigateBegin()));
    connect(ui->buttonEnd, SIGNAL(clicked(bool)), this, SLOT(OnNavigateEnd()));
    connect(ui->buttonGoto, SIGNAL(clicked(bool)), this, SLOT(OnNavigateGoto()));

    ui->table->OnVerticalScrollPosChanged = [this]()
    {
        OnVerticalScrollPosChanged();
    };
}

SdltmSqlView::~SdltmSqlView() {
    delete ui;
}

void SdltmSqlView::SetDb(DBBrowserDB& db)
{
	_db = &db;

	_model = new SqliteTableModel(db, this);
	ui->table->setModel(_model);

	connect(_model, &SqliteTableModel::finishedFetch, this, &SdltmSqlView::OnFetchedData);
}

void SdltmSqlView::SetSql(const QString& sql)
{
	_sql = sql;
    _columnsResized = false;
    _isError = false;
}

void SdltmSqlView::Log(const QString& msg, bool ok)
{
    if (!ok)
        _isError = true;
}

ExtendedTableWidget* SdltmSqlView::getTableResult()
{
    return ui->table;
}

void SdltmSqlView::OnRunQueryStarted()
{
    // show visually that we're executing
    ui->table->setEnabled(false);
    ui->navigateCtrl->setEnabled(false);
    _isRunning = true;
    _isError = false;
}

void SdltmSqlView::OnRunQueryFinished()
{
    _rowsResizedToContents.clear();
    ui->table->setEnabled(!_isError);
    ui->navigateCtrl->setEnabled(!_isError);
    _isRunning = false;
    if (!_isError)
        ui->labelRecordset->setText("1 of " + QString::number(_model->rowCount()));
    else
        ui->labelRecordset->setText("");
}

void SdltmSqlView::OnTickResizeRows() {
    _resizeRowTimer->stop();
    ResizeVisibleRows();
}


void SdltmSqlView::OnVerticalScrollPosChanged() {
    if (_ignoreUpdate > 0)
        return;

    _resizeRowTimer->start();
}

void SdltmSqlView::ResizeVisibleRows() {
    ++_ignoreUpdate;
    auto visibleCount = ui->table->numVisibleRows();
    auto topIdx = ui->table->rowAt(0) == -1 ? 0 : ui->table->rowAt(0);
    ++visibleCount;
    auto maxIdx = std::min(topIdx + visibleCount, _model->rowCount() - 1);

    for (int i = topIdx; i <= maxIdx; ++i)
        if (_rowsResizedToContents.find(i) == _rowsResizedToContents.end()) {
            _rowsResizedToContents.insert(i);
            ui->table->resizeRowToContents(i);
        }
    --_ignoreUpdate;

    auto newVisibleCount = ui->table->numVisibleRows();
    ui->table->verticalScrollBar()->setPageStep(newVisibleCount);
    SdltmLog("resize visible rows " + QString::number(visibleCount) + " to " + QString::number(newVisibleCount));
}

void SdltmSqlView::OnFetchedData()
{
    if (_model == nullptr)
        return;
    // Don't resize the columns more than once to fit their contents. This is necessary because the finishedFetch signal of the model
    // is emitted for each loaded prefetch block and we want to avoid resizes while scrolling down.
    if (_columnsResized)
        return;
    _columnsResized = true;

    for (int i = 0; i < _model->columnCount(); i++)
    {
        // ... Source or Target
        if (i == 1 || i == 2) {
            ui->table->setColumnWidth(i, 600);
            continue;
        }
        ui->table->setColumnWidth(i, 60);
    }
    // IMPORTANT: this would be waaaay too time consuming for lots of records
//    ui->table->resizeRowsToContents();
    ResizeVisibleRows();
}

void SdltmSqlView::OnNavigatePrevious()
{
    int curRow = ui->table->currentIndex().row();
    curRow -= ui->table->numVisibleRows() - 1;
    if (curRow < 0)
        curRow = 0;
    selectTableLine(curRow);
}

void SdltmSqlView::OnNavigateNext()
{
    int curRow = ui->table->currentIndex().row();
    curRow += ui->table->numVisibleRows() - 1;
    if (curRow >= _model->rowCount())
        curRow = _model->rowCount() - 1;
    selectTableLine(curRow);
}

void SdltmSqlView::OnNavigateBegin()
{
    selectTableLine(0);
}

void SdltmSqlView::OnNavigateEnd()
{
    selectTableLine(_model->rowCount() - 1);
}

void SdltmSqlView::OnNavigateGoto()
{
    int row = ui->editGoto->text().toInt();
    if (row <= 0)
        row = 1;
    if (row > _model->rowCount())
        row = _model->rowCount();

    selectTableLine(row - 1);
    ui->editGoto->setText(QString::number(row));
}


void SdltmSqlView::selectTableLine(int idx)
{
    ui->table->selectTableLine(idx);
    ui->labelRecordset->setText(QString::number(idx + 1) + " of " + QString::number(_model->rowCount()));
}

