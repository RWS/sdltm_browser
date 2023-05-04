#pragma once
#include <memory>
#include <QWidget>

#include "SdltmFilter.h"

namespace Ui {
class EditSdltmFilter;
}

class EditSdltmFilter : public QWidget
{
    Q_OBJECT
public:
    explicit EditSdltmFilter(QWidget* parent = nullptr);
    ~EditSdltmFilter() override;

    void resizeEvent(QResizeEvent* event) override;

    void setEditFilter(std::shared_ptr<SdltmFilter> filter, const std::vector<CustomField> customFields);

private:
    void saveFilter();
    void editRow(int idx);
    void showEditControlsForRow(int idx);
    void updateFieldCombo();
    void updateOperationCombo(int idx);

private slots:
    void onTableClicked(const QModelIndex&);

private:
    Ui::EditSdltmFilter* ui;
    std::shared_ptr< SdltmFilter > _filter;
    // these are the custom fields available for this database
    std::vector<CustomField> _customFields;

    std::vector<SdltmFilterItem> _hiddenFilterItems;
    std::vector<SdltmFilterItem> _editFilterItems;

};


