#include "EditSdltmFilter.h"

#include <iostream>
#include <qstandarditemmodel.h>
#include <sstream>

#include "SdltmCreateSqlSimpleFilter.h"
#include "ui_EditSdltmFilter.h"
#include "ui_PlotDock.h"

namespace 
{
	const int HEADER_HEIGHT = 30;
}

namespace 
{

	// for testing
	//SdltmFilter TestFilter()
	//{
	//	SdltmFilter test;
	//	test.FilterItems.push_back(SdltmFilterItem("Customer", SdltmFieldMetaType::Text));
	//	test.FilterItems.back().FieldValue = "Gigi";
	//	test.FilterItems.back().IndentLevel = 0;

	//	test.FilterItems.push_back(SdltmFilterItem("Listy", SdltmFieldMetaType::List));
	//	test.FilterItems.back().FieldValue = "Def";
	//	test.FilterItems.back().IndentLevel = 0;

	//	test.FilterItems.push_back(SdltmFilterItem("CheckListy", SdltmFieldMetaType::CheckboxList));
	//	test.FilterItems.back().FieldValues = { "Mno", "st", "zzz"};
	//	test.FilterItems.back().IndentLevel = 0;
	//	test.FilterItems.back().StringComparison = StringComparisonType::Contains;

	//	test.FilterItems.push_back(SdltmFilterItem("Country", SdltmFieldMetaType::Text));
	//	test.FilterItems.back().FieldValue = "Romania";
	//	test.FilterItems.back().IndentLevel = 1;
	//	test.FilterItems.push_back(SdltmFilterItem(SdltmFieldType::CreatedBy));
	//	test.FilterItems.back().FieldValue = "Jay";
	//	test.FilterItems.back().IndentLevel = 2;
	//	test.FilterItems.back().IsAnd = false;
	//	test.FilterItems.push_back(SdltmFilterItem(SdltmFieldType::UseageCount));
	//	test.FilterItems.back().FieldValue = "5";
	//	test.FilterItems.back().IndentLevel = 2;
	//	test.FilterItems.back().IsAnd = false;
	//	test.FilterItems.back().NumberComparison = NumberComparisonType::BiggerOrEqual;
	//	test.FilterItems.push_back(SdltmFilterItem(SdltmFieldType::CreatedOn));
	//	test.FilterItems.back().FieldValue = "2023/02/20";
	//	test.FilterItems.back().IndentLevel = 2;
	//	test.FilterItems.back().IsAnd = false;
	//	test.FilterItems.push_back(SdltmFilterItem(SdltmFieldType::SourceSegment));
	//	test.FilterItems.back().FieldValue = "extra";
	//	test.FilterItems.back().IndentLevel = 2;
	//	test.FilterItems.back().StringComparison = StringComparisonType::Contains;
	//	test.FilterItems.back().IsAnd = false;
	//	test.FilterItems.back().IsNegated = true;
	//	test.FilterItems.push_back(SdltmFilterItem(SdltmFieldType::LastModifiedBy));
	//	test.FilterItems.back().FieldValue = "JJ";
	//	test.FilterItems.back().IndentLevel = 1;
	//	test.FilterItems.push_back(SdltmFilterItem(SdltmFieldType::LastUsedBy));
	//	test.FilterItems.back().FieldValue = "TT";
	//	test.FilterItems.back().IndentLevel = 1;
	//	test.FilterItems.back().StringComparison = StringComparisonType::EndsWith;

	//	return test;
	//}




	//std::vector<CustomField> TestCustomFields()
	//{
	//	std::vector<CustomField> fields;
	//	CustomField a;
	//	a.FieldName = "Client";
	//	a.ID = 3;
	//	CustomField b;
	//	b.FieldName = "Country";
	//	CustomField c;
	//	c.FieldName = "Price";
	//	c.FieldType = SdltmFieldMetaType::Number;
	//	c.ID = 4;
	//	CustomField d;
	//	d.FieldName = "Listy";
	//	d.FieldType = SdltmFieldMetaType::List;
	//	d.Values = { "Abc", "Def", "Ghk", "Ij"};
	//	CustomField e;
	//	e.FieldName = "CheckListy";
	//	e.FieldType = SdltmFieldMetaType::CheckboxList;
	//	e.Values = { "Kl", "Mno", "Pqr", "st", "x-x-x", "y-y-y", "zzz"};
	//	fields.push_back(a);
	//	fields.push_back(b);
	//	fields.push_back(c);
	//	fields.push_back(d);
	//	fields.push_back(e);
	//	return fields;
	//}
}

