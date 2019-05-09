#include "memorymanager.h"
#include <sstream>
#include <iterator>

Logger logger;

// Make outputting to stdout or file easier
std::ostream& operator<<(std::ostream& os, const Page& p) {
	os << p.variable.varid << " " <<
		  p.variable.val   << " " <<
		  p.variable.lastAccess << " " <<
		  p.free;
	return os;
}

Page::Page(std::string vid, unsigned int v, unsigned int t, bool f) {
	this->variable.varid = vid;
	this->variable.val = v;
	this->variable.lastAccess = t;
	this->free = f;
}

MemoryManager::MemoryManager(unsigned int MEMSIZE, std::string fname) : disk(fname), MEMSIZE(MEMSIZE) {}

// return the index in memory of the variable with id `varid`
// Index is continuous so if the index returned is greater than MEMSIZE
// then it resides on the disk and its adress on the disk is address_returned - MEMSIZE
int MemoryManager::getAddress(std::string varid) {
    std::shared_lock lock(pt_lk);   // Multiple threads may read at the same time
	auto it = this->pageTable.find(varid);
	if (it == this->pageTable.end()) {
		return -1;
	}

	return it->second;
}

// UpdateTable will take a varid and its address and add it to the table.
// In the case that addr is less than 0, the varid will be deleted from the table
void MemoryManager::updateTable(std::string varid, int addr) {
    std::unique_lock lock(pt_lk);   // Only one writer can write to table at a time
	if (addr < 0) {
		auto it = pageTable.find(varid);
		if (it != pageTable.end()) {
			pageTable.erase(it);
			return;
		}
	}

	pageTable[varid] = addr;
}

/*** API Function - Allow Threads to Store/Overwrite Variables Safely ***/
void MemoryManager::Store(std::string varid, unsigned int v, unsigned int currtime) {
	auto addr = getAddress(varid);

	// Check that Variable doesn't exist in memory, so we create it in the first empty page we find
	if (addr < 0) {
		if (writeToMemory(varid, v, currtime)) {
			return;
		}

		// Memory is full, store it on disk
		addToDisk(varid, v, currtime);
		return;
	}

	// Determine if the var is located in Main memory or on disk
	if (addr < MEMSIZE) {
		modifyMemory(addr, v, currtime);
		return;
	}

	modifyDisk(addr - MEMSIZE, v, currtime);
}

/*** API Function - Allow Threads to Delete Variables Safely ***/
// Rather than overwriting the values with empty values, this function
// will simply set the `free` variable of the page to True and update the page Table
void MemoryManager::Release(std::string varid, unsigned int currtime) {
	auto addr = getAddress(varid);
	if (addr < 0) {
		return;
	}

	// Determine if the var is located in Memory or on the Disk
	if (addr < MEMSIZE) {
        std::scoped_lock lock(mm_lk);
		mainMemory[addr].free = true;
		updateTable(varid, -1);
		return;
	}

	// Free Disk Space
    std::scoped_lock lock(d_lk);
	auto data = readFromDisk();
	data[addr - MEMSIZE].free = true;
	writeToDisk(data);
	updateTable(varid, -1);
}

/*** API Function - Allow Threads to Retrieve the value of a variable stored in memory safely ***/
int MemoryManager::Lookup(std::string varid, unsigned int currtime) {
	auto addr = getAddress(varid);
	if (addr < 0) {
		return -1;
	}

	if (addr < MEMSIZE) {
        std::scoped_lock lock(mm_lk);
		auto val = mainMemory[addr].variable.val;
		return val;
	}

	// Find smallest access time in main memory
    /* Scoped_lock will acquire both locks and ensure deadlock doesn't occur */
    std::scoped_lock lock(mm_lk, d_lk);
	unsigned int smallest = -1;  // uints are modulo so -1 will set it to UINT_MAX
	auto index = 0;
	auto it = 0;
	for (const auto &p : mainMemory) {
		if (p.variable.lastAccess < smallest) {
			smallest = p.variable.lastAccess;
			index = it;
		}
		it++;
	}

	auto data = readFromDisk();
	Page swap_reg = mainMemory[index];
	mainMemory[index] = data[addr - MEMSIZE];
	data[addr - MEMSIZE] = swap_reg;
	writeToDisk(data);
	auto val = mainMemory[index].variable.val;

	// Update page table to reflect change
	updateTable(swap_reg.variable.varid, addr);
	updateTable(mainMemory[index].variable.varid, addr - MEMSIZE);

    Logger::memManagerSwapLog(currtime, "SWAP", swap_reg.variable.varid, mainMemory[index].variable.varid);

	return val;
}

