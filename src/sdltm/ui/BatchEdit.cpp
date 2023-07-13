#include "BatchEdit.h"

#include "SdltmUtil.h"
#include "ui_BatchEdit.h"

QString FindAndReplaceFieldInfo::OldValue() const {
	return Value(EditField.FieldType, OldText, OldDate, OldComboIndex);
}

QString FindAndReplaceFieldInfo::NewValue() const {
	return Value(EditField.FieldType, NewText, NewDate, NewComboIndex);
}

bool FindAndReplaceFieldInfo::HasOldValue() const {
	switch (EditField.FieldType) {
	case SdltmFieldMetaType::Int: assert(false); break;
	case SdltmFieldMetaType::Double: assert(false); break;
	case SdltmFieldMetaType::Text:
	case SdltmFieldMetaType::MultiText:
		return OldText != "";
	case SdltmFieldMetaType::Number:
		return OldText != "0";

	case SdltmFieldMetaType::List: 
		return OldComboIndex != -1;

	case SdltmFieldMetaType::CheckboxList: 
		return OldCheckIndexes.size() > 0;

	case SdltmFieldMetaType::DateTime:
		return OldDate > QDateTime(QDate(1990,0,0));
	default:;
	}
	assert(false);
	return "";
}

int FindAndReplaceFieldInfo::IndexToId(int idx) const {
	if (idx >= 0 && idx < EditField.Values.size())
		return EditField.ValueToID[idx];
	return -1;
}

QString FindAndReplaceFieldInfo::Value(SdltmFieldMetaType fieldType, const QString& text, const QDateTime& date, int comboValue) const {
	switch (fieldType) {
	case SdltmFieldMetaType::Int: assert(false); break;
	case SdltmFieldMetaType::Double: assert(false); break;
	case SdltmFieldMetaType::Text:
	case SdltmFieldMetaType::MultiText:
		return text;
	case SdltmFieldMetaType::Number:
		return text;

	case SdltmFieldMetaType::List:
		return comboValue >= 0 && comboValue < EditField.Values.size() ? EditField.Values[comboValue] : "";
	case SdltmFieldMetaType::CheckboxList: break;

	case SdltmFieldMetaType::DateTime: 
		return date.toString(Qt::ISODate);
	default:;
	}
	assert(false);
	return "";
}

BatchEdit::BatchEdit(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::BatchEdit)
{
	ui->setupUi(this);

	connect(ui->preview, SIGNAL(clicked(bool)), this, SLOT(OnClickPreview()));
	connect(ui->run, SIGNAL(clicked(bool)), this, SLOT(OnClickRun()));
	connect(ui->back, SIGNAL(clicked(bool)), this, SLOT(OnClickBack()));

	connect(ui->findUseRegex, SIGNAL(stateChanged(int)), this, SLOT(OnUseRegexChanged()));

	connect(ui->editField, SIGNAL(currentIndexChanged(int)), this, SLOT(OnFieldChange()));


	ui->findSearchInBoth->setChecked(true);
	ui->oldValue->setFixedWidth(ui->editField->width());
	ui->newValue->setFixedWidth(ui->editField->width());
}

BatchEdit::~BatchEdit() {
	delete ui;
}

void BatchEdit::SetCustomFields(const std::vector<CustomField>& fields) {
	_customFields = fields;
	++_ignoreUpdate;
	ui->editField->clear();
	for (const auto& fi : fields)
		ui->editField->addItem(fi.FieldName);
	ui->editField->setCurrentIndex(fields.size() > 0 ? 0 : -1);
	--_ignoreUpdate;
	OnFieldChange();
}

FindAndReplaceTextInfo BatchEdit::GetFindAndReplaceTextInfo() const {
	FindAndReplaceTextInfo far;
	far.Find = ui->findText->text();
	far.Replace = ui->replaceWithText->text();

	far.MatchCase = ui->findMatchCase->isChecked();
	far.WholeWordOnly = ui->findWordOnly->isChecked();
	far.UseRegex = ui->findUseRegex->isChecked();

	far.Type = FindAndReplaceTextInfo::SearchType::Both;
	if (ui->findSearchInSource->isChecked())
		far.Type = FindAndReplaceTextInfo::SearchType::Source;
	else if (ui->findSearchInTarget->isChecked())
		far.Type = FindAndReplaceTextInfo::SearchType::Target;
	return far;
}

