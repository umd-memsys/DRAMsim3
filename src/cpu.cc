#include "cpu.h"

using namespace std;
using namespace dramcore;

RandomCPU::RandomCPU(MemorySystem& memory_system) :
    memory_system_(memory_system),
    config_(*(memory_system.ptr_config_)),
    clk_(0),
    last_addr_(0, 0, 0, 0, 0, -1),
    req_log_("requests.log")
{}


void RandomCPU::ClockTick()
{
    // Create random CPU requests at random time intervals
    // With random row buffer hits
    // And insert them into the controller
    if ( rand() % 4 == 0) { 
        if(get_next_) {
            get_next_ = false;
            auto channel = rand() % config_.channels;
            auto rank = rand() % config_.ranks;
            auto bankgroup = rand() % config_.bankgroups;
            auto bank = rand() % config_.banks_per_group;
            auto row =  rand() % config_.rows;
            auto col = rand() % config_.columns;
            auto addr = rand() % 3 == 0 ? last_addr_ : Address(channel, rank, bankgroup, bank, row, col);
            last_addr_ = addr;
            auto cmd_type = rand() % 3 == 0 ? CommandType::WRITE : CommandType::READ;
            req_ = new Request(cmd_type, addr, clk_, req_id_);
        }
        if(memory_system_.ctrls_[req_->Channel()]->InsertReq(req_)) { //TODO - This is silly. Modify it.
            get_next_ = true;
            req_id_++;
            req_log_ << "Request Inserted at clk = " << clk_ << " " << *req_ << endl;
        }
    }
    clk_++;
    return;
}

TraceBasedCPU::TraceBasedCPU(MemorySystem& memory_system, std::string trace_file) :
    memory_system_(memory_system),
    config_(*(memory_system.ptr_config_)),
    clk_(0)
{
    trace_file_.open(trace_file);
    if(trace_file_.fail()) {
        cerr << "Trace file does not exist" << endl;
        AbruptExit(__FILE__, __LINE__);
    }
}

void TraceBasedCPU::ClockTick() {
    clk_++;
    if(!trace_file_.eof()) {
        if(get_next_) {
            get_next_ = false;
            Access access;
            trace_file_ >> access;
            req_ = FormRequest(access);
        }
        if(req_->arrival_time_ <= clk_) {
            if(memory_system_.ctrls_[req_->Channel()]->InsertReq(req_)) { //TODO - This is silly. Modify it.
                get_next_ = true;
                req_id_++;
            }
        }
    }
    return;
}

Request* TraceBasedCPU::FormRequest(const Access& access) {
    auto addr = AddressMapping(access.hex_addr_, config_);
    CommandType cmd_type;
    if( access.access_type_ == "READ")
        cmd_type = CommandType::READ;
    else if( access.access_type_ == "WRITE")
        cmd_type = CommandType::WRITE;
    else {
        cerr << "Unknown access type, should be either READ or WRITE" << endl;
        AbruptExit(__FILE__, __LINE__);
    }
    return new Request(cmd_type, addr, access.time_, req_id_);
}

