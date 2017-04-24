#include "config.h"
#include "common.h"
#include "../ext/inih/src/INIReader.h"

using namespace std;
using namespace dramcore;

Config::Config(std::string config_file)
{
    INIReader reader(config_file);

    if(reader.ParseError() < 0) {
        std::cerr << "Can't load config file - " << config_file << endl;
        AbruptExit(__FILE__, __LINE__);
    }

    //DRAM structure parameters
    channels = static_cast<unsigned int>(reader.GetInteger("dram_structure", "channels", 1));
    ranks = static_cast<unsigned int>(reader.GetInteger("dram_structure", "ranks", 2));
    bankgroups = static_cast<unsigned int>(reader.GetInteger("dram_structure", "bankgroups", 2));
    banks_per_group = static_cast<unsigned int>(reader.GetInteger("dram_structure", "banks_per_group", 2));
    rows = static_cast<unsigned int>(reader.GetInteger("dram_structure", "rows", 1 << 16));
    columns = static_cast<unsigned int>(reader.GetInteger("dram_structure", "columns", 1 << 10));

    //Timing Parameters
    tBurst = static_cast<unsigned int>(reader.GetInteger("timing", "tBurst", 8)); //tBL
    tCCDL = static_cast<unsigned int>(reader.GetInteger("timing", "tCCDL", 6));
    tCCDS = static_cast<unsigned int>(reader.GetInteger("timing", "tCCDS", 4));
    tRTRS = static_cast<unsigned int>(reader.GetInteger("timing", "tRTRS", 2));
    tRTP = static_cast<unsigned int>(reader.GetInteger("timing", "tRTP", 5));
    tCAS = static_cast<unsigned int>(reader.GetInteger("timing", "tCAS", 3)); //tCL
    tCWD = static_cast<unsigned int>(reader.GetInteger("timing", "tCWD", 3)); // =tCAS
    tWTR = static_cast<unsigned int>(reader.GetInteger("timing", "tWTR", 5));
    tWR = static_cast<unsigned int>(reader.GetInteger("timing", "tWR", 10));
    tRP = static_cast<unsigned int>(reader.GetInteger("timing", "tRP", 10));
    tRRD = static_cast<unsigned int>(reader.GetInteger("timing", "tRRD", 4));
    tRAS = static_cast<unsigned int>(reader.GetInteger("timing", "tRAS", 24));
    tRCD = static_cast<unsigned int>(reader.GetInteger("timing", "tRCD", 10));
    tRFC = static_cast<unsigned int>(reader.GetInteger("timing", "tRFC", 74));
    tRC = tRAS + tRP;
    tCKESR = static_cast<unsigned int>(reader.GetInteger("timing", "tCKESR", 50));
    tXS = static_cast<unsigned int>(reader.GetInteger("timing", "tXS", 10));
    tRFCb = static_cast<unsigned int>(reader.GetInteger("timing", "tRFCb", 20));
    tRREFD = static_cast<unsigned int>(reader.GetInteger("timing", "tRREFD", 5));
    tREFI = static_cast<unsigned int>(reader.GetInteger("timing", "tREFI", 7800));
    tREFIb = static_cast<unsigned int>(reader.GetInteger("timing", "tREFIb", 1950));
    tFAW = static_cast<unsigned int>(reader.GetInteger("timing", "tFAW", 50));

    activation_window_depth = static_cast<unsigned int>(reader.GetInteger("timing", "activation_window_depth", 4));

    //Controller Parameters
    address_mapping = reader.Get("controller", "address_mapping", "chrobabgraco");
    queue_structure = reader.Get("controller", "queue_structure", "PER_BANK");
    queue_size = static_cast<unsigned int>(reader.GetInteger("controller", "queue_size", 16));

    //Other Parameters
    validation_output_file = reader.Get("other", "validation_output", "");
    epoch_period = static_cast<unsigned int>(reader.GetInteger("other", "epoch_period", 0));

    channel_width_ = LogBase2(channels);
    rank_width_ = LogBase2(ranks);
    bankgroup_width_ = LogBase2(bankgroups);
    bank_width_ = LogBase2(banks_per_group);
    row_width_ = LogBase2(rows);
    column_width_ = LogBase2(columns);
}