EditSdltmFilter::EditSdltmFilter(QWidget* parent ) 
	: QWidget(parent) 
    , ui(new Ui::EditSdltmFilter)
{
	++_ignoreUpdate;
    ui->setupUi(this);
	--_ignoreUpdate;
	ui->editCondition->hide();
	ui->reenableSimpleFilterGrid->hide();

	connect(ui->simpleFilterTable, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onTableClicked(const QModelIndex&)));

	connect(ui->fieldCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onFieldComboChanged(int)));
	connect(ui->operationCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onOperationComboChanged(int)));
	connect(ui->andorCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onAndOrComboChanged(int)));

	connect(ui->notBox, SIGNAL(stateChanged(int)), this, SLOT(onNotChecked(int)));
	connect(ui->caseSensitive, SIGNAL(stateChanged(int)), this, SLOT(onCaseSensitiveChanged(int)));

	connect(ui->indentSpin, SIGNAL(valueChanged(int)), this, SLOT(onIndentChanged()));

	connect(ui->editFilterSave, SIGNAL(clicked(bool)), this, SLOT(onFilterSave()));
	connect(ui->editFilterCancel, SIGNAL(clicked(bool)), this, SLOT(onFilterCancel()));

	connect(ui->quickSearchSource, SIGNAL(textChanged(const QString&)), this, SLOT(onQuickSourceTextChanged()));
	connect(ui->quickSearchTarget, SIGNAL(textChanged(const QString&)), this, SLOT(onQuickTargetTextChanged()));

	connect(ui->quickSearchSourceAndTarget, SIGNAL(stateChanged(int)), this, SLOT(onQuickSourceAndTargetChanged()));
	connect(ui->quickSearchCaseSensitive, SIGNAL(stateChanged(int)), this, SLOT(onQuickCaseSensitiveChanged()));
	connect(ui->quickSearchWholeWordOnly, SIGNAL(stateChanged(int)), this, SLOT(onQuickWholeWordChanged()));
	connect(ui->quickSearchRegex, SIGNAL(stateChanged(int)), this, SLOT(onQuickRegexChanged()));

	connect(ui->textValue, SIGNAL(textChanged(const QString&)), this, SLOT(onFilterTextChanged()));
	connect(ui->dateValue, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(onFilterDateTimeChanged()));
	connect(ui->comboValue, SIGNAL(currentIndexChanged(int)), this, SLOT(onFilterComboChanged()));
	connect(ui->comboCheckboxesValue, SIGNAL(dataChanged()), this, SLOT(onFilterComboCheckboxChanged()));

	connect(ui->addSimpleFilterItem, SIGNAL(clicked(bool)), this, SLOT(onAddItem()));
	connect(ui->delSimpleFilterItem, SIGNAL(clicked(bool)), this, SLOT(onDelItem()));
	connect(ui->insertSimpleFilterItem, SIGNAL(clicked(bool)), this, SLOT(onInsertItem()));

	connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));
	connect(ui->reenableSimpleFilter, SIGNAL(clicked(bool)), this, SLOT(onReEnableClick()));

	ui->andorCombo->addItem("AND");
	ui->andorCombo->addItem("OR");

	ui->disabledBg->hide();

	_saveAdvancedFilterTimer = new QTimer(this);
	connect(_saveAdvancedFilterTimer, SIGNAL(timeout()), this, SLOT(onSaveAdvancedFilterTimer()));

	_lastFilterChange = QDateTime::currentDateTime();

	_applyFilterTimer = new QTimer(this);
	connect(_applyFilterTimer, SIGNAL(timeout()), this, SLOT(onApplyFilterTimer()));
	_applyFilterTimer->start(250);

	// testing
	//SetCustomFields(TestCustomFields());
	//SetEditFilter(std::make_shared<SdltmFilter>(TestFilter()));
}
EditSdltmFilter::~EditSdltmFilter() {
	delete ui;
}

void EditSdltmFilter::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	auto size = event->size();
	ui->tabWidget->setFixedWidth(size.width());
	ui->tabWidget->setFixedHeight(size.height());


	ui->disabledBg->setGeometry(0, HEADER_HEIGHT, size.width(), size.height() - HEADER_HEIGHT);
	ui->reenableSimpleFilterGrid->setGeometry(0, HEADER_HEIGHT, size.width(), size.height() - HEADER_HEIGHT);
}

namespace 
{
	void FilterItemToRow(const SdltmFilterItem & item, QStandardItemModel * model, int idx, bool isLast)
	{
		auto needsCreate = model->item(idx, 0) == nullptr;
		if (needsCreate)
		{
			auto* indent = new QStandardItem(QString::number(item.IndentLevel));
			auto * not = new QStandardItem(QString(item.IsNegated ? "x" : ""));
			auto* friendly = new QStandardItem(item.FriendlyString());
			auto* andOr = new QStandardItem(isLast ? "" : (item.IsAnd ? "AND" : "OR"));

			indent->setTextAlignment(Qt::AlignCenter);
			not->setTextAlignment(Qt::AlignCenter);
			friendly->setTextAlignment(Qt::AlignLeft);
			andOr->setTextAlignment(Qt::AlignCenter);
			model->setItem(idx, 0, indent);
			model->setItem(idx, 1, not);
			model->setItem(idx, 2, friendly);
			model->setItem(idx, 3, andOr);
		}
		else
		{
			model->item(idx, 0)->setText(QString::number(item.IndentLevel));
			model->item(idx, 1)->setText(QString(item.IsNegated ? "x" : ""));
			model->item(idx, 2)->setText(item.FriendlyString());
			model->item(idx, 3)->setText(isLast ? "" : (item.IsAnd ? "AND" : "OR"));
		}
	}

	QStandardItemModel *createFilterItemModel(const std::vector<SdltmFilterItem> & items)
	{
		// Indent | NOT | Condition | And/Or
		QStandardItemModel* model = new QStandardItemModel(items.size(), 4);
		for (int row = 0; row < items.size(); ++row)
			FilterItemToRow(items[row], model, row, row == items.size() - 1);

		model->setHorizontalHeaderItem(0, new QStandardItem("Indent"));
		model->setHorizontalHeaderItem(1, new QStandardItem("NOT"));
		model->setHorizontalHeaderItem(2, new QStandardItem("Condition"));
		model->setHorizontalHeaderItem(3, new QStandardItem("AND/OR"));
		return model;
	}
}

