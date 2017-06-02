#include <vector>
#include <iomanip>
#include "common.h"
#include "../ext/fmt/src/format.h"


using namespace std;

namespace dramcore {

ostream& operator<<(ostream& os, const Command& cmd) {
    vector<string> command_string = {
        "read",
        "read_p",
        "write",
        "write_p",
        "activate",
        "precharge",
        "refresh_bank",  // verilog model doesn't distinguish bank/rank refresh 
        "refresh",
        "self_refresh",
        "self_refresh_exit",
        "WRONG"
    };
    os << fmt::format("{:<12} {:>3} {:>3} {:>3} {:>3} {:>#8x} {:>#8x}", command_string[static_cast<int>(cmd.cmd_type_)],
                      cmd.Channel(), cmd.Rank(), cmd.Bankgroup(), cmd.Bank(), cmd.Row(), cmd.Column());
    return os;
}

ostream& operator<<(ostream& os, const Request& req) {
    os << "(" << req.arrival_time_ << "," << req.exit_time_ << "," << req.id_ << ")" << " " << req.cmd_;
    return os;
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
    uint32_t pos = config.throwaway_bits;
    uint32_t channel = 0, rank = 0, bank = 0, bankgroup = 0, row = 0, column = 0;
    //There could be as many as 6! = 720 different address mappings!
    if(config.address_mapping == "chrarocobabg") {  // channel:rank:row:column:bank:bankgroup
        bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        channel = ModuloWidth(hex_addr, config.channel_width_, pos);
    }
    else if(config.address_mapping == "chrocobabgra") {  // channel:row:column:bank:bankgroup:rank
        rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        channel = ModuloWidth(hex_addr, config.channel_width_, pos);
    }
    else if(config.address_mapping == "chrababgcoro") {  // channel:rank:bank:bankgroup:column:row
        row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        channel = ModuloWidth(hex_addr, config.channel_width_, pos);
    }
    else if(config.address_mapping == "chrababgroco") {  // channel:rank:bank:bankgroup:row:column
        column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        channel = ModuloWidth(hex_addr, config.channel_width_, pos);
    }
    else if(config.address_mapping == "chrocorababg") {  // channel:row:column:rank:bank:bankgroup
        bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        channel = ModuloWidth(hex_addr, config.channel_width_, pos);
    }
    else if(config.address_mapping == "chrobabgraco") {  // channel:row:bank:bankgroup:rank:column
        column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        channel = ModuloWidth(hex_addr, config.channel_width_, pos);
    }
    else if(config.address_mapping == "rocorababgch") {  // row:column:rank:bank:bankgroup:channel
        channel = ModuloWidth(hex_addr, config.channel_width_, pos);
        pos += config.channel_width_;
        bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        row = ModuloWidth(hex_addr, config.row_width_, pos);  
    }
    else {
        cerr << "Unknown address mapping" << endl;
        AbruptExit(__FILE__, __LINE__);
    } 
    return Address(channel, rank, bankgroup, bank, row, column);
}

uint32_t ModuloWidth(uint64_t addr, uint32_t bit_width, uint32_t pos) {
    addr >>= pos;
    auto store = addr;
    addr >>= bit_width;
    addr <<= bit_width;
    return static_cast<uint32_t>(store ^ addr);
}

uint32_t LogBase2(uint32_t power_of_two) {
    uint32_t i = 0;
    while( power_of_two > 1) {
        power_of_two /= 2;
        i++;
    }
    return i;
}

void AbruptExit(const std::string& file, int line) {
    std::cerr << "Exiting Abruptly - " << file << ":" << line << endl; \
    exit(-1);
}

//Dummy callback function for use when the simulator is not integrated with SST or other frontend feeders
void callback_func(uint64_t req_id) {
#ifdef DEBUG_OUTPUT
        //TODO - set correct request id
//    cout << "Request with id = " << req_id << " is returned" << endl;
#endif
    return;
}

}
