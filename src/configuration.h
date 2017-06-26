#ifndef __CONFIG_H
#define __CONFIG_H

#include <cassert>
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
    Config(std::string config_file);

    //DRAM physical structure
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
    uint32_t BL;

    //Generic DRAM timing parameters
    uint32_t burst_cycle;   // seperate BL with timing since fot GDDRx it's not BL/2
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
    // tCKSRE and tCKSRX are only useful for changing clock freq after entering SRE mode
    // we are not doing that, so tCKESR is sufficient
    uint32_t tCKE;
    uint32_t tCKESR;
    uint32_t tXS;
    uint32_t tXP;
    uint32_t tRFCb;
    uint32_t tRREFD;
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

    // Power-related parameters
    double VDD;  // had 
    uint32_t IDD0;
    uint32_t IDD2P;
    uint32_t IDD2F;
    uint32_t IDD3P;
    uint32_t IDD3N;
    uint32_t IDD4W;
    uint32_t IDD4R;
    uint32_t IDD5A;
    uint32_t IOUT;
    
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
    bool req_buffering_enabled;

    std::string validation_output_file;

    uint32_t epoch_period;
    std::string stats_file;
    std::string cummulative_stats_file;
    std::string epoch_stats_file;
    std::string stats_file_csv;
    std::string cummulative_stats_file_csv;
    std::string epoch_stats_file_csv;

    //Computed parameters
    uint32_t channel_width, rank_width, bankgroup_width, bank_width, row_width, column_width, throwaway_bits;

    bool IsGDDR() const {return (protocol == DRAMProtocol::GDDR5 || protocol == DRAMProtocol::GDDR5X);}
    bool IsHBM() const {return (protocol == DRAMProtocol::HBM || protocol == DRAMProtocol::HBM2);}
    bool IsHMC() const {return (protocol == DRAMProtocol::HMC);}

    uint32_t ideal_memory_latency;
private:
    DRAMProtocol GetDRAMProtocol(std::string protocol_str);
    void CalculateSize();
    void SetAddressMapping();
};

}
#endif
