#ifndef __BANKSTATE_H
#define __BANKSTATE_H

#include <map>
#include <list>
#include <utility>

enum class State {
    OPEN,
    CLOSED,
    SELF_REFRESH,
};

enum class CommandType {
    READ,
    READ_PRECHARGE,
    WRITE,
    WRITE_PRECHARGE,
    ACTIVATE,
    PRECHARGE,
    REFRESH,
    SELF_REFRESH_ENTER,
    SELF_REFRESH_EXIT,
};

enum class RequestType {
    READ,
    READ_PRECHARGE,
    WRITE,
    WRITE_PRECHARGE,
    REFRESH,
    SELF_REFRESH_ENTER,
    SEFL_REFRESH_EXIT,
};

class Command { 
    public:
        Command() {}
        CommandType cmd_type_;
        int channel_, rank_, bankgroup_, bank_, row_, column_;
};

class Request {
    public:
        Request() {}
        RequestType request_type_;
        int channel_, rank_, bankgroup_, bank_, row_, column_;
};

class BankState {
    public:
        BankState(int rank, int bankgroup, int bank);
        CommandType GetRequiredCommand(const Request& req);
        void UpdateState(const Command& cmd);
        void UpdateTiming(const CommandType cmd_type, long time);
        bool IsReady(const Command& cmd, long time);
    private:
        // Current state of the Bank
        // Apriori or instantaneously transitions on a command.
        State state_;

        // Earliest time when the particular Command can be executed in this bank
        std::map<CommandType, long> timing_;

        //Currently open row
        //Applicable only if the bank is in OPEN state
        int open_row_;

        int rank_, bankgroup_, bank_;
};

#endif