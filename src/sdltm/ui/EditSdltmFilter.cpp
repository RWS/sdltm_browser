#include "EditSdltmFilter.h"

#include <iostream>
#include <qstandarditemmodel.h>

#include "ui_EditSdltmFilter.h"
#include "ui_PlotDock.h"

namespace 
{

	// for testing
	SdltmFilter TestFilter()
	{
		SdltmFilter test;
		test.FilterItems.push_back(SdltmFilterItem("Customer", SdltmFieldMetaType::Text));
		test.FilterItems.back().FieldValue = "Gigi";
		test.FilterItems.back().IndentLevel = 0;
		test.FilterItems.back().StringComparison = StringComparisonType::Contains;
		test.FilterItems.push_back(SdltmFilterItem("Country", SdltmFieldMetaType::Text));
		test.FilterItems.back().FieldValue = "Romania";
		test.FilterItems.back().IndentLevel = 1;
		test.FilterItems.push_back(SdltmFilterItem(SdltmFieldType::CreatedBy, SdltmFieldMetaType::Number));
		test.FilterItems.back().FieldValue = "5";
		test.FilterItems.back().IndentLevel = 2;
		test.FilterItems.back().IsAnd = false;
		test.FilterItems.back().NumberComparison = NumberComparisonType::BiggerOrEqual;
		test.FilterItems.push_back(SdltmFilterItem(SdltmFieldType::CreatedOn, SdltmFieldMetaType::Text));
		test.FilterItems.back().FieldValue = "2023/02/20";
		test.FilterItems.back().IndentLevel = 2;
		test.FilterItems.back().IsAnd = false;
		test.FilterItems.push_back(SdltmFilterItem(SdltmFieldType::SourceSegment, SdltmFieldMetaType::Text));
		test.FilterItems.back().FieldValue = "extra";
		test.FilterItems.back().IndentLevel = 2;
		test.FilterItems.back().StringComparison = StringComparisonType::Contains;
		test.FilterItems.back().IsAnd = false;
		test.FilterItems.back().IsNegated = true;
		test.FilterItems.push_back(SdltmFilterItem(SdltmFieldType::LastModifiedBy, SdltmFieldMetaType::Text));
		test.FilterItems.back().FieldValue = "JJ";
		test.FilterItems.back().IndentLevel = 1;
		test.FilterItems.push_back(SdltmFilterItem(SdltmFieldType::LastUsedBy, SdltmFieldMetaType::Text));
		test.FilterItems.back().FieldValue = "TT";
		test.FilterItems.back().IndentLevel = 1;
		test.FilterItems.back().StringComparison = StringComparisonType::EndsWith;


		return test;
	}

	std::vector<CustomField> TestCustomFields()
	{
		std::vector<CustomField> fields;
		CustomField a;
		a.FieldName = "Customer";
		CustomField b;
		b.FieldName = "Country";
		CustomField c;
		c.FieldName = "Price";
		c.FieldType = SdltmFieldMetaType::Number;
		fields.push_back(a);
		fields.push_back(b);
		fields.push_back(c);
		return fields;
	}
}

EditSdltmFilter::EditSdltmFilter(QWidget* parent ) 
	: QWidget(parent) 
    , ui(new Ui::EditSdltmFilter)
{
    ui->setupUi(this);
	ui->editCondition->hide();

	// thank you QT, for bringing back Mesozoic Era to all the rest of us...
	connect(ui->simpleFilterTable, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onTableClicked(const QModelIndex&)));

	ui->andorCombo->addItem("AND");
	ui->andorCombo->addItem("OR");




	// testing
	setEditFilter(std::make_shared<SdltmFilter>(TestFilter()), TestCustomFields());
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
}

namespace 
{
	void filterItemToRow(const SdltmFilterItem & item, QStandardItemModel * model, int idx, bool isLast)
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

	QStandardItemModel *createFilterItemModel(const std::vector<SdltmFilterItem> & items)
	{
		// Indent | NOT | Condition | And/Or
		QStandardItemModel* model = new QStandardItemModel(items.size(), 4);
		for (int row = 0; row < items.size(); ++row)
			filterItemToRow(items[row], model, row, row == items.size() - 1);

		model->setHorizontalHeaderItem(0, new QStandardItem("Indent"));
		model->setHorizontalHeaderItem(1, new QStandardItem("NOT"));
		model->setHorizontalHeaderItem(2, new QStandardItem("Condition"));
		model->setHorizontalHeaderItem(3, new QStandardItem("AND/OR"));
		return model;
	}
}

