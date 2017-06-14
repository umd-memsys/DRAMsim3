#include "hmc.h"

using namespace std;
using namespace dramcore;

uint64_t gcd(uint64_t x, uint64_t y);
uint64_t lcm(uint64_t x, uint64_t y);

HMCRequest::HMCRequest(uint64_t req_id, HMCReqType req_type, uint64_t hex_addr):
    req_id(req_id),
    type(req_type),
    mem_operand(hex_addr)
{
    Address addr = AddressMapping(mem_operand);
    vault = addr.channel_;
    quad = vault >> 3;
    switch (req_type) {
        case HMCReqType::RD16:
        case HMCReqType::RD32:
        case HMCReqType::RD48:
        case HMCReqType::RD64:
        case HMCReqType::RD80:
        case HMCReqType::RD96:
        case HMCReqType::RD112:
        case HMCReqType::RD128:
        case HMCReqType::RD256:
            flits = 1;
            break;
        case HMCReqType::WR16:
        case HMCReqType::P_WR16:
            flits = 2;
            break;
        case HMCReqType::WR32:
        case HMCReqType::P_WR32:
            flits = 3;
            break;
        case HMCReqType::WR48:
        case HMCReqType::P_WR48:
            flits = 4;
            break;
        case HMCReqType::WR64:
        case HMCReqType::P_WR64:
            flits = 5;
            break;
        case HMCReqType::WR80:
        case HMCReqType::P_WR80:
            flits = 6;
            break;
        case HMCReqType::WR96:
        case HMCReqType::P_WR96:
            flits = 7;
            break;
        case HMCReqType::WR112:
        case HMCReqType::P_WR112:
            flits = 8;
            break;
        case HMCReqType::WR128:
        case HMCReqType::P_WR128:
            flits = 9;
            break;
        case HMCReqType::WR256:
        case HMCReqType::P_WR256:
            flits = 17;
            break;
        case HMCReqType::ADD8:
        case HMCReqType::ADD16:
            flits = 2;
            break;
        case HMCReqType::P_2ADD8:
        case HMCReqType::P_ADD16:
            flits = 2;
            break;
        case HMCReqType::ADDS8R:
        case HMCReqType::ADDS16R:
            flits = 2;
            break;
        case HMCReqType::INC8:
            flits = 1;
            break;
        case HMCReqType::P_INC8:
            flits = 1;
            break;
        case HMCReqType::XOR16:
        case HMCReqType::OR16:
        case HMCReqType::NOR16:
        case HMCReqType::AND16:
        case HMCReqType::NAND16:
        case HMCReqType::CASGT8:
        case HMCReqType::CASGT16:
        case HMCReqType::CASLT8:
        case HMCReqType::CASLT16:
        case HMCReqType::CASEQ8:
        case HMCReqType::CASZERO16:
            flits = 2;
            break;
        case HMCReqType::EQ8:
        case HMCReqType::EQ16:
        case HMCReqType::BWR:
            flits = 2;
            break;
        case HMCReqType::P_BWR:
            flits = 2;
            break;
        case HMCReqType::BWR8R:
        case HMCReqType::SWAP16:
            flits = 2;
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
            break;
    }
}


