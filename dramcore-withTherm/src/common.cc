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
        "self_refresh_enter",
        "self_refresh_exit",
        "WRONG"
    };
    os << fmt::format("{:<20} {:>3} {:>3} {:>3} {:>3} {:>#8x} {:>#8x}", command_string[static_cast<int>(cmd.cmd_type_)],
                      cmd.Channel(), cmd.Rank(), cmd.Bankgroup(), cmd.Bank(), cmd.Row(), cmd.Column());
    return os;
}

ostream& operator<<(ostream& os, const Request& req) {
    os << "(" << req.arrival_time_ << "," << req.exit_time_ << "," << req.id_ << ")" << " " << req.cmd_;
    return os;
} //TODO - Unused code. Remove?

istream& operator>>(istream& is, Access& access) {
    is >> hex >> access.hex_addr_ >> access.access_type_ >> dec >> access.time_;
    return is;
}

ostream& operator<<(ostream& os, const Access& access) {
    return os;
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

//Dummy callback function for use when the simulator is not integrated with zsim,SST or other frontend feeders
void callback_func(uint64_t addr) {
#ifdef LOG_REQUESTS
    cout << "Request with address = " << addr << " is returned" << endl;
#endif
    return;
}

}
