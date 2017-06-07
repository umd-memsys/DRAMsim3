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
    dram_clk_(0),
    insert_counter_(0)
{
    ptr_config_ = new Config(config_file);
    ptr_timing_ = new Timing(*ptr_config_);
    ptr_stats_ = new Statistics();

    // setting up clock

    // initialize vaults and crossbar
    // the first layer of xbar will be num_links * 4 (4 for quadrants)
    // the second layer will be a 1:8 xbar
    // (each quadrant has 8 vaults and each quadrant can access any ohter quadrant)
    queue_depth_ = ptr_config_->xbar_queue_depth;
    links_ = ptr_config_->num_links;
    link_req_queues_.reserve(links_);
    link_resp_queues_.reserve(links_);
    for (int i = 0; i < links; i++) {
        link_req_queues_.push_back(std::vector<HMCRequest*>());
        link_resp_queues_.push_back(std::vector<HMCRequest*>());
    }

    // don't want to hard coding it but there are 4 quads so it's kind of fixed
    quad_req_queues_.reserve(4);
    quad_resp_queues_.reserve(4);
    for (int i = 0; i < 4; i++) {
        quad_req_queues_.push_back(std::vector<HMCRequest*>());
        quad_resp_queues_.push_back(std::vector<HMCRequest*>());
    }

    link_busy.reserve(links_);
    link_age_counter.reserve(links_);
    for (int i = 0; i < links_; i++) {
        link_busy.push_back(0);
        link_age_counter.push_back(0);
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

void HMCSystem::SetClockRatio() {
    // according to the spec, the each vault can go up to 10GB/s 
    // so with 128b per transfer, it operates at 625MHz per cycle
    // and perhaps not coincidentaly, each flit is 128b
    // because xbar should process 1 flit per cycle
    // so with how fast the link runs, we can determine the speed of the logic
    // and therefore figure out the speed ratio of logci vs dram 
    // e.g. if the link is 30Gbps with 16 lanes
    // takes 8 cycles to transfer a flit on the link 
    // so the logic only needs to run at 30/8=3.25GHz
    // which is approximately 5.2x speed of the DRAM
    // all speed in MHz to avoid using floats
    float link_speed;
    uint64_t dram_speed = 625;

    // cast link speed to float
    if (ptr_config_->link_speed == 12 || ptr_config_->link_speed == 13) {
        link_speed = 1250.0;
    } else {
        link_speed = static_cast<float>(ptr_config_->link_speed) * 1000.0;
    }

    // 128 bits per flit
    float cycles_per_flit = static_cast<float>(128 / ptr_config_->link_width);
    uint64_t logic_speed = static_cast<int>(link_speed/cycles_per_flit);
    int freq_gcd = gcd(dram_speed, logic_speed);
    logic_time_inc_ = logic_speed / freq_gcd;
    dram_time_inc_ = dram_speed / freq_gcd;
    time_lcm_ = lcm(logic_time_inc_, dram_time_inc_);
#ifdef DEBUG_OUTPUT
    cout << "HMC Logic clock speed " << logic_speed << endl;
    cout << "HMC DRAM clock speed " << dram_speed << endl;
#endif
    return;
}

inline void HMCSystem::IterateNextLink() {
    // determinining which link a request goes to has great impact on performance
    // round robin , we can implement other schemes here later such as random
    // but there're only at most 4 links so I suspect it would make a difference
    next_link_ = (next_link_ + 1) % links;
    return;
}

bool HMCSystem::InsertReq(HMCRequest* req) {
    // most CPU models does not support simultaneous insertions
    // if you want to actually simulate the multi-link feature
    // then you have to call this function multiple times in 1 cycle
    // TODO put a cap limit on how many times you can call this function per cycle
    if (link_req_queues_[next_link_].size() < queue_depth_) {
        req->src_link = next_link_;
        link_req_queues_[next_link_].push_back(req);
        IterateNextLink();
        return true;
    } else {  // link buffer full, try other links
        bool found = false;
        int start_link = next_link_;
        IterateNextLink();
        while (start_link != next_link_) {
            if (link_req_queues_[next_link_].size() < queue_depth_) {
                link_req_queues_[next_link_].push_back(req);
                found = true;
                IterateNextLink();
                break;
            } else {
                IterateNextLink();
            }
        }
        return found;
    }
}

void HMCSystem::ClockTick() {
    // I just need to note this somewhere:
    // the links of HMC are full duplex, e.g. in full width links,
    // each link has 16 input lanes AND 16 output lanes 
    // therefore it makes no sense to have just 1 layer of xbar 
    // for both requests and responses. 
    // so 2 layers just sounds about right

    // 1. run DRAM clock if needed
    if (RunDRAMClock()) {
        DRAMClockTick();
    }

    // 2. drain xbar on both directions
    for (auto i:link_busy) {
        if (*i > 0) {
            *i --;
        }
    }

    for (auto i:quad_busy) {
        if (*i > 0) {
            *i --;
        }
    }

    // 3. xbar arbitrate using age/FIFO arbitration
    // What is set/updated here:
    // - link_busy, quad_busy indicators
    // - link req, resp queues, quad req, resp queues 
    // - age counter
    XbarArbitrate();

    logic_clk_ ++;
}

void HMCSystem::XbarArbitrate() {
    // arbitrage based on age / FIFO 
    // drain requests from link to quad buffers
    std::vector<int> age_queue = BuildAgeQueue(link_age_counter);
    while (!age_queue.empty()) {
        int src_link = age_queue.pop_back();
        int dest_quad = link_req_queues_[src_link].front()->dest_quad;
        if (quad_req_queues_[dest_quad].size() < queue_depth_ && 
            quad_busy[dest_quad] == 0) {
            HMCRequest *req = link_req_queues_.front();
            link_req_queues_.erase(link_req_queues_.begin());
            quad_req_queues_[dest_quad].push_back(req);
            quad_busy[dest_quad] = req->flits;
            link_age_counter[src_link] = 0;
        } else {  // stalled this cycle, update age counter 
            link_age_counter[src_link] ++;
        }
    }
    age_queue.clear();

    // drain responses from quad to link buffers
    age_queue = BuildAgeQueue(quad_age_counter);
    while (!age_queue.empty()) {
        int src_quad = age_queue.pop_back();
        int dest_link = quad_resp_queues_[src_quad].front()->src_link;
        if (link_resp_queues_[dest_link].size() < queue_depth_ && 
            link_busy[dest_link] == 0) {
            HMCRequest *resp = quad_resp_queues_.front();
            quad_resp_queues_.erase(quad_resp_queues_.begin());
            link_resp_queues_[dest_link].push_back(req);
            link_busy[dest_link] = req->flits;
            quad_age_counter[src_squd] = 0;
        } else {  // stalled this cycle, update age counter 
            quad_age_counter[src_quad] ++;
        }
    }
    age_queue.clear();
}

std::vector<int> HMCSystem::BuildAgeQueue(std::vector<int> &age_counter) {
    // return a vector of indices sorted from largest to smallest
    // meaning that the oldest age link/quad should be processed first
    std::vector<int> age_queue;
    int queue_len = age_counter.size()
    age_queue.reserve(queue_len);
    for (int i = 0; i < queue_len; i++) {
        if (age_counter[i] > 0) {
            bool is_inserted = false;
            for (auto age_ptr:age_queue) {
                if (age_counter[i] > *age_ptr) {
                    age_queue.insert(age_ptr, i);
                    is_inserted = true;
                    break;
                }
            }
            if (!is_inserted) {
                age_queue.push_back(i);
            }
        }
    }
    return age_queue;
}

void HMCSystem::DRAMClockTick() {
    dram_clk_ ++;
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

bool HMCSystem::RunDRAMClock() {
    dram_counter_ += dram_time_inc_;
    bool result;
    if (logic_counter_ < dram_counter_) {
        logic_counter_ += logic_time_inc_;
        result = true;
    } else {
        result = false;
    }
    if (logic_counter_ > time_lcm_ && dram_counter_ > time_lcm_) {
        logic_counter_ %= time_lcm_;
        dram_counter_ %= time_lcm_;
    }
    return result;
}

// the following 2 functions for domain crossing...
uint64_t gcd(uint64_t x, uint64_t y) {
   if(x < y)
      std::swap(x, y);
   while(y > 0)
   {
      uint64_t f = x % y;
      x = y;
      y = f;
   }
   return x;
 }

uint64_t lcm(uint64_t x, uint64_t y) {
    return (x*y)/gcd(x, y);
}
