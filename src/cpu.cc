#include "cpu.h"
#include <iostream>

using namespace std;

RandomCPU::RandomCPU()
{}

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

    channel_width_ = LogBase2(config_.channels);
    rank_width_ = LogBase2(config_.ranks);
    bankgroup_width_ = LogBase2(config_.bankgroups);
    bank_width_ = LogBase2(config_.banks_per_group);
    row_width_ = LogBase2(config_.rows);
    column_width_ = LogBase2(config_.columns);
    /*Access access;
    auto count = 0;
    uint64_t largest = 0;
    while(!trace_file_.eof()) {
        trace_file_ >> access;
        //cout << access;
        largest = access.hex_addr_ > largest ? access.hex_addr_ : largest;
        count++;
        //if(count >= 100) break;
    }
    cout << "Count = " << count << endl;
    cout << "Largest = " << hex << largest << dec << endl;*/
}


void TraceBasedCPU::ClockTick() {
    Access access;
    if(!trace_file_.eof()) {
        if(get_next_) {
            trace_file_ >> access;
            req_ = FormRequest(access);
            get_next_ = false;
        }
        if(access.time_ >= clk) {
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
    if(config.address_mapping == "rorababgchcl") {
        //int channel, rank, bankgroup, bank, row, column;
        
    }
    return Address();
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

int LogBase2(int power_of_two) {
    int i = 0;
    while( power_of_two > 0) {
        power_of_two /= 2;
    }
    return i;
}