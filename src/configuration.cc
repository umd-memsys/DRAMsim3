#include "configuration.h"
#include "../ext/inih/src/INIReader.h"
#include "common.h"

namespace dramsim3 {

std::function<Address(uint64_t)> AddressMapping;

Config::Config(std::string config_file, std::string out_dir)
    : output_dir(out_dir) {
    INIReader reader(config_file);

    if (reader.ParseError() < 0) {
        std::cerr << "Can't load config file - " << config_file << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

    // System/controller Parameters
    protocol =
        GetDRAMProtocol(reader.Get("dram_structure", "protocol", "DDR3"));
    channel_size =
        static_cast<int>(reader.GetInteger("system", "channel_size", 1024));
    channels = static_cast<int>(reader.GetInteger("system", "channels", 1));
    bus_width = static_cast<int>(reader.GetInteger("system", "bus_width", 64));
    address_mapping = reader.Get("system", "address_mapping", "chrobabgraco");
    queue_structure = reader.Get("system", "queue_structure", "PER_BANK");
    queue_size =
        static_cast<int>(reader.GetInteger("system", "queue_size", 16));
    refresh_strategy =
        reader.Get("system", "refresh_strategy", "RANK_LEVEL_STAGGERED");
    enable_self_refresh =
        reader.GetBoolean("system", "enable_self_refresh", false);
    idle_cycles_for_self_refresh = static_cast<int>(
        reader.GetInteger("system", "idle_cycles_for_self_refresh", 1000));
    aggressive_precharging_enabled =
        reader.GetBoolean("system", "aggressive_precharging_enabled", false);

    // DRAM organization
    bankgroups =
        static_cast<int>(reader.GetInteger("dram_structure", "bankgroups", 2));
    banks_per_group = static_cast<int>(
        reader.GetInteger("dram_structure", "banks_per_group", 2));
    bool bankgroup_enable =
        reader.GetBoolean("dram_structure", "bankgroup_enable", true);
    // GDDR5 can chose to enable/disable bankgroups
    if (!bankgroup_enable) {  // aggregating all banks to one group
        banks_per_group *= bankgroups;
        bankgroups = 1;
    }
    enable_hbm_dual_cmd =
        reader.GetBoolean("dram_structure", "hbm_dual_cmd", true);
    enable_hbm_dual_cmd &= IsHBM();  // Make sure only HBM enables this
    banks = bankgroups * banks_per_group;
    rows =
        static_cast<int>(reader.GetInteger("dram_structure", "rows", 1 << 16));
    columns = static_cast<int>(
        reader.GetInteger("dram_structure", "columns", 1 << 10));
    device_width = static_cast<int>(
        reader.GetInteger("dram_structure", "device_width", 8));
    BL = static_cast<int>(reader.GetInteger("dram_structure", "BL", 8));
    num_dies = static_cast<unsigned int>(
        reader.GetInteger("dram_structure", "num_dies", 1));

    // HMC Specific parameters
    num_links =
        static_cast<unsigned int>(reader.GetInteger("hmc", "num_links", 4));
    link_width =
        static_cast<unsigned int>(reader.GetInteger("hmc", "link_width", 16));
    link_speed =
        static_cast<unsigned int>(reader.GetInteger("hmc", "link_speed", 30));
    block_size =
        static_cast<unsigned int>(reader.GetInteger("hmc", "block_size", 32));
    xbar_queue_depth = static_cast<unsigned int>(
        reader.GetInteger("hmc", "xbar_queue_depth", 16));

    ProtocolAdjust();

    // calculate rank and re-calculate channel_size
    devices_per_rank = bus_width / device_width;
    int page_size = columns * device_width / 8;  // page size in bytes
    int megs_per_bank = page_size * (rows / 1024) / 1024;
    int megs_per_rank = megs_per_bank * banks * devices_per_rank;

    if (megs_per_rank > channel_size) {
        std::cout << "WARNING: Cannot create memory system of size "
                  << channel_size
                  << "MB with given device choice! Using default size "
                  << megs_per_rank << " instead!" << std::endl;
        ranks = 1;
        channel_size = megs_per_rank;
    } else {
        ranks = channel_size / megs_per_rank;
        channel_size =
            ranks * megs_per_rank;  // reset in case users inputs a weird number
    }

    SetAddressMapping();

    // Timing Parameters
    // TODO there is no need to keep all of these variables, they should just be
    // temporary ultimately we only need cmd to cmd Timing instead of these
    tCK = reader.GetReal("timing", "tCK", 1.0);
    AL = static_cast<int>(reader.GetInteger("timing", "AL", 0));
    CL = static_cast<int>(reader.GetInteger("timing", "CL", 12));
    CWL = static_cast<int>(reader.GetInteger("timing", "CWL", 12));
    tCCD_L = static_cast<int>(reader.GetInteger("timing", "tCCD_L", 6));
    tCCD_S = static_cast<int>(reader.GetInteger("timing", "tCCD_S", 4));
    tRTRS = static_cast<int>(reader.GetInteger("timing", "tRTRS", 2));
    tRTP = static_cast<int>(reader.GetInteger("timing", "tRTP", 5));
    tWTR_L = static_cast<int>(reader.GetInteger("timing", "tWTR_L", 5));
    tWTR_S = static_cast<int>(reader.GetInteger("timing", "tWTR_S", 5));
    tWR = static_cast<int>(reader.GetInteger("timing", "tWR", 10));
    tRP = static_cast<int>(reader.GetInteger("timing", "tRP", 10));
    tRRD_L = static_cast<int>(reader.GetInteger("timing", "tRRD_L", 4));
    tRRD_S = static_cast<int>(reader.GetInteger("timing", "tRRD_S", 4));
    tRAS = static_cast<int>(reader.GetInteger("timing", "tRAS", 24));
    tRCD = static_cast<int>(reader.GetInteger("timing", "tRCD", 10));
    tRFC = static_cast<int>(reader.GetInteger("timing", "tRFC", 74));
    tRC = tRAS + tRP;
    tCKE = static_cast<int>(reader.GetInteger("timing", "tCKE", 6));
    tCKESR = static_cast<int>(reader.GetInteger("timing", "tCKESR", 12));
    tXS = static_cast<int>(reader.GetInteger("timing", "tXS", 432));
    tXP = static_cast<int>(reader.GetInteger("timing", "tXP", 8));
    tRFCb = static_cast<int>(reader.GetInteger("timing", "tRFCb", 20));
    tREFI = static_cast<int>(reader.GetInteger("timing", "tREFI", 7800));
    tREFIb = static_cast<int>(reader.GetInteger("timing", "tREFIb", 1950));
    tFAW = static_cast<int>(reader.GetInteger("timing", "tFAW", 50));
    tRPRE = static_cast<int>(reader.GetInteger("timing", "tRPRE", 1));
    tWPRE = static_cast<int>(reader.GetInteger("timing", "tWPRE", 1));

    // LPDDR4 and GDDR5
    tPPD = static_cast<int>(reader.GetInteger("timing", "tPPD", 0));

    // GDDR5 only
    t32AW = static_cast<int>(reader.GetInteger("timing", "t32AW", 330));
    tRCDRD = static_cast<int>(reader.GetInteger("timing", "tRCDRD", 24));
    tRCDWR = static_cast<int>(reader.GetInteger("timing", "tRCDWR", 20));

    ideal_memory_latency = static_cast<int>(
        reader.GetInteger("timing", "ideal_memory_latency", 10));

    // calculated timing
    RL = AL + CL;
    WL = AL + CWL;
    read_delay = RL + burst_cycle;
    write_delay = WL + burst_cycle;

    // Power-related parameters
    double VDD = reader.GetReal("power", "VDD", 1.2);
    double IDD0 = reader.GetReal("power", "IDD0", 48);
    double IDD2P = reader.GetReal("power", "IDD2P", 25);
    double IDD2N = reader.GetReal("power", "IDD2N", 34);
    // double IDD3P = reader.GetReal("power", "IDD3P", 37);
    double IDD3N = reader.GetReal("power", "IDD3N", 43);
    double IDD4W = reader.GetReal("power", "IDD4W", 123);
    double IDD4R = reader.GetReal("power", "IDD4R", 135);
    double IDD5AB = reader.GetReal("power", "IDD5AB", 250);  // all-bank ref
    double IDD5PB = reader.GetReal("power", "IDD5PB", 5);    // per-bank ref
    double IDD6x =
        reader.GetReal("power", "IDD6x", 31);  // this changes with temp

    // energy increments per command/cycle, calculated as voltage * current *
    // time(in cycles) units are V * mA * Cycles and if we convert cycles to ns
    // then it's exactly pJ in energy and because a command take effects on all
    // devices per rank, also multiply that number
    double devices = static_cast<double>(devices_per_rank);
    act_energy_inc =
        VDD * (IDD0 * tRC - (IDD3N * tRAS + IDD2N * tRP)) * devices;
    read_energy_inc = VDD * (IDD4R - IDD3N) * burst_cycle * devices;
    write_energy_inc = VDD * (IDD4W - IDD3N) * burst_cycle * devices;
    ref_energy_inc = VDD * (IDD5AB - IDD3N) * tRFC * devices;
    refb_energy_inc = VDD * (IDD5PB - IDD3N) * tRFCb * devices;
    // the following are added per cycle
    act_stb_energy_inc = VDD * IDD3N * devices;
    pre_stb_energy_inc = VDD * IDD2N * devices;
    pre_pd_energy_inc = VDD * IDD2P * devices;
    sref_energy_inc = VDD * IDD6x * devices;

    epoch_period =
        static_cast<int>(reader.GetInteger("other", "epoch_period", 100000));
    // determine how much output we want:
    // -1: no file output at all
    // 0: no epoch file output, only outputs the summary in the end
    // 1: default value, adds epoch CSV output on level 0
    // 2: adds histogram epoch outputs in a different CSV format
    // 3: excessive output, adds epoch text output on level 2
    output_level = reader.GetInteger("other", "output_level", 1);
    // Other Parameters
    // give a prefix instead of specify the output name one by one...
    // this would allow outputing to a directory and you can always override
    // these values
    if (!DirExist(output_dir)) {
        std::cout << "WARNING: Output directory " << output_dir
                  << " not exists! Using current directory for output!"
                  << std::endl;
        output_dir = "./";
    } else {
        output_dir = output_dir + "/";
    }
    output_prefix =
        output_dir + reader.Get("other", "output_prefix", "dramsim_");
    stats_file = reader.Get("other", "stats_file", output_prefix + "stats.txt");
    epoch_stats_file = reader.Get("other", "epoch_stats_file",
                                  output_prefix + "epoch_stats.txt");
    stats_file_csv =
        reader.Get("other", "stats_file", output_prefix + "stats.csv");
    // cummulative_stats_file_csv = reader.Get("other",
    // "cummulative_stats_file", output_prefix + "cummulative_stats.csv");
    epoch_stats_file_csv = reader.Get("other", "epoch_stats_file",
                                      output_prefix + "epoch_stats.csv");
    histo_stats_file_csv = reader.Get("other", "histo_stat_file",
                                      output_prefix + "histo_stats.csv");

    // Thermal simulation parameters
    power_epoch_period = static_cast<int>(
        reader.GetInteger("thermal", "power_epoch_period", 100000));
    // numXgrids = static_cast<int>(reader.GetInteger("thermal",
    // "numXgrids", 1));  numYgrids =
    // static_cast<int>(reader.GetInteger("thermal", "numYgrids", 1));

    matX = static_cast<int>(reader.GetInteger("thermal", "matX", 512));
    matY = static_cast<int>(reader.GetInteger("thermal", "matY", 512));
    // RowTile = static_cast<int>(reader.GetInteger("thermal", "RowTile", 1));
    numXgrids = rows / matX;
    TileRowNum = rows;

    numYgrids = columns * device_width / matY;
    bank_asr = (double)numXgrids / numYgrids;
    RowTile = 1;
    if (bank_asr > 4 && banks_per_group == 1) {
        // YZY: I set the aspect ratio as 4
        // I assume if bank_asr <= 4, the dimension can be corrected by
        // arranging banks/vaults
        while (RowTile * RowTile * 4 < bank_asr) {
            RowTile *= 2;
        }
        // RowTile = numXgrids / (numYgrids * 8);
#ifdef DEBUG_OUTPUT
        std::cout << "RowTile = " << RowTile << std::endl;
#endif  // DEBUG_OUTPUT
        numXgrids = numXgrids / RowTile;
        TileRowNum = TileRowNum / RowTile;
        numYgrids = numYgrids * RowTile;
        bank_asr = (double)numXgrids / numYgrids;
    } else {
#ifdef DEBUG_OUTPUT
        std::cout << "No Need to Tile Rows\n";
#endif  // DEBUG_OUTPUT
    }

    // Thermal simulation parameters
    loc_mapping = reader.Get("thermal", "loc_mapping", "");
    bank_order =
        static_cast<int>(reader.GetInteger("thermal", "bank_order", 1));
    bank_layer_order =
        static_cast<int>(reader.GetInteger("thermal", "bank_layer_order", 0));
    numRowRefresh = static_cast<int>(ceil(rows / (64 * 1e6 / (tREFI * tCK))));
    ChipX = reader.GetReal("thermal", "ChipX", 0.01);
    ChipY = reader.GetReal("thermal", "ChipY", 0.01);
    Tamb0 = reader.GetReal("thermal", "Tamb0", 40);
    epoch_temperature_file_csv =
        reader.Get("other", "epoch_temperature_file",
                   output_prefix + "epoch_power_temperature.csv");
    epoch_max_temp_file_csv = reader.Get("other", "epoch_max_temp_file",
                                         output_prefix + "epoch_max_temp.csv");
    final_temperature_file_csv =
        reader.Get("other", "final_temperature_file",
                   output_prefix + "final_power_temperature.csv");
    bank_position_csv = reader.Get("other", "final_temperature_file",
                                   output_prefix + "bank_position.csv");
}

DRAMProtocol Config::GetDRAMProtocol(std::string protocol_str) {
    std::map<std::string, DRAMProtocol> protocol_pairs = {
        {"DDR3", DRAMProtocol::DDR3},     {"DDR4", DRAMProtocol::DDR4},
        {"GDDR5", DRAMProtocol::GDDR5},   {"GDDR5X", DRAMProtocol::GDDR5X},
        {"LPDDR", DRAMProtocol::LPDDR},   {"LPDDR3", DRAMProtocol::LPDDR3},
        {"LPDDR4", DRAMProtocol::LPDDR4}, {"HBM", DRAMProtocol::HBM},
        {"HBM2", DRAMProtocol::HBM2},     {"HMC", DRAMProtocol::HMC}};

    if (protocol_pairs.find(protocol_str) == protocol_pairs.end()) {
        std::cout << "Unkwown/Unsupported DRAM Protocol: " << protocol_str
                  << " Aborting!" << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

    return protocol_pairs[protocol_str];
}

void Config::ProtocolAdjust() {
    // Sanity checks and adjust parameters for different protocols
    // The most messed up thing that has to be done here is that
    // every protocol has a different defination of "column",
    // in DDR3/4, each column is exactly device_width bits,
    // but in GDDR5, a column is device_width * BL bits
    // and for HBM each column is device_width * 2 (prefetch)
    // as a result, different protocol has different method of calculating
    // page size, and address mapping...
    // To make life easier, we regulate the use of the term "column"
    // to only represent physical column (device width)
    if (IsHMC()) {
        if (num_links != 2 && num_links != 4) {
            std::cerr << "HMC can only have 2 or 4 links!" << std::endl;
            AbruptExit(__FILE__, __LINE__);
        }
        if (num_dies != 4 && num_dies != 8) {
            std::cerr << "HMC can only have 4/8 layers of dies!" << std::endl;
            AbruptExit(__FILE__, __LINE__);
        }
        if (link_width != 4 && link_width != 8 && link_width != 16) {
            std::cerr
                << "HMC link width can only be 4 (quater), 8 (half) or 16 "
                   "(full)!"
                << std::endl;
            AbruptExit(__FILE__, __LINE__);
        }
        if (link_speed != 10000 && link_speed != 12500 && link_speed != 15000 &&
            link_speed != 25000 && link_speed != 28000 && link_speed != 30000) {
            std::cerr << "HMC speed options: 12/13, 15, 25, 28, 30 Gbps"
                      << std::endl;
            AbruptExit(__FILE__, __LINE__);
        }
        // block_size = 0 to simulate ideal bandwidth situation
        if (block_size != 0 && block_size != 32 && block_size != 64 &&
            block_size != 128 && block_size != 256) {
            std::cerr << "HMC block size options: 32, 64, 128, 256 (bytes)!"
                      << std::endl;
            AbruptExit(__FILE__, __LINE__);
        }
        if (channels != 16 && channels != 32) {
            // vaults are basically channels here
            std::cerr << "HMC channel options: 16/32" << std::endl;
            AbruptExit(__FILE__, __LINE__);
        }
        // the BL for is determined by max block_size, which is a multiple of
        // 32B each "device" transfer 32b per half cycle, i.e. 8B per cycle
        // therefore BL is 4 for 32B block size
        BL = block_size * 8 / 32;

        // A lot of the following parameters are not configurable
        // according to the spec, so we just set them here
        rows = 65536;
        // meaning that for each column access,
        // and the (min) block size is 32, which is exactly BL = 8, and BL can
        // be up to 64... really?
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
    } else if (IsHBM()) {
        // HBM is more flexible layout
        if (num_dies != 2 && num_dies != 4 && num_dies != 8) {
            std::cerr << "HBM die options: 2/4/8" << std::endl;
            AbruptExit(__FILE__, __LINE__);
        }
    }
    // set burst cycle according to protocol
    if (protocol == DRAMProtocol::GDDR5) {
        burst_cycle = (BL == 0) ? 0 : BL / 4;
        BL = (BL == 0) ? 8 : BL;
    } else if (protocol == DRAMProtocol::GDDR5X) {
        burst_cycle = (BL == 0) ? 0 : BL / 8;
        BL = (BL == 0) ? 8 : BL;
    } else {
        burst_cycle = (BL == 0) ? 0 : BL / 2;
        BL = (BL == 0) ? (IsHBM() ? 4 : 8) : BL;
    }

    // re-adjust columns from spec columns to actual columns
    if (IsGDDR()) {
        columns *= BL;
    } else if (IsHBM()) {
        columns *= 2;
    } else if (IsHMC()) {  // 256B pages
        columns = 256 * 8 / device_width;
    }
}

void Config::SetAddressMapping() {
    // has to strictly follow the order of chan, rank, bg, bank, row, col
    channel_width = LogBase2(channels);
    rank_width = LogBase2(ranks);
    bankgroup_width = LogBase2(bankgroups);
    bank_width = LogBase2(banks_per_group);
    row_width = LogBase2(rows);
    column_width = LogBase2(columns);

    // memory addresses are byte addressable, but each request comes with
    // multiple bytes because of bus width, and burst length
    int bus_offset = LogBase2(bus_width / 8);
    request_size_bytes = bus_width / 8 * BL;
    int masked_col_bits = LogBase2(BL);
    unsigned col_mask = (0xFFFFFFFF << masked_col_bits);

#ifdef DEBUG_OUTPUT
    std::cout << "Address bits:" << std::endl;
    std::cout << setw(10) << "Channel " << channel_width << std::endl;
    std::cout << setw(10) << "Rank " << rank_width << std::endl;
    std::cout << setw(10) << "Bankgroup " << bankgroup_width << std::endl;
    std::cout << setw(10) << "Bank " << bank_width << std::endl;
    std::cout << setw(10) << "Row " << row_width << std::endl;
    std::cout << setw(10) << "Column " << column_width << std::endl;
#endif

    int field_pos[] = {0, 0, 0, 0, 0, 0};
    int field_widths[] = {0, 0, 0, 0, 0, 0};

    if (address_mapping.size() != 12) {
        std::cerr << "Unknown address mapping (6 fields each 2 chars required)"
                  << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

    // get address mapping position fields from config
    // each field must be 2 chars
    std::vector<std::string> fields;
    for (size_t i = 0; i < address_mapping.size(); i += 2) {
        std::string token = address_mapping.substr(i, 2);
        fields.push_back(token);
    }

    // starting position ignores those caused by bus width
    int pos = bus_offset;
    for (size_t i = fields.size(); i > 0; i--) {
        // do this in reverse order so that it matches the
        // sequence of the input string
        auto field_str = fields[i - 1];
        if (field_str == "ch") {
            field_pos[0] = pos;
            field_widths[0] = channel_width;
            pos += channel_width;
        } else if (field_str == "ra") {
            field_pos[1] = pos;
            field_widths[1] = rank_width;
            pos += rank_width;
        } else if (field_str == "bg") {
            field_pos[2] = pos;
            field_widths[2] = bankgroup_width;
            pos += bankgroup_width;
        } else if (field_str == "ba") {
            field_pos[3] = pos;
            field_widths[3] = bank_width;
            pos += bank_width;
        } else if (field_str == "ro") {
            field_pos[4] = pos;
            field_widths[4] = row_width;
            pos += row_width;
        } else if (field_str == "co") {
            field_pos[5] = pos;
            field_widths[5] = column_width;
            pos += column_width;
        } else {
            std::cerr << "Unrecognized field: " << fields[i] << std::endl;
            AbruptExit(__FILE__, __LINE__);
        }
    }

    AddressMapping = [field_pos, field_widths, col_mask](uint64_t hex_addr) {
        int channel = 0, rank = 0, bankgroup = 0, bank = 0, row = 0, column = 0;
        channel = ModuloWidth(hex_addr, field_widths[0], field_pos[0]);
        rank = ModuloWidth(hex_addr, field_widths[1], field_pos[1]);
        bankgroup = ModuloWidth(hex_addr, field_widths[2], field_pos[2]);
        bank = ModuloWidth(hex_addr, field_widths[3], field_pos[3]);
        row = ModuloWidth(hex_addr, field_widths[4], field_pos[4]);
        column = ModuloWidth(hex_addr, field_widths[5], field_pos[5]);
        column = column & col_mask;
        return Address(channel, rank, bankgroup, bank, row, column);
    };
}

}  // namespace dramsim3
