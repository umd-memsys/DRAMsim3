#include "configuration.h"

namespace dramsim3 {

std::function<Address(uint64_t)> AddressMapping;
std::function<int(uint64_t)> MapChannel;

Config::Config(std::string config_file, std::string out_dir)
    : output_dir(out_dir), reader_(new INIReader(config_file)) {
    if (reader_->ParseError() < 0) {
        std::cerr << "Can't load config file - " << config_file << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

    // The initialization of the parameters has to be strictly in this order
    // because of internal dependencies
    InitSystemParams();
    InitDRAMParams();
    CalculateSize();
    SetAddressMapping();
    InitTimingParams();
    InitPowerParams();
    InitOtherParams();
#ifdef THERMAL
    InitThermalParams();
#endif  // THERMAL
    delete (reader_);
}

void Config::CalculateSize() {
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
        channel_size = ranks * megs_per_rank;
    }
    return;
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

int Config::GetInteger(const std::string& sec, const std::string& opt,
                       int default_val) const {
    return static_cast<int>(reader_->GetInteger(sec, opt, default_val));
}

void Config::InitDRAMParams() {
    const auto& reader = *reader_;
    protocol =
        GetDRAMProtocol(reader.Get("dram_structure", "protocol", "DDR3"));
    bankgroups = GetInteger("dram_structure", "bankgroups", 2);
    banks_per_group = GetInteger("dram_structure", "banks_per_group", 2);
    bool bankgroup_enable =
        reader.GetBoolean("dram_structure", "bankgroup_enable", true);
    // GDDR5 can chose to enable/disable bankgroups
    if (!bankgroup_enable) {  // aggregating all banks to one group
        banks_per_group *= bankgroups;
        bankgroups = 1;
    }
    banks = bankgroups * banks_per_group;
    rows = GetInteger("dram_structure", "rows", 1 << 16);
    columns = GetInteger("dram_structure", "columns", 1 << 10);
    device_width = GetInteger("dram_structure", "device_width", 8);
    BL = GetInteger("dram_structure", "BL", 8);
    num_dies = GetInteger("dram_structure", "num_dies", 1);
    // HBM specific parameters
    enable_hbm_dual_cmd =
        reader.GetBoolean("dram_structure", "hbm_dual_cmd", true);
    enable_hbm_dual_cmd &= IsHBM();  // Make sure only HBM enables this
    // HMC specific parameters
    num_links = GetInteger("hmc", "num_links", 4);
    link_width = GetInteger("hmc", "link_width", 16);
    link_speed = GetInteger("hmc", "link_speed", 30);
    block_size = GetInteger("hmc", "block_size", 32);
    xbar_queue_depth = GetInteger("hmc", "xbar_queue_depth", 16);
    SanityCheck();
    return;
}

void Config::InitOtherParams() {
    const auto& reader = *reader_;
    epoch_period = GetInteger("other", "epoch_period", 100000);
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
    // stats_file = reader.Get("other", "stats_file", output_prefix +
    // "stats.txt"); epoch_stats_file = reader.Get("other", "epoch_stats_file",
    //                               output_prefix + "epoch_stats.txt");
    // stats_txt_file_csv =
    //     reader.Get("other", "stats_file", output_prefix + "stats.csv");
    // epoch_txt_file_csv = reader.Get("other", "epoch_stats_file",
    //                                   output_prefix + "epoch_stats.csv");
    // histo_stats_txt_file_csv = reader.Get("other", "histo_stat_file",
    //                                   output_prefix + "histo_stats.csv");
    return;
}

void Config::InitPowerParams() {
    const auto& reader = *reader_;
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
    double IDD6x = reader.GetReal("power", "IDD6x", 31);

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
    return;
}

void Config::InitSystemParams() {
    const auto& reader = *reader_;
    channel_size = GetInteger("system", "channel_size", 1024);
    channels = GetInteger("system", "channels", 1);
    bus_width = GetInteger("system", "bus_width", 64);
    address_mapping = reader.Get("system", "address_mapping", "chrobabgraco");
    delay_queue_cycles = GetInteger("system", "delay_queue_cycles", 0);
    queue_structure = reader.Get("system", "queue_structure", "PER_BANK");
    row_buf_policy = reader.Get("system", "row_buf_policy", "OPEN_PAGE");
    cmd_queue_size = GetInteger("system", "cmd_queue_size", 16);
    trans_queue_size = GetInteger("system", "trans_queue_size", 32);
    unified_queue = reader.GetBoolean("system", "unified_queue", false);
    std::string ref_policy =
        reader.Get("system", "refresh_policy", "RANK_LEVEL_STAGGERED");
    if (ref_policy == "RANK_LEVEL_SIMULTANEOUS") {
        refresh_policy = RefreshPolicy::RANK_LEVEL_SIMULTANEOUS;
    } else if (ref_policy == "RANK_LEVEL_STAGGERED") {
        refresh_policy = RefreshPolicy::RANK_LEVEL_STAGGERED;
    } else if (ref_policy == "BANK_LEVEL_STAGGERED") {
        refresh_policy = RefreshPolicy::BANK_LEVEL_STAGGERED;
    } else {
        AbruptExit(__FILE__, __LINE__);
    }

    enable_self_refresh =
        reader.GetBoolean("system", "enable_self_refresh", false);
    sref_threshold = GetInteger("system", "sref_threshold", 1000);
    aggressive_precharging_enabled =
        reader.GetBoolean("system", "aggressive_precharging_enabled", false);
    return;
}

#ifdef THERMAL
void Config::InitThermalParams() {
    const auto& reader = *reader_;
    power_epoch_period = GetInteger("thermal", "power_epoch_period", 100000);
    mat_dim_x = GetInteger("thermal", "mat_dim_x", 512);
    mat_dim_y = GetInteger("thermal", "mat_dim_y", 512);
    // RowTile = GetInteger("thermal", "RowTile", 1));
    numXgrids = rows / mat_dim_x;
    TileRowNum = rows;

    numYgrids = columns * device_width / mat_dim_y;
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
        loc_mapping = reader.Get("thermal", "loc_mapping", "");
        logic_bg_power = reader.GetReal("thermal", "logic_bg_power", 1.0);
        logic_max_power = reader.GetReal("thermal", "logic_max_power", 20.0);
        bank_order = GetInteger("thermal", "bank_order", 1);
        bank_layer_order = GetInteger("thermal", "bank_layer_order", 0);
        numRowRefresh =
            static_cast<int>(ceil(rows / (64 * 1e6 / (tREFI * tCK))));
        chip_dim_x = reader.GetReal("thermal", "chip_dim_x", 0.01);
        chip_dim_y = reader.GetReal("thermal", "chip_dim_y", 0.01);
        amb_temp = reader.GetReal("thermal", "amb_temp", 40);

        // Technically the following ones are in "other" section but they're
        // only available when THREMAL is defined
        epoch_temperature_file_csv =
            reader.Get("other", "epoch_temperature_file",
                       output_prefix + "epoch_power_temperature.csv");
        epoch_max_temp_file_csv =
            reader.Get("other", "epoch_max_temp_file",
                       output_prefix + "epoch_max_temp.csv");
        final_temperature_file_csv =
            reader.Get("other", "final_temperature_file",
                       output_prefix + "final_power_temperature.csv");
        bank_position_csv = reader.Get("other", "final_temperature_file",
                                       output_prefix + "bank_position.csv");
    }
    return;
}
#endif  // THERMAL

void Config::SanityCheck() {
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
        // 32B, each "device" transfer 32b per half cycle
        // therefore BL is 8 for 32B block size
        BL = block_size * 8 / device_width;
    } else if (IsHBM()) {
        // HBM is more flexible layout
        if (num_dies != 2 && num_dies != 4 && num_dies != 8) {
            std::cerr << "HBM die options: 2/4/8" << std::endl;
            AbruptExit(__FILE__, __LINE__);
        }
    }
    // set burst cycle according to protocol
    // We use burst_cycle for timing and use BL for capaticyt calculation
    // BL = 0 simulate perfect BW
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
    // some specs say it has x columns but x is actually the number
    // that took into account of BL.
    if (IsGDDR()) {
        columns *= BL;
    } else if (IsHBM()) {
        columns *= 2;
    }
    return;
}

