#include "BackgroundProgressDialog.h"

#include <qcoreapplication.h>
#include <QProgressDialog>

namespace  {
	const double MAXIMUM = 1000;
}

BackgroundProgressDialog::BackgroundProgressDialog(TaskFunc task) : _task(task) {
}

BackgroundProgressDialog::~BackgroundProgressDialog() {
}

void BackgroundProgressDialog::ShowDialog() {
	auto thread = new FuncThread();
	thread->Func = [this]() { PerformTask(); };
	thread->start();

	QProgressDialog *progress = new QProgressDialog(TaskName, "Cancel", 0, MAXIMUM);
	progress->setWindowModality(Qt::ApplicationModal);
	progress->setValue(10);
	progress->show();
	// wait for thread to complete or user to press cancel
	while (true) {
		auto isCancelled = progress->wasCanceled();
		bool isComplete = thread->isFinished();
		double progressValue;
		{ Lock lock(_mutex);
		_cancelled = isCancelled;
		progressValue = _progressValue;
		}
		if (_cancelled || isComplete)
			break;

		double pv = std::min( std::max(progressValue * MAXIMUM, 10.), MAXIMUM);
		progress->setValue(pv);

		qApp->processEvents();
	}

	// here, I wait for the other thread to complete
	while (true) {
		bool isComplete = thread->isFinished();
		if (isComplete)
			break;

		qApp->processEvents();
	}

	try {
		delete thread;
	}
	catch (...) {
	}

	progress->hide();
	qApp->processEvents();
}

void BackgroundProgressDialog::PerformTask() {
	try {
		CallbackFunc f = [this](double progress)
		{
			Lock lock(_mutex);
			if (_cancelled)
				return false;
			_progressValue = progress;
			return true;
		};
		_task(f);
	}
	catch (...) {
	}

	Complete();
}


void BackgroundProgressDialog::Complete() {
}
