#pragma once
#include <cassert>
#include <functional>
#include <qobjectdefs.h>
#include <QThread>

class FuncThread : public QThread{
	Q_OBJECT
public:
	explicit FuncThread(QObject* parent = nullptr);

	std::function<void()> Func;
	void run() override;
};

