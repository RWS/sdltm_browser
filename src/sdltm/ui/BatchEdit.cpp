#include "BatchEdit.h"

#include "ui_BatchEdit.h"

BatchEdit::BatchEdit(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::BatchEdit)
{
	ui->setupUi(this);

	connect(ui->preview, SIGNAL(clicked(bool)), this, SLOT(OnClickPreview()));
	connect(ui->run, SIGNAL(clicked(bool)), this, SLOT(OnClickRun()));
	connect(ui->back, SIGNAL(clicked(bool)), this, SLOT(OnClickBack()));

	connect(ui->findUseRegex, SIGNAL(stateChanged(int)), this, SLOT(OnUseRegexChanged()));

	ui->findSearchInBoth->setChecked(true);
}

BatchEdit::~BatchEdit() {
	delete ui;
}

FindAndReplaceInfo BatchEdit::GetFindAndReplaceInfo() const {
	FindAndReplaceInfo far;
	far.Find = ui->findText->text();
	far.Replace = ui->replaceWithText->text();

	far.MatchCase = ui->findMatchCase->isChecked();
	far.WholeWordOnly = ui->findWordOnly->isChecked();
	far.UseRegex = ui->findUseRegex->isChecked();

	far.Type = FindAndReplaceInfo::SearchType::Both;
	if (ui->findSearchInSource->isChecked())
		far.Type = FindAndReplaceInfo::SearchType::Source;
	else if (ui->findSearchInTarget->isChecked())
		far.Type = FindAndReplaceInfo::SearchType::Target;
	return far;
}

void BatchEdit::OnClickPreview() {
	if (Preview)
		Preview(GetFindAndReplaceInfo());
}

void BatchEdit::OnClickRun() {
	if (FindAndReplace)
		FindAndReplace(GetFindAndReplaceInfo());
}

void BatchEdit::OnClickBack() {
	if (Back)
		Back();
}

void BatchEdit::OnUseRegexChanged() {
	ui->findMatchCase->setEnabled(!ui->findUseRegex->isChecked());
	ui->findWordOnly->setEnabled(!ui->findUseRegex->isChecked());
}
