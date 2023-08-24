#pragma once
#include <qobjectdefs.h>
#include <QString>
#include <QWidget>

#include "CustomFieldService.h"

namespace Ui {
	class EditCustomFields;
}

class EditCustomFields : public QWidget 
{
Q_OBJECT

public:
	explicit EditCustomFields(QWidget* parent = nullptr);
	~EditCustomFields() override;

	std::function<void(CustomFieldValue & oldValue, const CustomFieldValue & newValue)> Save;

	void SetFieldValues(const std::vector<CustomFieldValue>& values, const std::vector<CustomField> & allFields);
	void SetIsEditingSdltmQuery(bool isEditingSdltmQuery);

	bool IsEditingSdltmQuery() const { return _isEditingSdltmQuery; }
protected:
	void resizeEvent(QResizeEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* event) override;
	void focusInEvent(QFocusEvent* event) override;

private slots:
	void EditSave();
	void EditCancel();
	void OnTableClicked(const QModelIndex&);
private:
	void FieldToEdit();
	void EditToField();

private:
	Ui::EditCustomFields* ui;

	bool _isEditingSdltmQuery = false;

	std::vector< CustomFieldValue> _oldValues;
	std::vector< CustomFieldValue> _newValues;

	int _editIdx = -1;

	int _ignoreUpdate = 0;
};

