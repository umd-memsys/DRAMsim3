#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

namespace dramsim3 {

struct Address {
    Address()
        : channel(-1), rank(-1), bankgroup(-1), bank(-1), row(-1), column(-1) {}
    Address(int channel, int rank, int bankgroup, int bank, int row, int column)
        : channel(channel),
          rank(rank),
          bankgroup(bankgroup),
          bank(bank),
          row(row),
          column(column) {}
    Address(const Address& addr)
        : channel(addr.channel),
          rank(addr.rank),
          bankgroup(addr.bankgroup),
          bank(addr.bank),
          row(addr.row),
          column(addr.column) {}
    int channel;
    int rank;
    int bankgroup;
    int bank;
    int row;
    int column;
};

uint32_t ModuloWidth(uint64_t addr, uint32_t bit_width, uint32_t pos);
extern std::function<Address(uint64_t)> AddressMapping;
extern std::function<int(uint64_t)> MapChannel;
int GetBitInPos(uint64_t bits, int pos);
// it's 2017 and c++ std::string still lacks a split function, oh well
std::vector<std::string> StringSplit(const std::string& s, char delim);
template <typename Out>
void StringSplit(const std::string& s, char delim, Out result);

uint32_t LogBase2(uint32_t power_of_two);
void AbruptExit(const std::string& file, int line);
void read_callback_func(uint64_t req_id);
void write_callback_func(uint64_t req_id);
bool DirExist(std::string dir);
void RenameFileWithNumber(std::string& file_name, int number);

enum class State { OPEN, CLOSED, SELF_REFRESH, SIZE };

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

struct Command {
    Command() : cmd_type(CommandType::SIZE) {}
    Command(CommandType cmd_type, const Address& addr)
        : cmd_type(cmd_type), addr(addr) {}

    bool IsValid() const { return cmd_type != CommandType::SIZE; }
    bool IsRefresh() const {
        return cmd_type == CommandType::REFRESH ||
               cmd_type == CommandType::REFRESH_BANK;
    }
    bool IsRead() const {
        return cmd_type == CommandType::READ ||
               cmd_type == CommandType ::READ_PRECHARGE;
    }
    bool IsWrite() const {
        return cmd_type == CommandType ::WRITE ||
               cmd_type == CommandType ::WRITE_PRECHARGE;
    }
    bool IsReadWrite() const { return IsRead() || IsWrite(); }
    CommandType cmd_type;
    Address addr;
    int id;

    int Channel() const { return addr.channel; }
    int Rank() const { return addr.rank; }
    int Bankgroup() const { return addr.bankgroup; }
    int Bank() const { return addr.bank; }
    int Row() const { return addr.row; }
    int Column() const { return addr.column; }

    friend std::ostream& operator<<(std::ostream& os, const Command& cmd);
};

struct Transaction {
    Transaction() {}
    Transaction(uint64_t addr, bool is_write) 
        : addr(addr), is_write(is_write) {};
    uint64_t addr;
    uint64_t added_cycle;
    uint64_t complete_cycle;
    bool is_write;
};


class Access {
   public:
    uint64_t hex_addr_;
    std::string access_type_;
    uint64_t time_;
    friend std::istream& operator>>(std::istream& is, Access& access);
    friend std::ostream& operator<<(std::ostream& os, const Access& access);
};
}  // namespace dramsim3
#endif
