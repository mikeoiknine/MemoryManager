/*
 *  Written by Michael Oiknine
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <iterator>
#include <atomic>
#include <thread>
#include <chrono>
#include "memorymanager.h"
#include "process.h"
#include "command.h"


std::atomic<int> SYS_CLOCK{0};  // Global System clock

std::vector<Process> processes; // Processes currently on the system
MemoryManager *memoryManager;
std::vector<Command> commands;

// Will parse the three config files and set up the system
void parseConfig(std::string processTxt, std::string memConfTxt, std::string cmdTxt) {
	std::fstream procfile(processTxt);
	std::fstream memfile(memConfTxt);
	std::fstream cmdfile(cmdTxt);
	if (!procfile || !memfile || !cmdfile) {
		std::cout << "Error opening config files" << std::endl;
		exit(1);
	}

	// Parse process conf
	std::string proccnt;
	std::getline(procfile, proccnt);
	int pnum = std::stoi(proccnt);

	// Create vector of tokens based on whitespace
    std::vector<std::string> tok((std::istream_iterator<std::string>(procfile)),
								  std::istream_iterator<std::string>());
    auto currDuration = 0;
	for (int i = 0; i < pnum * 2; i += 2) {
        currDuration = std::stoi(tok[i]) + std::stoi(tok[i + 1]);
		Process p = Process{std::stoi(tok[i]), currDuration};
		processes.push_back(std::move(p));
	}


	// Parse memory size and initialize the memory manager
	std::string memsize;
	std::getline(memfile, memsize);
	memoryManager = new MemoryManager(std::stoi(memsize), "vm.txt");

	// Parse commands file
	// Create vector of tokens based on whitespace
    std::vector<std::string> cmds((std::istream_iterator<std::string>(cmdfile)),
								  std::istream_iterator<std::string>());

	for (int i = 0; i < cmds.size();) {
        if (cmds[i] == "Store") {
            commands.push_back(Command(cmds[i], cmds[i + 1], std::stoi(cmds[i + 2])));
            i += 3;
        } else {
            commands.push_back(Command(cmds[i], cmds[i + 1], 0));
            i += 2;
        }
	}
}

void process(int pid, int endTime) {

    while(1) {
	    // Sleep between 1 and 1000 ms
	    int sleepTime = (rand() % 1000 + SYS_CLOCK);
	    while(SYS_CLOCK <= sleepTime);

	    // Check if process has gone beyond its duration
	    if (SYS_CLOCK >= endTime) {
            Logger::processStatusLog(SYS_CLOCK, pid, "Finished");
	    	return;
	    }

	    // Choose a random command to execute
	    int choice = rand() % commands.size();
        int val = (int)commands[choice].val;
        if (commands[choice].cmd == "Store" ) {
            memoryManager->Store(commands[choice].varid, commands[choice].val, SYS_CLOCK);
        } else if (commands[choice].cmd == "Release") {
            memoryManager->Release(commands[choice].varid, SYS_CLOCK);
        } else if (commands[choice].cmd == "Lookup") {
            val = memoryManager->Lookup(commands[choice].varid, SYS_CLOCK);
        }
        Logger::processLog(SYS_CLOCK, pid, commands[choice].cmd, commands[choice].varid, val);
    }
}

// Simulates the system clock, the SYS_CLOCK variable is a global atomic integer,
// it is only modified in this function
void clk() {
	while(SYS_CLOCK >= 0) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(1)); // Without the wait, the programs terminates too quickly
		SYS_CLOCK++;
	}
    return;
}

// The run function will simply wait until a process is ready to start and then
// start it on its own thread.
void run() {
    // start clock
	std::thread clockProc(clk);

	std::vector<std::thread> threads;
	while(1) {
		if (processes.empty()) {
			break;
	    }

		std::vector<Process>::iterator it = processes.begin();
		while (it != processes.end()) {
			if (SYS_CLOCK >= it->getStartTime() && !it->isRunning()) {
                Logger::processStatusLog(SYS_CLOCK, it->getPid(), "Started");
				threads.push_back(std::thread(process, it->getPid(), SYS_CLOCK + it->getDuration()));
                it = processes.erase(it);
                continue;
			}
            it++;
		}


	}

    // Wait for all the threads to terminate
	for (auto &t : threads) {
		t.join();
	}

    // Once all threads are finished, tell the clock process to exit
    SYS_CLOCK = -1000;
    clockProc.join();
}

// Expects three argument in the form of text files
// 1 - process.txt
// 2 - memconfig.txt
// 3 - commands.txt
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Missing config files" << std::endl;
        exit(1);
    }
	parseConfig(argv[1], argv[2], argv[3]);
    run();
}