// modifyMemory will overwrite the value of the variable at position `addr` in Main Memory
// with param `v` as well as its last access time
void MemoryManager::modifyMemory(unsigned int addr, unsigned int v, unsigned int currtime) {
    std::scoped_lock lock(mm_lk);
	mainMemory[addr].variable.val = v;
	mainMemory[addr].variable.lastAccess = currtime;
}

// Takes a variable and will look for a free page in memory to store it.
//
// If it finds a free slot, it will store the variable, set the `free` bool of the page to false
// and will update the page table
//
// If no free slot exists, it will return false
bool MemoryManager::writeToMemory(std::string varid, unsigned int v, unsigned int currtime) {
    std::scoped_lock lock(mm_lk);
	auto i = 0;
	while(i < mainMemory.size() && i < MEMSIZE) {
		if (mainMemory[i].free)	{
			mainMemory[i] = Page(varid, v, currtime, false);
			updateTable(varid, i);
			return true;
		}
		++i;
	}

	if (i < MEMSIZE) {
		mainMemory.push_back(Page(varid, v, currtime, false));
		updateTable(varid, i);
		return true;
	}

	return false;
}

// Takes a variable and will store it on the disk. Although we have unlimited disk space,
// we set the variable in the first free space starting from the beginning of disk space.
//
// If we did not do this, the disk file would continue to grow with holes in memory, this
// is essentially external fragmentation which defeats the purpose of using a paging scheme
void MemoryManager::addToDisk(std::string varid, unsigned int v, unsigned int currtime) {
    std::scoped_lock lock(d_lk);
	auto data = readFromDisk();
	auto i = 0;
	while(i < data.size() && !data[i].free) {
		i++;
	}


	if (i < data.size()) {
		data[i] = Page(varid, v, currtime, false);
	} else {
		data.push_back(Page(varid, v, currtime, false));
	}

	// Update Page Table
	updateTable(varid, i + MEMSIZE);

	// Save modifications to disk
	writeToDisk(data);
}

// given a page's address on the disk, modify only its contents
void MemoryManager::modifyDisk(unsigned int addr, unsigned int v, unsigned int currtime) {
    std::scoped_lock lock(d_lk);
	auto data = readFromDisk();
	data[addr].variable.val = v;
	data[addr].variable.lastAccess = currtime;
	writeToDisk(data);
}

/* Write the data vector onto the disk (vm.txt) */
void MemoryManager::writeToDisk(const std::vector<Page>& data) {
    std::scoped_lock lock(f_lk);
	std::ofstream f(this->disk, std::ofstream::out | std::ofstream::trunc);
	for (const auto &var : data) {
		f << var << std::endl;
	}
	f.close();
}

/* Read the data from the disk (vm.txt) and return the contents as Page vector*/
std::vector<Page> MemoryManager::readFromDisk() {
    std::scoped_lock lock(f_lk);
	std::ifstream f(this->disk, std::ifstream::in);
	std::vector<std::string> tok((std::istream_iterator<std::string>(f)),
								  std::istream_iterator<std::string>());
	std::vector<Page> data;
	for (auto i = 0; i < tok.size(); i += 4) {
		if (tok[i + 3] == "1") {
			data.push_back(Page("", 0, 0, 1));		// Free page in memory
		} else {
			data.push_back(Page(tok[i], std::stoi(tok[i + 1]), std::stoi(tok[i + 2]), 0));
		}
	}
	f.close();

	return data;
}
