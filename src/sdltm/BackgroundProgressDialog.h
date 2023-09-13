#pragma once
#include <functional>

#include "FuncThread.h"

class BackgroundProgressDialog
{
public:
	typedef std::function<bool(double)> CallbackFunc;
	typedef std::function<void(CallbackFunc)> TaskFunc;
	BackgroundProgressDialog(TaskFunc task);
	~BackgroundProgressDialog();

	void ShowDialog();

	QString TaskName;

private:
	void PerformTask();
	void Complete();

private:
	// pass the progress 0 - 1 
	TaskFunc _task;

	typedef std::lock_guard<std::recursive_mutex> Lock;
	std::recursive_mutex _mutex;

	bool _cancelled = false;
	double _progressValue = 0;

};

