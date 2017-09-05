#include "memory_system.h"
#include "../ext/fmt/src/format.h"
#include <assert.h>

using namespace std;
using namespace dramcore;

BaseMemorySystem::BaseMemorySystem(const std::string &config_file, const std::string &output_dir, std::function<void(uint64_t)> read_callback, std::function<void(uint64_t)> write_callback) :
    read_callback_(read_callback),
    write_callback_(write_callback),
    clk_(0)
{
    ptr_config_ = new Config(config_file);
    ptr_timing_ = new Timing(*ptr_config_);
    ptr_stats_ = new Statistics(*ptr_config_);

    //Stats output files
    //TODO - Implement checks to see if a directory exits or else to create one.
    cout << fmt::format("***Note***\nEnsure that the directory '{}' exists in the working directory\notherwise dramcore output stat files won't be printed\n", output_dir);
    const std::string output_dir_path = output_dir + "/";

    stats_file_.open(output_dir_path + ptr_config_->stats_file);
    cummulative_stats_file_.open(output_dir_path + ptr_config_->cummulative_stats_file);
    epoch_stats_file_.open(output_dir_path + ptr_config_->epoch_stats_file);
    stats_file_csv_.open(output_dir_path + ptr_config_->stats_file_csv);
    cummulative_stats_file_csv_.open(output_dir_path + ptr_config_->cummulative_stats_file_csv);
    epoch_stats_file_csv_.open(output_dir_path + ptr_config_->epoch_stats_file_csv);

    ptr_stats_->PrintStatsCSVHeader(stats_file_csv_);
    ptr_stats_->PrintStatsCSVHeader(cummulative_stats_file_csv_);
    ptr_stats_->PrintStatsCSVHeader(epoch_stats_file_csv_);
#ifdef GENERATE_TRACE
    address_trace_.open("address.trace");
#endif
}

BaseMemorySystem::~BaseMemorySystem() {
    delete(ptr_stats_);
    delete(ptr_timing_);
    delete(ptr_config_);

    stats_file_.close();
    cummulative_stats_file_.close();
    epoch_stats_file_.close();
    stats_file_csv_.close();
    cummulative_stats_file_csv_.close();
    epoch_stats_file_csv_.close();
#ifdef GENERATE_TRACE
    address_trace_.close();
#endif
}

void BaseMemorySystem::PrintIntermediateStats() {
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


void BaseMemorySystem::PrintStats() {
    // update one last time before print
    ptr_stats_->UpdateEpoch(clk_);
    cout << "-----------------------------------------------------" << endl;
    cout << "Printing final stats -- " << endl;
    cout << "-----------------------------------------------------" << endl;
    cout << *ptr_stats_;
    cout << "-----------------------------------------------------" << endl;
    cout << "The stats are also written to the file " << "dramcore_stats.txt" << endl;
    ptr_stats_->PrintStats(stats_file_);
    ptr_stats_->PrintStatsCSVFormat(stats_file_csv_);
    return;
}

MemorySystem::MemorySystem(const string &config_file, const std::string &output_dir, std::function<void(uint64_t)> read_callback, std::function<void(uint64_t)> write_callback) :
    BaseMemorySystem(config_file, output_dir, read_callback, write_callback)
{
    if (ptr_config_->IsHMC()) {
        cerr << "Initialized a memory system with an HMC config file!" << endl;
        AbruptExit(__FILE__, __LINE__);
    } //TODO - Unnecessary. To remove.

    ctrls_.resize(ptr_config_->channels);
    for(auto i = 0; i < ptr_config_->channels; i++) {
        ctrls_[i] = new Controller(i, *ptr_config_, *ptr_timing_, *ptr_stats_, read_callback_, write_callback_);
    }
}

MemorySystem::~MemorySystem()
{
    for(auto i = 0; i < ptr_config_->channels; i++) {
        delete(ctrls_[i]);
    }
}

bool MemorySystem::IsReqInsertable(uint64_t hex_addr, bool is_write) {
    CommandType cmd_type = is_write ? CommandType::WRITE : CommandType ::READ;
    Request* temp_req = new Request(cmd_type, hex_addr, clk_, 0); //TODO - This is probably not very efficient
    bool status = ctrls_[temp_req->Channel()]->IsReqInsertable(temp_req);
    delete temp_req;
    return status;
}

bool MemorySystem::InsertReq(uint64_t hex_addr, bool is_write) { //TODO - make it return void
    //Record trace - Record address trace for debugging or other purposes
#ifdef GENERATE_TRACE
    address_trace_ << fmt::format("{:#x} {} {}\n", hex_addr, is_write ? "WRITE" : "READ", clk_);
#endif

    CommandType cmd_type = is_write ? CommandType::WRITE : CommandType ::READ;
    id_++;
    Request* req = new Request(cmd_type, hex_addr, clk_, id_);

    bool is_insertable = ctrls_[req->Channel()]->InsertReq(req);
    assert(is_insertable);
    return is_insertable;
}

void MemorySystem::ClockTick() {
    for( auto ctrl : ctrls_)
        ctrl->ClockTick();


    if( clk_ % ptr_config_->epoch_period == 0) {
        ptr_stats_->UpdatePreEpoch(clk_);
        PrintIntermediateStats();
        ptr_stats_->UpdateEpoch(clk_);
    }
    
    clk_++;
    ptr_stats_->dramcycles++;
    return;
}


IdealMemorySystem::IdealMemorySystem(const std::string &config_file, const std::string &output_dir, std::function<void(uint64_t)> read_callback, std::function<void(uint64_t)> write_callback):
    BaseMemorySystem(config_file, output_dir, read_callback, write_callback),
    latency_(ptr_config_->ideal_memory_latency)
{

}

IdealMemorySystem::~IdealMemorySystem() {}

bool IdealMemorySystem::InsertReq(uint64_t hex_addr, bool is_write) {
    CommandType cmd_type = is_write ? CommandType::WRITE : CommandType ::READ;
    id_++;
    Request* req = new Request(cmd_type, hex_addr, clk_, id_);
    infinite_buffer_q_.push_back(req);
    return true;
}

void IdealMemorySystem::ClockTick() {
    clk_++;
    ptr_stats_->dramcycles++;
    for(auto req_itr = infinite_buffer_q_.begin(); req_itr != infinite_buffer_q_.end(); req_itr++) {
        auto req = *req_itr;
        if(clk_ - req->arrival_time_ >= latency_) {
            if(req->cmd_.cmd_type_ == CommandType::READ) {
                ptr_stats_->numb_read_reqs_issued++;
            }
            else if(req->cmd_.cmd_type_ == CommandType::WRITE) {
                ptr_stats_->numb_write_reqs_issued++;
            }
            ptr_stats_->access_latency.AddValue(clk_ - req->arrival_time_);
            if(req->cmd_.IsRead())
                read_callback_(req->hex_addr_);
            else if(req->cmd_.IsWrite())
                write_callback_(req->hex_addr_);
            else
                AbruptExit(__FILE__, __LINE__);
            delete(req);
            infinite_buffer_q_.erase(req_itr++);
        }
        else
            break; //Requests are always ordered w.r.t to their arrival times, so need to check beyond.
    }

    if( clk_ % ptr_config_->epoch_period == 0) {
        PrintIntermediateStats();
        ptr_stats_->UpdateEpoch(clk_);
    }
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