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

namespace dramcore {

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

    int Channel() const { return addr.channel; }
    int Rank() const { return addr.rank; }
    int Bankgroup() const { return addr.bankgroup; }
    int Bank() const { return addr.bank; }
    int Row() const { return addr.row; }
    int Column() const { return addr.column; }

    friend std::ostream& operator<<(std::ostream& os, const Command& cmd);
};

class Request {
   public:
    // For refresh requests
    Request(CommandType cmd_type, const Address& addr)
        : cmd_(Command(cmd_type, addr)),
          hex_addr_(-1),
          arrival_time_(-1),
          exit_time_(-1) {}
    // For read/write requets
    Request(CommandType cmd_type, uint64_t hex_addr, uint64_t arrival_time,
            int64_t id)
        : cmd_(Command(cmd_type, AddressMapping(hex_addr))),
          hex_addr_(hex_addr),
          arrival_time_(arrival_time),
          exit_time_(-1),
          id_(id) {}

    Command cmd_;
    uint64_t hex_addr_;
    uint64_t arrival_time_;
    uint64_t exit_time_;
    uint64_t id_;

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
}  // namespace dramcore
#endif