void EditSdltmFilter::UpdateCustomExpressionUserEditableArgs()
{
	// FIXME at this time, if I update a custom expression to use other names, I can end up with the old user-editable values as well

	std::vector<SdltmFilterItem> customExprWithEditableArgs;
	std::copy_if(_filter.FilterItems.begin(), _filter.FilterItems.end(), std::back_inserter(customExprWithEditableArgs),
		[](const SdltmFilterItem& fi) { return fi.IsCustomExpressionWithUserEditableArgs(); });

	// the idea: any user-editable arg needs to be visible to the user
	for (const auto& meta : customExprWithEditableArgs)
	{
		auto editableUserArgs = meta.ToUserEditableFilterItems();
		for (const auto & arg : editableUserArgs)
		{
			auto found = std::find_if(_filter.FilterItems.begin(), _filter.FilterItems.end(),
				[arg](const SdltmFilterItem& fi) { return fi.CustomFieldName == arg.CustomFieldName; });
			if (found == _filter.FilterItems.end())
				_filter.FilterItems.push_back(arg);
		}
	}

	for (auto& item : _filter.FilterItems)
		if (item.IsUserEditableArg)
			item.IsVisible = !EditCustomExpressionWithUserEditableArgs;

	for (auto& item : _filter.FilterItems)
		if (item.IsCustomExpressionWithUserEditableArgs())
			item.IsVisible = EditCustomExpressionWithUserEditableArgs;

	_userEditableArgsItems.clear();
	if (!EditCustomExpressionWithUserEditableArgs)
		std::copy_if(_filter.FilterItems.begin(), _filter.FilterItems.end(), std::back_inserter(_userEditableArgsItems),
			[](const SdltmFilterItem& fi) { return fi.IsUserEditableArg; });
}

void EditSdltmFilter::SetEditFilter(const SdltmFilter & filter)
{
	_filter = filter;

	for (auto& item : _filter.FilterItems)
		if (item.CustomFieldName != "" && std::find_if(_customFields.begin(), _customFields.end(), 
			[item](const CustomField& f) { return f.FieldName == item.CustomFieldName; }) == _customFields.end())
			// custom field not available for this database
			item.IsVisible = false;

	UpdateCustomExpressionUserEditableArgs();

	_editableFilterItems.clear();
	_hiddenFilterItems.clear();
	std::copy_if(_filter.FilterItems.begin(), _filter.FilterItems.end(), std::back_inserter(_editableFilterItems), [](const SdltmFilterItem& fi) { return fi.IsVisible; });
	std::copy_if(_filter.FilterItems.begin(), _filter.FilterItems.end(), std::back_inserter(_hiddenFilterItems), [](const SdltmFilterItem& fi) { return !fi.IsVisible; });

	ui->simpleFilterTable->setModel(createFilterItemModel(_editableFilterItems));
	ui->simpleFilterTable->setColumnWidth(0, 60);
	ui->simpleFilterTable->setColumnWidth(1, 60);
	ui->simpleFilterTable->setColumnWidth(2, 450);
	ui->simpleFilterTable->setColumnWidth(3, 80);

	++_ignoreUpdate;
	UpdateQuickSearchVisibility();
	ui->quickSearchSourceAndTarget->setChecked(_filter.QuickSearchSearchSourceAndTarget);
	ui->quickSearchCaseSensitive->setChecked(_filter.QuickSearchCaseSensitive);
	ui->quickSearchWholeWordOnly->setChecked(_filter.QuickSearchWholeWordOnly);
	ui->quickSearchRegex->setChecked(_filter.QuickSearchUseRegex);

	if (_filter.AdvancedSql != "")
	{
		ui->advancedEdit->setText(_filter.AdvancedSql);
		ui->tabWidget->setCurrentIndex(1); // go to advanced
	}
	else
	{
		SdltmCreateSqlSimpleFilter createFilter(_filter, _customFields);
		auto str = createFilter.ToSqlFilter();
		ui->advancedEdit->setText(str);

		ui->tabWidget->setCurrentIndex(0); // go to simple
	}
	EnableSimpleTab(_filter.AdvancedSql == "");

	--_ignoreUpdate;

	onTabChanged(ui->tabWidget->currentIndex());
	if (OnApply)
		OnApply(FilterString());
}

void EditSdltmFilter::SetCustomFields(const std::vector<CustomField> customFields)
{
	_customFields = customFields;
}

void EditSdltmFilter::Close()
{
	if (_editRowIndex >= 0)
		onFilterSave();
}

void EditSdltmFilter::ForceSaveNow() {
	if (ui->tabWidget->currentIndex() == 1)
		onSaveAdvancedFilterTimer();
	else if (_editRowIndex >= 0)
		onFilterSave();
}



void EditSdltmFilter::UpdateQuickSearchVisibility()
{
	++_ignoreUpdate;
	ui->quickSearchSourceTextLabel->setText(_filter.QuickSearchSearchSourceAndTarget ? "Text" : "Source Text");
	ui->quickSearchSource->setText(_filter.QuickSearch);
	ui->quickSearchTarget->setText(_filter.QuickSearchTarget);
	ui->quickSearchTarget->setVisible(!_filter.QuickSearchSearchSourceAndTarget);
	ui->quickSearchTargetLabel->setVisible(!_filter.QuickSearchSearchSourceAndTarget);
	--_ignoreUpdate;
}


void EditSdltmFilter::SaveFilter()
{
	_filter.FilterItems.clear();
	std::copy(_hiddenFilterItems.begin(), _hiddenFilterItems.end(), std::back_inserter(_filter.FilterItems));
	std::copy(_editableFilterItems.begin(), _editableFilterItems.end(), std::back_inserter(_filter.FilterItems));

	if(OnSave)
		OnSave(_filter);
}