HMCResponse::HMCResponse(uint64_t id, HMCReqType req_type, int dest_link, int src_quad):
        resp_id(id),
        link(dest_link),
        quad(src_quad),
        completed_flits(1)  // initialzed to 1 for header + tail
    {   
        switch(req_type) {
            case HMCReqType::RD16:
                type = HMCRespType::RD_RS;
                flits = 2;
                break;
            case HMCReqType::RD32:
                type = HMCRespType::RD_RS;
                flits = 3;
                break;
            case HMCReqType::RD48:
                type = HMCRespType::RD_RS;
                flits = 4;
                break;
            case HMCReqType::RD64:
                type = HMCRespType::RD_RS;
                flits = 5;
                break;
            case HMCReqType::RD80:
                type = HMCRespType::RD_RS;
                flits = 6;
                break;
            case HMCReqType::RD96:
                type = HMCRespType::RD_RS;
                flits = 7;
                break;
            case HMCReqType::RD112:
                type = HMCRespType::RD_RS;
                flits = 8;
                break;
            case HMCReqType::RD128:
                type = HMCRespType::RD_RS;
                flits = 9;
                break;
            case HMCReqType::RD256:
                type = HMCRespType::RD_RS;
                flits = 17;
                break;
            case HMCReqType::WR16:
            case HMCReqType::WR32:
            case HMCReqType::WR48:
            case HMCReqType::WR64:
            case HMCReqType::WR80:
            case HMCReqType::WR96:
            case HMCReqType::WR112:
            case HMCReqType::WR128:
            case HMCReqType::WR256:
                type = HMCRespType::WR_RS;
                flits = 1;
                break;
            case HMCReqType::P_WR16:
            case HMCReqType::P_WR32:
            case HMCReqType::P_WR48:
            case HMCReqType::P_WR64:
            case HMCReqType::P_WR80:
            case HMCReqType::P_WR96:
            case HMCReqType::P_WR112:
            case HMCReqType::P_WR128:
            case HMCReqType::P_WR256:
                type = HMCRespType::NONE;
                flits = 0;
                break;
            case HMCReqType::ADD8:
            case HMCReqType::ADD16:
                type = HMCRespType::WR_RS;
                flits = 1;
                break;
            case HMCReqType::P_2ADD8:
            case HMCReqType::P_ADD16:
                type = HMCRespType::NONE;
                flits = 0;
                break;
            case HMCReqType::ADDS8R:
            case HMCReqType::ADDS16R:
                type = HMCRespType::RD_RS;
                flits = 2;
                break;
            case HMCReqType::INC8:
                type = HMCRespType::WR_RS;
                flits = 1;
                break;
            case HMCReqType::P_INC8:
                type = HMCRespType::NONE;
                flits = 0;
                break;
            case HMCReqType::XOR16:
            case HMCReqType::OR16:
            case HMCReqType::NOR16:
            case HMCReqType::AND16:
            case HMCReqType::NAND16:
            case HMCReqType::CASGT8:
            case HMCReqType::CASGT16:
            case HMCReqType::CASLT8:
            case HMCReqType::CASLT16:
            case HMCReqType::CASEQ8:
            case HMCReqType::CASZERO16:
                type = HMCRespType::RD_RS;
                flits = 2;
                break;
            case HMCReqType::EQ8:
            case HMCReqType::EQ16:
            case HMCReqType::BWR:
                type = HMCRespType::WR_RS;
                flits = 1;
                break;
            case HMCReqType::P_BWR:
                type = HMCRespType::NONE;
                flits = 0;
                break;
            case HMCReqType::BWR8R:
            case HMCReqType::SWAP16: 
                type = HMCRespType::RD_RS;
                flits = 2;
                break;
            default:
                AbruptExit(__FILE__, __LINE__);
                break;
        }
    }


