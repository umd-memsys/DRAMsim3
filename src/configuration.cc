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
    BL = static_cast<unsigned int>(reader.GetInteger("dram_structure", "BL", 8)); 

    if (IsHMC()) {  // need to do sanity check and overwrite some values...
        num_links = static_cast<unsigned int>(reader.GetInteger("hmc", "num_links", 4));
        num_dies = static_cast<unsigned int>(reader.GetInteger("hmc", "num_dies", 8));
        link_width = static_cast<unsigned int>(reader.GetInteger("hmc", "link_width", 16));
        // there is a 12.5 we will just use 12 or 13 to simplify coding...
        link_speed = static_cast<unsigned int>(reader.GetInteger("hmc", "link_speed", 30));
        block_size = static_cast<unsigned int>(reader.GetInteger("hmc", "block_size", 32));

        // sanity checks 
        if (num_links !=2 || num_links != 4) {
            cerr << "HMC can only have 2 or 4 links!" << endl;
            AbruptExit(__FILE__, __LINE__);
        }
        if (num_dies != 4 || num_dies != 8) {
            cerr << "HMC can only have 4/8 layers of dies!" << endl;
            AbruptExit(__FILE__, __LINE__);
        }
        if (link_width != 4 || link_width != 8 || link_width != 16) {
            cerr << "HMC link width can only be 4 (quater), 8 (half) or 16 (full)!" << endl;
            AbruptExit(__FILE__, __LINE__);
        } 
        if (link_speed != 15 || link_speed != 25 || link_speed != 28 || 
            link_speed != 30 || link_speed != 12 or link_speed != 13) {
            cerr << "HMC speed options: 12/13, 15, 25, 28, 30" << endl;
            AbruptExit(__FILE__, __LINE__);
        }
        if (block_size != 32 || block_size != 64 || block_size != 128 || block_size != 256) {
            cerr << "HMC block size options: 32, 64, 128, 256 (bytes)!" << endl;
            AbruptExit(__FILE__, __LINE__);
        }
        // there is no BL in HMC spec but I believe a block is a BL
        // given the way the it describes in p.34 of HMC gen 2 specs
        // which would wrap the access within the block if the address exceeds
        // the block alignment
        BL = block_size / 16;  
        // vaults are basically channels here 
        num_vaults = 32;
        channels = num_vaults;  

        // A lot of the following parameters are not configurable 
        // according to the spec, so we just set them here
        rows = 16384;
        columns = 64;
        device_width = 128;  // spec says 1M * 16B per bank, exactly like HBM
        bus_width = 128;
        if (num_dies == 4) {
            banks = 8;  // NOTE this is banks per vault 
            channel_size = 128;
        } else {
            banks = 16;
            channel_size = 256;
        }
        bankgroups = 1;
        banks_per_group = banks;
    }
    
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

    // LPDDR4 and GDDR5
    tPPD = static_cast<unsigned int>(reader.GetInteger("timing", "tPPD", 0));

    // GDDR5 only 
    t32AW = static_cast<unsigned int>(reader.GetInteger("timing", "t32AW", 330));
    tRCDRD = static_cast<unsigned int>(reader.GetInteger("timing", "tRCDRD", 24));
    tRCDWR = static_cast<unsigned int>(reader.GetInteger("timing", "tRCDWR", 20));


    RL = AL + CL;
    WL = AL + CWL;

    // set burst cycle according to protocol
    if (protocol == DRAMProtocol::GDDR5) {
        burst_cycle = BL/4;
    } else if (protocol == DRAMProtocol::GDDR5X) {
        burst_cycle = BL/8;
    } else {
        burst_cycle = BL/2;
    }

    read_delay = RL + burst_cycle;
    write_delay = WL + burst_cycle;

    // Other Parameters
    validation_output_file = reader.Get("other", "validation_output", "");
    epoch_period = static_cast<unsigned int>(reader.GetInteger("other", "epoch_period", 100000));
    stats_file = reader.Get("other", "stats_file", "dramcore_stats.txt");
    cummulative_stats_file = reader.Get("other", "cummulative_stats_file", "dramcore_cummulative_stats.txt");
    epoch_stats_file = reader.Get("other", "epoch_stats_file", "dramcore_epoch_stats.txt");
    stats_file_csv = reader.Get("other", "stats_file", "dramcore_stats.csv");
    cummulative_stats_file_csv = reader.Get("other", "cummulative_stats_file", "dramcore_cummulative_stats.csv");
    epoch_stats_file_csv = reader.Get("other", "epoch_stats_file", "dramcore_epoch_stats.csv");

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
        {"HBM", DRAMProtocol::HBM},
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

    // The calculation for different protocols are different,
    // Some take into account of the prefetch/burst length in columns some don't
    // So instead of hard coding it into the ini files, 
    // calculating them here would be a better option
    unsigned int megs_per_bank, megs_per_rank; 
    
    if (IsGDDR()) {
        // For GDDR5(x), each column access gives you device_width * BL bits 
        megs_per_bank = ((rows * columns * BL) >> 20)  * device_width / 8;
        megs_per_rank = megs_per_bank * banks * devices_per_rank;
    } else if (IsHBM()) {
        // Similar to GDDR5(x), but HBM has both BL2 and BL4, and only 1 device_width, 
        // meaning it will have different prefetch length and burst length
        // so we will use the prefetch length of 2 here
        megs_per_bank = ((rows * columns * 2) >> 20)  * device_width / 8;
        megs_per_rank = megs_per_bank * banks * devices_per_rank;
    } else if (IsHMC()) {
        // nothing talks about the prefetch in HMC DRAM in the spec
        // so we will just go with it...
        megs_per_bank = ((rows * columns) >> 20) * device_width / 8
        megs_per_rank = megs_per_bank * banks * devices_per_rank;
    } else {
        // shift 20 bits first so that we won't have an overflow problem...
        megs_per_bank = ((rows * columns) >> 20) * device_width / 8;
        megs_per_rank = megs_per_bank * banks * devices_per_rank;
    }
    
    
#ifdef DEBUG_OUTPUT
    cout << "Meg Bytes per bank " << megs_per_bank << endl;
    cout << "Meg Bytes per rank " << megs_per_rank << endl;
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
