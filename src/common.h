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
    REFRESH,
    REFRESH_BANK,
    SELF_REFRESH_ENTER,
    SELF_REFRESH_EXIT,
    SIZE
};

enum class RequestType {
    READ,
    READ_PRECHARGE,
    WRITE,
    WRITE_PRECHARGE,
    REFRESH,
    REFRESH_BANK,
    SELF_REFRESH_ENTER,
    SELF_REFRESH_EXIT,
    SIZE
};


class Command { 
    public:
        Command(CommandType cmd_type, int channel, int rank, int bankgroup, int bank, int row, int column) :
            cmd_type_(cmd_type), channel_(channel), rank_(rank), bankgroup_(bankgroup), bank_(bank), row_(row), column_(column) {}
        CommandType cmd_type_;
        int channel_, rank_, bankgroup_, bank_, row_, column_;
};

class Request {
    public:
        Request() {}
        RequestType request_type_;
        int channel_, rank_, bankgroup_, bank_, row_, column_;
};

#endif