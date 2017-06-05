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
    bool InserReq();
    Config* ptr_config_;
private:
    uint64_t logic_clk_;
    uint64_t dram_clk_;
    uint64_t id_;
    Timing* ptr_timing_;
    Statistics* ptr_stats_;

    // these are essentially input/output buffers for xbars
    std::vector<std::vector<HMCRequest*>> link_req_queues_;
    std::vector<std::vector<HMCRequest*>> link_resp_queues_;
    std::vector<std::vector<HMCRequest*>> xbar_req_queues_;
    std::vector<std::vector<HMCRequest*>> xbar_resp_queues_;
    

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

