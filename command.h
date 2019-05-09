#include <iostream>

struct Command {
        std::string cmd;
        std::string varid;
        unsigned int val;

        Command(std::string, std::string, unsigned int);
};
