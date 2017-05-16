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
    protocol = GetDRAMProtocol(reader.Get("dram_structure", "protocol", "DDR3"));
    channel_size = static_cast<unsigned int>(reader.GetInteger("system", "channel_size", 1024));
    channels = static_cast<unsigned int>(reader.GetInteger("system", "channels", 1));
    bus_width = static_cast<unsigned int>(reader.GetInteger("system", "bus_width", 64));
    address_mapping = reader.Get("system", "address_mapping", "chrobabgraco");
    queue_structure = reader.Get("system", "queue_structure", "PER_BANK");
    queue_size = static_cast<unsigned int>(reader.GetInteger("system", "queue_size", 16));
    req_buffering_enabled = reader.GetBoolean("system", "req_buffering_enabled", false);

    // DRAM organization
    bool bankgroup_enable = reader.GetBoolean("dram_structure", "bankgroup_enable", true);
    bankgroups = static_cast<unsigned int>(reader.GetInteger("dram_structure", "bankgroups", 2));
    banks_per_group = static_cast<unsigned int>(reader.GetInteger("dram_structure", "banks_per_group", 2));
    if (!bankgroup_enable) {  // aggregating all banks to one group
        banks_per_group *= bankgroups;
        bankgroups = 1;
    }
    banks = bankgroups * banks_per_group;
    rows = static_cast<unsigned int>(reader.GetInteger("dram_structure", "rows", 1 << 16));
    columns = static_cast<unsigned int>(reader.GetInteger("dram_structure", "columns", 1 << 10));
    device_width = static_cast<unsigned int>(reader.GetInteger("dram_structure", "device_width", 8));
    BL = static_cast<unsigned int>(reader.GetInteger("dram_structure", "BL", 8)); //tBL

    // calculate rank and re-calculate channel_size
    CalculateSize();

    // Timing Parameters
    // TODO there is no need to keep all of these variables, they should just be temporary
    // ultimately we only need cmd to cmd Timing instead of these     
    AL = static_cast<unsigned int>(reader.GetInteger("timing", "AL", 0));
    CL = static_cast<unsigned int>(reader.GetInteger("timing", "CL", 12));
    CWL = static_cast<unsigned int>(reader.GetInteger("timing", "CWL", 12));
    tCCD_L = static_cast<unsigned int>(reader.GetInteger("timing", "tCCD_L", 6));
    tCCD_S = static_cast<unsigned int>(reader.GetInteger("timing", "tCCD_S", 4));
    tRTRS = static_cast<unsigned int>(reader.GetInteger("timing", "tRTRS", 2));
    tRTP = static_cast<unsigned int>(reader.GetInteger("timing", "tRTP", 5));
    tWTR_L = static_cast<unsigned int>(reader.GetInteger("timing", "tWTR_L", 5));
    tWTR_S = static_cast<unsigned int>(reader.GetInteger("timing", "tWTR_S", 5));
    tWR = static_cast<unsigned int>(reader.GetInteger("timing", "tWR", 10));
    tRP = static_cast<unsigned int>(reader.GetInteger("timing", "tRP", 10));
    tRRD_L = static_cast<unsigned int>(reader.GetInteger("timing", "tRRD_L", 4));
    tRRD_S = static_cast<unsigned int>(reader.GetInteger("timing", "tRRD_S", 4));
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
    tRPRE = static_cast<unsigned int>(reader.GetInteger("timing", "tRPRE", 1));
    tWPRE = static_cast<unsigned int>(reader.GetInteger("timing", "tWPRE", 1));

    // GDDR5 only 
    tRCDRD = static_cast<unsigned int>(reader.GetInteger("timing", "tRCDRD", 24));
    tRCDWR = static_cast<unsigned int>(reader.GetInteger("timing", "tRCDWR", 20));


    RL = AL + CL;
    WL = AL + CWL;

    // Protocol specific timing
    if (protocol == DRAMProtocol::GDDR5) {
        burst_cycle = BL/4;
    } else if (protocol == DRAMProtocol::GDDR5X) {
        burst_cycle = BL/8;
    } else {
        burst_cycle = BL/2;
    }

    read_delay = RL + burst_cycle;
    write_delay = WL + burst_cycle;

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
    unsigned int transaction_size = bus_width / 8 * BL;  // transaction size in bytes

    // for each address given, because we're transimitting trascation_size bytes per transcation
    // therefore there will be throwaway_bits not used in the address
    // part of it is due to the bytes offset, the other part is the burst len 
    // (same as column auto increment)
    // so effectively only column_width_ -(throwaway_bits - bytes_offset) will be used in column addressing
    throwaway_bits = LogBase2(transaction_size);
    column_width_ -= (throwaway_bits - bytes_offset);

#ifdef DEBUG_OUTPUT
    cout << "Address bits:" << endl;
    cout << setw(10) << "Channel " << channel_width_ << endl;
    cout << setw(10) << "Rank " << rank_width_ << endl;
    cout << setw(10) << "Bankgroup " << bankgroup_width_ << endl;
    cout << setw(10) << "Bank " << bank_width_ << endl;
    cout << setw(10) << "Row " << row_width_ << endl;
    cout << setw(10) << "Column " << column_width_ << endl;
#endif

}


DRAMProtocol Config::GetDRAMProtocol(std::string protocol_str) {
    std::vector<std::pair<std::string, DRAMProtocol>> protocol_pairs = {
        {"DDR3", DRAMProtocol::DDR3},
        {"DDR4", DRAMProtocol::DDR4},
        {"GDDR5", DRAMProtocol::GDDR5},
        {"GDDR5X", DRAMProtocol::GDDR5X},
        {"LPDDR", DRAMProtocol::LPDDR},
        {"LPDDR3", DRAMProtocol::LPDDR3},
        {"LPDDR4", DRAMProtocol::LPDDR4},
        {"HBM2", DRAMProtocol::HBM2},
        {"HMC", DRAMProtocol::HMC}
    };

    for (auto pair_itr:protocol_pairs) {
        if (pair_itr.first == protocol_str) {
#ifdef DEBUG_OUTPUT
    cout << "DRAM Procotol " << protocol_str << endl;
#endif 
            return pair_itr.second;
        }
    }
    cout << "Unkwown/Unsupported DRAM Protocol: " << protocol_str << " Aborting!" << endl;
    AbruptExit(__FILE__, __LINE__);
}


void Config::CalculateSize() {
    unsigned int devices_per_rank = bus_width / device_width;

    // shift 20 bits first so that we won't have an overflow problem...
    unsigned int megs_per_bank = ((rows * columns) >> 20) * device_width / 8;
    unsigned int megs_per_rank = megs_per_bank * banks * devices_per_rank;
#ifdef DEBUG_OUTPUT
    cout << "megs per bank " << megs_per_bank << endl;
    cout << "megs per rank " << megs_per_rank << endl;
#endif 
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
