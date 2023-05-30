#pragma once
#include <functional>
#include <memory>
#include <QWidget>

#include "SdltmFilter.h"

namespace Ui {
	class SdltmFiltersList;
}

class SdltmFiltersList : public QWidget
{
	Q_OBJECT
public:
	explicit SdltmFiltersList(QWidget* parent = nullptr);

	std::function<void(const std::vector<SdltmFilter>&)> Save;
	std::function<void(const SdltmFilter&)> Edit;

	void SetFilters(const std::vector<SdltmFilter>& filters);
	void SaveEdit(const SdltmFilter& filter);
private:
	void SaveFilters();
	void AddFilter(const SdltmFilter& filter);
	void EditFilter();

private slots:
	void OnAdd();
	void OnDel();
	void OnCopy();
	void OnReset();
	void OnImport();
	void OnExport();
	void OnFilterChange(int);
	void OnFilterNameChange(const QString& text);

private:
	Ui::SdltmFiltersList* ui;
	std::vector<SdltmFilter> _filters;
	int _ignoreUpdate;
	int _editIdx;
};

