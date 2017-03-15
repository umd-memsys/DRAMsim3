#include "cpu.h"
#include <iostream>

using namespace std;

RandomCPU::RandomCPU()
{}

TraceBasedCPU::TraceBasedCPU()
{
    trace_file_.open("sample_trace.txt");
    if(trace_file_.fail()) {
        cerr << "Trace file does not exist" << endl << " Exiting abruptly" << endl;
        exit(-1);
    }

    Access access;
    while(!trace_file_.eof()) {
        trace_file_ >> access;
        cout << access;
    }
}


istream& operator>>(istream& is, Access& access) {
    is >> hex >> access.hex_addr_ >> access.access_type_ >> dec >> access.time_;
    return is;
}

ostream& operator<<(ostream& os, const Access& access) {
    os << hex << access.hex_addr_ << " " << access.access_type_ << " " << dec << access.time_ << endl;
    return os;
}