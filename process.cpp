#include "process.h"

static int id = 1;
Process::Process(int st, int d) : startTime(st), duration(d), pid(id++) {}

int Process::getStartTime() {
	return this->startTime;
}

int Process::getDuration() {
	return this->duration;
}

int Process::getPid() {
	return this->pid;
}

void Process::start() {
    this->started = true;
}

bool Process::isRunning() {
    return this->started;
}
