#ifndef __COMMON_H
#define __COMMON_H

#include <iostream>
#include <stdint.h>
#include "configuration.h"

namespace dramcore {

class Address {
    public:
        Address() :
            channel_(-1), rank_(-1), bankgroup_(-1), bank_(-1), row_(-1), column_(-1) {}
        Address(int channel, int rank, int bankgroup, int bank, int row, int column) :
            channel_(channel), rank_(rank), bankgroup_(bankgroup), bank_(bank), row_(row), column_(column) {}
        Address(const Address& addr) :
            channel_(addr.channel_), rank_(addr.rank_), bankgroup_(addr.bankgroup_), bank_(addr.bank_), row_(addr.row_), column_(addr.column_) {}
        int channel_;
        int rank_;
        int bankgroup_;
        int bank_;
        int row_;
        int column_;
};

unsigned int ModuloWidth(uint64_t addr, unsigned int bit_width, unsigned int pos);
Address AddressMapping(uint64_t hex_addr, const Config& config);
unsigned int LogBase2(unsigned int power_of_two);
void AbruptExit(const std::string& file, int line);
void callback_func(uint64_t req_id);

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
            cmd_type_(CommandType::SIZE) {}
        Command(CommandType cmd_type, const Address& addr) :
            cmd_type_(cmd_type), addr_(addr) {}

        bool IsValid() { return cmd_type_ != CommandType::SIZE; }
        bool IsRefresh() { return cmd_type_ == CommandType::REFRESH || cmd_type_ == CommandType::REFRESH_BANK; }
        bool IsRead() { return cmd_type_ == CommandType::READ || cmd_type_ == CommandType ::READ_PRECHARGE; }
        bool IsReadWrite() const { return cmd_type_ == CommandType::READ || cmd_type_ == CommandType::READ_PRECHARGE ||
                                          cmd_type_ == CommandType::WRITE || cmd_type_ == CommandType::WRITE_PRECHARGE; }
        CommandType cmd_type_;
        Address addr_;

        int Channel() const { return addr_.channel_ ; }
        int Rank() const { return  addr_.rank_; }
        int Bankgroup() const { return addr_.bankgroup_; }
        int Bank() const { return addr_.bank_;  }
        int Row() const { return  addr_.row_; }
        int Column() const { return addr_.column_; }

        friend std::ostream& operator<<(std::ostream& os, const Command& cmd);
};


class Request {
    public:
        //TODO - These constructors are terrible. Fix them ASAP
        Request(CommandType cmd_type, const Address& addr) :
                cmd_(Command(cmd_type, addr)), hex_addr_(0), arrival_time_(0), exit_time_(0), id_(-1) {}

        Request(CommandType cmd_type, uint64_t hex_addr, const Config& config) :
            cmd_(Command(cmd_type, AddressMapping(hex_addr, config))), hex_addr_(hex_addr), arrival_time_(0), exit_time_(0), id_(-1) {}

        Request(CommandType cmd_type, const Address& addr, uint64_t arrival_time, int64_t id) :
            cmd_(Command(cmd_type, addr)), hex_addr_(0), arrival_time_(arrival_time), exit_time_(0), id_(id) {}

        Request(CommandType cmd_type, uint64_t hex_addr, const Config& config, uint64_t arrival_time, int64_t id) :
            cmd_(Command(cmd_type, AddressMapping(hex_addr, config))), hex_addr_(hex_addr), arrival_time_(arrival_time), exit_time_(0), id_(id) {}

        Command cmd_;
        uint64_t hex_addr_;
        uint64_t arrival_time_;
        uint64_t exit_time_;
        int64_t id_ = 0;

        int Channel() const { return cmd_.Channel(); }
        int Rank() const { return cmd_.Rank(); }
        int Bankgroup() const { return cmd_.Bankgroup(); }
        int Bank() const { return cmd_.Bank(); }
        int Row() const { return cmd_.Row(); }
        int Column() const { return cmd_.Column(); }

        friend std::ostream& operator<<(std::ostream& os, const Request& req);
};

class Access {
public:
    uint64_t hex_addr_;
    std::string access_type_;
    uint64_t time_;
    friend std::istream& operator>>(std::istream& is, Access& access);
    friend std::ostream& operator<<(std::ostream& os, const Access& access);
};
}
#endif