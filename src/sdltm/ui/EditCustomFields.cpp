#include "EditCustomFields.h"

#include "ui_EditCustomFields.h"

namespace {
	void FieldItemToRow(const CustomFieldValue& item, QStandardItemModel* model, int idx)
	{
		auto needsCreate = model->item(idx, 0) == nullptr;
		if (needsCreate)
		{
			auto* name = new QStandardItem(item.Field().FieldName);
			auto* value = new QStandardItem(item.FriendlyValue());

			name->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			value->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			model->setItem(idx, 0, name);
			model->setItem(idx, 1, value);
		}
		else
		{
			model->item(idx, 0)->setText(item.Field().FieldName);
			model->item(idx, 1)->setText(item.FriendlyValue());
		}
	}

	QStandardItemModel* createModel(const std::vector<CustomFieldValue>& items)
	{
		// Indent | NOT | Condition | And/Or
		QStandardItemModel* model = new QStandardItemModel(items.size(), 2);
		for (int row = 0; row < items.size(); ++row)
			FieldItemToRow(items[row], model, row);

		model->setHorizontalHeaderItem(0, new QStandardItem("Name"));
		model->setHorizontalHeaderItem(1, new QStandardItem("Value"));
		return model;
	}

}

EditCustomFields::EditCustomFields(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::EditCustomFields)
{
	ui->setupUi(this);
	qApp->installEventFilter(this);

	ui->list->setVisible(false);

	ui->text->setVisible(false);
	ui->dateTime->setVisible(false);
	ui->multiText->setVisible(false);
	ui->combo->setVisible(false);
	ui->checkList->setVisible(false);

	connect(ui->list, SIGNAL(clicked(const QModelIndex&)), this, SLOT(OnTableClicked(const QModelIndex&)));

}

EditCustomFields::~EditCustomFields() {
	delete ui;
}

namespace  {
	// the idea - we're always showing all the field values, and we're showing them in the order they are in the db
	std::vector<CustomFieldValue> GetAllSortedFieldValues(const std::vector<CustomFieldValue>& values, const std::vector<CustomField>& allFields) {
		std::vector<CustomFieldValue> sorted;
		for (const auto& f : allFields)
			sorted.push_back(f);

		for(const auto & value : values) {
			auto found = std::find_if(sorted.begin(), sorted.end(), [=](const CustomFieldValue& v) { return v.Field().ID == value.Field().ID; });
			if (found != sorted.end())
				*found = value;
			else {
				// should never happen - a field not found in the db
				assert(false);
				sorted.push_back(value);
			}
		}
		return sorted;
	}
}

void EditCustomFields::SetFieldValues(const std::vector<CustomFieldValue>& values, const std::vector<CustomField>& allFields) {
	if (_editIdx >= 0) {
		EditSave();
	}
	auto sorted = GetAllSortedFieldValues(values, allFields);
	_oldValues = sorted;
	_newValues = sorted;

	ui->list->setModel(createModel(_oldValues));
	ui->list->setColumnWidth(0, 160);
	ui->list->setColumnWidth(1, 160);
}

void EditCustomFields::SetIsEditingSdltmQuery(bool isEditingSdltmQuery) {
	_isEditingSdltmQuery = isEditingSdltmQuery;
	ui->list->setVisible(_isEditingSdltmQuery);
}

void EditCustomFields::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);

	auto size = event->size();
	ui->list->setFixedWidth(size.width());
	ui->list->setFixedHeight(size.height());
}

bool EditCustomFields::eventFilter(QObject* obj, QEvent* event) {
	if (event->type() == QEvent::KeyPress && (obj == ui->text || obj == ui->dateTime || obj == ui->combo || obj == ui->multiText || obj == ui->checkList))
	{
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		auto isEnter = (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return);
		auto isEscape = (keyEvent->key() == Qt::Key_Escape);
		bool isShift = (keyEvent->modifiers() & Qt::ShiftModifier) != 0;
		if (isEnter && obj != ui->multiText) // on multi-text i allow Enter
			EditSave();
		else if (isEnter && obj == ui->multiText && !isShift)
			EditSave(); // Shift-Enter on multi-text -> adds a new line, Enter -> saves
		else if (isEscape)
			EditCancel();
	}

	return QWidget::eventFilter(obj, event);
}

void EditCustomFields::focusInEvent(QFocusEvent* event) {
	EditCancel();
	QWidget::focusInEvent(event);
}

void EditCustomFields::EditSave() {
	ui->checkList->setVisible(false);
	ui->text->setVisible(false);
	ui->dateTime->setVisible(false);
	ui->combo->setVisible(false);
	ui->multiText->setVisible(false);

	EditToField();
	FieldItemToRow(_newValues[_editIdx], static_cast<QStandardItemModel*>(ui->list->model()), _editIdx);

	if (Save)
		Save(_oldValues[_editIdx], _newValues[_editIdx]);

	_oldValues[_editIdx] = _newValues[_editIdx];
	_editIdx = -1;
}