void EditSdltmFilter::EditRow(int idx)
{
	ShowEditControlsForRow(idx);

	_ignoreUpdate++;
	ui->disabledBg->show();

	auto& item = _editableFilterItems[idx];
	_editFilterItem = _originalEditFilterItem = item;

	ui->indentSpin->setValue(item.IndentLevel);
	ui->notBox->setChecked(item.IsNegated);
	ui->andorCombo->setCurrentIndex(item.IsAnd ? 0 : 1);
	ui->textValue->setText(item.FieldValue);
	// for custom fields -> no case sensitivity
	ui->caseSensitive->setVisible(item.CustomFieldName == "" && (item.FieldMetaType == SdltmFieldMetaType::Text || item.FieldMetaType == SdltmFieldMetaType::MultiText) );
	ui->caseSensitive->setChecked(item.CaseSensitive);

	// custom sql expression -> combo makes no sense
	// user-editable arg -> user just needs to enter a value for the arg
	ui->operationCombo->setVisible(item.FieldType != SdltmFieldType::CustomSqlExpression && !item.IsUserEditableArg);

	UpdateFieldCombo();
	UpdateOperationCombo();
	UpdateValue();

	if (item.CustomFieldName != "")
	{
		auto userEditable = std::find_if(_userEditableArgsItems.begin(), _userEditableArgsItems.end(), 
										 [item](const SdltmFilterItem& fi) { return fi.CustomFieldName == item.CustomFieldName; });
		if (userEditable != _userEditableArgsItems.end())
		{
			auto idx = userEditable - _userEditableArgsItems.begin();
			ui->fieldCombo->setCurrentIndex(idx);
		}
		else
		{
			auto field = std::find_if(_customFields.begin(), _customFields.end(), 
									  [item](const CustomField& cf) { return cf.FieldName == item.CustomFieldName; });
			auto idx = field - _customFields.begin();
			ui->fieldCombo->setCurrentIndex(idx + _userEditableArgsItems.size());
		}
	}
	else
	{
		// preset
		ui->fieldCombo->setCurrentIndex(_customFields.size() + _userEditableArgsItems.size() + (int)item.FieldType);
	}


	_ignoreUpdate--;
}

void EditSdltmFilter::ShowEditControlsForRow(int idx)
{
	int vertHeaderWidth = ui->simpleFilterTable->verticalHeader()->width();
	int horizHeaderHeight = ui->simpleFilterTable->horizontalHeader()->height();
	auto indentCol = ui->simpleFilterTable->columnWidth(0);
	auto notCol = ui->simpleFilterTable->columnWidth(1);
	auto conditionCol = ui->simpleFilterTable->columnWidth(2);
	auto andorCol = ui->simpleFilterTable->columnWidth(3);
	int height = ui->simpleFilterTable->rowHeight(idx);
	int y = ui->simpleFilterTable->rowViewportPosition(idx);

	auto tablePos = ui->simpleFilterTable->mapTo(ui->tabWidget, QPoint(0, 0));
	ui->editCondition->move(vertHeaderWidth + tablePos.x(), y + tablePos.y() + horizHeaderHeight);
	ui->editCondition->setFixedWidth(indentCol + notCol + conditionCol + andorCol);
	ui->editCondition->setFixedHeight(height * 2);

	ui->indentSpin->setFixedWidth(indentCol);
	ui->notBoxWrapper->setFixedWidth(notCol);
	ui->andorCombo->setFixedWidth(andorCol);

	ui->editCondition->show();
	ui->editCondition->activateWindow();
	ui->editCondition->raise();
}

namespace
{
	std::vector<QString> _SdltmFieldTypes = {
	"Last Modified On",
	"Last Modified By",
	"Last Used On",
	"Last Used By",
	"Usage Count",
	"Created On",
	"Created By",
	"Source Segment",
	"Source Segment Length",
	"Target Segment",
	"Target Segment Length",
	"Number Of Tags In Source",
	"Number Of Tags In Target",
	"Custom Sql Expression",
	};
}

void EditSdltmFilter::UpdateFieldCombo()
{
	ui->fieldCombo->clear();

	for (const auto& fi : _userEditableArgsItems)
		ui->fieldCombo->addItem(fi.CustomFieldName);

	for (const auto& cf : _customFields)
		ui->fieldCombo->addItem(cf.FieldName);

	for (const auto& ft : _SdltmFieldTypes)
		ui->fieldCombo->addItem(ft);
}

namespace
{
	enum class ComparisonType
	{
		Number, String, MultiString, List, CheckList,
	};

