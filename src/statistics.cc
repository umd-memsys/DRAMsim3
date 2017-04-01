#include "statistics.h"

using namespace std;

Statistics::Statistics():
        numb_read_reqs_issued(0),
        numb_write_reqs_issued(0),
        numb_row_hits(0),
        numb_read_row_hits(0),
        numb_write_row_hits(0),
        numb_aggressive_precharges(0),
        numb_ondemand_precharges(0)
{}


ostream& operator<<(ostream& os, const Statistics& stats) {
    os << "numb_read_reqs_issued = " << stats.numb_read_reqs_issued << endl;
    os << "numb_write_reqs_issued = " << stats.numb_write_reqs_issued << endl;
    os << "numb_row_hits = " << stats.numb_row_hits << endl;
    os << "numb_read_row_hits = " << stats.numb_read_row_hits << endl;
    os << "numb_write_row_hits = " << stats.numb_write_row_hits << endl;
    os << "numb_aggressive_precharges = " << stats.numb_aggressive_precharges << endl;
    os << "numb_ondemand_precharges = " << stats.numb_ondemand_precharges << endl;
    return os;
}