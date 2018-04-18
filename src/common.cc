#include "common.h"
#include <iomanip>
#include <vector>
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
        "WRONG"};
    os << fmt::format("{:<20} {:>3} {:>3} {:>3} {:>3} {:>#8x} {:>#8x}",
                      command_string[static_cast<int>(cmd.cmd_type_)],
                      cmd.Channel(), cmd.Rank(), cmd.Bankgroup(), cmd.Bank(),
                      cmd.Row(), cmd.Column());
    return os;
}

ostream& operator<<(ostream& os, const Request& req) {
    os << "(" << req.arrival_time_ << "," << req.exit_time_ << "," << req.id_
       << ")"
       << " " << req.cmd_;
    return os;
}  // TODO - Unused code. Remove?

istream& operator>>(istream& is, Access& access) {
    is >> hex >> access.hex_addr_ >> access.access_type_ >> dec >> access.time_;
    return is;
}

ostream& operator<<(ostream& os, const Access& access) { return os; }

uint32_t ModuloWidth(uint64_t addr, uint32_t bit_width, uint32_t pos) {
    addr >>= pos;
    auto store = addr;
    addr >>= bit_width;
    addr <<= bit_width;
    return static_cast<uint32_t>(store ^ addr);
}

int GetBitInPos(uint64_t bits, int pos) {
    // given a uint64_t value get the binary value of pos-th bit
    // from MSB to LSB indexed as 63 - 0
    return (bits >> pos) & 1;
}

uint32_t LogBase2(uint32_t power_of_two) {
    uint32_t i = 0;
    while (power_of_two > 1) {
        power_of_two /= 2;
        i++;
    }
    return i;
}

std::vector<std::string> StringSplit(const std::string& s, char delim) {
    std::vector<std::string> elems;
    StringSplit(s, delim, std::back_inserter(elems));
    return elems;
}

template <typename Out>
void StringSplit(const std::string& s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (!item.empty()) {
            *(result++) = item;
        }
    }
}

void AbruptExit(const std::string& file, int line) {
    std::cerr << "Exiting Abruptly - " << file << ":" << line << endl;
    exit(-1);
}

// Dummy callback functions for use when the simulator is not integrated with a
// frontend feeders
void read_callback_func(uint64_t addr) {
#ifdef LOG_REQUESTS
    cout << "Read Request with address = " << addr << " is returned" << endl;
#endif
    return;
}

void write_callback_func(uint64_t addr) {
#ifdef LOG_REQUESTS
    cout << "Write Request with address = " << addr << " is returned" << endl;
#endif
}

bool DirExist(std::string dir) {
    // courtesy to stackoverflow
    struct stat info;
    if (stat(dir.c_str(), &info) != 0) {
        return false;
    } else if (info.st_mode & S_IFDIR) {
        return true;
    } else {  // exists but is file
        return false;
    }
}

void RenameFileWithNumber(std::string& file_name, int number) {
    int last_dot_index = file_name.find_last_of(".");
    std::string prefix = file_name.substr(0, last_dot_index);
    std::string postfix = file_name.substr(last_dot_index, file_name.size());
    file_name = prefix + "_" + std::to_string(number) + postfix;
    return;
}

}  // namespace dramcore
