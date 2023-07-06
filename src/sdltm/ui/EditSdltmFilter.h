#pragma once
#include <functional>
#include <memory>
#include <QDateTime>
#include <QWidget>

#include "CustomFieldService.h"
#include "SdltmFilter.h"

class QGraphicsOpacityEffect;

namespace Ui {
class EditSdltmFilter;
}

class EditSdltmFilter : public QWidget
{
    Q_OBJECT
public:
    std::function<void(const SdltmFilter &filter)> OnSave;
    std::function<void(const QString&)> OnApply;
    std::function<bool()> IsQueryRunning;
public:
    explicit EditSdltmFilter(QWidget* parent = nullptr);
    ~EditSdltmFilter() override;

    void resizeEvent(QResizeEvent* event) override;

    void SetEditFilter(const SdltmFilter& filter);
    void SetCustomFields(const std::vector<CustomField> customFields);
    void Close();

    void ForceSaveNow();
    void ReapplyFilter();
private:
    SdltmFilterItem NewItem(int basedOnItemIdx);

    void UpdateCustomExpressionUserEditableArgs();
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
    void onQuickWholeWordChanged();
    void onQuickRegexChanged();

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
    int _ignoreUpdate = 0;

    QDateTime _lastAdvancedTextChange;
    QString _lastFilterString;
    QTimer* _saveAdvancedFilterTimer;
    QTimer* _applyFilterTimer;

    QString _lastAppliedFilter;
    QDateTime _lastFilterChange;


    Ui::EditSdltmFilter* ui;
    SdltmFilter  _filter;
    // these are the custom fields available for this database
    std::vector<CustomField> _customFields;

    std::vector<SdltmFilterItem> _hiddenFilterItems;
    std::vector<SdltmFilterItem> _userEditableArgsItems;
    std::vector<SdltmFilterItem> _editableFilterItems;

    int _editRowIndex = -1;

    // if true, it's an INS or ADD
    bool _isNew = false;

    SdltmFilterItem _originalEditFilterItem;
    SdltmFilterItem _editFilterItem;

    // true - we allow editing custom users filters (custom sql expressions that contain user-editable values)
    //        in this case, we visually show those custom sql expressions and hide the user-editable values
    //
    // false (default) - we don't allow editing users filters
    //                   in this case, the user only sees the user-editable values
    static const bool EditCustomExpressionWithUserEditableArgs = false;// true;
};



