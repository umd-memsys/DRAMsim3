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

    void display() {
        std::cout<<"Address: CH["<<std::hex<<channel<<"]RK["<<std::hex<<rank<<
                    "]BG["<<std::hex<<bankgroup<<"]BK["<<std::hex<<bank<<
                    "]BOW["<<std::hex<<row<<"]COL["<<std::hex<<column<<"]"<<std::endl;                   
    }    
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
    Transaction() {is_valid=false;}
    Transaction(uint64_t addr, bool is_write)
        : addr(addr),
          added_cycle(0),
          complete_cycle(0),
          is_write(is_write),
          is_MRS(false) {is_valid=true;}
    Transaction(uint64_t addr, bool is_write, bool is_mrs)
        : addr(addr),
          added_cycle(0),
          complete_cycle(0),
          is_write(is_write),
          is_MRS(is_mrs) {
            assert(!(!is_write && is_mrs)); // MRS Command is Write-Type 
            is_valid=true;
          }          
    Transaction(uint64_t addr, bool is_write, bool is_mrs, std::vector<u_int64_t> &payload_)
        : addr(addr),
          added_cycle(0),
          complete_cycle(0),
          is_write(is_write),
          is_MRS(is_mrs) {
            assert(!(!is_write && is_mrs)); // MRS Command is Write-Type 
            // payload Value Copy 
            assert(payload.size() == 0); // If size of payload must be zero when generating object, assert
            for(uint32_t i=0;i<payload_.size();i++) payload.push_back(payload_[i]);
            is_valid=true;
          }                
    Transaction(const Transaction& tran)
        : addr(tran.addr),
          added_cycle(tran.added_cycle),
          complete_cycle(tran.complete_cycle),
          is_write(tran.is_write),
          is_MRS(tran.is_MRS) {
            // payload Value Copy 
            for(uint32_t i=0;i<tran.payload.size();i++) {
                if(payload.size() < tran.payload.size()) payload.push_back(tran.payload[i]);
                else payload[i] = tran.payload[i];
            }
            is_valid=true;
          }
    uint64_t addr;
    uint64_t added_cycle;
    uint64_t complete_cycle;
    std::vector<uint64_t> payload;
    bool is_write;
    bool is_MRS; // Temporaily add an MRS Flag @TODO it must be removed later
    bool is_valid;
    void updatePayload(std::vector<u_int64_t> &payload_) {
        for(uint32_t i=0;i<payload_.size();i++) {
            if(payload.size() < payload_.size()) payload.push_back(payload_[i]);
            else payload[i] = payload_[i];
        }        
    }
    void display() {
        std::cout<<"Transaction:Addr["<<std::setw(10)<<std::hex<<"0x"<<addr<<
                   "]:added_cycle["<<std::setw(10)<<std::dec<<added_cycle<<
                   "]:is_write["<<std::setw(4)<<std::boolalpha<<is_write<<
                   "]:is_MRS["<<std::setw(4)<<std::boolalpha<<is_MRS<<
                   "]"<<std::endl;
                    std::cout<<"-> D:";
                    for(auto value : payload) std::cout<<"["<<std::setw(16)<<std::hex<<value<<"]";
                    std::cout<<std::endl;                   
    }
    friend std::ostream& operator<<(std::ostream& os, const Transaction& trans);
    friend std::istream& operator>>(std::istream& is, Transaction& trans);
};


static constexpr uint64_t wr_dq_map_per_db[2][8][8] = {
    {{4,6,5,7,3,1,2,0},  // DB0 for Rank0, 2 ( 4, 6, 5, 7, 3, 1, 2, 0)
    {5,7,4,6,1,3,0,2},  // DB1 for Rank0, 2 (13,15,12,14, 9,11, 8,10)  
    {4,7,5,6,2,0,3,1},  // DB2 for Rank0, 2 (20,23,21,22,18,16,19,17)
    {4,6,5,7,2,0,3,1},  // DB3 for Rank0, 2 (28,30,29,31,26,24,27,25)
    {5,7,4,6,1,3,0,2},  // DB4 for Rank0, 2 (37,39,36,38,33,35,32,34)
    {5,7,4,6,3,1,2,0},  // DB5 for Rank0, 2 (45,47,44,46,43,41,42,40)
    {7,5,6,4,1,3,0,2},  // DB6 for Rank0, 2 (55,53,54,52,49,51,48,50)
    {7,5,6,4,0,3,1,2}}, // DB7 for Rank0, 2 (63,61,62,60,56,59,57,58)
    {{6,4,7,5,1,3,0,2},  // DB0 for Rank1, 3 ( 6, 4, 7, 5, 1, 3, 0, 2)
    {7,5,6,4,3,1,2,0},  // DB1 for Rank1, 3 (15,13,14,12,11, 9,10, 8) 
    {7,4,6,5,0,2,1,3},  // DB2 for Rank1, 3 (23,20,22,21,16,18,17,19)
    {6,4,7,5,0,2,1,3},  // DB3 for Rank1, 3 (30,28,31,29,24,26,25,27)
    {7,5,6,4,3,1,2,0},  // DB4 for Rank1, 3 (39,37,38,36,35,33,34,32)
    {7,5,6,4,1,3,0,2},  // DB5 for Rank1, 3 (47,45,46,44,41,43,40,42)
    {5,7,4,6,3,1,2,0},  // DB6 for Rank1, 3 (53,55,52,54,51,49,50,48)
    {5,7,4,6,3,0,2,1}}};// DB7 for Rank1, 3 (61,63,60,62,59,56,58,57)

static constexpr uint64_t rd_dq_map_per_db[2][8][8] = {
{{4,6,5,7,3,1,2,0},  // DB0 for Rank0, 2 
    {6,4,7,5,2,0,3,1},  // DB1 for Rank0, 2   
    {6,4,5,7,1,3,0,2},  // DB2 for Rank0, 2 
    {4,6,5,7,1,3,0,2},  // DB3 for Rank0, 2 
    {6,4,7,5,2,0,3,1},  // DB4 for Rank0, 2 
    {6,4,7,5,3,1,2,0},  // DB5 for Rank0, 2 
    {7,5,6,4,2,0,3,1},  // DB6 for Rank0, 2 
    {7,5,6,4,2,0,1,3}}, // DB7 for Rank0, 2 
    {{5,7,4,6,2,0,3,1},  // DB0 for Rank1, 3 
    {7,5,6,4,3,1,2,0},  // DB1 for Rank1, 3  
    {7,5,4,6,0,2,1,3},  // DB2 for Rank1, 3 
    {5,7,4,6,0,2,1,3},  // DB3 for Rank1, 3 
    {7,5,6,4,3,1,2,0},  // DB4 for Rank1, 3 
    {7,5,6,4,2,0,3,1},  // DB5 for Rank1, 3 
    {6,4,7,5,3,1,2,0},  // DB6 for Rank1, 3 
    {6,4,7,5,3,1,0,2}}};// DB7 for Rank1, 3          
         
}  // namespace dramsim3
#endif