FindAndReplaceFieldInfo BatchEdit::GetFindAndReplaceEditInfo() const {
	FindAndReplaceFieldInfo far;
	far.EditField = _editField;
	switch (_editField.FieldType) {
	case SdltmFieldMetaType::Int: assert(false); break;
	case SdltmFieldMetaType::Double: assert(false); break;
	case SdltmFieldMetaType::Text: 
	case SdltmFieldMetaType::MultiText: 
		far.OldText = ui->oldText->text();
		far.NewText = ui->newText->text();
		break;
	case SdltmFieldMetaType::Number:
		far.OldText = QString::number(ui->oldText->text().toInt()) ;
		far.NewText = QString::number(ui->newText->text().toInt()) ;
		break;
	case SdltmFieldMetaType::List: 
		far.OldComboIndex = ui->oldList->currentIndex();
		far.NewComboIndex = ui->newList->currentIndex();
		break;
	case SdltmFieldMetaType::CheckboxList: {
		int count = _editField.ValueToID.size();
		auto oldStates = ui->oldCheckList->checkStates();
		auto newStates = ui->newCheckList->checkStates();
		for (int i = 0; i < count; ++i) {
			if (oldStates[i])
				far.OldCheckIndexes.push_back(i);
			if (newStates[i])
				far.NewCheckIndexes.push_back(i);
		}
	} 
		break;
	case SdltmFieldMetaType::DateTime: 
		far.OldDate = ui->oldDate->dateTime();
		far.NewDate = ui->newDate->dateTime();
		break;
	default: ;
	}
	return far;
}

void BatchEdit::UpdateFieldTypeVisibility() {
	bool isText = false, isDate = false, isList = false, isChecklist = false;
	switch (_editField.FieldType) {
	case SdltmFieldMetaType::Int: assert(false); break;
	case SdltmFieldMetaType::Double: assert(false); break;
	case SdltmFieldMetaType::Text: 
	case SdltmFieldMetaType::MultiText:
		isText = true;
		break;
	case SdltmFieldMetaType::Number: isText = true; break;
	case SdltmFieldMetaType::List: isList = true; break;
	case SdltmFieldMetaType::CheckboxList: isChecklist = true; break;
	case SdltmFieldMetaType::DateTime: isDate = true; break;
	default: ;
	}

	ui->oldText->setVisible(isText);
	ui->oldDate->setVisible(isDate);
	ui->oldList->setVisible(isList);
	ui->oldCheckList->setVisible(isChecklist);
	ui->newText->setVisible(isText);
	ui->newDate->setVisible(isDate);
	ui->newList->setVisible(isList);
	ui->newCheckList->setVisible(isChecklist);

	if (isList) {
		ui->oldList->clear();
		ui->newList->clear();
		for (const auto & value : _editField.Values) {
			ui->oldList->addItem(value);
			ui->newList->addItem(value);
		}
	} else if (isChecklist) {
		ui->oldCheckList->clear();
		ui->newCheckList->clear();

		_oldChecklistStates.clear();
		_newCheckListStates.clear();
		for (int i = 0; i < _editField.Values.size(); ++i) {
			_oldChecklistStates.push_back(false);
			_newCheckListStates.push_back(false);
		}

		std::vector< std::pair<QString, bool> > oldStates, newStates;
		for (int i = 0; i < _editField.Values.size(); ++i) {
			oldStates.push_back(std::make_pair(_editField.Values[i], _oldChecklistStates[i]));
			newStates.push_back(std::make_pair(_editField.Values[i], _newCheckListStates[i]));
		}

		ui->oldCheckList->setCheckStates(oldStates);
		ui->newCheckList->setCheckStates(newStates);
	}
}

void BatchEdit::OnClickPreview() {
	if (Preview)
		Preview(GetFindAndReplaceTextInfo());
}

void BatchEdit::OnClickRun() {
	switch(ui->tab->currentIndex()) {
	case 0:
		if (FindAndReplaceText)
			FindAndReplaceText(GetFindAndReplaceTextInfo());
		break;
	case 1:
		if (FindAndReplaceField)
			FindAndReplaceField(GetFindAndReplaceEditInfo());
		break;
	case 2:
		break;
	}
}

void BatchEdit::OnClickBack() {
	if (Back)
		Back();
}

void BatchEdit::OnUseRegexChanged() {
	ui->findMatchCase->setEnabled(!ui->findUseRegex->isChecked());
	ui->findWordOnly->setEnabled(!ui->findUseRegex->isChecked());
}

void BatchEdit::OnFieldChange() {
	if (_ignoreUpdate > 0)
		return;
	auto idx = ui->editField->currentIndex();
	if (idx >= 0 && idx < _customFields.size()) {
		_editField = _customFields[idx];
		UpdateFieldTypeVisibility();
	}
}
