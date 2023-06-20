#include "SdltmFiltersList.h"

#include "SdltmUtil.h"
#include "ui_PlotDock.h"
#include "ui_SdltmFiltersList.h"

SdltmFiltersList::SdltmFiltersList(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::SdltmFiltersList)
{
	ui->setupUi(this);

	connect(ui->add, SIGNAL(clicked(bool)), this, SLOT( OnAdd()));
	connect(ui->del, SIGNAL(clicked(bool)), this, SLOT(OnDel()));
	connect(ui->copy, SIGNAL(clicked(bool)), this, SLOT(OnCopy()));
	connect(ui->reset, SIGNAL(clicked(bool)), this, SLOT(OnReset()));
	connect(ui->importFilters, SIGNAL(clicked(bool)), this, SLOT(OnImport()));
	connect(ui->exportFilters, SIGNAL(clicked(bool)), this, SLOT(OnExport()));

	connect(ui->filterCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(OnFilterChange(int)));
	connect(ui->filterCombo, SIGNAL(editTextChanged(const QString&)), this, SLOT(OnFilterNameChange(const QString&)));

	_saveTimer = new QTimer(this);
	connect(_saveTimer, SIGNAL(timeout()), this, SLOT(OnCheckSave()));
	_saveTimer->start(2000);
}

SdltmFiltersList::~SdltmFiltersList() {
	delete ui;
}

void SdltmFiltersList::SetFilters(const std::vector<SdltmFilter>& filters)
{
	_filters = filters;
	++_ignoreUpdate;
	ui->filterCombo->clear();
	for (const auto& filter : _filters)
		ui->filterCombo->addItem(filter.Name);
	--_ignoreUpdate;

	// set the first one - if not empty
	if (!filters.empty())
	{
		ui->filterCombo->setCurrentIndex(-1);
		ui->filterCombo->setCurrentIndex(0);
	}
	else
		OnAdd();
}

// called when the user had changed something in the current filter
void SdltmFiltersList::SaveEdit(const SdltmFilter& filter)
{
	assert(_editIdx >= 0);
	if (_editIdx >= 0 && _editIdx < _filters.size())
		_filters[_editIdx] = filter;
	SaveFilters();
}


void SdltmFiltersList::SaveFilters()
{
	_needsSave = true;
}

void SdltmFiltersList::AddFilter(const SdltmFilter& filter)
{
	++_ignoreUpdate;
	_filters.push_back(filter);
	ui->filterCombo->addItem(filter.Name);
	--_ignoreUpdate;

	// ... just in case already selected, trigger a "change" event
	ui->filterCombo->setCurrentIndex(-1);
	ui->filterCombo->setCurrentIndex(_filters.size() - 1);
}

void SdltmFiltersList::EditFilter()
{
	if (Edit && _editIdx >= 0)
		Edit(_filters[_editIdx]);

	DebugWriteLine("Editing filter " + QString::number(_editIdx));
}

void SdltmFiltersList::Close()
{
	if (Save)
		Save(_filters);
}

void SdltmFiltersList::OnAdd()
{
	// find unique name
	auto idx = _filters.size() + 1;
	QString name = "Filter " + QString::number(idx);
	while (std::find_if(_filters.begin(), _filters.end(), [name](const SdltmFilter& f) { return f.Name == name; }) != _filters.end())
		name = "Filter " + QString::number(++idx);

	SdltmFilter filter;
	filter.Name = name;
	AddFilter(filter);
}

void SdltmFiltersList::OnDel()
{
	++_ignoreUpdate;
	_filters.erase(_filters.begin() + _editIdx);
	ui->filterCombo->removeItem(_editIdx);
	if (_editIdx >= _filters.size())
		_editIdx = _filters.size() - 1;
	--_ignoreUpdate;

	if (!_filters.empty())
	{
		// ... just in case already selected, trigger a "change" event
		auto editIdx = _editIdx;
		ui->filterCombo->setCurrentIndex(-1);
		ui->filterCombo->setCurrentIndex(editIdx);
	}
	else
		OnAdd();
}

void SdltmFiltersList::OnCopy()
{
	if (_filters.empty())
	{
		OnAdd();
		return;
	}

	// find unique name
	auto idx = 1;
	auto prefix = _filters[_editIdx].Name + " Copy";
	QString name = prefix;
	while (std::find_if(_filters.begin(), _filters.end(), [name](const SdltmFilter& f) { return f.Name == name; }) != _filters.end())
		name = prefix + " (" + QString::number(++idx) + ")";

	SdltmFilter filter = _filters[_editIdx];
	filter.Name = name;
	AddFilter(filter);
}

void SdltmFiltersList::OnReset()
{
	if (QMessageBox::question(this, QApplication::applicationName(), "Are you sure you want to reset the filters list to default?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		auto filters = LoadFilters(DefaultFiltersFile());
		SetFilters(filters);
		SaveFilters();
	}
}

void SdltmFiltersList::OnImport()
{
	QString file = QFileDialog::getOpenFileName(this, QApplication::applicationName(), AppDir(), "SDLTM Filter files (*.sdlfilters)");
	if (file != "")
	{
		// do import
		auto import = ::LoadFilters(file);
		for(auto & filter : import)
		{
			bool nameOk = false;
			QString name;
			for (int suffix = 0; !nameOk ; ++suffix)
			{
				name = filter.Name + (suffix > 0 ? " Copy " + QString::number(suffix) : "");
				nameOk = std::find_if(_filters.begin(), _filters.end(), [name](const SdltmFilter& f) { return f.Name == name; }) == _filters.end();
			}
			filter.Name = name;
			_filters.push_back(filter);
		}
	}
	SetFilters(_filters);
	SaveFilters();
}

void SdltmFiltersList::OnExport()
{
	QString file = QFileDialog::getSaveFileName(this, QApplication::applicationName(), AppDir(), "SDLTM Filter files (*.sdlfilters)");
	if (file != "")
	{
		// do export
		::SaveFilters(_filters, file);
	}
}

void SdltmFiltersList::OnFilterChange(int idx)
{
	if(_ignoreUpdate > 0)
		return;
	_editIdx = idx;
	if (_editIdx < 0)
		return;

	EditFilter();
}

void SdltmFiltersList::OnFilterNameChange(const QString& text)
{
	if (_ignoreUpdate > 0)
		return;
	if (_editIdx < 0)
		return;

	if(std::find_if(_filters.begin(), _filters.end(), [text](const SdltmFilter& f) { return f.Name == text; }) != _filters.end())
		// there's no other way to find out if user manually moved to another combo item
		// (the OnFilterChange triggers too late)
		return;

	if (ui->filterCombo->lineEdit()-> hasFocus())
	{
		_filters[_editIdx].Name = text;

		++_ignoreUpdate;
		ui->filterCombo->removeItem(_editIdx);
		ui->filterCombo->insertItem(_editIdx, text);
		ui->filterCombo->setCurrentIndex(_editIdx);
		--_ignoreUpdate;

		SaveFilters();
	}
}

void SdltmFiltersList::OnCheckSave()
{
	if(!_needsSave)
		return;
	_needsSave = false;
	if (Save)
		Save(_filters);
}
