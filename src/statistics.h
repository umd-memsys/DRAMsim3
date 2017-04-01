#ifndef __STATISTICS_H
#define __STATISTICS_H

#include <iostream>

class Statistics {
public:
    Statistics();
    int numb_read_reqs_issued;
    int numb_write_reqs_issued;

    friend std::ostream& operator<<(std::ostream& os, const Statistics& stats);
};


#endif