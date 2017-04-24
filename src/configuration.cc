#include "configuration.h"
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

    // System/controller Parameters
    channel_size = static_cast<unsigned int>(reader.GetInteger("system", "channel_size", 1024));
    channels = static_cast<unsigned int>(reader.GetInteger("system", "channels", 1));
    bus_width = static_cast<unsigned int>(reader.GetInteger("system", "bus_width", 64));
    address_mapping = reader.Get("system", "address_mapping", "chrobabgraco");
    queue_structure = reader.Get("system", "queue_structure", "PER_BANK");
    queue_size = static_cast<unsigned int>(reader.GetInteger("system", "queue_size", 16));
    req_buffering_enabled = reader.GetBoolean("system", "req_buffering_enabled", false);

    // DRAM organization
    bankgroups = static_cast<unsigned int>(reader.GetInteger("dram_structure", "bankgroups", 2));
    banks_per_group = static_cast<unsigned int>(reader.GetInteger("dram_structure", "banks_per_group", 2));
    banks = bankgroups * banks_per_group;
    rows = static_cast<unsigned int>(reader.GetInteger("dram_structure", "rows", 1 << 16));
    columns = static_cast<unsigned int>(reader.GetInteger("dram_structure", "columns", 1 << 10));
    device_width = static_cast<unsigned int>(reader.GetInteger("dram_structure", "device_width", 8));
    burst_len = static_cast<unsigned int>(reader.GetInteger("dram_structure", "BL", 8)); //tBL

    // calculate rank and re-calculate channel_size
    CalculateSize();

    // Timing Parameters
    // TODO there is no need to keep all of these variables, they should just be temporary
    // ultimately we only need cmd to cmd Timing instead of these     
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

    // Other Parameters
    validation_output_file = reader.Get("other", "validation_output", "");
    epoch_period = static_cast<unsigned int>(reader.GetInteger("other", "epoch_period", 0));

    channel_width_ = LogBase2(channels);
    rank_width_ = LogBase2(ranks);
    bankgroup_width_ = LogBase2(bankgroups);
    bank_width_ = LogBase2(banks_per_group);
    row_width_ = LogBase2(rows);
    column_width_ = LogBase2(columns);
    unsigned int bytes_offset = LogBase2(bus_width / 8);
    unsigned int transaction_size = bus_width / 8 * burst_len;  // transaction size in bytes

    // for each address given, because we're transimitting trascation_size bytes per transcation
    // therefore there will be throwaway_bits not used in the address
    // part of it is due to the bytes offset, the other part is the burst len 
    // (same as column auto increment)
    // so effectively only column_width_ -(throwaway_bits - bytes_offset) will be used in column addressing
    throwaway_bits = LogBase2(transaction_size);
    column_width_ -= (throwaway_bits - bytes_offset);
}

void Config::CalculateSize() {
    unsigned int devices_per_rank = bus_width / device_width;

    // shift 20 bits first so that we won't have an overflow problem...
    unsigned int megs_per_bank = ((rows * columns) >> 20) * device_width / 8;
    unsigned int megs_per_rank = megs_per_bank * banks * devices_per_rank;

    if (megs_per_rank > channel_size) {
        std::cout<< "WARNING: Cannot create memory system of size " << channel_size 
            << "MB with given device choice! Using default size " << megs_per_rank 
            << " instead!" << std::endl;
        ranks = 1;
        channel_size = megs_per_rank;
    } else {
        ranks = channel_size / megs_per_rank;
        channel_size = ranks * megs_per_rank;  // reset this in case users inputs a weird number...
    }
    return;
}
