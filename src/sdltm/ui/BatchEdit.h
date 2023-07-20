#pragma once
#include <functional>
#include <QDateTime>
#include <QWidget>
#include "CustomFieldService.h"

namespace Ui {
	class BatchEdit;
}

struct FindAndReplaceTextInfo {
	QString Find;
	QString Replace;
	enum class SearchType {
		Source, Target, Both,
	};
	SearchType Type = SearchType::Both;
	bool MatchCase = false;
	bool WholeWordOnly = false;
	bool UseRegex = false;
};

struct FindAndReplaceFieldInfo {
	CustomField EditField;

	QString OldText;
	QString NewText;

	QDateTime OldDate = QDateTime(QDate(1990,1,1));
	QDateTime NewDate;

	int OldComboIndex = -1;
	int NewComboIndex = -1;

	std::vector<int> OldCheckIndexes;
	std::vector<int> NewCheckIndexes;

	std::vector<QString> OldMultiText;
	std::vector<QString> NewMultiText;

	QString OldValue() const;
	QString NewValue() const;

	bool HasOldValue() const;

	int NewComboId() const {
		return IndexToId(NewComboIndex);
	}

	std::vector<QString> OldComboNames() const {
		std::vector<QString> names;
		for (const auto& index : OldCheckIndexes)
			if (IndexToId(index) >= 0)
				names.push_back(EditField.Values[index]);
		return names;
	}
	std::vector<QString> NewComboNames() const {
		std::vector<QString> names;
		for (const auto& index : NewCheckIndexes)
			if (IndexToId(index) >= 0)
				names.push_back(EditField.Values[index]);
		return names;
	}

	int OldComboId() const {
		return IndexToId(OldComboIndex);
	}

	std::vector<int> OldComboIds() const {
		std::vector<int> ids;
		for (const auto& index : OldCheckIndexes)
			if (IndexToId(index) >= 0)
				ids.push_back(IndexToId(index));
		return ids;
	}

	std::vector<int> NewComboIds() const {
		std::vector<int> ids;
		for (const auto& index : NewCheckIndexes)
			if (IndexToId(index) >= 0)
				ids.push_back(IndexToId(index));
		return ids;
	}
private:
	int IndexToId(int idx) const;
	QString Value(SdltmFieldMetaType fieldType, const QString& text, const QDateTime& date, int comboValue) const;
};

class BatchEdit : public QWidget
{
	Q_OBJECT
public:
	explicit BatchEdit(QWidget* parent = nullptr);
	~BatchEdit() override;

	std::function<void()> Back;
	std::function<void(const FindAndReplaceTextInfo&)> FindAndReplaceText;
	std::function<void(const FindAndReplaceFieldInfo&)> FindAndReplaceField;
	std::function<void(const CustomField&)> FindAndReplaceDeleteField;
	std::function<void()> FindAndReplaceDeleteTags;

	// note: perhaps we won't need this
	std::function<void(const FindAndReplaceTextInfo&)> Preview;

	void SetCustomFields(const std::vector<CustomField>& fields);

private:
	FindAndReplaceTextInfo GetFindAndReplaceTextInfo() const;
	FindAndReplaceFieldInfo GetFindAndReplaceEditInfo() const;

	void UpdateEditFieldTypeVisibility();

private slots:
	void OnClickPreview();
	void OnClickRun();
	void OnClickBack();
	void OnUseRegexChanged();

	void OnFieldChange();
	void OnDelFieldChange();

private:
	Ui::BatchEdit* ui;
	std::vector<CustomField> _customFields;
	int _ignoreUpdate = 0;
	CustomField _editField;

	std::vector<bool> _oldChecklistStates;
	std::vector<bool> _newCheckListStates;

	std::vector<QString> _oldMultiText;
	std::vector<QString> _newMultiText;

	CustomField _delField;
};