void EditCustomFields::EditCancel() {
	if (_editIdx < 0)
		return; // nothing to do

	FieldToEdit();
	ui->checkList->setVisible(false);
	ui->text->setVisible(false);
	ui->dateTime->setVisible(false);
	ui->combo->setVisible(false);
	ui->multiText->setVisible(false);

	_editIdx = -1;
}

void EditCustomFields::OnTableClicked(const QModelIndex& idx) {
	if (_editIdx >= 0)
		EditSave();

	_editIdx = idx.row();
	FieldToEdit();
}

// allows editing this field
void EditCustomFields::FieldToEdit() {
	assert(_editIdx >= 0);

	auto rect = ui->list->visualRect(ui->list->model()->index(_editIdx, 1));
	auto x = rect.x() + ui->list->verticalHeader()->width();
	auto y = rect.y() + ui->list->horizontalHeader()->height();
	auto w = rect.width();
	auto h = rect.height();


	bool isText = false, isTime = false, isMultiText = false, isCombo = false, isChecklist = false;
	auto edit = _newValues[_editIdx];
	++_ignoreUpdate;
	switch (edit.Field().FieldType) {
	case SdltmFieldMetaType::Int:
	case SdltmFieldMetaType::Double:
	case SdltmFieldMetaType::Number:
		isText = true;
		ui->text->setText(QString::number(edit.Text.toInt()));
		break;

	case SdltmFieldMetaType::Text:
		isText = true;
		ui->text->setText(edit.Text);
		break;

	case SdltmFieldMetaType::MultiText: {
		isMultiText = true;
		QString multi;
		for(const auto & line : edit.MultiText) {
			if (multi != "")
				multi += "\r\n";
			multi += line;
		}
		ui->multiText->setPlainText(multi);
	}
		break;

	case SdltmFieldMetaType::List: {
		isCombo = true;
		ui->combo->clear();
		ui->combo->addItem("<None>");
		for (const auto& comboValue : edit.Field().Values)
			ui->combo->addItem(comboValue);
		ui->combo->setCurrentIndex(edit.ComboIndex + 1);
	}
		break;

	case SdltmFieldMetaType::CheckboxList: {
		std::vector< std::pair<QString, bool> > states;
		for (int i = 0; i < edit.Field().Values.size(); ++i) 
			states.push_back(std::make_pair(edit.Field().Values[i], edit.CheckboxIndexes[i]));

		ui->checkList->setCheckStates(states);

		isChecklist = true;
	}
		break;

	case SdltmFieldMetaType::DateTime: {
		isTime = true;
		ui->dateTime->setDateTime(edit.Time);
	}
		break;

	default: assert(false); break;
	}
	--_ignoreUpdate;
	ui->text->setVisible(isText);
	ui->dateTime->setVisible(isTime);
	ui->multiText->setVisible(isMultiText);
	ui->combo->setVisible(isCombo);
	ui->checkList->setVisible(isChecklist);

	QWidget* editCtrl = nullptr;
	if (isText)
		editCtrl = ui->text;
	else if (isTime)
		editCtrl = ui->dateTime;
	else if (isMultiText) {
		editCtrl = ui->multiText;
		h = ui->multiText->height();
		if (y + h >= height()) {
			y = height() - h;
		}
	}
	else if (isCombo)
		editCtrl = ui->combo;
	else if (isChecklist)
		editCtrl = ui->checkList;

	editCtrl->setFixedWidth(w);
	editCtrl->setFixedHeight(h);
	editCtrl->move(x, y);
	editCtrl->setFocus();
}

// saves the current edit
void EditCustomFields::EditToField() {
	assert(_editIdx >= 0);

	++_ignoreUpdate;
	auto &edit = _newValues[_editIdx];
	switch (edit.Field().FieldType) {
	case SdltmFieldMetaType::Int:
	case SdltmFieldMetaType::Double:
	case SdltmFieldMetaType::Number:
		if (ui->text->text().trimmed() != "")
			edit.Text = QString::number(ui->text->text().toInt());
		else
			edit.Text = "";
		break;

	case SdltmFieldMetaType::Text:
		edit.Text = ui->text->text();
		break;

	case SdltmFieldMetaType::MultiText: {
		// multi-text: ignore empty lines
		auto lines = ui->multiText->toPlainText().split("\n", QString::SkipEmptyParts);
		edit.MultiText.clear();
		for (const auto& line : lines)
			if (line.trimmed() != "")
				edit.MultiText.push_back(line);
	}
		break;

	case SdltmFieldMetaType::List:
		edit.ComboIndex = ui->combo->currentIndex() - 1;
		break;
	case SdltmFieldMetaType::CheckboxList: {
		edit.CheckboxIndexes = ui->checkList->checkStates();
	}
		break;
	case SdltmFieldMetaType::DateTime:
		edit.Time = ui->dateTime->dateTime();
		break;
	default: assert(false); break;
	}
	--_ignoreUpdate;
}
