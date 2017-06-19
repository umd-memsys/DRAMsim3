#ifndef __COMMON_H
#define __COMMON_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <functional>
#include <stdint.h>
#include <vector>

namespace dramcore {

class Address {
    public:
        Address() :
            channel_(-1), rank_(-1), bankgroup_(-1), bank_(-1), row_(-1), column_(-1) {}
        Address(int channel, int rank, int bankgroup, int bank, int row, int column) :
            channel_(channel), rank_(rank), bankgroup_(bankgroup), bank_(bank), row_(row), column_(column) {}
        Address(const Address& addr) :
            channel_(addr.channel_), rank_(addr.rank_), bankgroup_(addr.bankgroup_), bank_(addr.bank_), row_(addr.row_), column_(addr.column_) {}
        int32_t channel_;
        int32_t rank_;
        int32_t bankgroup_;
        int32_t bank_;
        int32_t row_;
        int32_t column_;
};

uint32_t ModuloWidth(uint64_t addr, uint32_t bit_width, uint32_t pos);
extern std::function<Address(uint64_t)> AddressMapping;
// void SetAddressMapping(Config* config);
uint32_t LogBase2(uint32_t power_of_two);
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
        bool IsWrite() { return cmd_type_ == CommandType ::WRITE || cmd_type_ == CommandType ::WRITE_PRECHARGE; }
        bool IsReadWrite() const { return cmd_type_ == CommandType::READ || cmd_type_ == CommandType::READ_PRECHARGE ||
                                          cmd_type_ == CommandType::WRITE || cmd_type_ == CommandType::WRITE_PRECHARGE; }
        CommandType cmd_type_;
        Address addr_;

        int32_t Channel() const { return addr_.channel_ ; }
        int32_t Rank() const { return  addr_.rank_; }
        int32_t Bankgroup() const { return addr_.bankgroup_; }
        int32_t Bank() const { return addr_.bank_;  }
        int32_t Row() const { return  addr_.row_; }
        int32_t Column() const { return addr_.column_; }

        friend std::ostream& operator<<(std::ostream& os, const Command& cmd);
        void print(std::ofstream& val_file); //TODO - Remove?
};


class Request {
    public:
        // For refresh requests
        Request(CommandType cmd_type, const Address& addr) :
                cmd_(Command(cmd_type, addr)), hex_addr_(-1), arrival_time_(-1), exit_time_(-1) {}
        //For read/write requets
        Request(CommandType cmd_type, uint64_t hex_addr, uint64_t arrival_time) :
                cmd_(Command(cmd_type, AddressMapping(hex_addr))), hex_addr_(hex_addr), arrival_time_(arrival_time), exit_time_(-1) {}

        Command cmd_;
        uint64_t hex_addr_;
        uint64_t arrival_time_;
        uint64_t exit_time_;

        int32_t Channel() const { return cmd_.Channel(); }
        int32_t Rank() const { return cmd_.Rank(); }
        int32_t Bankgroup() const { return cmd_.Bankgroup(); }
        int32_t Bank() const { return cmd_.Bank(); }
        int32_t Row() const { return cmd_.Row(); }
        int32_t Column() const { return cmd_.Column(); }

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
