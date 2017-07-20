#include "configuration.h"
#include "common.h"
#include "../ext/inih/src/INIReader.h"

using namespace std;
using namespace dramcore;

std::function<Address(uint64_t)> dramcore::AddressMapping;

Config::Config(std::string config_file)
{
    INIReader reader(config_file);

    if(reader.ParseError() < 0) {
        std::cerr << "Can't load config file - " << config_file << endl;
        AbruptExit(__FILE__, __LINE__);
    }

    // System/controller Parameters
    protocol = GetDRAMProtocol(reader.Get("dram_structure", "protocol", "DDR3"));
    channel_size = static_cast<uint32_t>(reader.GetInteger("system", "channel_size", 1024));
    channels = static_cast<uint32_t>(reader.GetInteger("system", "channels", 1));
    bus_width = static_cast<uint32_t>(reader.GetInteger("system", "bus_width", 64));
    address_mapping = reader.Get("system", "address_mapping", "chrobabgraco");
    queue_structure = reader.Get("system", "queue_structure", "PER_BANK");
    queue_size = static_cast<uint32_t>(reader.GetInteger("system", "queue_size", 16));
    refresh_strategy = reader.Get("system", "refresh_strategy", "RANK_LEVEL_STAGGERED");
    enable_self_refresh = reader.GetBoolean("system", "enable_self_refresh", false);
    idle_cycles_for_self_refresh = static_cast<uint32_t>(reader.GetInteger("system", "idle_cycles_for_self_refresh", 1000));
    aggressive_precharging_enabled = reader.GetBoolean("system", "aggressive_precharging_enabled", false);
    req_buffering_enabled = reader.GetBoolean("system", "req_buffering_enabled", false);

    // DRAM organization
    bankgroups = static_cast<uint32_t>(reader.GetInteger("dram_structure", "bankgroups", 2));
    banks_per_group = static_cast<uint32_t>(reader.GetInteger("dram_structure", "banks_per_group", 2));
    bool bankgroup_enable = reader.GetBoolean("dram_structure", "bankgroup_enable", true);
    // GDDR5 can chose to enable/disable bankgroups
    if (!bankgroup_enable) {  // aggregating all banks to one group 
        banks_per_group *= bankgroups;
        bankgroups = 1;
    }
    banks = bankgroups * banks_per_group;
    rows = static_cast<uint32_t>(reader.GetInteger("dram_structure", "rows", 1 << 16));
    columns = static_cast<uint32_t>(reader.GetInteger("dram_structure", "columns", 1 << 10));
    device_width = static_cast<uint32_t>(reader.GetInteger("dram_structure", "device_width", 8));
    BL = static_cast<uint32_t>(reader.GetInteger("dram_structure", "BL", 8));

    if (IsHMC()) {  // need to do sanity check and overwrite some values...
        num_links = static_cast<unsigned int>(reader.GetInteger("hmc", "num_links", 4));
        num_dies = static_cast<unsigned int>(reader.GetInteger("hmc", "num_dies", 8));
        link_width = static_cast<unsigned int>(reader.GetInteger("hmc", "link_width", 16));
        // there is a 12.5 we will just use 12 or 13 to simplify coding...
        link_speed = static_cast<unsigned int>(reader.GetInteger("hmc", "link_speed", 30));
        block_size = static_cast<unsigned int>(reader.GetInteger("hmc", "block_size", 32));
        xbar_queue_depth = static_cast<unsigned int>(reader.GetInteger("hmc", "xbar_queue_depth", 16));
        
        // sanity checks 
        if (num_links !=2 && num_links != 4) {
            cerr << "HMC can only have 2 or 4 links!" << endl;
            AbruptExit(__FILE__, __LINE__);
        }
        if (num_dies != 4 && num_dies != 8) {
            cerr << "HMC can only have 4/8 layers of dies!" << endl;
            AbruptExit(__FILE__, __LINE__);
        }
        if (link_width != 4 && link_width != 8 && link_width != 16) {
            cerr << "HMC link width can only be 4 (quater), 8 (half) or 16 (full)!" << endl;
            AbruptExit(__FILE__, __LINE__);
        } 
        if (link_speed != 10000 && link_speed != 12500 && link_speed != 15000 && 
            link_speed != 25000 && link_speed != 28000 && link_speed != 30000  ) {
            cerr << "HMC speed options: 12/13, 15, 25, 28, 30 Gbps" << endl;
            AbruptExit(__FILE__, __LINE__);
        }
        if (block_size != 32 && block_size != 64 && block_size != 128 && block_size != 256) {
            cerr << "HMC block size options: 32, 64, 128, 256 (bytes)!" << endl;
            AbruptExit(__FILE__, __LINE__);
        }
        if (channels != 16 && channels != 32) {
            // vaults are basically channels here 
            cerr << "HMC channel options: 16/32" << endl;
            AbruptExit(__FILE__, __LINE__);
        }
        
        // the BL for is determined by max block_size, which is a multiple of 32B
        // each "device" transfer 32b per half cycle, i.e. 8B per cycle
        // therefore BL is 4 for 32B block size
        BL = block_size * 8 / 32;  

        // A lot of the following parameters are not configurable 
        // according to the spec, so we just set them here
        rows = 65536;
        columns = 16;
        // TODO column access granularity(16B) != device width (4B), oooooooooops
        // meaning that for each column access, 
        // and the (min) block size is 32, which is exactly BL = 8, and BL can be up to 64... really?
        device_width = 32; 
        bus_width = 32;
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
    tCK = reader.GetReal("timing", "tCK", 1.0);
    AL = static_cast<uint32_t>(reader.GetInteger("timing", "AL", 0));
    CL = static_cast<uint32_t>(reader.GetInteger("timing", "CL", 12));
    CWL = static_cast<uint32_t>(reader.GetInteger("timing", "CWL", 12));
    tCCD_L = static_cast<uint32_t>(reader.GetInteger("timing", "tCCD_L", 6));
    tCCD_S = static_cast<uint32_t>(reader.GetInteger("timing", "tCCD_S", 4));
    tRTRS = static_cast<uint32_t>(reader.GetInteger("timing", "tRTRS", 2));
    tRTP = static_cast<uint32_t>(reader.GetInteger("timing", "tRTP", 5));
    tWTR_L = static_cast<uint32_t>(reader.GetInteger("timing", "tWTR_L", 5));
    tWTR_S = static_cast<uint32_t>(reader.GetInteger("timing", "tWTR_S", 5));
    tWR = static_cast<uint32_t>(reader.GetInteger("timing", "tWR", 10));
    tRP = static_cast<uint32_t>(reader.GetInteger("timing", "tRP", 10));
    tRRD_L = static_cast<uint32_t>(reader.GetInteger("timing", "tRRD_L", 4));
    tRRD_S = static_cast<uint32_t>(reader.GetInteger("timing", "tRRD_S", 4));
    tRAS = static_cast<uint32_t>(reader.GetInteger("timing", "tRAS", 24));
    tRCD = static_cast<uint32_t>(reader.GetInteger("timing", "tRCD", 10));
    tRFC = static_cast<uint32_t>(reader.GetInteger("timing", "tRFC", 74));
    tRC = tRAS + tRP;
    tCKE = static_cast<uint32_t>(reader.GetInteger("timing", "tCKE", 6));
    tCKESR = static_cast<uint32_t>(reader.GetInteger("timing", "tCKESR", 12));
    tXS = static_cast<uint32_t>(reader.GetInteger("timing", "tXS", 432)); 
    tXP = static_cast<uint32_t>(reader.GetInteger("timing", "tXP", 8));
    tRFCb = static_cast<uint32_t>(reader.GetInteger("timing", "tRFCb", 20));
    tREFI = static_cast<uint32_t>(reader.GetInteger("timing", "tREFI", 7800));
    tREFIb = static_cast<uint32_t>(reader.GetInteger("timing", "tREFIb", 1950));
    tFAW = static_cast<uint32_t>(reader.GetInteger("timing", "tFAW", 50));
    tRPRE = static_cast<uint32_t>(reader.GetInteger("timing", "tRPRE", 1));
    tWPRE = static_cast<uint32_t>(reader.GetInteger("timing", "tWPRE", 1));

    // LPDDR4 and GDDR5
    tPPD = static_cast<uint32_t>(reader.GetInteger("timing", "tPPD", 0));

    // GDDR5 only 
    t32AW = static_cast<uint32_t>(reader.GetInteger("timing", "t32AW", 330));
    tRCDRD = static_cast<uint32_t>(reader.GetInteger("timing", "tRCDRD", 24));
    tRCDWR = static_cast<uint32_t>(reader.GetInteger("timing", "tRCDWR", 20));


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

    // Power-related parameters
    double VDD = reader.GetReal("power", "VDD", 1.2);
    double IDD0 = reader.GetReal("power", "IDD0", 48);
    double IDD2P = reader.GetReal("power", "IDD2P", 25);
    double IDD2N = reader.GetReal("power", "IDD2N", 34);
    double IDD3P = reader.GetReal("power", "IDD3P", 37);
    double IDD3N = reader.GetReal("power", "IDD3N", 43);
    double IDD4W = reader.GetReal("power", "IDD4W", 123);
    double IDD4R = reader.GetReal("power", "IDD4R", 135);
    double IDD5AB = reader.GetReal("power", "IDD5AB", 250);  // all-bank ref
    double IDD5PB = reader.GetReal("power", "IDD5PB", 5);  // per-bank ref
    double IDD6x = reader.GetReal("power", "IDD6x", 31);  // this changes with temp

    // energy increments per command/cycle, calculated as voltage * current * time(in cycles)
    // units are V * mA * Cycles and if we convert cycles to ns then it's exactly pJ in energy
    // and because a command take effects on all devices per rank, also multiply that number
    double devices = static_cast<double>(devices_per_rank);
    act_energy_inc = VDD * (IDD0 * tRC - (IDD3N * tRAS + IDD2N * tRP)) * devices;
    read_energy_inc = VDD * (IDD4R - IDD3N) * burst_cycle * devices;
    write_energy_inc = VDD * (IDD4W - IDD3N) * burst_cycle * devices;
    ref_energy_inc = VDD * (IDD5AB - IDD3N) * tRFC * devices;
    refb_energy_inc = VDD * (IDD5PB - IDD3N) * tRFCb * devices;
    // the following are added per cycle
    act_stb_energy_inc = VDD * IDD3N * devices;
    pre_stb_energy_inc = VDD * IDD2N * devices;
    pre_pd_energy_inc = VDD * IDD2P * devices;
    sref_energy_inc = VDD * IDD6x * devices;

    // Other Parameters
    // give a prefix instead of specify the output name one by one... 
    // this would allow outputing to a directory and you can always override these values
    output_prefix = reader.Get("other", "output_prefix", "dramsim3-output-");
    epoch_period = static_cast<uint32_t>(reader.GetInteger("other", "epoch_period", 100000));
    stats_file = reader.Get("other", "stats_file", output_prefix + "stats.txt");
    cummulative_stats_file = reader.Get("other", "cummulative_stats_file", output_prefix + "cummulative-stats.txt");
    epoch_stats_file = reader.Get("other", "epoch_stats_file", output_prefix + "epoch-stats.txt");
    stats_file_csv = reader.Get("other", "stats_file", output_prefix + "stats.csv");
    cummulative_stats_file_csv = reader.Get("other", "cummulative_stats_file", output_prefix + "cummulative_stats.csv");
    epoch_stats_file_csv = reader.Get("other", "epoch_stats_file", output_prefix + "epoch-stats.csv");

    channel_width = LogBase2(channels);
    rank_width = LogBase2(ranks);
    bankgroup_width = LogBase2(bankgroups);
    bank_width = LogBase2(banks_per_group);
    row_width = LogBase2(rows);
    column_width = LogBase2(columns);
    uint32_t bytes_offset = LogBase2(bus_width / 8);
    request_size_bytes = bus_width / 8 * BL;  // transaction size in bytes

    // for each address given, because we're transimitting trascation_size bytes per transcation
    // therefore there will be throwaway_bits not used in the address
    // part of it is due to the bytes offset, the other part is the burst len 
    // (same as column auto increment)
    // so effectively only column_width_ -(throwaway_bits - bytes_offset) will be used in column addressing
    throwaway_bits = LogBase2(request_size_bytes);
    column_width -= (throwaway_bits - bytes_offset);

    SetAddressMapping();

#ifdef DEBUG_OUTPUT
    cout << "Address bits:" << endl;
    cout << setw(10) << "Channel " << channel_width << endl;
    cout << setw(10) << "Rank " << rank_width << endl;
    cout << setw(10) << "Bankgroup " << bankgroup_width << endl;
    cout << setw(10) << "Bank " << bank_width << endl;
    cout << setw(10) << "Row " << row_width << endl;
    cout << setw(10) << "Column " << column_width << endl;
#endif

    ideal_memory_latency = static_cast<uint32_t>(reader.GetInteger("timing", "ideal_memory_latency", 10));

}


DRAMProtocol Config::GetDRAMProtocol(std::string protocol_str) {
    std::map<std::string, DRAMProtocol> protocol_pairs = {
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

    if(protocol_pairs.find(protocol_str) == protocol_pairs.end()) {
        cout << "Unkwown/Unsupported DRAM Protocol: " << protocol_str << " Aborting!" << endl;
        AbruptExit(__FILE__, __LINE__);
    }

#ifdef DEBUG_OUTPUT
    cout << "DRAM Procotol " << protocol_str << endl;
#endif 
    return protocol_pairs[protocol_str];
}


void Config::CalculateSize() {
    devices_per_rank = bus_width / device_width;

    // The calculation for different protocols are different,
    // Some take into account of the prefetch/burst length in columns some don't
    // So instead of hard coding it into the ini files, 
    // calculating them here would be a better option
    uint32_t megs_per_bank, megs_per_rank;
    
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
        // had to hard code here since it has nothing to do with the width
        megs_per_bank = (rows * 256) >> 20 ;  // 256B page size
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

void Config::SetAddressMapping() {
    // has to strictly follow the order of chan, rank, bg, bank, row, col
    int field_pos[] = {0, 0, 0, 0, 0, 0};
    int field_widths[] = {0, 0, 0, 0, 0, 0};

    if (address_mapping.size() != 12) {
        cerr << "Unknown address mapping (6 fields each 2 chars required)" << endl;
        AbruptExit(__FILE__, __LINE__);
    }

    // get address mapping position fields from config
    // each field must be 2 chars
    std::vector<std::string> fields;
    for (int i = 0; i < address_mapping.size(); i += 2) {
        std::string token = address_mapping.substr(i, 2);
        fields.push_back(token);
    }

    int pos = throwaway_bits;
    for (int i = fields.size() - 1; i >=0 ; i--) {
        // do this in reverse order so that it matches the 
        // sequence of the input string
        if (fields[i] == "ch") {
            field_pos[0] = pos;
            field_widths[0] = channel_width;
            pos += channel_width;
        } else if (fields[i] == "ra") {
            field_pos[1] = pos;
            field_widths[1] = rank_width;
            pos += rank_width;
        } else if (fields[i] == "bg") {
            field_pos[2] = pos;
            field_widths[2] = bankgroup_width;
            pos += bankgroup_width;
        } else if (fields[i] == "ba") {
            field_pos[3] = pos;
            field_widths[3] = bank_width;
            pos += bank_width;
        } else if (fields[i] == "ro") {
            field_pos[4] = pos;
            field_widths[4] = row_width;
            pos += row_width;
        } else if (fields[i] == "co") {
            field_pos[5] = pos;
            field_widths[5] = column_width;
            pos += column_width;
        } else {
            cerr << "Unrecognized field: " << fields[i] << endl;
            AbruptExit(__FILE__, __LINE__);
        }
    }

    AddressMapping = [field_pos, field_widths](uint64_t hex_addr) {
        uint32_t channel = 0, rank = 0, bankgroup = 0, bank = 0, row = 0, column = 0;
        channel = ModuloWidth(hex_addr, field_widths[0], field_pos[0]);
        rank = ModuloWidth(hex_addr, field_widths[1], field_pos[1]);
        bankgroup = ModuloWidth(hex_addr, field_widths[2], field_pos[2]);
        bank = ModuloWidth(hex_addr, field_widths[3], field_pos[3]);
        row = ModuloWidth(hex_addr, field_widths[4], field_pos[4]);
        column = ModuloWidth(hex_addr, field_widths[5], field_pos[5]);
        return Address(channel, rank, bankgroup, bank, row, column);
    };
}
