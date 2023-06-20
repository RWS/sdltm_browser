#pragma once
#include <functional>
#include <QWidget>

namespace Ui {
	class BatchEdit;
}

struct FindAndReplaceInfo {
	QString Find;
	QString Replace;
	enum class SearchType {
		Source, Target, Both,
	};
	SearchType Type = SearchType::Both;
	bool MatchCase = false;
};

class BatchEdit : public QWidget
{
	Q_OBJECT
public:
	explicit BatchEdit(QWidget* parent = nullptr);
	~BatchEdit() override;

	std::function<void()> Back;
	std::function<void(const FindAndReplaceInfo&)> FindAndReplace;
	std::function<void(const FindAndReplaceInfo&)> Preview;

private:
	FindAndReplaceInfo GetFindAndReplaceInfo() const;
private slots:
	void OnClickPreview();
	void OnClickRun();
	void OnClickBack();

private:
	Ui::BatchEdit* ui;
};

