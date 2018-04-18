#ifndef __CONFIG_H
#define __CONFIG_H

#include <cassert>
#include <cmath>
#include <string>
#include <utility>
#include <vector>
#include "common.h"

namespace dramcore {

enum class DRAMProtocol {
    DDR3,
    DDR4,
    GDDR5,
    GDDR5X,
    LPDDR,
    LPDDR3,
    LPDDR4,
    HBM,
    HBM2,
    HMC,
    SIZE
};

class Config {
   public:
    Config(std::string config_file, std::string out_dir);

    // DRAM physical structure
    DRAMProtocol protocol;
    uint32_t channel_size;
    uint32_t channels;
    uint32_t ranks;
    uint32_t banks;
    uint32_t bankgroups;
    uint32_t banks_per_group;
    uint32_t rows;
    uint32_t columns;
    uint32_t device_width;
    uint32_t bus_width;
    uint32_t devices_per_rank;
    uint32_t BL;

    // Generic DRAM timing parameters
    double tCK;
    uint32_t
        burst_cycle;  // seperate BL with timing since fot GDDRx it's not BL/2
    uint32_t AL;
    uint32_t CL;
    uint32_t CWL;
    uint32_t RL;
    uint32_t WL;
    uint32_t tCCD_L;
    uint32_t tCCD_S;
    uint32_t tRTRS;
    uint32_t tRTP;
    uint32_t tWTR_L;
    uint32_t tWTR_S;
    uint32_t tWR;
    uint32_t tRP;
    uint32_t tRRD_L;
    uint32_t tRRD_S;
    uint32_t tRAS;
    uint32_t tRCD;
    uint32_t tRFC;
    uint32_t tRC;
    // tCKSRE and tCKSRX are only useful for changing clock freq after entering
    // SRE mode we are not doing that, so tCKESR is sufficient
    uint32_t tCKE;
    uint32_t tCKESR;
    uint32_t tXS;
    uint32_t tXP;
    uint32_t tRFCb;
    uint32_t tREFI;
    uint32_t tREFIb;
    uint32_t tFAW;
    uint32_t tRPRE;  // read preamble and write preamble are important
    uint32_t tWPRE;
    uint32_t read_delay;
    uint32_t write_delay;

    // LPDDR4 and GDDR5
    uint32_t tPPD;
    // GDDR5
    uint32_t t32AW;
    uint32_t tRCDRD;
    uint32_t tRCDWR;

    // pre calculated power parameters
    double act_energy_inc;
    double pre_energy_inc;
    double read_energy_inc;
    double write_energy_inc;
    double ref_energy_inc;
    double refb_energy_inc;
    double act_stb_energy_inc;
    double pre_stb_energy_inc;
    double pre_pd_energy_inc;
    double sref_energy_inc;

    // HMC
    uint32_t num_links;
    uint32_t num_dies;
    uint32_t link_width;
    uint32_t link_speed;
    uint32_t num_vaults;
    uint32_t block_size;  // block size in bytes
    uint32_t xbar_queue_depth;

    std::string address_mapping;
    std::string queue_structure;
    uint32_t queue_size;
    std::string refresh_strategy;
    bool enable_self_refresh;
    uint32_t idle_cycles_for_self_refresh;
    bool aggressive_precharging_enabled;
    bool enable_hbm_dual_cmd;

    std::string output_prefix;

    uint32_t epoch_period;
    int output_level;
    std::string output_dir;
    std::string stats_file;
    std::string epoch_stats_file;
    std::string stats_file_csv;
    std::string epoch_stats_file_csv;
    std::string histo_stats_file_csv;

    // Computed parameters
    uint32_t request_size_bytes;
    uint32_t channel_width, rank_width, bankgroup_width, bank_width, row_width,
        column_width;

    bool IsGDDR() const {
        return (protocol == DRAMProtocol::GDDR5 ||
                protocol == DRAMProtocol::GDDR5X);
    }
    bool IsHBM() const {
        return (protocol == DRAMProtocol::HBM ||
                protocol == DRAMProtocol::HBM2);
    }
    bool IsHMC() const { return (protocol == DRAMProtocol::HMC); }
    // yzy: add another function
    bool IsDDR4() const { return (protocol == DRAMProtocol::DDR4); }

    uint32_t ideal_memory_latency;

    // thermal simulator
    std::string loc_mapping;
    uint32_t power_epoch_period;
    uint32_t numRowRefresh;  // number of rows to be refreshed for one time
    double ChipX;
    double ChipY;
    double Tamb0;  // the ambient temperature in [C]
    int numXgrids;
    int numYgrids;
    int matX;
    int matY;
    int bank_order;        // 0: x-direction priority, 1: y-direction priority
    int bank_layer_order;  // 0; low-layer priority, 1: high-layer priority
    int RowTile;
    int TileRowNum;
    double bank_asr;  // the aspect ratio of a bank: #row_bits / #col_bits
    std::string epoch_max_temp_file_csv;
    std::string epoch_temperature_file_csv;
    std::string final_temperature_file_csv;
    std::string bank_position_csv;

   private:
    DRAMProtocol GetDRAMProtocol(std::string protocol_str);
    void ProtocolAdjust();
    void CalculateSize();
    void SetAddressMapping();
};

}  // namespace dramcore
#endif