void Config::InitTimingParams() {
    // Timing Parameters
    // TODO there is no need to keep all of these variables, they should
    // just be temporary, ultimately we only need cmd to cmd Timing
    const auto& reader = *reader_;
    tCK = reader.GetReal("timing", "tCK", 1.0);
    AL = GetInteger("timing", "AL", 0);
    CL = GetInteger("timing", "CL", 12);
    CWL = GetInteger("timing", "CWL", 12);
    tCCD_L = GetInteger("timing", "tCCD_L", 6);
    tCCD_S = GetInteger("timing", "tCCD_S", 4);
    tRTRS = GetInteger("timing", "tRTRS", 2);
    tRTP = GetInteger("timing", "tRTP", 5);
    tWTR_L = GetInteger("timing", "tWTR_L", 5);
    tWTR_S = GetInteger("timing", "tWTR_S", 5);
    tWR = GetInteger("timing", "tWR", 10);
    tRP = GetInteger("timing", "tRP", 10);
    tRRD_L = GetInteger("timing", "tRRD_L", 4);
    tRRD_S = GetInteger("timing", "tRRD_S", 4);
    tRAS = GetInteger("timing", "tRAS", 24);
    tRCD = GetInteger("timing", "tRCD", 10);
    tRFC = GetInteger("timing", "tRFC", 74);
    tRC = tRAS + tRP;
    tCKE = GetInteger("timing", "tCKE", 6);
    tCKESR = GetInteger("timing", "tCKESR", 12);
    tXS = GetInteger("timing", "tXS", 432);
    tXP = GetInteger("timing", "tXP", 8);
    tRFCb = GetInteger("timing", "tRFCb", 20);
    tREFI = GetInteger("timing", "tREFI", 7800);
    tREFIb = GetInteger("timing", "tREFIb", 1950);
    tFAW = GetInteger("timing", "tFAW", 50);
    tRPRE = GetInteger("timing", "tRPRE", 1);
    tWPRE = GetInteger("timing", "tWPRE", 1);

    // LPDDR4 and GDDR5
    tPPD = GetInteger("timing", "tPPD", 0);

    // GDDR5
    t32AW = GetInteger("timing", "t32AW", 330);
    tRCDRD = GetInteger("timing", "tRCDRD", 24);
    tRCDWR = GetInteger("timing", "tRCDWR", 20);

    ideal_memory_latency = GetInteger("timing", "ideal_memory_latency", 10);

    // calculated timing
    RL = AL + CL;
    WL = AL + CWL;
    read_delay = RL + burst_cycle;
    write_delay = WL + burst_cycle;
    return;
}

