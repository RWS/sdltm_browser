#pragma once
#include <QWidget>
#include <functional>

class QListWidgetItem;

namespace Ui {
	class MultiTextEdit;
}

class MultiTextEdit : public QWidget 
{
	Q_OBJECT
public:
	explicit MultiTextEdit(QWidget* parent = nullptr);
	~MultiTextEdit() override;

	void SetMultiText(const std::vector<QString> &texts);
	std::vector<QString> GetMultiText() const;

	void resizeEvent(QResizeEvent* event) override;
private:
	void AddNewItemRow();
	void UiToTexts();
private slots:
	void OnItemClicked(QListWidgetItem* item);
	void OnItemChanged(QListWidgetItem* item);
	void OnSelectionChanged();
	void OnDelete();

public:
	// called on add/del/edit
	std::function<void()> OnMultiTextChange;
private:
	Ui::MultiTextEdit* ui;
	int _ignoreUpdate = 0;
	std::vector<QString> _texts;
};

