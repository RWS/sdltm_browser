#include "MultiTextEdit.h"

#include <QResizeEvent>

#include "ui_MultiTextEdit.h"

namespace {
	const QString NEW_ITEM = "<new>";
}

MultiTextEdit::MultiTextEdit(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::MultiTextEdit)
{
	ui->setupUi(this);
	connect(ui->list, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(OnItemClicked(QListWidgetItem*)));
	connect(ui->list, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(OnItemChanged(QListWidgetItem*)));
	connect(ui->list, SIGNAL(itemSelectionChanged()), this, SLOT(OnSelectionChanged()));

	connect(ui->del, SIGNAL(clicked(bool)), this, SLOT(OnDelete()));

	ui->list->setSelectionMode(QAbstractItemView::SingleSelection);
}

MultiTextEdit::~MultiTextEdit() {
	delete ui;
}

void MultiTextEdit::SetMultiText(const std::vector<QString>& texts) {
	_texts = texts;
	_ignoreUpdate++;
	ui->list->clear();
	for (const auto & text : _texts) {
		auto item = new QListWidgetItem(text, ui->list);
		item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsSelectable);
		item->setTextColor(QColor::fromRgb(0,0,0));
		ui->list->addItem(item);
	}
	AddNewItemRow();
	_ignoreUpdate--;
}

std::vector<QString> MultiTextEdit::GetMultiText() const {
	return _texts;
}

void MultiTextEdit::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	auto size = event->size();
	ui->list->setFixedWidth(size.width());
	ui->list->setFixedHeight(size.height());
	ui->del->hide();
}

void MultiTextEdit::AddNewItemRow() {
	++_ignoreUpdate;
	auto item = new QListWidgetItem(NEW_ITEM, ui->list);
	item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsSelectable);
	item->setTextColor(QColor::fromRgb(192, 192, 192));
	ui->list->addItem(item);
	--_ignoreUpdate;
}

void MultiTextEdit::UiToTexts() {
	std::vector<QString> newTexts;
	auto count = ui->list->count();
	for (int i = 0; i < count; ++i) {
		auto row = ui->list->item(i);
		auto text = row->text();
		if (text.trimmed() != "" && text != NEW_ITEM)
			newTexts.push_back(text);
	}
	_texts = newTexts;
}

void MultiTextEdit::OnItemClicked(QListWidgetItem* item) {
	ui->list->editItem(item);
}

void MultiTextEdit::OnItemChanged(QListWidgetItem* item) {
	if (_ignoreUpdate > 0)
		return;

	UiToTexts();
	auto count = ui->list->count();

	auto isNonEmpty = item->text() != NEW_ITEM;
	if (isNonEmpty && item->textColor() == QColor::fromRgb(192, 192, 192) ) {
		// was the "<new>" row
		++_ignoreUpdate;
		item->setTextColor(QColor::fromRgb(0, 0, 0));
		--_ignoreUpdate;
	}
	if (ui->list->row(item) == count - 1 && isNonEmpty) {
		AddNewItemRow();
	}
}

void MultiTextEdit::OnSelectionChanged() {
	if (_ignoreUpdate > 0)
		return;

	auto sel = ui->list->selectedItems();
	if (sel.size() < 1) {
		ui->del->hide();
		return;
	}
	auto isLast = ui->list->row(sel[0]) == ui->list->count() - 1;
	if (isLast) {
		ui->del->hide();
		return;
	}

	auto rect = ui->list->visualItemRect(sel[0]);
	auto diffY = (ui->del->height() - rect.height()) / 2;
	ui->del->setGeometry(width() - ui->del->width(), rect.y() -diffY, ui->del->width(), ui->del->height());
	ui->del->show();
}

void MultiTextEdit::OnDelete() {
	if (_ignoreUpdate > 0)
		return;

	auto sel = ui->list->selectedItems();
	if (sel.size() < 1)
		return;

	++_ignoreUpdate;
	if (ui->list->isPersistentEditorOpen(sel[0]))
		ui->list->closePersistentEditor(sel[0]);
	delete sel[0];
	ui->list->clearSelection();
	--_ignoreUpdate;

	UiToTexts();
	ui->del->hide();
}
