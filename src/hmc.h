#ifndef __HMC_H
#define __HMC_H

#include <functional>
#include "common.h"
#include "configuration.h"


namespace dramcore {


enum class HMCReqType {
    RD,
    WR,
    P_WR,
    2ADD8,
    ADD16,
    P_2ADD8,
    P_ADD16,
    2ADDS8R,
    ADDS16R,
    INC8,
    P_INC8,
    XOR16,
    OR16,
    NOR16,
    AND16,
    NAND16,
    CASGT,
    CASLT,
    CASEQ,
    EQ,
    BWR,
    P_BWR,
    BWR8R,
    SWAP16,
    SIZE
};

enum class HMCRespType {
    NONE,
    RD_RS,
    WR_RS,
    ERR,
    SIZE
};


enum class HMCLinkType {
    HOST_TO_DEV,
    DEV_TO_DEV,
    SIZE
};


class HMCRequest {
public:
    HMCRequest(uint64_t req_id, HMCReqType req_type, uint64_t hex_addr_1, uint64_t hex_addr2, int operand_len);
    uint64_t req_id_;
    HMCReqType type;
    uint64_t operand_1,
    uint64_t operand_2,
    int src_link;
    int dest_quad;
    int dest_vault;
    int operand_len_;
    int flits;
    HMCRespType resp_type;
    int resp_flits;
};

class HMCLink {
public:
    HMCLink(const Config &config, std::function<void(uint64_t)>& callback);
    std::function<void(uint64_t req_id)> callback_;
private:

};


class HMCSystem {
public:
    HMCSystem(const std::string &config_file, std::function<void(uint64_t)> callback);
    ~HMCSystem();
    // assuming there are 2 clock domains one for logic die one for DRAM
    // we can unify them as one but then we'll have to convert all the 
    // slow dram time units to faster logic units...
    void ClockTick();
    void DRAMClockTick();
    bool InsertReq(HMCRequest* req);
    Config* ptr_config_;
private:
    uint64_t logic_clk_;
    uint64_t logic_counter_, dram_counter_;
    int logic_time_inc_, dram_time_inc_;
    uint64_t time_lcm_;
    uint64_t id_;
    Timing* ptr_timing_;
    Statistics* ptr_stats_;

    void SetClockRatio();
    bool RunDRAMClock();
    std::vector<int> BuildAgeQueue(std::vector<int> age_counter);
    void XbarArbitrate();

    int next_link_;
    int links_;
    int queue_depth_;

    // these are essentially input/output buffers for xbars
    std::vector<std::vector<HMCRequest*>> link_req_queues_;
    std::vector<std::vector<HMCRequest*>> link_resp_queues_;
    std::vector<std::vector<HMCRequest*>> quad_req_queues_;
    std::vector<std::vector<HMCRequest*>> quad_resp_queues_;

    // input/output busy indicators, since each packet could be several
    // flits, as long as this != 0 then they're busy 
    std::vector<int> link_busy;
    std::vector<int> quad_busy = {0, 0, 0, 0};
    // used for arbitration
    std::vector<int> link_age_counter;
    std::vector<int> quad_age_counter = {0, 0, 0, 0};
    
    inline void IterateNextLink();

    //Stats output files
    std::ofstream stats_file_;
    std::ofstream cummulative_stats_file_;
    std::ofstream epoch_stats_file_;
    std::ofstream stats_file_csv_;
    std::ofstream cummulative_stats_file_csv_;
    std::ofstream epoch_stats_file_csv_;
};


}

#endif