void Config::SetAddressMapping() {
    // memory addresses are byte addressable, but each request comes with
    // multiple bytes because of bus width, and burst length
    request_size_bytes = bus_width / 8 * BL;
    int shift_bits = LogBase2(request_size_bytes);
    int col_low_bits = LogBase2(BL);
    int actual_col_bits = LogBase2(columns) - col_low_bits;

    // has to strictly follow the order of chan, rank, bg, bank, row, col
    std::map<std::string, int> field_widths;
    field_widths["ch"] = LogBase2(channels);
    field_widths["ra"] = LogBase2(ranks);
    field_widths["bg"] = LogBase2(bankgroups);
    field_widths["ba"] = LogBase2(banks_per_group);
    field_widths["ro"] = LogBase2(rows);
    field_widths["co"] = actual_col_bits;

    if (address_mapping.size() != 12) {
        std::cerr << "Unknown address mapping (6 fields each 2 chars required)"
                  << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

    // // get address mapping position fields from config
    // // each field must be 2 chars
    std::vector<std::string> fields;
    for (size_t i = 0; i < address_mapping.size(); i += 2) {
        std::string token = address_mapping.substr(i, 2);
        fields.push_back(token);
    }

    std::map<std::string, int> field_pos;
    int pos = 0;
    while (!fields.empty()) {
        auto token = fields.back();
        fields.pop_back();
        if (field_widths.find(token) == field_widths.end()) {
            std::cerr << "Unrecognized field: " << token << std::endl;
            AbruptExit(__FILE__, __LINE__);
        }
#ifdef DEBUG_OUTPUT
        std::cout << "Address mapping:" << std::endl;
        std::cout << token << " pos:" << pos << " width:" << field_widths[token]
                  << std::endl;
#endif
        field_pos[token] = pos;
        pos += field_widths[token];
    }
    MapChannel = [field_pos, field_widths, shift_bits](uint64_t hex_addr) {
        hex_addr >>= shift_bits;
        return ModuloWidth(hex_addr, field_widths.at("ch"), field_pos.at("ch"));
    };

    AddressMapping = [field_pos, field_widths, shift_bits](uint64_t hex_addr) {
        hex_addr >>= shift_bits;
        int channel = 0, rank = 0, bankgroup = 0, bank = 0, row = 0, column = 0;
        channel =
            ModuloWidth(hex_addr, field_widths.at("ch"), field_pos.at("ch"));
        rank = ModuloWidth(hex_addr, field_widths.at("ra"), field_pos.at("ra"));
        bankgroup =
            ModuloWidth(hex_addr, field_widths.at("bg"), field_pos.at("bg"));
        bank = ModuloWidth(hex_addr, field_widths.at("ba"), field_pos.at("ba"));
        row = ModuloWidth(hex_addr, field_widths.at("ro"), field_pos.at("ro"));
        column =
            ModuloWidth(hex_addr, field_widths.at("co"), field_pos.at("co"));
        return Address(channel, rank, bankgroup, bank, row, column);
    };
}

}  // namespace dramsim3
