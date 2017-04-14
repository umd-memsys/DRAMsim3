#include <vector>
#include <iomanip>
#include "common.h"


using namespace std;

namespace dramcore {

ostream& operator<<(ostream& os, const Command& cmd) {
    vector<string> command_string = {
        "READ",
        "READ_PRECHARGE",
        "WRITE",
        "WRITE_PRECHARGE",
        "ACTIVATE",
        "PRECHARGE",
        "REFRESH_BANK",
        "REFRESH",
        "SELF_REFRESH_ENTER",
        "SELF_REFRESH_EXIT",
        "SIZE"
    };
    os << setw(10) << command_string[static_cast<int>(cmd.cmd_type_)] << " " << cmd.Channel() << " " << cmd.Rank() << " " << cmd.Bankgroup() << " " << cmd.Bank() << " " << cmd.Row();
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
    unsigned int pos = 3; //TODO - Remove hardcoding
    //There could be as many as 6! = 720 different address mappings!
    if(config.address_mapping == "chrarocobabg") {  // channel:rank:row:column:bank:bankgroup
        auto bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        auto bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        auto column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        auto row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        auto rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        auto channel = ModuloWidth(hex_addr, config.channel_width_, pos);
        return Address(channel, rank, bankgroup, bank, row, column);
    }
    else if(config.address_mapping == "chrocobabgra") {  // channel:row:column:bank:bankgroup:rank
        auto rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        auto bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        auto bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        auto column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        auto row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        auto channel = ModuloWidth(hex_addr, config.channel_width_, pos);
        return Address(channel, rank, bankgroup, bank, row, column);
    }
    else if(config.address_mapping == "chrababgcoro") {  // channel:rank:bank:bankgroup:column:row
        auto row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        auto column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        auto bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        auto bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        auto rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        auto channel = ModuloWidth(hex_addr, config.channel_width_, pos);
        return Address(channel, rank, bankgroup, bank, row, column);
    }
    else if(config.address_mapping == "chrababgroco") {  // channel:rank:bank:bankgroup:row:column
        auto column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        auto row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        auto bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        auto bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        auto rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        auto channel = ModuloWidth(hex_addr, config.channel_width_, pos);
        return Address(channel, rank, bankgroup, bank, row, column);
    }
    else if(config.address_mapping == "chrocorababg") {  // channel:row:column:rank:bank:bankgroup
        auto bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        auto bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        auto rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        auto column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        auto row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        auto channel = ModuloWidth(hex_addr, config.channel_width_, pos);
        return Address(channel, rank, bankgroup, bank, row, column);
    }
    else if(config.address_mapping == "chrobabgraco") {  // channel:row:bank:bankgroup:rank:column
        auto column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        auto rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        auto bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        auto bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        auto row = ModuloWidth(hex_addr, config.row_width_, pos);
        pos += config.row_width_;
        auto channel = ModuloWidth(hex_addr, config.channel_width_, pos);
        return Address(channel, rank, bankgroup, bank, row, column);
    }
    else if(config.address_mapping == "rocorababgch") {  // row:column:rank:bank:bankgroup:channel
        auto channel = ModuloWidth(hex_addr, config.channel_width_, pos);
        pos += config.channel_width_;
        auto bankgroup = ModuloWidth(hex_addr, config.bankgroup_width_, pos);
        pos += config.bankgroup_width_;
        auto bank = ModuloWidth(hex_addr, config.bank_width_, pos);
        pos += config.bank_width_;
        auto rank = ModuloWidth(hex_addr, config.rank_width_, pos);
        pos += config.rank_width_;
        auto column = ModuloWidth(hex_addr, config.column_width_, pos);
        pos += config.column_width_;
        auto row = ModuloWidth(hex_addr, config.row_width_, pos);
        return Address(channel, rank, bankgroup, bank, row, column);
    }
    else {
        cerr << "Unknown address mapping" << endl;
        AbruptExit(__FILE__, __LINE__);
    }
}

unsigned int ModuloWidth(uint64_t addr, unsigned int bit_width, unsigned int pos) {
    addr >>= pos;
    auto store = addr;
    addr >>= bit_width;
    addr <<= bit_width;
    return static_cast<unsigned int>(store ^ addr);
}

unsigned int LogBase2(unsigned int power_of_two) {
    unsigned int i = 0;
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
    cout << "Request with id = " << req_id << " is returned" << endl;
    return;
}

}