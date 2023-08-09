#include "FuncThread.h"

FuncThread::FuncThread(QObject* parent) : QThread(parent) {
}

void FuncThread::run() {
	assert(Func);
	Func();
}
