#include "bob.h"
#include <assert.h>

namespace dramsim3 {

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

}
LRDIMM::LRDIMM(uint32_t dimm_idx, const Config& config, 
               SimpleStats& simple_stats) 
    : dimm_idx_(dimm_idx),
      config_(config),
      simple_stats_(simple_stats),
      rcd_(dimm_idx,config_,simple_stats_),
      clk_(0) {       

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
    }
    else return;
}

void LRDIMM::updateLRDIMM() {
    ClockTick();
    auto bcom_cmd = rcd_.updateRCD();
    if(bcom_cmd.IsValid()) {
        #ifdef MY_DEBUG
        std::cout<<"["<<std::setw(10)<<std::dec<<clk_<<"] ";
        std::cout<<"== "<<__func__<<" == ";
        std::cout<<"RCD Issue BCOM Commmand to DB"<<std::endl;
        #endif            
    }
    for(int i=0;i<config_.dbs_per_dimm;i++) {
        dbs_[i].updateDB();
        if(bcom_cmd.IsValid()) dbs_[i].recBCOMcmd(bcom_cmd);
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
        std::cout<<"USE LRDIMM !!"<<std::endl;
        dimms_.reserve(config_.dimms);
        for(int i=0;i<config_.dimms;i++) {
            dimms_.push_back(LRDIMM(i,config_,simple_stats_));
        }
    }
}

void BufferOnBoard::recDDRcmd(const Command& cmd) {
    if(!cmd.IsValid()) {
        return;
    }

    /*
    Currently, DIMM is selected by Rank address 
    DIMM IDX = Rank Address / Ranks per Rank 
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

void BufferOnBoard::updateBoB() {
    ClockTick();
    for(int i=0;i<config_.dimms;i++) {
        dimms_[i].updateLRDIMM();
    }
}
} // namespace dramsim3