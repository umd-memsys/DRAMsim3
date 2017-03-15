#ifndef __CPU_H
#define __CPU_H

#include <fstream>


class Access {
    uint64_t hex_addr_;
    std::string access_type_;
    long time_;
    friend std::istream& operator>>(std::istream& is, Access& access);
    friend std::ostream& operator<<(std::ostream& os, const Access& access);
};


class RandomCPU {
    RandomCPU();
};

class TraceBasedCPU {
    public:
        TraceBasedCPU();
    private:
        std::ifstream trace_file_;
};

#endif