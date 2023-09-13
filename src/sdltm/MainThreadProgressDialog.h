#pragma once
#include <functional>
#include <mutex>
#include <QString>

class MainThreadProgressDialog
{
public:
	typedef std::function<bool(double)> CallbackFunc;
	typedef std::function<void(CallbackFunc)> TaskFunc;
	MainThreadProgressDialog(TaskFunc task);
	~MainThreadProgressDialog();

	void ShowDialog();

	QString TaskName;

private:
	// pass the progress 0 - 1 
	TaskFunc _task;

};

