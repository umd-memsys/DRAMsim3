#include "bob.h"
#include <assert.h>

namespace dramsim3 {

// constexpr uint64_t LRDIMM::wr_dq_map_per_db[2][8][8];
// constexpr uint64_t LRDIMM::rd_dq_map_per_db[2][8][8];

RCD::RCD(uint32_t dimm_idx, const Config& config, 
         SimpleStats& simple_stats) 
    : dimm_idx_(dimm_idx),
      config_(config),
      simple_stats_(simple_stats),
      clk_(0) {    

    /*
        (A[12:8] == 5'b00100) F0RC4x:  CW Source Selection Control (DA[7:0])
            DA[7:5] : Function Space 0 ~ 7
            DA[4:0] : A12 ~ A8
        (A[12:8] == 5'b00110) F0RC6x: CW Data Control Word (DA[7:0])
            DA[7:0] : A7 ~ A0
        (A[12:4] == 10'b100000110) F0RC06: Command Space Control Word          
            DA[3:0] == 0101 : CMD5 (CW Write operation)
            DA[3:0] == 0100 : CMD4 (CW Read operation)
    */
    f0rc4x = 0; // CW Source Selection Control 
    f0rc6x = 0; // CW Data Control Word
    f0rc06 = 0; // Command Space Control Word          

}

void RCD::recDDRcmd(const Command& cmd) {
    if(cmd.IsRead()) {
        bcom_cmds_.push_back(BCOMCmd(BCOMCmdType::BCOM_RD));
        bcom_cmds_timing_.push_back(4); // 4 tCK
        simple_stats_.Increment("num_bcom_rd");
        simple_stats_.IncrementVec("bcom_rd_cmd",dimm_idx_);
        
        #ifdef MY_DEBUG
        std::cout<<"["<<std::setw(10)<<std::dec<<clk_<<"] ";
        std::cout<<"== "<<__func__<<" == ";
        std::cout<<"RCD Issue BCOM_RD wait 4 tCK"<<std::endl;
        #endif          
    }
    else if(cmd.IsWrite()) {
        bcom_cmds_.push_back(BCOMCmd(BCOMCmdType::BCOM_WR));
        bcom_cmds_timing_.push_back(4); // 4 tCK
        simple_stats_.Increment("num_bcom_wr");
        simple_stats_.IncrementVec("bcom_wr_cmd",dimm_idx_);
        #ifdef MY_DEBUG
        std::cout<<"["<<std::setw(10)<<std::dec<<clk_<<"] ";
        std::cout<<"== "<<__func__<<" == ";
        std::cout<<"RCD Issue BCOM_WR wait 4 tCK"<<std::endl;
        #endif                  
    }
    else if(cmd.IsMRSCMD()) {
        // We Use Only 13 bits of Row Address 
        // @TODO: Require Veification 
        int da = cmd.Row() & 0x1FFF;
        if((da >> 8) ==  0x4) { // Setting F0RC4x
            f0rc4x = da & 0xFF;
        } else if((da >> 8) ==  0x6) { // Settting CW Data Control Word
            f0rc6x = da & 0xFF;
        } else if((da >> 4) ==  0x106) { // Settting CW Data Control Word
            int cmd_type = da & 0xF;
            assert(cmd_type == 0x4); // Current Only Support WR Write Operation
            int fs = (f0rc4x >> 5) & 0x7;
            int bcw_da = (f0rc4x & 0x1F) << 7 | f0rc6x;            
            bcom_cmds_.push_back(BCOMCmd(BCOMCmdType::BCOM_BCW_WR,bcw_da,fs));
            bcom_cmds_timing_.push_back(8); // 8 tCK
            simple_stats_.Increment("num_bcom_bcw_wr");
            simple_stats_.IncrementVec("bcom_bcw_wr_cmd",dimm_idx_);
        }        
    }
}

BCOMCmd RCD::updateRCD() {
    ClockTick();
    // BCOM Command Timing Check and Send BCOM to DB 
    bool is_issued = false;
    for(uint32_t i=0;i<bcom_cmds_timing_.size();i++) {
        bcom_cmds_timing_[i]--;
        assert(bcom_cmds_timing_[i]>=0);
        if(bcom_cmds_timing_[i]==0) {
            assert(i==0);
            if(is_issued) assert(false);
            is_issued = true;
        }
    }
    if(is_issued) {
        auto bcom_cmd = bcom_cmds_[0];
        bcom_cmds_.erase(bcom_cmds_.begin());
        bcom_cmds_timing_.erase(bcom_cmds_timing_.begin());
        return bcom_cmd;
    }
    else {
        return BCOMCmd();
    }
    
}

DB::DB(uint32_t dimm_idx, uint32_t idx, const Config& config, 
       SimpleStats& simple_stats) 
    : dimm_idx_(dimm_idx),
      db_idx_(idx),
      config_(config),
      simple_stats_(simple_stats),
      clk_(0) {}

void DB::updateDB() {
    ClockTick();
}

void DB::recBCOMcmd(const BCOMCmd& cmd) {
    //TBD
}

void DB::recWRData(std::vector<uint16_t> &payload) {
    //TBD
}

void DB::recRDData(std::vector<uint16_t> &payload) {
    //TBD
}

LRDIMM::LRDIMM(uint32_t dimm_idx, const Config& config, 
               SimpleStats& simple_stats) 
    : dimm_idx_(dimm_idx),
      config_(config),
      simple_stats_(simple_stats),
      rcd_(dimm_idx,config_,simple_stats_),
      clk_(0) {       
    
      // Access Granularity = bus_width * BL
      // Payload Size = Access Granularity / 64 bit    
      payload_size = config_.bus_width * config_.BL / sizeof(uint64_t) / 8;
      for(int i=0;i<config_.dbs_per_dimm;i++) {
          dbs_.push_back(DB(dimm_idx,i,config_,simple_stats_));
      }      
}

void LRDIMM::recDDRcmd(const Command& cmd) {
    if(!cmd.IsValid()) {
        assert(false);
    }    
    if(cmd.IsReadWrite() || cmd.IsMRSCMD()) {
        rcd_.recDDRcmd(cmd);
        if(cmd.IsRead()) rd_addr_pipe.push_back(cmd.hex_addr);
    }
    else return;
}

void LRDIMM::enqueWRData(uint64_t hex_addr, std::vector<uint64_t> &payload) {
    payload_q.push_back(make_pair(hex_addr,payload));
}

void LRDIMM::wrDataMem(uint64_t hex_addr, std::vector<uint64_t> &payload) {  
    // Currently, data payload size is fixed to 8 (hardcode)
    auto addr = config_.AddressMapping(hex_addr);
    if(data_memory.find(hex_addr) != data_memory.end()) {
        assert(data_memory[hex_addr].size() == payload.size());
        for(uint32_t i=0;i<data_memory[hex_addr].size();i++) 
            data_memory[hex_addr][i] = data_reshape_wr(addr.rank, i, payload[i]);
    }
    else {
        std::vector<uint64_t> new_payload;
        for(uint32_t i=0;i<payload.size();i++) new_payload.push_back(data_reshape_wr(addr.rank, i, payload[i]));
        data_memory.insert(std::make_pair(hex_addr,new_payload));
    }
}


std::vector<uint64_t> LRDIMM::rdDataMem(u_int64_t hex_addr) {
    if(data_memory.find(hex_addr) != data_memory.end()) { 
        auto addr = config_.AddressMapping(hex_addr);
        std::vector<uint64_t> new_payload;
        auto written_payload = data_memory[hex_addr];
        for(uint32_t i=0;i<written_payload.size();i++) new_payload.push_back(data_reshape_rd(addr.rank, i, written_payload[i]));
        return new_payload;
    }
    else {
        // Read unwritten address
        std::cerr<<"(WARNING) ACCESS UNWRITTEN MEMORY"<<std::endl;
        std::vector<uint64_t> unwritten_memory;
        for(uint32_t i=0;i<payload_size;i++) unwritten_memory.push_back(0xFFFFFFFFFFFFFFFF);        
        return unwritten_memory;
    }
}

uint64_t LRDIMM::data_reshape_wr(int rank_idx, uint64_t db_idx, uint64_t wr_data) {
    // Currently, data payload size is fixed to 8 (hardcode)
    assert(db_idx<8);
    uint64_t reshaped_data = 0;
    uint64_t one_byte      = 0;
    for(uint32_t burst_idx=0;burst_idx<(uint32_t)config_.BL;burst_idx++) {
        one_byte = (wr_data >> (8*burst_idx)) & 0xFF; 
        for(uint32_t bit_idx=0;bit_idx<8;bit_idx++) 
            reshaped_data|= (((one_byte >> wr_dq_map_per_db[rank_idx%2][db_idx][7-bit_idx]) & 
                            0x1)) << (8*burst_idx+bit_idx);
    }    
    return reshaped_data;
}

uint64_t LRDIMM::data_reshape_rd(int rank_idx, uint64_t db_idx, uint64_t wr_data) {
    // Currently, data payload size is fixed to 8 (hardcode)
    assert(db_idx<8);
    uint64_t reshaped_data = 0;
    uint64_t one_byte      = 0;    
    for(uint32_t burst_idx=0;burst_idx<(uint32_t)config_.BL;burst_idx++) {
        one_byte = (wr_data >> (8*burst_idx)) & 0xFF; 
        for(uint32_t bit_idx=0;bit_idx<8;bit_idx++) 
            reshaped_data|= (((one_byte >> rd_dq_map_per_db[rank_idx%2][db_idx][7-bit_idx]) & 
                            0x1)) << (8*burst_idx+bit_idx);
    }
    return reshaped_data;
}

bool LRDIMM::isRDResp() { 
    if(rd_resp_timing_.size()!=0) 
        if(rd_resp_timing_[0]==0) return true;

    return false;
}

rd_resp_t LRDIMM::getRDResp() { 
    if(isRDResp()) {
        assert(rd_cmd_pipe.size() > 0);
        auto it = rd_cmd_pipe.front();
        rd_cmd_pipe.erase(rd_cmd_pipe.begin());
        rd_resp_timing_.erase(rd_resp_timing_.begin());
        return it;
    }

    return make_pair(Command(),null_payload);
}

void LRDIMM::updateLRDIMM() {
    ClockTick();

    //update RD Resp Timing
    for(u_int32_t i=0;i<rd_resp_timing_.size();i++)
        rd_resp_timing_[i]--;

    //update WR Data Timing and Flow Data Path from Host to DB/DRAM
    for(u_int32_t i=0;i<wr_data_timing_.size();i++)
        wr_data_timing_[i]--;

    if(!wr_data_timing_.empty()) 
        if(wr_data_timing_[0]==0) {                        
            //pop the oldest data from the payload queue, split it, and insert it into each DB.
            auto it = payload_q.front();  
            //Write Data Memory                        
            wrDataMem(it.first,it.second);

            //Insert partial data into each DB
            std::vector<uint16_t> payload_4b;
            for(u_int32_t i=0;i<it.second.size();i++) {
                for(u_int32_t j=0;j<4;j++)
                    payload_4b.push_back(static_cast<uint16_t>(0xFFFF & (it.second[i]>>j*16)));
            }
            uint32_t each_db_payload_size = sizeof(uint64_t) * payload_size/config_.dbs_per_dimm/sizeof(uint16_t);
            uint32_t offset = 0;
            for(int i=0;i<config_.dbs_per_dimm;i++) {
                std::vector<uint16_t> a;
                for(u_int32_t j=0;j<each_db_payload_size;j++) a.push_back(payload_4b[(offset+j)]);    
                dbs_[i].recWRData(a);            
                offset+=each_db_payload_size;
            }
            // Delete the payload at the payload queue and also delete its timing value at the write timing queue
            payload_q.erase(payload_q.begin());  
            wr_data_timing_.erase(wr_data_timing_.begin());            
        }

    //update WR Data Timing and Flow Data Path from Host to DB/DRAM
    for(u_int32_t i=0;i<rd_data_timing_.size();i++)
        rd_data_timing_[i]--;

    if(!rd_data_timing_.empty()) 
        if(rd_data_timing_[0]==0) {            
            auto it = rd_addr_pipe.front();
            auto rd_data = rdDataMem(it);
            //Insert partial data into each DB
            std::vector<uint16_t> payload_4b;
            for(u_int32_t i=0;i<rd_data.size();i++) {
                for(u_int32_t j=0;j<4;j++)
                    payload_4b.push_back(static_cast<uint16_t>(0xFFFF & (rd_data[i]>>j*16)));
            }
            uint32_t each_db_payload_size = payload_size/config_.dbs_per_dimm/sizeof(uint16_t);
            uint32_t offset = 0;
            for(int i=0;i<config_.dbs_per_dimm;i++) {
                std::vector<uint16_t> a;
                for(u_int32_t j=0;j<each_db_payload_size;j++) a.push_back(payload_4b[(offset+j)]);                
                dbs_[i].recRDData(a);            
                offset+=each_db_payload_size;
            }
            // Data from DB to MC
            rd_resp_timing_.push_back(config_.tPDM_RD);                  
            auto addr = config_.AddressMapping(it);
            rd_cmd_pipe.push_back(std::make_pair(Command(CommandType::READ, addr, it),rd_data));
            // Delete the address at the address queue and also delete its timing value at the write timing queue
            rd_addr_pipe.erase(rd_addr_pipe.begin());
            rd_data_timing_.erase(rd_data_timing_.begin());
        }
    auto bcom_cmd = rcd_.updateRCD();
    if(bcom_cmd.IsValid()) {
        if(bcom_cmd.IsWrite()) {                  
            // DB receives Data after wr latency
            int wr_latency = config_.WL + config_.AL + 
                                config_.burst_cycle - config_.tPDM_WR - 5;
            wr_data_timing_.push_back(wr_latency);
        }
        else if(bcom_cmd.IsRead()) {                    
            int rd_latency = config_.RL + config_.AL + 
                                static_cast<int>(ceil((double)config_.tRPRE/2)) +
                                config_.burst_cycle - 5;
            rd_data_timing_.push_back(rd_latency);
        }
    }
    for(int i=0;i<config_.dbs_per_dimm;i++) {
        dbs_[i].updateDB();
        if(bcom_cmd.IsValid()) {
            dbs_[i].recBCOMcmd(bcom_cmd);
        } 
    }
}

BufferOnBoard::BufferOnBoard(const Config& config, 
                              SimpleStats& simple_stats) 
    : config_(config),
      simple_stats_(simple_stats),
      clk_(0) {
    #ifdef MY_DEBUG
    std::cout<<"== "<<__func__<<" == ";
    std::cout<<"constructor"<<std::endl;
    #endif           

    if(config_.is_LRDIMM) {
        dimms_.reserve(config_.dimms);
        for(int i=0;i<config_.dimms;i++) {
            dimms_.push_back(LRDIMM(i,config_,simple_stats_));
        }
    }
}

rd_resp_t BufferOnBoard::getRDResp() { 

    for(int i=0;i<config_.dimms;i++) {
        if(dimms_[i].isRDResp()) {
            #ifdef MY_DEBUG
            std::cout<<"["<<std::setw(10)<<std::dec<<clk_<<"] ";
            std::cout<<"== "<<__func__<<" == ";
            std::cout<<" RD Data Resp Done "<<i<<
                       "-th DIMM "<<std::endl;               
                       //<<std::endl;
            #endif                  
            return dimms_[i].getRDResp();
        }
    }

    return dimms_[0].getRDResp(); // return null cmd & data
}

void BufferOnBoard::recDDRcmd(const Command& cmd) {
    if(!cmd.IsValid()) {
        return;
    }

    /*
    Currently, DIMM is selected by Rank address 
    DIMM IDX = Rank Address / Ranks per DIMM 
    */
    int dimm_idx = cmd.Rank() / config_.ranks_per_dimm;
    assert(dimm_idx>=0 && dimm_idx <config_.dimms);   
    #ifdef MY_DEBUG
    std::cout<<"["<<std::setw(10)<<std::dec<<clk_<<"] ";
    std::cout<<"== "<<__func__<<" == ";
    // std::cout<<command_string[static_cast<int>(cmd.cmd_type)];
    std::cout<<" Rec DDR Command (R:"<<cmd.Rank()<<
               ") and Send "<<dimm_idx<<
               "-th DIMM [";
    std::cout<<cmd<<"]"<<std::endl;               
               //<<std::endl;
    #endif         
    dimms_[dimm_idx].recDDRcmd(cmd);
}

void BufferOnBoard::enqueWRData(int rank, uint64_t hex_addr, std::vector<uint64_t> &payload) {
    dimms_[rank / config_.ranks_per_dimm].enqueWRData(hex_addr, payload);
}

void BufferOnBoard::updateBoB() {
    ClockTick();
    for(int i=0;i<config_.dimms;i++) {
        dimms_[i].updateLRDIMM();
    }
}
} // namespace dramsim3