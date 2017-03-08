#ifndef __COMMON_H
#define __COMMON_H

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
            cmd_type_(CommandType::SIZE), channel_(-1), rank_(-1), bankgroup_(-1), bank_(-1), row_(-1), column_(-1) {}
        Command(CommandType cmd_type, int channel, int rank, int bankgroup, int bank, int row, int column) :
            cmd_type_(cmd_type), channel_(channel), rank_(rank), bankgroup_(bankgroup), bank_(bank), row_(row), column_(column) {}
        Command(const Command& cmd, CommandType cmd_type) :
            cmd_type_(cmd_type), channel_(cmd.channel_), rank_(cmd.rank_), bankgroup_(cmd.bankgroup_), bank_(cmd.bank_), row_(cmd.row_), column_(cmd.column_) {}

        bool IsValid() { return cmd_type_ != CommandType::SIZE; }
        CommandType cmd_type_;
        int channel_, rank_, bankgroup_, bank_, row_, column_;
};


class Request {
    public:
        Request() {}
        Command cmd_;
        long arrival_time_;
        long exit_time_;
};

#endif