HMCSystem::HMCSystem(const std::string &config_file, std::function<void(uint64_t)> callback):
    BaseMemorySystem(callback),
    logic_clk_(0),
    dram_clk_(0),
    next_link_(0)
{
    ptr_config_ = new Config(config_file);
    ptr_timing_ = new Timing(*ptr_config_);
    ptr_stats_ = new Statistics();

    // setting up clock
    SetClockRatio();

    vault_callback_ = std::bind(&HMCSystem::VaultCallback, this, std::placeholders::_1);
    vaults_.reserve(32);
    for (int i = 0; i < 32; i++) {
        vaults_.push_back(new Controller(i, *ptr_config_, *ptr_timing_, *ptr_stats_, vault_callback_));
    }
    // initialize vaults and crossbar
    // the first layer of xbar will be num_links * 4 (4 for quadrants)
    // the second layer will be a 1:8 xbar
    // (each quadrant has 8 vaults and each quadrant can access any ohter quadrant)
    queue_depth_ = ptr_config_->xbar_queue_depth;
    links_ = ptr_config_->num_links;
    link_req_queues_.reserve(links_);
    link_resp_queues_.reserve(links_);
    for (int i = 0; i < links_; i++) {
        link_req_queues_.push_back(std::vector<HMCRequest*>());
        link_resp_queues_.push_back(std::vector<HMCResponse*>());
    }

    // don't want to hard coding it but there are 4 quads so it's kind of fixed
    quad_req_queues_.reserve(4);
    quad_resp_queues_.reserve(4);
    for (int i = 0; i < 4; i++) {
        quad_req_queues_.push_back(std::vector<HMCRequest*>());
        quad_resp_queues_.push_back(std::vector<HMCResponse*>());
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
    next_link_ = (next_link_ + 1) % links_;
    return;
}


bool HMCSystem::InsertReq(uint64_t req_id, uint64_t hex_addr, bool is_write) {
    // to be compatible with other protocol we have this interface
    // when using this intreface the size of each transaction will be block_size
    HMCReqType req_type;
    if (is_write) {
        switch(ptr_config_->block_size) {
            case 32:
                req_type = HMCReqType::WR32;
                break;
            case 64:
                req_type = HMCReqType::WR64;
                break;
            case 128:
                req_type = HMCReqType::WR128;
                break;
            case 256:
                req_type = HMCReqType::WR256;
                break;
            default:
                AbruptExit(__FILE__, __LINE__);
                break;
        }
    } else {
        switch(ptr_config_->block_size) {
            case 32:
                req_type = HMCReqType::RD32;
                break;
            case 64:
                req_type = HMCReqType::RD64;
                break;
            case 128:
                req_type = HMCReqType::RD128;
                break;
            case 256:
                req_type = HMCReqType::RD256;
                break;
            default:
                AbruptExit(__FILE__, __LINE__);
                break;
        }
    }
    HMCRequest* req = new HMCRequest(req_id, req_type, hex_addr);
    return InsertHMCReq(req);
}


bool HMCSystem::InsertReqToLink(HMCRequest* req, int link) {
    // These things need to happen when an HMC request is inserted to a link:
    // 1. check if link queue full
    // 2. set link field in the request packet
    // 3. create corresponding response
    // 4. increment link_age_counter so that arbitrate logic works
    if (link_req_queues_[link].size() < queue_depth_) {
        req->link = link;
        link_req_queues_[link].push_back(req);
        HMCResponse *resp = new HMCResponse(req->mem_operand, req->type, link, req->quad);
        resp_lookup_table.insert(std::pair<uint64_t, HMCResponse*>(resp->resp_id, resp));
        link_age_counter[link] = 1;
        return true;
    } else {
        return false;
    }
}

// kk if we're using a different interface then it won't get called... wtf...
bool HMCSystem::InsertHMCReq(HMCRequest* req) {
    // most CPU models does not support simultaneous insertions
    // if you want to actually simulate the multi-link feature
    // then you have to call this function multiple times in 1 cycle
    // TODO put a cap limit on how many times you can call this function per cycle
    bool is_inserted = InsertReqToLink(req, next_link_);
    if (!is_inserted) {
        int start_link = next_link_;
        IterateNextLink();
        while (start_link != next_link_) {
            if (InsertReqToLink(req, next_link_)) {
                IterateNextLink();
                return true;
            } else {
                IterateNextLink();
            }
        }
        return false;
    } else {
        IterateNextLink();
        return true;
    }
}


void HMCSystem::ClockTick() {
    // I just need to note this somewhere:
    // the links of HMC are full duplex, e.g. in full width links,
    // each link has 16 input lanes AND 16 output lanes 
    // therefore it makes no sense to have just 1 layer of xbar 
    // for both requests and responses. 
    // so 2 layers just sounds about right


    // 0. return responses to CPU
    for (int i =0; i < links_; i++) {
        if (!link_resp_queues_[i].empty()) {
            HMCResponse* resp = link_resp_queues_[i].front();
            if (resp->exit_time >= logic_clk_) {
                callback_(resp->resp_id);
                delete(resp);
                link_resp_queues_[i].erase(link_resp_queues_[i].begin());
            }
        }
    }

    // 1. run DRAM clock if needed
    if (RunDRAMClock()) {
        DRAMClockTick();
    }

    // 2. drain quad request queue to vaults
    for (int i = 0; i < 4; i++) {
        if (!quad_req_queues_[i].empty()) {
            HMCRequest *req = quad_req_queues_[i].front();
            if (req->exit_time >= logic_clk_) {
                InsertReqToDRAM(req);
                delete(req);
                quad_req_queues_[i].erase(quad_req_queues_[i].begin());
            }
        }
    }

    // 3.a step xbar
    for (auto&& i:link_busy) {
        if (i > 0) {
            i --;
        }
    }

    for (auto&& i:quad_busy) {
        if (i > 0) {
            i --;
        }
    }

    // 3.b xbar arbitrate using age/FIFO arbitration
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
        int src_link = age_queue.front();
        int dest_quad = link_req_queues_[src_link].front()->quad;
        if (quad_req_queues_[dest_quad].size() < queue_depth_ && 
            quad_busy[dest_quad] == 0) {
            HMCRequest *req = link_req_queues_[src_link].front();
            link_req_queues_[src_link].erase(link_req_queues_[src_link].begin());
            quad_req_queues_[dest_quad].push_back(req);
            quad_busy[dest_quad] = req->flits;
            req->exit_time = logic_clk_ + req->flits;
            link_age_counter[src_link] = 0;
        } else {  // stalled this cycle, update age counter 
            link_age_counter[src_link] ++;
        }
        age_queue.erase(age_queue.begin());
    }
    age_queue.clear();

    // drain responses from quad to link buffers
    age_queue = BuildAgeQueue(quad_age_counter);
    while (!age_queue.empty()) {
        int src_quad = age_queue.front();
        int dest_link = quad_resp_queues_[src_quad].front()->link;
        if (link_resp_queues_[dest_link].size() < queue_depth_ && 
            link_busy[dest_link] == 0) {
            HMCResponse *resp = quad_resp_queues_[src_quad].front();
            quad_resp_queues_[src_quad].erase(quad_resp_queues_[src_quad].begin());
            link_resp_queues_[dest_link].push_back(resp);
            link_busy[dest_link] = resp->flits;
            resp->exit_time = logic_clk_ + resp->flits;
            quad_age_counter[src_quad] = 0;
        } else {  // stalled this cycle, update age counter 
            quad_age_counter[src_quad] ++;
        }
        age_queue.erase(age_queue.begin());
    }
    age_queue.clear();
}