void EditSdltmFilter::setEditFilter(std::shared_ptr<SdltmFilter> filter, const std::vector<CustomField> customFields)
{
	_filter = filter;
	_customFields = customFields;

	for (auto& item : filter->FilterItems)
		if (item.CustomFieldName != "" && std::find_if(_customFields.begin(), _customFields.end(), 
			[item](const CustomField& f) { return f.FieldName == item.CustomFieldName; }) == _customFields.end())
			// custom field not available for this database
			item.IsVisible = false;

	if (filter->IsLocked)
	{
		// for Locked filters - only editable values are visible
		for (auto& item : filter->FilterItems)
			item.IsVisible = false;

		std::vector<SdltmFilterItem> metaItems;
		std::copy_if(filter->FilterItems.begin(), filter->FilterItems.end(), std::back_inserter(metaItems), [](const SdltmFilterItem& fi) { return fi.IsMetaFieldValue(); });
		for(const auto & meta : metaItems)
		{
			auto editableName = meta.MetaFieldValue();
			auto found = std::find_if(filter->FilterItems.begin(), filter->FilterItems.end(), [editableName](const SdltmFilterItem& fi) { return fi.CustomFieldName == editableName; });
			if (found != filter->FilterItems.end())
				found->IsVisible = true;
			else
				filter->FilterItems.push_back(meta.ToUserEditableFilterItem());
		}
	}

	ui->addSimpleFilterItem->setEnabled(!filter->IsLocked);
	ui->insertSimpleFilterItem->setEnabled(!filter->IsLocked);
	ui->delSimpleFilterItem->setEnabled(!filter->IsLocked);

	_editFilterItems.clear();
	_hiddenFilterItems.clear();
	std::copy_if(filter->FilterItems.begin(), filter->FilterItems.end(), std::back_inserter(_editFilterItems), [](const SdltmFilterItem& fi) { return fi.IsVisible; });
	std::copy_if(filter->FilterItems.begin(), filter->FilterItems.end(), std::back_inserter(_hiddenFilterItems), [](const SdltmFilterItem& fi) { return !fi.IsVisible; });

	ui->simpleFilterTable->setModel(createFilterItemModel(_editFilterItems));
	ui->simpleFilterTable->setColumnWidth(0, 60);
	ui->simpleFilterTable->setColumnWidth(1, 60);
	ui->simpleFilterTable->setColumnWidth(2, 450);
	ui->simpleFilterTable->setColumnWidth(3, 80);
}

void EditSdltmFilter::saveFilter()
{
	_filter->FilterItems.clear();
	std::copy(_hiddenFilterItems.begin(), _hiddenFilterItems.end(), std::back_inserter(_filter->FilterItems));
	std::copy(_editFilterItems.begin(), _editFilterItems.end(), std::back_inserter(_filter->FilterItems));
}

void EditSdltmFilter::editRow(int idx)
{
	showEditControlsForRow(idx);

	auto& item = _editFilterItems[idx];
	ui->indentSpin->setValue(item.IndentLevel);
	ui->notBox->setChecked(item.IsNegated);
	ui->andorCombo->setCurrentIndex(item.IsAnd ? 0 : 1);
	ui->textValue->setPlainText(item.FieldValue);

	updateFieldCombo();
	updateOperationCombo(idx);

	if (item.CustomFieldName != "")
	{
		auto field = std::find_if(_customFields.begin(), _customFields.end(), [item](const CustomField& cf) { return cf.FieldName == item.CustomFieldName; });
		auto idx = field - _customFields.begin();
		ui->fieldCombo->setCurrentIndex(idx);
	} else
	{
		// preset
		ui->fieldCombo->setCurrentIndex(_customFields.size() + (int)item.FieldType);
	}

	switch (item.FieldMetaType)
	{
		// number
	case SdltmFieldMetaType::Int:
	case SdltmFieldMetaType::Double:
	case SdltmFieldMetaType::Number:
	case SdltmFieldMetaType::DateTime:
		ui->operationCombo->setCurrentIndex((int)item.NumberComparison);
		break;

		// string
	case SdltmFieldMetaType::Text:
		ui->operationCombo->setCurrentIndex((int)item.StringComparison);
		break;

		// multi-string
	case SdltmFieldMetaType::MultiText:
		ui->operationCombo->setCurrentIndex((int)item.MultiStringComparison);
		break;

		// multi-comparison (has-item) -- single value
	case SdltmFieldMetaType::List:
		ui->operationCombo->setCurrentIndex(0);
		break;
		// multi-comparison (has-item) -- several values
	case SdltmFieldMetaType::CheckboxList:
		ui->operationCombo->setCurrentIndex(0);
		break;
	default:;
	}

}

void EditSdltmFilter::showEditControlsForRow(int idx)
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
	ui->editCondition->setFixedHeight(height);

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

void EditSdltmFilter::updateFieldCombo()
{
	ui->fieldCombo->clear();
	for (const auto& cf : _customFields)
		ui->fieldCombo->addItem(cf.FieldName);

	for (const auto& ft : _SdltmFieldTypes)
		ui->fieldCombo->addItem(ft);
}

void EditSdltmFilter::updateOperationCombo(int idx)
{
	std::vector<QString> operations;
	auto& item = _editFilterItems[idx];
	switch (item.FieldMetaType)
	{
		// number
	case SdltmFieldMetaType::Int: 
	case SdltmFieldMetaType::Double: 
	case SdltmFieldMetaType::Number: 
	case SdltmFieldMetaType::DateTime:
		operations = { "Equal", "Less", "Less or Equal", "Bigger", "Bigger or Equal"};
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
		operations = { "Has Any Of" };
		break;
	default: ;
	}

	ui->operationCombo->clear();
	for (const auto& o : operations)
		ui->operationCombo->addItem(o);
}


void EditSdltmFilter::onTableClicked(const QModelIndex& row)
{
	auto idx = row.row();
	editRow(idx);
}
