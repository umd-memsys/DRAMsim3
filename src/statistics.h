#ifndef __STATISTICS_H
#define __STATISTICS_H

#include <iostream>

namespace dramcore {

class Statistics {
public:
    Statistics();
    int numb_read_reqs_issued;
    int numb_write_reqs_issued;
    int numb_row_hits;
    int numb_read_row_hits;
    int numb_write_row_hits;
    int numb_aggressive_precharges;
    int numb_ondemand_precharges;

    friend std::ostream& operator<<(std::ostream& os, const Statistics& stats);
};

}
#endif