	ComparisonType FieldMetaTypeToComparison(SdltmFieldMetaType metaType)
	{
		switch (metaType)
		{
			// number
		case SdltmFieldMetaType::Int:
		case SdltmFieldMetaType::Double:
		case SdltmFieldMetaType::Number:
		case SdltmFieldMetaType::DateTime:
			return ComparisonType::Number;
			// string
		case SdltmFieldMetaType::Text:
			return ComparisonType::String;
			// multi-string
		case SdltmFieldMetaType::MultiText:
			return ComparisonType::MultiString;
			// multi-comparison (has-item) -- single value
		case SdltmFieldMetaType::List:
			return ComparisonType::List;
			// multi-comparison (has-item) -- several values
		case SdltmFieldMetaType::CheckboxList:
			return ComparisonType::CheckList;
		default:;
			return ComparisonType::Number;
		}
	}
}
void EditSdltmFilter::UpdateOperationCombo()
{
	std::vector<QString> operations;
	auto& item = _editFilterItem;
	switch (item.FieldMetaType)
	{
		// number
	case SdltmFieldMetaType::Int: 
	case SdltmFieldMetaType::Double: 
	case SdltmFieldMetaType::Number: 
	case SdltmFieldMetaType::DateTime:
		operations = { "Equal", "Less", "Less or Equal", "Greater", "Greater or Equal"};
		break;

		// string
	case SdltmFieldMetaType::Text: 
		operations = { "Equals", "Contains", "Starts With", "Ends With"};
		break;

		// multi-string
	case SdltmFieldMetaType::MultiText: 
		operations = { "Any Equals", "Any Contains", "Any Starts With", "Any Ends With"};
		break;

		// multi-comparison (has-item) -- single value
	case SdltmFieldMetaType::List: 
		operations = { "Has Value" };
		break;
		// multi-comparison (has-item) -- several values
	case SdltmFieldMetaType::CheckboxList: 
		operations = { "Has Any Of", "Has All Of", "Equals" };
		break;
	default: ;
	}

	++_ignoreUpdate;
	ui->operationCombo->clear();
	for (const auto& o : operations)
		ui->operationCombo->addItem(o);

	switch (FieldMetaTypeToComparison(item.FieldMetaType))
	{
	case ComparisonType::Number: 
		ui->operationCombo->setCurrentIndex((int)item.NumberComparison);
		break;
	case ComparisonType::String: 
		ui->operationCombo->setCurrentIndex((int)item.StringComparison);
		break;
	case ComparisonType::MultiString: 
		ui->operationCombo->setCurrentIndex((int)item.MultiStringComparison);
		break;
	case ComparisonType::List: 
		// multi-comparison (has-item) -- single value
		ui->operationCombo->setCurrentIndex(0);
		break;
	case ComparisonType::CheckList: 
		// multi-comparison (has-item) -- several values
		ui->operationCombo->setCurrentIndex((int)item.ChecklistComparison);
		break;
	default: ;
	}

	--_ignoreUpdate;
}

void EditSdltmFilter::UpdateValueVisibility()
{
	bool isNumber = false, isString = false, isList = false, isChecklist = false, isDatetime = false;
	switch (FieldMetaTypeToComparison(_editFilterItem.FieldMetaType))
	{
	case ComparisonType::Number: isNumber = true; break;
	case ComparisonType::String: 
	case ComparisonType::MultiString: isString = true; break;
	case ComparisonType::List: isList = true; break;
	case ComparisonType::CheckList: isChecklist = true; break;
	default: assert(false);
	}

	if (_editFilterItem.FieldMetaType == SdltmFieldMetaType::DateTime)
	{
		isDatetime = true;
		isNumber = false;
	}

	ui->textValue->setVisible(isNumber || isString);
	ui->dateValue->setVisible(isDatetime);
	ui->comboValue->setVisible(isList);
	ui->comboCheckboxesValue->setVisible(isChecklist);
}

namespace
{
	void tokenize(std::string const& str, const char delim, std::vector<std::string>& out)
	{
		std::stringstream ss(str);

		std::string s;
		while (std::getline(ss, s, delim)) {
			out.push_back(s);
		}
	}
	void tokenize(std::wstring const& str, const wchar_t delim, std::vector<std::wstring>& out)
	{
		std::wstringstream ss(str);

		std::wstring s;
		while (std::getline(ss, s, delim)) {
			out.push_back(s);
		}
	}

	void ParseIntsFromString(QString const& str, const char delim, std::vector<int>& out)
	{
		std::stringstream ss(str.toStdString());

		std::string s;
		while (std::getline(ss, s, delim)) {
			try
			{
				out.push_back(std::stoi(s));
			}
			catch (...)
			{} // ignore invalid number
		}
	}
}

void EditSdltmFilter::UpdateValue()
{
	UpdateValueVisibility();

	bool isNumber = false, isString = false, isList = false, isChecklist = false, isDatetime = false;
	switch (FieldMetaTypeToComparison(_editFilterItem.FieldMetaType))
	{
	case ComparisonType::Number: isNumber = true; break;
	case ComparisonType::String:
	case ComparisonType::MultiString: isString = true; break;
	case ComparisonType::List: isList = true; break;
	case ComparisonType::CheckList: isChecklist = true; break;
	default: assert(false);
	}

	if (_editFilterItem.FieldMetaType == SdltmFieldMetaType::DateTime)
	{
		isDatetime = true;
		isNumber = false;
	}

	if (isNumber || isString)
		ui->textValue->setText(_editFilterItem.FieldValue);
	else if (isDatetime)
	{
		QDateTime dt = QDateTime::fromString(_editFilterItem.FieldValue, Qt::ISODate);
		if (!dt.isValid())
			dt = QDateTime::currentDateTime();

		ui->dateValue->setDateTime(dt);
	}
	else if (isList)
	{
		ui->comboValue->clear();
		auto valuesIt = std::find_if(_customFields.begin(), _customFields.end(), [this](const CustomField& f) { return f.FieldName == _editFilterItem.CustomFieldName; });
		if (valuesIt != _customFields.end())
		{
			const auto& values = valuesIt->Values;
			for (const auto& value : values)
				ui->comboValue->addItem(value);
			auto foundValue = std::find(values.begin(), values.end(), _editFilterItem.FieldValue);
			if (foundValue != values.end())
				ui->comboValue->setCurrentIndex(foundValue - values.begin());
			else if (!values.empty())
				ui->comboValue->setCurrentIndex(0);
		}
	}
	else if (isChecklist)
	{
		ui->comboCheckboxesValue->clear();
		auto valuesIt = std::find_if(_customFields.begin(), _customFields.end(), [this](const CustomField& f) { return f.FieldName == _editFilterItem.CustomFieldName; });
		if (valuesIt != _customFields.end())
		{

			std::vector< std::pair<QString, bool> > checkValues;
			const auto& values = valuesIt->Values;
			auto curIdx = 0;
			for (const auto& value : values)
			{
				auto isChecked = std::find(_editFilterItem.FieldValues.begin(), _editFilterItem.FieldValues.end(), value) != _editFilterItem.FieldValues.end();
				checkValues.push_back(std::make_pair(value, isChecked));
				++curIdx;
			}

			ui->comboCheckboxesValue->setCheckStates(checkValues);
		}
	}
	else
		assert(false);
}