std::vector<int> HMCSystem::BuildAgeQueue(std::vector<int>& age_counter) {
    // return a vector of indices sorted in decending order
    // meaning that the oldest age link/quad should be processed first
    std::vector<int> age_queue;
    int queue_len = age_counter.size();
    age_queue.reserve(queue_len);
    for (int i = 0; i < queue_len; i++) {
        if (age_counter[i] > 0) {
            bool is_inserted = false;
            for (auto it = age_queue.begin(); it != age_queue.end(); it++) {
                if (age_counter[i] > *it) {
                    age_queue.insert(it, i);
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
    for (auto vault:vaults_) {
        vault->ClockTick();
    }
    dram_clk_ ++;
    return;
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


void HMCSystem::InsertReqToDRAM(HMCRequest *req) {
    Request *dram_req;
    switch(req->type) {
        // TODO: For read/write, we need to know 
        // what's the actual BL, and what's its relation to block_size
        // For now we will just insert requests to same address because they'll be on the 
        // same row so it's basically no difference
        case HMCReqType::RD16:
        case HMCReqType::RD32:
            // only 1 request is needed, RD16 will be chopped, similar for 48, 80...
            dram_req = new Request(CommandType::READ_PRECHARGE, req->mem_operand);
            vaults_[req->vault]->InsertReq(dram_req);
            break;
        case HMCReqType::RD48:
        case HMCReqType::RD64:
            dram_req = new Request(CommandType::READ, req->mem_operand);
            vaults_[req->vault]->InsertReq(dram_req);
            dram_req = new Request(CommandType::READ_PRECHARGE, req->mem_operand );
            vaults_[req->vault]->InsertReq(dram_req);
            break;
        case HMCReqType::RD80:
        case HMCReqType::RD96:
            for (int i = 0; i < 2; i++) {
                dram_req = new Request(CommandType::READ, req->mem_operand);
                vaults_[req->vault]->InsertReq(dram_req);
            }
            dram_req = new Request(CommandType::READ_PRECHARGE, req->mem_operand);
            vaults_[req->vault]->InsertReq(dram_req);
            break;
        case HMCReqType::RD112:
        case HMCReqType::RD128:
            for (int i = 0; i < 3; i++) {
                dram_req = new Request(CommandType::READ, req->mem_operand);
                vaults_[req->vault]->InsertReq(dram_req);
            }
            dram_req = new Request(CommandType::READ_PRECHARGE, req->mem_operand);
            vaults_[req->vault]->InsertReq(dram_req);
            break;
        case HMCReqType::RD256:
            for (int i = 0; i < 7; i++) {
                dram_req = new Request(CommandType::READ, req->mem_operand);
                vaults_[req->vault]->InsertReq(dram_req);
            }
            dram_req = new Request(CommandType::READ_PRECHARGE, req->mem_operand);
            vaults_[req->vault]->InsertReq(dram_req);
            break;
        case HMCReqType::WR16:
        case HMCReqType::WR32:
        case HMCReqType::P_WR16:
        case HMCReqType::P_WR32:
            dram_req = new Request(CommandType::WRITE_PRECHARGE, req->mem_operand);
            vaults_[req->vault]->InsertReq(dram_req);
            break;
        case HMCReqType::WR48:
        case HMCReqType::WR64:
        case HMCReqType::P_WR48:
        case HMCReqType::P_WR64:
            dram_req = new Request(CommandType::WRITE, req->mem_operand);
            vaults_[req->vault]->InsertReq(dram_req);
            dram_req = new Request(CommandType::WRITE_PRECHARGE, req->mem_operand);
            vaults_[req->vault]->InsertReq(dram_req);
            break;
        case HMCReqType::WR80:
        case HMCReqType::WR96:
        case HMCReqType::P_WR80:
        case HMCReqType::P_WR96:
            for (int i = 0; i < 2; i++) {
                dram_req = new Request(CommandType::WRITE, req->mem_operand);
                vaults_[req->vault]->InsertReq(dram_req);
            }
            dram_req = new Request(CommandType::WRITE_PRECHARGE, req->mem_operand);
            vaults_[req->vault]->InsertReq(dram_req);
            break;
        case HMCReqType::WR112:
        case HMCReqType::WR128:
        case HMCReqType::P_WR112:
        case HMCReqType::P_WR128:
            for (int i = 0; i < 3; i++) {
                dram_req = new Request(CommandType::WRITE, req->mem_operand);
                vaults_[req->vault]->InsertReq(dram_req);
            }
            dram_req = new Request(CommandType::WRITE_PRECHARGE, req->mem_operand);
            vaults_[req->vault]->InsertReq(dram_req);
            break;
        case HMCReqType::WR256:
        case HMCReqType::P_WR256:
            for (int i = 0; i < 7; i++) {
                dram_req = new Request(CommandType::WRITE, req->mem_operand);
                vaults_[req->vault]->InsertReq(dram_req);
            }
            dram_req = new Request(CommandType::WRITE_PRECHARGE, req->mem_operand);
            vaults_[req->vault]->InsertReq(dram_req);
            break;
        // TODO real question here is, if an atomic operantion 
        // generate a read and a write request, 
        // where is the computation performed
        // and when is the write request issued
        case HMCReqType::ADD8:
        case HMCReqType::P_2ADD8:
            // 2 8Byte imm operands + 8 8Byte mem operands
            // read then write
        case HMCReqType::ADD16:
        case HMCReqType::P_ADD16:
            // single 16B imm operand + 16B mem operand
            // read then write
        case HMCReqType::ADDS8R:
        case HMCReqType::ADDS16R:
            // read, return(the original), then write
        case HMCReqType::INC8:
        case HMCReqType::P_INC8:
            // 8 Byte increment 
            // read update write 
        case HMCReqType::XOR16:
        case HMCReqType::OR16:
        case HMCReqType::NOR16:
        case HMCReqType::AND16:
        case HMCReqType::NAND16:
            // boolean op on imm operand and mem operand 
            // read update write
            break;
        // comparison are the most headache ones... 
        // since you don't know if you need to issue 
        // a write request until you get the data
        case HMCReqType::CASGT8:
        case HMCReqType::CASGT16:
        case HMCReqType::CASLT8:
        case HMCReqType::CASLT16:
        case HMCReqType::CASEQ8:
        case HMCReqType::CASZERO16:
            // read then may-or-may-not write
            break;
        // For EQ ops, only a READ is issued, no WRITE
        case HMCReqType::EQ8:
        case HMCReqType::EQ16:
            break;
        case HMCReqType::BWR:
        case HMCReqType::P_BWR:
            // bit write, 8 Byte mask, 8 Byte value
            // read update write 
        case HMCReqType::BWR8R:
            // bit write with return 
        case HMCReqType::SWAP16:
            // swap imm operand and mem operand 
            // read and then write
            break;
        default:
            break;
    }
}

void HMCSystem::VaultCallback(uint64_t req_id) {
    // the vaults cannot directly talk to the CPU so this callback 
    // will be passed to the vaults and is responsible to 
    // put the responses back to response queues
    
    // a packet could contain multiple transactions from DRAM
    // and in this case each DRAM transaction provides 2 flits of data
    auto it = resp_lookup_table.find(req_id);
    HMCResponse *resp = it->second;
    resp->completed_flits += 2;
    if (resp->completed_flits >= resp->flits) {
        // all data from dram received, put packet in xbar and return
        resp_lookup_table.erase(it);
        // put it in xbar
        quad_resp_queues_[resp->quad].push_back(resp);
    }
}


HMCSystem::~HMCSystem() {
    for (auto i = 0; i < ptr_config_->channels; i++) {
        delete(vaults_[i]);
    }
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


void HMCSystem::PrintStats() {
    cout << "-----------------------------------------------------" << endl;
    cout << "Printing final stats -- " << endl;
    cout << "-----------------------------------------------------" << endl;
    cout << *ptr_stats_;
    cout << "-----------------------------------------------------" << endl;
    cout << "The stats are also written to the file " << "dramcore.out" << endl;
    ptr_stats_->PrintStats(stats_file_);
    ptr_stats_->PrintStatsCSVFormat(stats_file_csv_);
    return;
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
