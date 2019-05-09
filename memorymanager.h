#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <fstream>
#include <vector>
#include <map>

#ifndef LOGGER_H
#include "logger.h"
#define LOGGER_H
#endif

struct Variable {
	std::string varid;
	unsigned int val;
	unsigned int lastAccess;
};

struct Page {
	Variable variable;
	bool free;

	friend std::ostream& operator<<(std::ostream&, const Page&);
	Page(std::string, unsigned int, unsigned int, bool);
};

class MemoryManager {
	private:
		const unsigned int MEMSIZE;		            // Number of Pages in RAM
		std::string disk;				            // Disk file name
		std::vector<Page> mainMemory;	            // RAM
		std::map<std::string, int> pageTable;		// Map of variable ID to index in memory

        // Mutex Locks
        std::shared_mutex pt_lk;    // PageTable lock
        std::mutex mm_lk;           // MainMemory lock
        std::mutex d_lk;            // Disk lock
        std::mutex f_lk;            // Fstream lock

		// Overwrite single Page
		void modifyMemory(unsigned int, unsigned int, unsigned int);
		void modifyDisk(unsigned int, unsigned int, unsigned int);

		// Write Variable to first available page
		bool writeToMemory(std::string, unsigned int, unsigned int);
		void writeToDisk(const std::vector<Page>&);

		// Get disk
		std::vector<Page> readFromDisk();

		// Store new variable on disk
		void addToDisk(std::string, unsigned int, unsigned int);

		// Modify Page Table
		int getAddress(std::string);
		void updateTable(std::string, int);

	public:
		MemoryManager(unsigned int, std::string);

		// API Functions
		void Store(std::string, unsigned int, unsigned int);
		void Release(std::string, unsigned int);
		int Lookup(std::string, unsigned int);
};
