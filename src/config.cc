#include "config.h"

using namespace dramcore;

Config::Config()
{

    channel_width_ = LogBase2(channels);
    rank_width_ = LogBase2(ranks);
    bankgroup_width_ = LogBase2(bankgroups);
    bank_width_ = LogBase2(banks_per_group);
    row_width_ = LogBase2(rows);
    column_width_ = LogBase2(columns);
}

namespace dramcore {

unsigned int LogBase2(unsigned int power_of_two) {
    unsigned int i = 0;
    while( power_of_two > 1) {
        power_of_two /= 2;
        i++;
    }
    return i;
}

}