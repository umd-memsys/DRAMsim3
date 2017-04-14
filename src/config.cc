#include "config.h"
#include "common.h"
#include "../ext/inih/src/INIReader.h"

using namespace std;
using namespace dramcore;

Config::Config(std::string filename)
{
    INIReader reader("./../configs/dummy_config.ini");
    cout << reader.GetInteger("timing", "tRP", -1) << endl;


    channel_width_ = LogBase2(channels);
    rank_width_ = LogBase2(ranks);
    bankgroup_width_ = LogBase2(bankgroups);
    bank_width_ = LogBase2(banks_per_group);
    row_width_ = LogBase2(rows);
    column_width_ = LogBase2(columns);
}