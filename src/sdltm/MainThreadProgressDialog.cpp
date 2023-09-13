#include "MainThreadProgressDialog.h"

#include <qcoreapplication.h>
#include <QProgressDialog>

namespace {
	const double MAXIMUM = 1000;
}

MainThreadProgressDialog::MainThreadProgressDialog(TaskFunc task) : _task(task) {
}

MainThreadProgressDialog::~MainThreadProgressDialog() {
}

void MainThreadProgressDialog::ShowDialog() {
	QProgressDialog* progress = new QProgressDialog(TaskName, "Cancel", 0, MAXIMUM);
	progress->setWindowModality(Qt::ApplicationModal);
	progress->setValue(10);
	progress->show();
	qApp->processEvents();

	CallbackFunc f = [=](double progressValue)
		{
			if (progress->wasCanceled())
				return false;
			double pv = std::min(std::max(progressValue * MAXIMUM, 10.), MAXIMUM);
			progress->setValue(pv);
			qApp->processEvents();
			return true;
		};

	_task(f);
	progress->hide();
}