void EditSdltmFilter::onTableClicked(const QModelIndex& row)
{
	if (_editRowIndex >= 0)
		// save previous filter
		onFilterSave();

	auto idx = row.row();
	_editRowIndex = idx;
	EditRow(idx);
}

void EditSdltmFilter::onFieldComboChanged(int idx)
{
	if (_ignoreUpdate > 0)
		return;

	// IMPORTANT: the custom fields are first
	if (idx < _customFields.size())
	{
		// it's a custom field
		_editFilterItem.FieldType = SdltmFieldType::CustomField;
		_editFilterItem.FieldMetaType = _customFields[idx].FieldType;
		_editFilterItem.CustomFieldName = _customFields[idx].FieldName;
	}
	else
	{
		_editFilterItem.FieldType = (SdltmFieldType)(idx - _customFields.size());
		_editFilterItem.FieldMetaType = SdltmFilterItem::PresetFieldMetaType(_editFilterItem.FieldType);
		_editFilterItem.CustomFieldName = "";
	}

	UpdateOperationCombo();
	UpdateValue();
}

void EditSdltmFilter::onOperationComboChanged(int idx)
{
	if (_ignoreUpdate > 0)
		return;

	switch (FieldMetaTypeToComparison(_editFilterItem.FieldMetaType))
	{
	case ComparisonType::Number:
		_editFilterItem.NumberComparison = static_cast<NumberComparisonType>(idx);
		break;
	case ComparisonType::String: 
		_editFilterItem.StringComparison = static_cast<StringComparisonType>(idx);
		break;
	case ComparisonType::MultiString:
		_editFilterItem.MultiStringComparison = static_cast<MultiStringComparisonType>(idx);
		break;
	case ComparisonType::List: 
		// nothing to do, single value
		break;
	case ComparisonType::CheckList: 
		_editFilterItem.ChecklistComparison = static_cast<ChecklistComparisonType>(idx);
		break;
	default: ;
	}

}

void EditSdltmFilter::onAndOrComboChanged(int idx)
{
	if (_ignoreUpdate > 0)
		return;

	auto isAnd = ui->andorCombo->currentIndex() == 0;
	_editFilterItem.IsAnd = isAnd;
}

void EditSdltmFilter::onNotChecked(int state)
{
	if (_ignoreUpdate > 0)
		return;

	_editFilterItem.IsNegated = ui->notBox->isChecked();
}

void EditSdltmFilter::onCaseSensitiveChanged(int state)
{
	if (_ignoreUpdate > 0)
		return;

	_editFilterItem.CaseSensitive = ui->caseSensitive->isChecked();
}


void EditSdltmFilter::onIndentChanged()
{
	if (_ignoreUpdate > 0)
		return;

	_editFilterItem.IndentLevel = ui->indentSpin->value();
}

void EditSdltmFilter::onFilterSave()
{
	if (_ignoreUpdate > 0)
		return;

	// the idea -> even if the user clicks on a different row, we still want to save it
	assert(_editRowIndex >= 0);
	auto idx = _editRowIndex;

	auto isAndChanged = _editableFilterItems[idx].IsAnd != _editFilterItem.IsAnd;
	_editableFilterItems[idx] = _editFilterItem;
	FilterItemToRow(_editableFilterItems[idx], static_cast<QStandardItemModel*>(ui->simpleFilterTable->model()), idx, idx == _editableFilterItems.size() - 1);
	if (idx == _editableFilterItems.size() - 1 && idx > 0 && _isNew)
		// the idea - we have a new item appended at the end - thus, we now need to show the AND/OR of the former end (which was empty)
		FilterItemToRow(_editableFilterItems[idx - 1], static_cast<QStandardItemModel*>(ui->simpleFilterTable->model()), idx - 1, false);

	if (isAndChanged)
	{
		// update all adjacent rows at the same indent level
		for (int i = idx - 1; i >= 0; --i)
			if (_editableFilterItems[i].IndentLevel == _editFilterItem.IndentLevel)
			{
				_editableFilterItems[i].IsAnd = _editFilterItem.IsAnd;
				FilterItemToRow(_editableFilterItems[i],
					static_cast<QStandardItemModel*>(ui->simpleFilterTable->model()), i, i == _editableFilterItems.size() - 1);
			}
			else
				break;

		for (int i = idx + 1; i < _editableFilterItems.size() ; ++i)
			if (_editableFilterItems[i].IndentLevel == _editFilterItem.IndentLevel)
			{
				_editableFilterItems[i].IsAnd = _editFilterItem.IsAnd;
				FilterItemToRow(_editableFilterItems[i],
					static_cast<QStandardItemModel*>(ui->simpleFilterTable->model()), i, i == _editableFilterItems.size() - 1);
			}
			else
				break;
	}

	SaveFilter();
	ui->disabledBg->hide();
	ui->editCondition->hide();
	_editRowIndex = -1;
	_isNew = false;
}

