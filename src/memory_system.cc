#include "memory_system.h"

using namespace std;
using namespace dramcore;

MemorySystem::MemorySystem(const string &config_file, std::function<void(uint64_t)> callback) :
    callback_(callback),
    clk_(0)
{
    ptr_config_ = new Config(config_file);
    ptr_timing_ = new Timing(*ptr_config_);
    ptr_stats_ = new Statistics();
    ctrls_.resize(ptr_config_->channels);
    for(auto i = 0; i < ptr_config_->channels; i++) {
        ctrls_[i] = new Controller(i, *ptr_config_, *ptr_timing_, *ptr_stats_, callback_);
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

MemorySystem::~MemorySystem() {
    for(auto i = 0; i < ptr_config_->channels; i++) {
        delete(ctrls_[i]);
    }
    delete(ptr_stats_);
    delete(ptr_timing_);
    delete(ptr_config_);
    stats_file_.close();
    cummulative_stats_file_.close();
    epoch_stats_file_.close();
}

bool MemorySystem::InsertReq(uint64_t req_id, uint64_t hex_addr, bool is_write) {
    CommandType cmd_type = is_write ? CommandType::WRITE : CommandType ::READ;
    id_++;
    Request* req = new Request(cmd_type, hex_addr, *ptr_config_, clk_, id_);
    /*
    Address addr = AddressMapping(hex_addr, *ptr_config_);
    Request* req = new Request(cmd_type, addr);
     */

    // Some CPU simulators might not model the backpressure because queues are full.
    // An approximate way of addressing this scenario is to buffer all such requests here in the DRAM simulator and then
    // feed them into the actual memory controller queues as and when space becomes available.
    // Note - This is an approximation and if the size of such buffer queue becomes large during the course of the
    // simulation, then the accuracy sought of devolves into that of a trace based simulation.
    bool is_insertable = ctrls_[req->Channel()]->InsertReq(req);
    if((*ptr_config_).req_buffering_enabled && !is_insertable) {
        buffer_q_.push_back(req);
        is_insertable = true;
        ptr_stats_->numb_buffered_requests++;
    }
    return is_insertable;
}

void MemorySystem::ClockTick() {
    clk_++;
    ptr_stats_->dramcycles++;
    for( auto ctrl : ctrls_)
        ctrl->ClockTick();

    //Insert requests stored in the buffer_q as and when space is available
    if(!buffer_q_.empty()) {
        for(auto req_itr = buffer_q_.begin(); req_itr != buffer_q_.end(); req_itr++) {
            auto req = *req_itr;
            if(ctrls_[req->Channel()]->InsertReq(req)) {
                buffer_q_.erase(req_itr);
                break;  // either break or set req_itr to the return value of erase()
            }
        }
    }

    if( clk_ % ptr_config_->epoch_period == 0) {
        PrintIntermediateStats();
        ptr_stats_->UpdateEpoch();
    }
    return;
}

void MemorySystem::PrintIntermediateStats() {
    cummulative_stats_file_ << "-----------------------------------------------------" << endl;
    cummulative_stats_file_ << "Cummulative stats at clock = " << clk_ << endl;
    cummulative_stats_file_ << "-----------------------------------------------------" << endl;
    ptr_stats_->PrintStats(cummulative_stats_file_);
    cummulative_stats_file_ << "-----------------------------------------------------" << endl;

    epoch_stats_file_ << "-----------------------------------------------------" << endl;
    epoch_stats_file_ << "Epoch stats from clock = " << clk_ - ptr_config_->epoch_period << " to " << clk_<< endl;
    epoch_stats_file_ << "-----------------------------------------------------" << endl;
    ptr_stats_->PrintEpochStats(epoch_stats_file_);
    epoch_stats_file_ << "-----------------------------------------------------" << endl;

    ptr_stats_->PrintStatsCSVFormat(cummulative_stats_file_csv_);
    ptr_stats_->PrintEpochStatsCSVFormat(epoch_stats_file_csv_);
    return;
}


void MemorySystem::PrintStats() {
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


// This function can be used by autoconf AC_CHECK_LIB since
// apparently it can't detect C++ functions.
// Basically just an entry in the symbol table
extern "C"
{
void libdramcore_is_present(void)
{
    ;
}
}