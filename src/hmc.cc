#include "hmc.h"

namespace dramsim3 {

uint64_t gcd(uint64_t x, uint64_t y);
uint64_t lcm(uint64_t x, uint64_t y);

HMCRequest::HMCRequest(HMCReqType req_type, uint64_t hex_addr)
    : type(req_type), mem_operand(hex_addr) {
    Address addr = AddressMapping(mem_operand);
    vault = addr.channel;
    // given that vaults could be 16 (Gen1) or 32(Gen2), using % 4
    // to partition vaults to quads
    quad = vault % 4;
    switch (req_type) {
        case HMCReqType::RD0:
        case HMCReqType::WR0:
            flits = 0;
            break;
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

HMCResponse::HMCResponse(uint64_t id, HMCReqType req_type, int dest_link,
                         int src_quad)
    : resp_id(id), link(dest_link), quad(src_quad) {
    switch (req_type) {
        case HMCReqType::RD0:
            type = HMCRespType::RD_RS;
            flits = 0;
            break;
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
        case HMCReqType::WR0:
            flits = 0;
            type = HMCRespType::WR_RS;
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
    return;
}

HMCMemorySystem::HMCMemorySystem(const std::string &config_file,
                                 const std::string &output_dir,
                                 std::function<void(uint64_t)> read_callback,
                                 std::function<void(uint64_t)> write_callback)
    : BaseDRAMSystem(config_file, output_dir, read_callback, write_callback),
      ref_tick_(0),
      logic_clk_(0),
      next_link_(0) {
    // sanity check, this constructor should only be intialized using HMC
    if (!ptr_config_->IsHMC()) {
        std::cerr << "Initialzed an HMC system without an HMC config file!"
                  << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

    // setting up clock
    SetClockRatio();

    vault_callback_ =
        std::bind(&HMCMemorySystem::VaultCallback, this, std::placeholders::_1);
    vaults_.reserve(ptr_config_->channels);
    for (int i = 0; i < ptr_config_->channels; i++) {
#ifdef THERMAL
        vaults_.push_back(new Controller(i, *ptr_config_, *ptr_timing_,
                                         *ptr_stats_, ptr_thermCal_,
                                         vault_callback_, vault_callback_));
#else
        vaults_.push_back(new Controller(i, *ptr_config_, *ptr_timing_,
                                         *ptr_stats_, vault_callback_,
                                         vault_callback_));
#endif  // THERMAL
    }
    // initialize vaults and crossbar
    // the first layer of xbar will be num_links * 4 (4 for quadrants)
    // the second layer will be a 1:8 xbar
    // (each quadrant has 8 vaults and each quadrant can access any ohter
    // quadrant)
    queue_depth_ = static_cast<size_t>(ptr_config_->xbar_queue_depth);
    links_ = ptr_config_->num_links;
    link_req_queues_.reserve(links_);
    link_resp_queues_.reserve(links_);
    for (int i = 0; i < links_; i++) {
        link_req_queues_.push_back(std::vector<HMCRequest *>());
        link_resp_queues_.push_back(std::vector<HMCResponse *>());
    }

    // don't want to hard coding it but there are 4 quads so it's kind of fixed
    quad_req_queues_.reserve(4);
    quad_resp_queues_.reserve(4);
    for (int i = 0; i < 4; i++) {
        quad_req_queues_.push_back(std::vector<HMCRequest *>());
        quad_resp_queues_.push_back(std::vector<HMCResponse *>());
    }

    link_busy.reserve(links_);
    link_age_counter.reserve(links_);
    for (int i = 0; i < links_; i++) {
        link_busy.push_back(0);
        link_age_counter.push_back(0);
    }
}

void HMCMemorySystem::SetClockRatio() {
    int dram_speed = 1250;
    int link_speed = ptr_config_->link_speed;

    // 128 bits per flit, this is to calculate logic speed
    // e.g. if it takes 8 link cycles to transfer a flit
    // the logic has to be running in similar speed so that
    // the logic or the link don't have to wait for each other
    int cycles_per_flit =
        128 / ptr_config_->link_width;  // Unit interval per flit: 8, 16, or 32
    int logic_speed_needed = link_speed / cycles_per_flit;

    // In the ClockTick() we will use DRAM's 1250MHz as basic clock ticks
    // because logic clock speed should vary to reflect link speed or link width
    // so when hooked up to a cpu simulator, which may require a clock freq,
    // give it the DRAM's constant 1250MHz would be a better solution
    // we know HMC's ref clock is 125MHz, so by doing the following
    // we can get the clock tick ratio of DRAM and logic, e.g. when logic is
    // 2500MHz then each DRAM clock tick will run 2 logic ticks
    dram_clk_ticks_ = logic_speed_needed / 125;
    logic_clk_ticks_ = dram_speed / 125;

    if (dram_clk_ticks_ < 10) {
        dram_clk_ticks_ = 10;  // better not slower than DRAM...
    }

    clk_tick_product_ = dram_clk_ticks_ * logic_clk_ticks_;

#ifdef DEBUG_OUTPUT
    std::cout << "HMC Logic clock speed " << dram_clk_ticks_ << std::endl;
    std::cout << "HMC DRAM clock speed " << logic_clk_ticks_ << std::endl;
#endif

    return;
}

inline void HMCMemorySystem::IterateNextLink() {
    // determinining which link a request goes to has great impact on
    // performance round robin , we can implement other schemes here later such
    // as random but there're only at most 4 links so I suspect it would make a
    // difference
    next_link_ = (next_link_ + 1) % links_;
    return;
}

bool HMCMemorySystem::WillAcceptTransaction(uint64_t hex_addr, bool is_write) {
    bool insertable = false;
    for (auto link_queue = link_req_queues_.begin();
         link_queue != link_req_queues_.end(); link_queue++) {
        if ((*link_queue).size() < queue_depth_) {
            insertable = true;
            break;
        }
    }
    return insertable;
}

bool HMCMemorySystem::AddTransaction(uint64_t hex_addr, bool is_write) {
    // to be compatible with other protocol we have this interface
    // when using this intreface the size of each transaction will be block_size
    HMCReqType req_type;
    if (is_write) {
        switch (ptr_config_->block_size) {
            case 0:
                req_type = HMCReqType::WR0;
                break;
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
                req_type = HMCReqType::SIZE;
                AbruptExit(__FILE__, __LINE__);
                break;
        }
    } else {
        switch (ptr_config_->block_size) {
            case 0:
                req_type = HMCReqType::RD0;
                break;
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
                req_type = HMCReqType::SIZE;
                AbruptExit(__FILE__, __LINE__);
                break;
        }
    }
    HMCRequest *req = new HMCRequest(req_type, hex_addr);
    return InsertHMCReq(req);
}

bool HMCMemorySystem::InsertReqToLink(HMCRequest *req, int link) {
    // These things need to happen when an HMC request is inserted to a link:
    // 1. check if link queue full
    // 2. set link field in the request packet
    // 3. create corresponding response
    // 4. increment link_age_counter so that arbitrate logic works
    if (link_req_queues_[link].size() < queue_depth_) {
        req->link = link;
        link_req_queues_[link].push_back(req);
        HMCResponse *resp =
            new HMCResponse(req->mem_operand, req->type, link, req->quad);
        resp_lookup_table.insert(
            std::pair<uint64_t, HMCResponse *>(resp->resp_id, resp));
        link_age_counter[link] = 1;
        ptr_stats_->interarrival_latency.AddValue(clk_ - last_req_clk_);
        last_req_clk_ = clk_;
        return true;
    } else {
        return false;
    }
}

bool HMCMemorySystem::InsertHMCReq(HMCRequest *req) {
    // most CPU models does not support simultaneous insertions
    // if you want to actually simulate the multi-link feature
    // then you have to call this function multiple times in 1 cycle
    // TODO put a cap limit on how many times you can call this function per
    // cycle
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

void HMCMemorySystem::LogicClockTickPre() {
    // I just need to note this somewhere:
    // the links of HMC are full duplex, e.g. in full width links,
    // each link has 16 input lanes AND 16 output lanes
    // therefore it makes no sense to have just 1 layer of xbar
    // for both requests and responses.
    // so 2 layers just sounds about right

    // return responses to CPU
    for (int i = 0; i < links_; i++) {
        if (!link_resp_queues_[i].empty()) {
            HMCResponse *resp = link_resp_queues_[i].front();
            if (resp->exit_time <= logic_clk_) {
                if (resp->type == HMCRespType::RD_RS) {
                    read_callback_(resp->resp_id);
                } else {
                    write_callback_(resp->resp_id);
                }
                delete (resp);
                link_resp_queues_[i].erase(link_resp_queues_[i].begin());
                ptr_stats_->hmc_reqs_done++;
            }
        }
    }
    return;
}

void HMCMemorySystem::LogicClockTickPost() {
    // This MUST happen after DRAMClockTick

    // drain quad request queue to vaults
    for (int i = 0; i < 4; i++) {
        if (!quad_req_queues_[i].empty() &&
            quad_resp_queues_[i].size() < queue_depth_) {
            HMCRequest *req = quad_req_queues_[i].front();
            if (req->exit_time <= logic_clk_) {
                InsertReqToDRAM(req);
                delete (req);
                quad_req_queues_[i].erase(quad_req_queues_[i].begin());
            }
        }
    }

    // step xbar
    // the decremented values here basically represents the internal BW of
    // a xbar, to match the external link, transfering 2 flits are
    // usually be needed
    for (auto &&i : link_busy) {
        if (i > 0) {
            i -= 2;
        }
    }

    for (auto &&i : quad_busy) {
        if (i > 0) {
            i -= 2;
        }
    }

    // xbar arbitrate using age/FIFO arbitration
    // What is set/updated here:
    // - link_busy, quad_busy indicators
    // - link req, resp queues, quad req, resp queues
    // - age counter
    XbarArbitrate();
    return;
}

void HMCMemorySystem::DRAMClockTick() {
    for (auto vault : vaults_) {
        vault->ClockTick();
    }
    if (clk_ % ptr_config_->epoch_period == 0 && clk_ != 0) {
        int queue_usage_total = 0;
        for (auto vault : vaults_) {
            queue_usage_total += vault->QueueUsage();
        }
        ptr_stats_->queue_usage.epoch_value =
            static_cast<double>(queue_usage_total);
        ptr_stats_->PreEpochCompute(clk_);
        PrintIntermediateStats();
        ptr_stats_->UpdateEpoch(clk_);
    }
    clk_++;
    ptr_stats_->dramcycles++;
    return;
}

void HMCMemorySystem::ClockTick() {
    bool dram_ran = false;
    // because we're using DRAM clock as base, so the faster logic clock
    // may run several cycles in one DRAM cycle, and some things should
    // happen before a DRAM Clock and some after...
    for (int i = 0; i < dram_clk_ticks_; i++) {
        if (ref_tick_ % logic_clk_ticks_ == 0) {
            LogicClockTickPre();
            if (ref_tick_ % clk_tick_product_ == 0) {
                DRAMClockTick();
                dram_ran = true;
            }
            LogicClockTickPost();
            logic_clk_++;
        }
        ref_tick_++;
    }

    // in cases DRAM and logic both run at this cycle
    // don't repeat
    if (!dram_ran) {
        DRAMClockTick();
    }
    return;
}

void HMCMemorySystem::XbarArbitrate() {
    // arbitrage based on age / FIFO
    // drain requests from link to quad buffers
    std::vector<int> age_queue = BuildAgeQueue(link_age_counter);
    while (!age_queue.empty()) {
        int src_link = age_queue.front();
        int dest_quad = link_req_queues_[src_link].front()->quad;
        if (quad_req_queues_[dest_quad].size() < queue_depth_ &&
            quad_busy[dest_quad] <= 0) {
            HMCRequest *req = link_req_queues_[src_link].front();
            link_req_queues_[src_link].erase(
                link_req_queues_[src_link].begin());
            quad_req_queues_[dest_quad].push_back(req);
            quad_busy[dest_quad] = req->flits;
            req->exit_time = logic_clk_ + req->flits;
            if (link_req_queues_[src_link].empty()) {
                link_age_counter[src_link] = 0;
            } else {
                link_age_counter[src_link] = 1;
            }
        } else {  // stalled this cycle, update age counter
            link_age_counter[src_link]++;
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
            link_busy[dest_link] <= 0) {
            HMCResponse *resp = quad_resp_queues_[src_quad].front();
            quad_resp_queues_[src_quad].erase(
                quad_resp_queues_[src_quad].begin());
            link_resp_queues_[dest_link].push_back(resp);
            link_busy[dest_link] = resp->flits;
            resp->exit_time = logic_clk_ + resp->flits;
            if (quad_resp_queues_[src_quad].size() == 0) {
                quad_age_counter[src_quad] = 0;
            } else {
                quad_age_counter[src_quad] = 1;
            }
        } else {  // stalled this cycle, update age counter
            quad_age_counter[src_quad]++;
        }
        age_queue.erase(age_queue.begin());
    }
    age_queue.clear();
}

std::vector<int> HMCMemorySystem::BuildAgeQueue(std::vector<int> &age_counter) {
    // return a vector of indices sorted in decending order
    // meaning that the oldest age link/quad should be processed first
    std::vector<int> age_queue;
    int queue_len = age_counter.size();
    age_queue.reserve(queue_len);
    int start_pos = logic_clk_ % queue_len;  // round robin start pos
    for (int i = 0; i < queue_len; i++) {
        int pos = (i + start_pos) % queue_len;
        if (age_counter[pos] > 0) {
            bool is_inserted = false;
            for (auto it = age_queue.begin(); it != age_queue.end(); it++) {
                if (age_counter[pos] > *it) {
                    age_queue.insert(it, pos);
                    is_inserted = true;
                    break;
                }
            }
            if (!is_inserted) {
                age_queue.push_back(pos);
            }
        }
    }
    return age_queue;
}

void HMCMemorySystem::InsertReqToDRAM(HMCRequest *req) {
    Transaction trans;
    switch (req->type) {
        case HMCReqType::RD0:
        case HMCReqType::RD16:
        case HMCReqType::RD32:
        case HMCReqType::RD48:
        case HMCReqType::RD64:
        case HMCReqType::RD80:
        case HMCReqType::RD96:
        case HMCReqType::RD112:
        case HMCReqType::RD128:
        case HMCReqType::RD256:
            // only 1 request is needed, if the request length is shorter than
            // block_size it will be chopped and therefore results in a waste of
            // bandwidth
            trans =  Transaction(req->mem_operand, false);
            vaults_[req->vault]->AddTransaction(trans);
            break;
        case HMCReqType::WR0:
        case HMCReqType::WR16:
        case HMCReqType::WR32:
        case HMCReqType::P_WR16:
        case HMCReqType::P_WR32:
        case HMCReqType::WR48:
        case HMCReqType::WR64:
        case HMCReqType::P_WR48:
        case HMCReqType::P_WR64:
        case HMCReqType::WR80:
        case HMCReqType::WR96:
        case HMCReqType::P_WR80:
        case HMCReqType::P_WR96:
        case HMCReqType::WR112:
        case HMCReqType::WR128:
        case HMCReqType::P_WR112:
        case HMCReqType::P_WR128:
        case HMCReqType::WR256:
        case HMCReqType::P_WR256:
            trans = Transaction(req->mem_operand, true);
            vaults_[req->vault]->AddTransaction(trans);
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
    return;
}

void HMCMemorySystem::VaultCallback(uint64_t req_id) {
    // we will use hex addr as the req_id and use a multimap to lookup the
    // requests the vaults cannot directly talk to the CPU so this callback will
    // be passed to the vaults and is responsible to put the responses back to
    // response queues

    auto it = resp_lookup_table.find(req_id);
    HMCResponse *resp = it->second;
    // all data from dram received, put packet in xbar and return
    resp_lookup_table.erase(it);
    // put it in xbar
    quad_resp_queues_[resp->quad].push_back(resp);
    quad_age_counter[resp->quad] = 1;
    return;
}

HMCMemorySystem::~HMCMemorySystem() {
    for (auto i = 0; i < ptr_config_->channels; i++) {
        delete (vaults_[i]);
    }
}

// the following 2 functions for domain crossing...
uint64_t gcd(uint64_t x, uint64_t y) {
    if (x < y) std::swap(x, y);
    while (y > 0) {
        uint64_t f = x % y;
        x = y;
        y = f;
    }
    return x;
}

uint64_t lcm(uint64_t x, uint64_t y) { return (x * y) / gcd(x, y); }

}  // namespace dramsim3
