#include "BatchEdit.h"

#include <QMessageBox>

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
		return OldText != "";
	case SdltmFieldMetaType::MultiText:
		return OldMultiText.size() > 0;
	case SdltmFieldMetaType::Number:
		return OldText != "";

	case SdltmFieldMetaType::List: 
		return OldComboIndex != -1;

	case SdltmFieldMetaType::CheckboxList: 
		return OldCheckIndexes.size() > 0;

	case SdltmFieldMetaType::DateTime:
		return OldDate > QDateTime(QDate(1990,1,1));
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

	connect(ui->run, SIGNAL(clicked(bool)), this, SLOT(OnClickRun()));
	connect(ui->back, SIGNAL(clicked(bool)), this, SLOT(OnClickBack()));

	connect(ui->findUseRegex, SIGNAL(stateChanged(int)), this, SLOT(OnUseRegexChanged()));

	connect(ui->editField, SIGNAL(currentIndexChanged(int)), this, SLOT(OnFieldChange()));
	connect(ui->delField, SIGNAL(currentIndexChanged(int)), this, SLOT(OnDelFieldChange()));

	connect(ui->tab, SIGNAL(currentChanged(int)), this, SLOT(OnTabChanged()));
	connect(ui->findText, &QLineEdit::textChanged, this, &BatchEdit::OnFindTextChanged);

	connect(ui->findMatchCase, &QCheckBox::stateChanged, this, &BatchEdit::OnFindAndReplaceCheckChanged);
	connect(ui->findWordOnly, &QCheckBox::stateChanged, this, &BatchEdit::OnFindAndReplaceCheckChanged);
	connect(ui->findUseRegex, &QCheckBox::stateChanged, this, &BatchEdit::OnFindAndReplaceCheckChanged);

	ui->findSearchInBoth->setChecked(true);
	ui->oldMultiText->OnMultiTextChange = [this]()
	{
		_oldMultiText = ui->oldMultiText->GetMultiText();
	};
	ui->newMultiText->OnMultiTextChange = [this]()
	{
		_newMultiText = ui->newMultiText->GetMultiText();
	};

	ui->oldDate->setDateTime(QDateTime(QDate(1990, 1, 1)));
}

BatchEdit::~BatchEdit() {
	delete ui;
}

void BatchEdit::SetCustomFields(const std::vector<CustomField>& fields) {
	_customFields = fields;
	++_ignoreUpdate;
	ui->editField->clear();
	ui->delField->clear();
	for (const auto& fi : fields) {
		ui->editField->addItem(fi.FieldName);
		ui->delField->addItem(fi.FieldName);
	}
	ui->editField->setCurrentIndex(fields.size() > 0 ? 0 : -1);
	ui->delField->setCurrentIndex(fields.size() > 0 ? 0 : -1);
	--_ignoreUpdate;
	OnFieldChange();
	OnDelFieldChange();
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

namespace {
	// if not a number, returns ""
	QString TryStrToNumber(const QString & s) {
		bool ok = true;
		auto n = s.toInt(&ok);
		if (ok)
			return QString::number(n);
		else
			return "";
	}
}

FindAndReplaceFieldInfo BatchEdit::GetFindAndReplaceEditInfo() const {
	FindAndReplaceFieldInfo far;
	far.EditField = _editField;
	switch (_editField.FieldType) {
	case SdltmFieldMetaType::Int: assert(false); break;
	case SdltmFieldMetaType::Double: assert(false); break;
	case SdltmFieldMetaType::Text: 
		far.OldText = ui->oldText->text();
		far.NewText = ui->newText->text();
		break;
	case SdltmFieldMetaType::MultiText:
		far.OldMultiText = ui->oldMultiText->GetMultiText();
		far.NewMultiText = ui->newMultiText->GetMultiText();
		break;
	case SdltmFieldMetaType::Number: 
		far.OldText = TryStrToNumber(ui->oldText->text());
		far.NewText = TryStrToNumber(ui->newText->text());
		break;
	case SdltmFieldMetaType::List:
		// ... first entry is "<Any>"
		far.OldComboIndex = ui->oldList->currentIndex() - 1;
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

void BatchEdit::UpdateEditFieldTypeVisibility() {
	bool isText = false, isDate = false, isList = false, isChecklist = false, isMulti = false;
	switch (_editField.FieldType) {
	case SdltmFieldMetaType::Int: assert(false); break;
	case SdltmFieldMetaType::Double: assert(false); break;
	case SdltmFieldMetaType::Text: 
		isText = true;
		break;
	case SdltmFieldMetaType::MultiText:
		isMulti = true;
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
	ui->oldMultiText->setVisible(isMulti);
	ui->newMultiText->setVisible(isMulti);

	if (isMulti) {
		ui->oldMultiText->SetMultiText(_oldMultiText);
		ui->newMultiText->SetMultiText(_newMultiText);
	} else if (isList) {
		ui->oldList->clear();
		ui->newList->clear();
		ui->oldList->addItem("<Any>");
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

void BatchEdit::RaiseFindTextChanged() {
	auto findInfo = GetFindAndReplaceTextInfo();
	if (ui->tab->currentIndex() != 0) {
		// we're on a different tab
		findInfo.Find = "";
		findInfo.UseRegex = false;
	}

	if (FindTextChanged)
		FindTextChanged(findInfo);
}


void BatchEdit::OnClickRun() {
	QString suffix;
	switch (ui->tab->currentIndex()) {
	case 0:
		suffix = "Replace Text";
		break;
	case 1:
		suffix = "Change Field Value";
		break;
	case 2:
		suffix = "Delete Field Value";
		break;
	case 3:
		suffix = "Delete Tags";
		break;
	}

	if (QMessageBox::question(this, qApp->applicationName(), "Are you sure you want to Batch " + suffix + "?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
		return;

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
		if (FindAndReplaceDeleteField)
			FindAndReplaceDeleteField(_delField);
		break;
	case 3:
		if (FindAndReplaceDeleteTags)
			FindAndReplaceDeleteTags();
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
		UpdateEditFieldTypeVisibility();
	}
}

void BatchEdit::OnDelFieldChange() {
	if (_ignoreUpdate > 0)
		return;
	auto idx = ui->delField->currentIndex();
	if (idx >= 0 && idx < _customFields.size()) {
		_delField = _customFields[idx];
	}
}

void BatchEdit::OnTabChanged() {
	RaiseFindTextChanged();
}

void BatchEdit::OnFindTextChanged() {
	RaiseFindTextChanged();
}

void BatchEdit::OnFindAndReplaceCheckChanged() {
	RaiseFindTextChanged();
}
