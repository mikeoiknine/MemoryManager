#include "logger.h"

// Define static members
const std::string Logger::outfile = "output.txt";

void Logger::processLog(unsigned int clk, int pid, std::string cmd, std::string varid, int val) {
    static std::mutex f_lk;
    std::scoped_lock lock(f_lk);

    std::ofstream out(outfile, std::ofstream::app);
    if (!out) {
        std::cout << "Error opening log file from processLog" << std::endl;
    }

    if (cmd == "Release" ) {
        std::cout << "Clock: " << clk << ", Process " << pid << ": "  << cmd << ": Variable " << varid << std::endl;
        out << "Clock: " << clk << ", Process " << pid << ": "  << cmd << ": Variable " << varid << std::endl;
    } else {
        std::cout << "Clock: " << clk << ", Process " << pid << ": "  << cmd <<
                     ": Variable " << varid << ", Value: " << val << std::endl;
        out << "Clock: " << clk << ", Process " << pid << ": "  << cmd <<
                     ": Variable " << varid << ", Value: " << val << std::endl;
    }
}

void Logger::processStatusLog(unsigned int clk, int pid, std::string status) {
    static std::mutex f_lk;
    std::scoped_lock lock(f_lk);

    std::ofstream out(outfile, std::ofstream::app);
    if (!out) {
        std::cout << "Error opening log file from processStatusLog" << std::endl;
    }

    std::cout << "Clock: " << clk << ", Process " << pid << ": " << status << std::endl;
    out << "Clock: " << clk << ", Process " << pid << ": " << status << std::endl;
}

void Logger::memManagerSwapLog(unsigned int clk, std::string event, std::string varid1, std::string varid2) {
    static std::mutex f_lk;
    std::scoped_lock lock(f_lk);

    std::ofstream out(outfile, std::ofstream::app);
    if (!out) {
        std::cout << "Error opening log file from memManagerSwapLog" << std::endl;
    }

    std::cout << "Clock: " << clk << ", Memory Manager, " << event << ": Variable " << varid1 << " with Variable " << varid2 << std::endl;
    out << "Clock: " << clk << ", Memory Manager, " << event << ": Variable " << varid1 << " with Variable " << varid2 << std::endl;
}


