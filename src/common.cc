#include <vector>
#include "common.h"


using namespace std;

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
    os << command_string[int(cmd.cmd_type_)] << " " << cmd.rank_ << " " << cmd.bankgroup_ << " " << cmd.bank_ << " " << cmd.row_;
    return os;
}

ostream& operator<<(ostream& os, const Request& req) {
    os << "(" << req.arrival_time_ << "," << req.exit_time_ << "," << req.id_ << ")" << " " << req.cmd_;
    return os;
}