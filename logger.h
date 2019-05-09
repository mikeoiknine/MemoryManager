#include <iostream>
#include <fstream>
#include <mutex>

/* Logger provides a thread-safe logging to standard output and log file */
class Logger {
    private:
        static const std::string outfile;

    public:
        static void processLog(unsigned int, int, std::string, std::string, int);
        static void processStatusLog(unsigned int, int, std::string);
        static void memManagerSwapLog(unsigned int, std::string, std::string, std::string);
};
