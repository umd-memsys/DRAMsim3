#ifndef __BUFFER_ON_BOARD_H
#define __BUFFER_ON_BOARD_H

#include "common.h"
#include "configuration.h"
#include "simple_stats.h"

namespace dramsim3 {

typedef std::pair<Command,std::vector<uint64_t>> rd_resp_t;
enum class BCOMCmdType {
    BCOM_RD,                                            
    BCOM_WR,
    BCOM_BCW_WR,
    BCOM_BCW_RD, // Maybe.. not use
    SIZE
};

struct BCOMCmd {
    BCOMCmd() : cmd_type(BCOMCmdType::SIZE), da(0), function_space(0) {}
    BCOMCmd(BCOMCmdType cmd_type) : cmd_type(cmd_type), da(0), function_space(0) {}
    BCOMCmd(BCOMCmdType cmd_type, int da, int function_space) : cmd_type(cmd_type), da(da), function_space(function_space) {}

    bool IsValid() const { return cmd_type != BCOMCmdType::SIZE; }
    bool IsRead() const { return cmd_type == BCOMCmdType::BCOM_RD; }
    bool IsWrite() const { return cmd_type == BCOMCmdType::BCOM_WR; }
    bool IsBCWWrite() const { return cmd_type == BCOMCmdType::BCOM_WR; }
    BCOMCmdType cmd_type;
    int da; 
    int function_space; 
};
class RCD {
    public:
        RCD(uint32_t dimm_idx, const Config& config, SimpleStats& simple_stats);  
        BCOMCmd updateRCD();                // Update RCD State and Send BCOM command to DB
        void recDDRcmd(const Command& cmd); // Received DDR Command from MC
        void ClockTick() { clk_ += 1; };    // Update Clock Tick 

    private:
        uint32_t dimm_idx_;
        const Config& config_;
        SimpleStats& simple_stats_;      
        uint64_t clk_;

        // RCD Register for BCW Command for future NDP function
        uint16_t f0rc4x;
        uint16_t f0rc6x;
        uint16_t f0rc06;        

        //  BCOM Command Timing Tacking 
        std::vector<BCOMCmd> bcom_cmds_;
        std::vector<int> bcom_cmds_timing_;
};

class DB {
    public:
        DB(uint32_t dimm_idx, uint32_t idx, const Config& config, SimpleStats& simple_stats);
        void recBCOMcmd(const BCOMCmd& cmd);               // recieved  BCOM Command From RCD
        void updateDB();                                   // Update LRDIMM State 
        void ClockTick() { clk_ += 1; };                   // Update Clock Tick
        void recWRData(std::vector<uint16_t> &payload);    // Write Data into DB
        void recRDData(std::vector<uint16_t> &payload);    // Write Data into DB

    private:
        uint32_t dimm_idx_;
        uint32_t db_idx_;        
        const Config& config_;
        SimpleStats& simple_stats_; 
        uint64_t clk_;     
        
};   
    
class LRDIMM {  
    public:
        LRDIMM(uint32_t dimm_idx, const Config& config, SimpleStats& simple_stats);
        void recDDRcmd(const Command& cmd);                                                  // Send DDR Command to RCD
        void updateLRDIMM();                                                                 // Update LRDIMM State 
        void ClockTick() { clk_ += 1; };                                                     // Update Clock Tick
        void enqueWRData(u_int64_t hex_addr, std::vector<uint64_t> &payload);                // Write Data into payload Queue
        void wrDataMem(u_int64_t hex_addr, std::vector<uint64_t> &payload);                  // Write Data into data memory
        rd_resp_t getRDResp();                                                               // Host Get Read Resp with Data        
        bool isRDResp();                                                                     // is Resp Data              
        std::vector<uint64_t> rdDataMem(u_int64_t hex_addr);                                 // Read Data into data memory

    private:
        uint32_t dimm_idx_;

        const Config& config_;
        SimpleStats& simple_stats_;   
        RCD rcd_; // LDRIMM Submodule

        uint64_t clk_;     
        uint32_t payload_size;

        std::vector<DB> dbs_; // LDRIMM Submodule
        std::vector<std::pair<uint64_t, std::vector<uint64_t>>> payload_q;
        std::unordered_map<uint64_t,std::vector<uint64_t>> data_memory;
        // @TODO: If memory accesses to data_memory cause high latency, you should replace the data structure of data_memory with another data structure.

        //  BCOM Command Timing Tacking 
        std::vector<int> wr_data_timing_; 
        std::vector<int> rd_data_timing_;        
        std::vector<int> rd_resp_timing_;        
        std::vector<u_int64_t> rd_addr_pipe;       
        std::vector<rd_resp_t> rd_cmd_pipe;
        std::vector<uint64_t> null_payload;
};


class BufferOnBoard {
    public:
    BufferOnBoard(const Config& config, SimpleStats& simple_stats);
    void recDDRcmd(const Command& cmd);                                               // Send DDR Command to target DIMM  
    void updateBoB();                                                                 // Update BoB State 
    void ClockTick() { clk_ += 1; };                                                  // Update Clock Tick
    void enqueWRData(int rank, uint64_t hex_addr, std::vector<uint64_t> &payload);    // Write Data into payload Queue
    rd_resp_t getRDResp();         // Host Get Read Resp with Data

    private:
        const Config& config_;
        SimpleStats& simple_stats_;
        uint64_t clk_;
        
        // LRDIMM Vector 
        std::vector<LRDIMM> dimms_;

};    
}  // namespace dramsim3
#endif
