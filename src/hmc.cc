#include "hmc.h"

using namespace std;
using namespace dramcore;

HMCRequest::HMCRequest(uint64_t req_id, HMCReqType req_type, uint64_t hex_addr_1, uint64_t hex_addr2, int operand_len):
    req_id_(req_id),
    type(req_type),
    operand_1(hex_addr_1),
    operand_2(hex_addr_2)
{
    switch (req_type) {
        case HMCReqType::RD:
            flits = 1;
            resp_type = HMCRespType::RD_RS;
            resp_flits = operand_len >> 4 + 1;
            break;
        case HMCReqType::WR:
            flits = operand_len >> 4 + 1;
            resp_type = HMCRespType::WR_RS;
            resp_flits = 1;
            break;
        case HMCReqType::P_WR:
            flits = operand_len >> 4 + 1;
            resp_type = HMCRespType::NONE;
            resp_flits = 0;
            break;
        case HMCReqType::2ADD8:
        case HMCReqType::ADD16:
            flits = 2;
            resp_type = HMCRespType::WR_RS;
            resp_flits = 1;
            break;
        case HMCReqType::P_2ADD8:
        case HMCReqType::P_ADD16:
            flits = 2;
            resp_type = HMCRespType::NONE;
            resp_flits = 0;
            break;
        case HMCReqType::2ADDS8R:
        case HMCReqType::ADDS16R:
            flits = 2;
            resp_type = HMCRespType::RD_RS;
            resp_flits = 2;
            break;
        case HMCReqType::INC8:
            flits = 1;
            resp_type = HMCRespType::WR_RS;
            resp_flits = 1;
            break;
        case HMCReqType::P_INC8:
            flits = 1;
            resp_type = HMCRespType::NONE;
            resp_flits = 0;
            break;
        case HMCReqType::XOR16:
        case HMCReqType::OR16:
        case HMCReqType::NOR16:
        case HMCReqType::AND16:
        case HMCReqType::NAND16:
        case HMCReqType::CASGT:
        case HMCReqType::CASLT:
        case HMCReqType::CASEQ:
            flits = 2;
            resp_type = HMCRespType::RD_RS;
            resp_flits = 2;
            break;
        case HMCReqType::EQ:
        case HMCReqType::BWR:
            flits = 2;
            resp_type = HMCRespType::WR_RS;
            resp_flits = 1;
            break;
        case HMCReqType::P_BWR:
            flits = 2;
            resp_type = HMCRespType::NONE;
            resp_flits = 0;
            break;
        case HMCReqType::BWR8R:
        case HMCReqType::SWAP16:
            flits = 2;
            resp_type = HMCRespType::RD_RS;
            resp_flits = 2;
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
            break;
    }
}


HMCSystem::HMCSystem(const std::string &config_file, std::function<void(uint64_t)> callback):
    callback_(callback),
    logic_clk_(0),
    dram_clk_(0)
{
    ptr_config_ = new Config(config_file);
    ptr_timing_ = new Timing(*ptr_config_);
    ptr_stats_ = new Statistics();

    // initialize vaults and crossbar
    // the first layer of xbar will be num_links * 4 (4 for quadrants)
    // the second layer will be a 1:8 xbar
    // (each quadrant has 8 vaults and each quadrant can access any ohter quadrant)
    link_req_queues_.reserve(ptr_config_->num_links);
    link_resp_queues_.reserve(ptr_config_->num_links);
    for (unsigned i = 0; i < ptr_config_->num_links; i++) {
        link_req_queues_.push_back(std::vector<HMCRequest*>());
        link_resp_queues_.push_back(std::vector<HMCRequest*>());
    }

    // don't want to hard coding it but there are 4 quads so it's kind of fixed
    xbar_req_queues_.reserve(4);
    xbar_resp_queues_.reserve(4);
    for (unsigned i = 0; i < 4; i++) {
        xbar_req_queues_.push_back(std::vector<HMCRequest*>());
        xbar_resp_queues_.push_back(std::vector<HMCRequest*>());
    }

    //Stats output files
    stats_file_.open(ptr_config_->stats_file);
    cummulative_stats_file_.open(ptr_config_->cummulative_stats_file);
    epoch_stats_file_.open(ptr_config_->epoch_stats_file);
    stats_file_csv_.open(ptr_config_->stats_file_csv);
    cummulative_stats_file_csv_.open(ptr_config_->cummulative_stats_file_csv);
    epoch_stats_file_csv_.open(ptr_config_->epoch_stats_file_csv);

    ptr_stats_->PrintStatsCSVHeader(stats_file_csv_);
    ptr_stats_->PrintStatsCSVHeader(cummulative_stats_file_csv_);
    ptr_stats_->PrintStatsCSVHeader(epoch_stats_file_csv_);
}


HMCSystem::~HMCSystem() {
    
    delete(ptr_stats_);
    delete(ptr_timing_);
    delete(ptr_config_);
    stats_file_.close();
    cummulative_stats_file_.close();
    epoch_stats_file_.close();
    stats_file_csv_.close();
    cummulative_stats_file_csv_.close();
    epoch_stats_file_csv_.close();
}
