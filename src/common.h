#ifndef __COMMON_H
#define __COMMON_H

#include <iostream>

enum class State {
    OPEN,
    CLOSED,
    SELF_REFRESH,
    SIZE
};

enum class CommandType {
    READ,
    READ_PRECHARGE,
    WRITE,
    WRITE_PRECHARGE,
    ACTIVATE,
    PRECHARGE,
    REFRESH_BANK,
    REFRESH,
    SELF_REFRESH_ENTER,
    SELF_REFRESH_EXIT,
    SIZE
};

class Command { 
    public:
        Command() :
            cmd_type_(CommandType::SIZE), channel_(-1), rank_(-1), bankgroup_(-1), bank_(-1), row_(-1) {}
        Command(CommandType cmd_type, int channel, int rank, int bankgroup, int bank, int row) :
            cmd_type_(cmd_type), channel_(channel), rank_(rank), bankgroup_(bankgroup), bank_(bank), row_(row) {}
        Command(const Command& cmd, CommandType cmd_type) :
            cmd_type_(cmd_type), channel_(cmd.channel_), rank_(cmd.rank_), bankgroup_(cmd.bankgroup_), bank_(cmd.bank_), row_(cmd.row_) {}

        bool IsValid() { return cmd_type_ != CommandType::SIZE; }
        CommandType cmd_type_;
        int channel_, rank_, bankgroup_, bank_, row_;

        friend std::ostream& operator<<(std::ostream& os, const Command& cmd);
};


class Request {
    public:
        //These constructors are hopelessly stupid. Create a addr sturct and make them right
        Request(CommandType cmd_type, int rank) :
            cmd_(Command(cmd_type, -1, rank, -1, -1, -1)), arrival_time_(-1), exit_time_(-1) {}
        Request(CommandType cmd_type, int rank, int bankgroup, int bank, int row) :
            cmd_(Command(cmd_type, -1, rank, bankgroup, bank, row)), arrival_time_(-1), exit_time_(-1) {}
        Command cmd_;
        long arrival_time_;
        long exit_time_;

        friend std::ostream& operator<<(std::ostream& os, const Request& req);
};

#endif