void EditSdltmFilter::onFilterCancel()
{
	if (_ignoreUpdate > 0)
		return;

	// the idea -> even if the user clicks on a different row, we still want to save it
	assert(_editRowIndex >= 0);
	auto idx = _editRowIndex;
	_editableFilterItems[idx] = _originalEditFilterItem;
	ui->disabledBg->hide();
	ui->editCondition->hide();

	if (_isNew) {
		onDelItem();
	}

	_editRowIndex = -1;
	_isNew = false;
}

void EditSdltmFilter::onQuickSourceTextChanged()
{
	if (_ignoreUpdate > 0)
		return;

	_filter.QuickSearch = ui->quickSearchSource->text();
	SaveFilter();
}

void EditSdltmFilter::onQuickTargetTextChanged()
{
	if (_ignoreUpdate > 0)
		return;

	_filter.QuickSearchTarget = ui->quickSearchTarget->text();
	SaveFilter();
}

void EditSdltmFilter::onQuickSourceAndTargetChanged()
{
	if (_ignoreUpdate > 0)
		return;

	_filter.QuickSearchSearchSourceAndTarget = ui->quickSearchSourceAndTarget->isChecked();
	UpdateQuickSearchVisibility();
	SaveFilter();
}

void EditSdltmFilter::onQuickCaseSensitiveChanged()
{
	if (_ignoreUpdate > 0)
		return;

	_filter.QuickSearchCaseSensitive = ui->quickSearchCaseSensitive->isChecked();
	SaveFilter();
}

void EditSdltmFilter::onQuickWholeWordChanged() {
	if (_ignoreUpdate > 0)
		return;

	_filter.QuickSearchWholeWordOnly = ui->quickSearchWholeWordOnly->isChecked();
	SaveFilter();
}

void EditSdltmFilter::onQuickRegexChanged() {
	if (_ignoreUpdate > 0)
		return;
	ui->quickSearchCaseSensitive->setEnabled(!ui->quickSearchRegex->isChecked());
	ui->quickSearchWholeWordOnly->setEnabled(!ui->quickSearchRegex->isChecked());

	_filter.QuickSearchUseRegex = ui->quickSearchRegex->isChecked();
	SaveFilter();
}

void EditSdltmFilter::onFilterTextChanged()
{
	if (_ignoreUpdate > 0)
		return;

	_editFilterItem.FieldValue = ui->textValue->text();
	SaveFilter();
}

void EditSdltmFilter::onFilterDateTimeChanged()
{
	if (_ignoreUpdate > 0)
		return;

	_editFilterItem.FieldValue = ui->dateValue->dateTime().toString(Qt::ISODate);
	SaveFilter();
}

void EditSdltmFilter::onFilterComboChanged()
{
	if (_ignoreUpdate > 0)
		return;

	_editFilterItem.FieldValue = ui->comboValue->currentText();
	SaveFilter();
}

void EditSdltmFilter::onFilterComboCheckboxChanged()
{
	if (_ignoreUpdate > 0)
		return;

	auto checkedStates = ui->comboCheckboxesValue->checkStates();
	auto valuesIt = std::find_if(_customFields.begin(), _customFields.end(), [this](const CustomField& f) { return f.FieldName == _editFilterItem.CustomFieldName; });
	const auto & values = valuesIt->Values;

	int curIdx = 0;
	std::vector<QString> FieldValues;
	for (const auto & state : checkedStates)
	{
		if (state)
			FieldValues.push_back(values[curIdx]);
		++curIdx;
	}

	_editFilterItem.FieldValues = FieldValues;
	SaveFilter();
}


SdltmFilterItem EditSdltmFilter::NewItem(int basedOnItemIdx) {
	SdltmFilterItem newItem(SdltmFieldType::LastModifiedOn);
	if (basedOnItemIdx >= 0)
	{
		newItem.IndentLevel = _editableFilterItems[basedOnItemIdx].IndentLevel;
		newItem.IsAnd = _editableFilterItems[basedOnItemIdx].IsAnd;
	}
	return newItem;
}


void EditSdltmFilter::onAddItem()
{
	ui->editCondition->hide();
	SdltmFilterItem newItem = NewItem( _editableFilterItems.size() - 1);
	_editableFilterItems.push_back(newItem);

	auto model = (QStandardItemModel*)ui->simpleFilterTable->model();
	model->setRowCount(_editableFilterItems.size());
	FilterItemToRow(newItem, model, _editableFilterItems.size() - 1, true);
	ui->simpleFilterTable->scrollTo(model->item(_editableFilterItems.size() - 1)->index());

	_editRowIndex = _editableFilterItems.size() - 1;
	_isNew = true;
	EditRow(_editRowIndex);
}

void EditSdltmFilter::onInsertItem()
{
	ui->editCondition->hide();
	auto editIdx = _editRowIndex;
	if (editIdx < 0)
		editIdx = ui->simpleFilterTable->currentIndex().row();
	if (editIdx < 0 && _editableFilterItems.size() > 0)
		editIdx = 0;

	if (editIdx < 0) {
		onAddItem();
		return;
	}

	SdltmFilterItem newItem = NewItem(editIdx);
	_editableFilterItems.insert(_editableFilterItems.begin() + editIdx, newItem);

	auto model = (QStandardItemModel*)ui->simpleFilterTable->model();
	QList<QStandardItem*> list;
	list.append(nullptr);
	model->insertRow(editIdx, list);
	FilterItemToRow(newItem, model, editIdx, true);

	_editRowIndex = editIdx;
	_isNew = true;
	EditRow(_editRowIndex);
}

