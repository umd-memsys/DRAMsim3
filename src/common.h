#ifndef __COMMON_H
#define __COMMON_H

#include <iostream>

class Address {
    public:
        Address() :
            channel_(-1), rank_(-1), bankgroup_(-1), bank_(-1), row_(-1), column_(-1) {}
        Address(int channel, int rank, int bankgroup, int bank, int row, int column) :
            channel_(channel), rank_(rank), bankgroup_(bankgroup), bank_(bank), row_(row), column_(column) {}
        int channel_;
        int rank_;
        int bankgroup_;
        int bank_;
        int row_;
        int column_;
};

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
        bool IsRefresh() { return cmd_type_ == CommandType::REFRESH || cmd_type_ == CommandType::REFRESH_BANK; }
        bool IsReadWrite() const { return cmd_type_ == CommandType::READ || cmd_type_ == CommandType::READ_PRECHARGE ||
                                          cmd_type_ == CommandType::WRITE || cmd_type_ == CommandType::WRITE_PRECHARGE; }
        CommandType cmd_type_;
        int channel_, rank_, bankgroup_, bank_, row_;

        friend std::ostream& operator<<(std::ostream& os, const Command& cmd);
};


class Request {
    public:
        //These constructors are hopelessly stupid. Create a addr sturct and make them right
        Request(CommandType cmd_type, int rank) :
            cmd_(Command(cmd_type, -1, rank, -1, -1, -1)), arrival_time_(-1), exit_time_(-1) {}
        Request(CommandType cmd_type, int rank, int bankgroup, int bank, int row, int id) :
            cmd_(Command(cmd_type, -1, rank, bankgroup, bank, row)), arrival_time_(-1), exit_time_(-1), id_(id) {}
        Request(CommandType cmd_type, const Address& addr, int arrival_time, int id) :
            cmd_(Command(cmd_type, addr.channel_, addr.rank_, addr.bankgroup_, addr.bank_, addr.row_)),
             arrival_time_(arrival_time), exit_time_(-1), id_(id) {}

        Command cmd_;
        long arrival_time_;
        long exit_time_;
        int id_ = 0;

        friend std::ostream& operator<<(std::ostream& os, const Request& req);
};

#endif