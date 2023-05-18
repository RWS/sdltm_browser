#pragma once
#include <functional>
#include <memory>
#include <QDateTime>
#include <QWidget>

#include "SdltmFilter.h"

class QGraphicsOpacityEffect;

namespace Ui {
class EditSdltmFilter;
}

class EditSdltmFilter : public QWidget
{
    Q_OBJECT
public:
    std::function<void()> OnSave;
    std::function<void(const QString&)> OnApply;
    std::function<bool()> IsQueryRunning;
public:
    explicit EditSdltmFilter(QWidget* parent = nullptr);
    ~EditSdltmFilter() override;

    void resizeEvent(QResizeEvent* event) override;

    void setEditFilter(std::shared_ptr<SdltmFilter> filter, const std::vector<CustomField> customFields);

private:
    void SaveFilter();
    void EditRow(int idx);
    void ShowEditControlsForRow(int idx);
    void UpdateFieldCombo();
    void UpdateOperationCombo();
    void UpdateValueVisibility();
    void UpdateValue();

    void UpdateQuickSearchVisibility();
    void EnableSimpleTab(bool enable);

    bool CanApplyCurrentFilter();
    QString FilterString() const;

private slots:
    void onTableClicked(const QModelIndex&);

    void onFieldComboChanged(int idx);
    void onOperationComboChanged(int idx);
    void onAndOrComboChanged(int idx);

    void onNotChecked(int);
    void onCaseSensitiveChanged(int);

    void onIndentChanged();

    void onFilterSave();
    void onFilterCancel();

    void onQuickSourceTextChanged();
    void onQuickTargetTextChanged();
    void onQuickSourceAndTargetChanged();
    void onQuickCaseSensitiveChanged();

    void onFilterTextChanged();
    void onFilterDateTimeChanged();
    void onFilterComboChanged();
    void onFilterComboCheckboxChanged();

    void onAddItem();
    void onInsertItem();
    void onDelItem();

    void onTabChanged(int tabIndex);
    void onReEnableClick();

    void onSaveAdvancedFilterTimer();
    void onApplyFilterTimer();

private:
    int _ignoreUpdate;

    QDateTime _lastAdvancedTextChange;
    QString _lastFilterString;
    QTimer* _saveAdvancedFilterTimer;
    QTimer* _applyFilterTimer;

    QString _lastAppliedFilter;
    QDateTime _lastFilterChange;


    Ui::EditSdltmFilter* ui;
    std::shared_ptr< SdltmFilter > _filter;
    // these are the custom fields available for this database
    std::vector<CustomField> _customFields;

    std::vector<SdltmFilterItem> _hiddenFilterItems;
    std::vector<SdltmFilterItem> _editableFilterItems;

    int _editRowIndex;
    SdltmFilterItem _originalEditFilterItem;
    SdltmFilterItem _editFilterItem;
};


