#include "cpu.h"
#include <iostream>

using namespace std;

RandomCPU::RandomCPU(vector<Controller*>& ctrls, const Config& config) :
    ctrls_(ctrls),
    config_(config),
    clk_(0),
    last_addr_(0, 0, 0, 0, 0, -1),
    req_log_("requests.log")
{}


void RandomCPU::ClockTick()
{
    // Create random CPU requests at random time intervals
    // With random row buffer hits
    // And insert them into the controller
    auto column = -1;
    if ( rand() % 4 == 0) { 
        if(get_next_) {
            get_next_ = false;
            auto channel = rand() % config_.channels;
            auto rank = rand() % config_.ranks;
            auto bankgroup = rand() % config_.bankgroups;
            auto bank = rand() % config_.banks_per_group;
            auto row =  rand() % config_.rows;
            auto addr = rand() % 3 == 0 ? last_addr_ : Address(channel, rank, bankgroup, bank, row, column);
            last_addr_ = addr;
            auto cmd_type = rand() % 3 == 0 ? CommandType::WRITE : CommandType::READ;
            req_ = new Request(cmd_type, addr, clk_, req_id_);
        }
        if(ctrls_[req_->cmd_.channel_]->InsertReq(req_)) {
            get_next_ = true;
            req_id_++;
            req_log_ << "Request Inserted at clk = " << clk_ << " " << *req_ << endl;
        }
    }
    clk_++;
    return;
}

TraceBasedCPU::TraceBasedCPU(vector<Controller*>& ctrls, const Config& config) :
    ctrls_(ctrls),
    config_(config),
    clk(0)
{
    trace_file_.open("sample_trace.txt");
    if(trace_file_.fail()) {
        cerr << "Trace file does not exist" << endl << " Exiting abruptly" << endl;
        exit(-1);
    }
}

void TraceBasedCPU::ClockTick() {
    if(!trace_file_.eof()) {
        if(get_next_) {
            get_next_ = false;
            Access access;
            trace_file_ >> access;
            req_ = FormRequest(access);
        }
        if(req_->arrival_time_ <= clk) {
            if(ctrls_[req_->cmd_.channel_]->InsertReq(req_)) {
                get_next_ = true;
                req_id_++;
            }
        }
    }
    clk++;
    return;
}

istream& operator>>(istream& is, Access& access) {
    is >> hex >> access.hex_addr_ >> access.access_type_ >> dec >> access.time_;
    return is;
}

ostream& operator<<(ostream& os, const Access& access) {
    return os;
}

Address AddressMapping(uint64_t hex_addr, const Config& config) {
    //Implement address mapping functionality
    int pos = 3;
    if(config.address_mapping == "rorababgchcl") {
       auto column = ModuloWidth(hex_addr, config.column_width_, pos);
       pos += config.column_width_;
       auto channel = ModuloWidth(hex_addr, config.channel_width_, pos);
       pos += config.channel_width_;
       auto rank =  ModuloWidth(hex_addr, config.rank_width_, pos);
       pos += config.rank_width_;
       auto bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
       pos += config.bankgroup_width_;
       auto bank = ModuloWidth(hex_addr, config.bank_width_, pos);
       pos += config.bank_width_;
       auto row = ModuloWidth(hex_addr, config.row_width_, pos);
       return Address(channel, rank, bankgroup, bank, row, column);
    };
    cerr << "Unknown address_mapping" << endl;
    exit(-1);
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
        exit(-1);
    }
    return new Request(cmd_type, addr, access.time_, req_id_);
}

int ModuloWidth(uint32_t addr, int bit_width, int pos) {
    addr >>= pos;
    auto store = addr;
    addr >>= bit_width;
    addr <<= bit_width;
    return store ^ addr;
}