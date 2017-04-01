#include "statistics.h"

using namespace std;

Statistics::Statistics():
        numb_read_reqs_issued(0),
        numb_write_reqs_issued(0)
{}


ostream& operator<<(ostream& os, const Statistics& stats) {
    os << "numb_read_reqs_issued == " << stats.numb_read_reqs_issued << endl;
    os << "numb_write_reqs_issued == " << stats.numb_write_reqs_issued << endl;
    return os;
}