void EditSdltmFilter::onDelItem()
{
	ui->editCondition->hide();

	if (_editRowIndex < 0)
		_editRowIndex = ui->simpleFilterTable->currentIndex().row();

	if (_editRowIndex >= 0)
	{
		_editableFilterItems.erase(_editableFilterItems.begin() + _editRowIndex);
		ui->simpleFilterTable->model()->removeRow(_editRowIndex);
		_editRowIndex = -1;
		SaveFilter();
	}
}

namespace 
{
	const std::wstring WHITESPACE = L" \n\r\t\f\v";

	std::wstring ltrim(const std::wstring& s)
	{
		size_t start = s.find_first_not_of(WHITESPACE);
		return (start == std::wstring::npos) ? L"" : s.substr(start);
	}

	std::wstring rtrim(const std::wstring& s)
	{
		size_t end = s.find_last_not_of(WHITESPACE);
		return (end == std::wstring::npos) ? L"" : s.substr(0, end + 1);
	}

	std::wstring trim(const std::wstring& s) {
		return rtrim(ltrim(s));
	}

	bool SameStringLineByLine(const QString &a, const QString & b)
	{
		std::vector<std::wstring> aLines, bLines;
		tokenize(a.toStdWString(), L'\r', aLines);
		tokenize(b.toStdWString(), L'\r', bLines);
		aLines.erase( std::remove_if(aLines.begin(), aLines.end(), [](const std::wstring& s) { return trim(s).empty(); }), aLines.end());
		bLines.erase( std::remove_if(bLines.begin(), bLines.end(), [](const std::wstring& s) { return trim(s).empty(); }), bLines.end());
		if (aLines.size() != bLines.size())
			return false;

		for (int i = 0; i < aLines.size(); ++i)
			if (trim(aLines[i]) != trim(bLines[i]))
				return false;

		return true;
	}
}

void EditSdltmFilter::onTabChanged(int tabIndex)
{
	if (_ignoreUpdate > 0)
		return;

	bool isSimple = tabIndex == 0;
	bool isAdvanced = tabIndex == 1;
	SdltmCreateSqlSimpleFilter createFilter(_filter, _customFields);
	if (isSimple)
	{
		// advanced to simple
		_saveAdvancedFilterTimer->stop();

		auto simple = createFilter.ToSqlFilter();
		auto advanced = ui->advancedEdit->text();
		if (!SameStringLineByLine(simple, advanced))
		{
			// user modified something non-trivial in Advanced tab
			_filter.AdvancedSql = advanced;
			EnableSimpleTab(false);
		} else
			EnableSimpleTab(true);
	}
	else if (isAdvanced)
	{
		// simple to advanced
		if (ui->editCondition->isVisible())
			onFilterCancel();
		EnableSimpleTab(true);
		ui->advancedEdit->setText(FilterString());

		_saveAdvancedFilterTimer->start(100);
	}
	else
		assert(false);
}

void EditSdltmFilter::onReEnableClick()
{
	_filter.AdvancedSql = "";
	EnableSimpleTab(true);
}

void EditSdltmFilter::onSaveAdvancedFilterTimer()
{
	auto curText = ui->advancedEdit->text();
	if (SameStringLineByLine(curText, FilterString()))
		// in this case -> user hasn't yet modified anything consequential
		return;

	if (_filter.AdvancedSql != curText && (QDateTime::currentDateTime().toMSecsSinceEpoch() - _lastAdvancedTextChange.toMSecsSinceEpoch()) > 1000)
	{
		_lastAdvancedTextChange = QDateTime::currentDateTime();
		_filter.AdvancedSql = curText;
		SaveFilter();
	}
}

void EditSdltmFilter::onApplyFilterTimer()
{
	if (CanApplyCurrentFilter() )
		ReapplyFilter();
}

void EditSdltmFilter::ReapplyFilter() {
	auto filterText = FilterString();
	_lastFilterString = filterText;
	_lastFilterChange = QDateTime::currentDateTime();
	if (OnApply)
		OnApply(filterText);
}


void EditSdltmFilter::EnableSimpleTab(bool enable)
{
	if (enable)
	{
		ui->disabledBg->hide();
		ui->reenableSimpleFilterGrid->hide();
	}
	else
	{
		auto size = ui->tabWidget->size();
		ui->disabledBg->setGeometry(0, HEADER_HEIGHT, size.width(), size.height() - HEADER_HEIGHT);
		ui->reenableSimpleFilterGrid->setGeometry(0, HEADER_HEIGHT, size.width(), size.height() - HEADER_HEIGHT);
		ui->disabledBg->show();
		ui->reenableSimpleFilterGrid->show();
	}
}

bool EditSdltmFilter::CanApplyCurrentFilter()
{
	if (ui->editCondition->isVisible())
		// user is editing a filter item
		return false;

	if ((QDateTime::currentDateTime().toMSecsSinceEpoch() - _lastFilterChange.toMSecsSinceEpoch()) < 2000)
		return false;

	if (IsQueryRunning)
		if (IsQueryRunning())
			return false; // wait for current query to end

	auto filterText = FilterString();
	if (SameStringLineByLine(filterText, _lastFilterString))
		return false; // user hasn't changed anything

	return true;
}

QString EditSdltmFilter::FilterString() const
{
	if (_filter.IsEmpty())
		return "";

	SdltmCreateSqlSimpleFilter createFilter(_filter, _customFields);
	auto filterText = _filter.AdvancedSql != "" ? _filter.AdvancedSql : createFilter.ToSqlFilter();
	return filterText;
}

