#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>
#include <iostream>
#include <iomanip> 
#include <locale>
#include <vector>
#include <assert.h>

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

inline uint32_t ModuloWidth(uint64_t addr, uint32_t bit_width, uint32_t pos) {
    addr >>= pos;
    auto store = addr;
    addr >>= bit_width;
    addr <<= bit_width;
    return static_cast<uint32_t>(store ^ addr);
}

// extern std::function<Address(uint64_t)> AddressMapping;
int GetBitInPos(uint64_t bits, int pos);
// it's 2017 and c++ std::string still lacks a split function, oh well
std::vector<std::string> StringSplit(const std::string& s, char delim);
template <typename Out>
void StringSplit(const std::string& s, char delim, Out result);

int LogBase2(int power_of_two);
void AbruptExit(const std::string& file, int line);
bool DirExist(std::string dir);

enum class CommandType {
    READ,
    READ_PRECHARGE,
    WRITE,
    WRITE_PRECHARGE,
    ACTIVATE,
    PRECHARGE,
    REFRESH_BANK,
    REFRESH,
    SREF_ENTER,
    SREF_EXIT,
    MRS,
    /* Mode Register Set (MRS) Command requires Bank and Row Address to set MRx
     */
    SIZE
};

struct Command {
    Command() : cmd_type(CommandType::SIZE), hex_addr(0) {}
    Command(CommandType cmd_type, const Address& addr, uint64_t hex_addr)
        : cmd_type(cmd_type), addr(addr), hex_addr(hex_addr) {}
    // Command(const Command& cmd) {}

    bool IsValid() const { return cmd_type != CommandType::SIZE; }
    bool IsRefresh() const {
        return cmd_type == CommandType::REFRESH ||
               cmd_type == CommandType::REFRESH_BANK;
    }
    bool IsRead() const {
        return cmd_type == CommandType::READ ||
               cmd_type == CommandType::READ_PRECHARGE;
    }
    bool IsWrite() const {
        return cmd_type == CommandType::WRITE ||
               cmd_type == CommandType::WRITE_PRECHARGE;
    }
    bool IsReadWrite() const { return IsRead() || IsWrite(); }
    bool IsRankCMD() const {
        return cmd_type == CommandType::REFRESH    ||
               cmd_type == CommandType::SREF_ENTER ||
               cmd_type == CommandType::SREF_EXIT  ||
               cmd_type == CommandType::MRS;
    }
    bool IsMRSCMD() const {
        return cmd_type == CommandType::MRS;
    }    
    CommandType cmd_type;
    Address addr;
    uint64_t hex_addr;

    int Channel() const { return addr.channel; }
    int Rank() const { return addr.rank; }
    int Bankgroup() const { return addr.bankgroup; }
    int Bank() const { return addr.bank; }
    int Row() const { return addr.row; }
    int Column() const { return addr.column; }

    // void display() const {
    //     std::cout<<"CMD:cmd["<<cmd_type<<
    //                 "]CH["<<Channel()<<
    //                 "]RK["<<Rank()<<
    //                 "]BK["<<Bank()<<
    //                 "]ROW["<<Row()<<
    //                 "]COL["<<Column()<<std::endl;
    // }    

    friend std::ostream& operator<<(std::ostream& os, const Command& cmd);
};
/*
    @TODO: It requires a method to receive MRS commands via transaction.
*/
struct Transaction {
    Transaction() {}
    Transaction(uint64_t addr, bool is_write)
        : addr(addr),
          added_cycle(0),
          complete_cycle(0),
          is_write(is_write),
          is_MRS(false) {}
    Transaction(uint64_t addr, bool is_write, bool is_mrs)
        : addr(addr),
          added_cycle(0),
          complete_cycle(0),
          is_write(is_write),
          is_MRS(is_mrs) {
            assert(!(!is_write && is_mrs)); // MRS Command is Write-Type 
          }          
    Transaction(const Transaction& tran)
        : addr(tran.addr),
          added_cycle(tran.added_cycle),
          complete_cycle(tran.complete_cycle),
          is_write(tran.is_write),
          is_MRS(tran.is_MRS) {}
    uint64_t addr;
    uint64_t added_cycle;
    uint64_t complete_cycle;
    bool is_write;
    bool is_MRS; // Temporaily add an MRS Flag @TODO it must be removed later
    void display() {
        std::cout<<"Transaction:Addr["<<std::setw(10)<<std::hex<<"0x"<<addr<<
                   "]:added_cycle["<<std::setw(10)<<std::dec<<added_cycle<<
                   "]:is_write["<<std::setw(4)<<std::boolalpha<<is_write<<
                   "]:is_MRS["<<std::setw(4)<<std::boolalpha<<is_MRS<<
                   "]"<<std::endl;
    }
    friend std::ostream& operator<<(std::ostream& os, const Transaction& trans);
    friend std::istream& operator>>(std::istream& is, Transaction& trans);
};

}  // namespace dramsim3
#endif
