#ifndef __CCPU_H
#define __CCPU_H

#include <fstream>
#include <functional>
#include <random>
#include <string>
#include "memory_system.h"
#include <unordered_map>
#include "common.h"

namespace dramsim3 {

class CUSTOM_CPU {
   public:
    CUSTOM_CPU(const std::string& config_file, const std::string& output_dir,
               const std::string& gen_type)
        : memory_system_(
              config_file, output_dir,
              std::bind(&CUSTOM_CPU::ReadCallBack, this, std::placeholders::_1),
              std::bind(&CUSTOM_CPU::WriteCallBack, this, std::placeholders::_1)),
          clk_(0), gen_type_(gen_type) {
            trans_cnt   = 0;
            hex_addr    = 0;
            rd_pass_cnt = 0;
            rd_fail_cnt = 0;
            wr_cnt      = 0;
            rd_cnt      = 0;
            wr_resp_cnt = 0;
            rd_resp_cnt = 0;
            use_data = memory_system_.isLRDIMM();

            std::cout<<"================================"<<std::endl;
            std::cout<<"======= DRAMSim3 Start ========="<<std::endl;
            std::cout<<"================================"<<std::endl;
            std::cout<<std::endl;            

            std::cout<<"Configure File  :"<<config_file<<std::endl;
            // std::cout<<"Output Path     :"<<output_dir<<std::endl;
            std::cout<<"Generation Mode :"<<gen_type_<<std::endl;
            std::cout<<"LRDIMM Mode     :"<<std::boolalpha<<use_data<<std::endl;
          }
    void ClockTick();
    void ReadCallBack(uint64_t addr);
    void WriteCallBack(uint64_t addr);
    void PrintStats() { memory_system_.PrintStats(); }
    Transaction genTransaction();
    void genTransactionVector();
    void StoreWRTrans(uint64_t hex_addr, std::vector<uint64_t> &payload);
    bool CheckRD(uint64_t hex_addr, std::vector<uint64_t> &payload);
    void printResult();

    std::vector<uint64_t> access_history;
    std::vector<Transaction> trans_vec;
   private:
    // To check error
    std::unordered_map<uint64_t,std::vector<uint64_t>> wr_req;

   protected:
    MemorySystem memory_system_;
    uint64_t clk_;
    std::mt19937_64 gen;
    bool get_next_ = true;
    bool use_data;
    Transaction trans;
    std::string gen_type_; 

    uint32_t trans_cnt;
    uint32_t hex_addr;

    uint64_t wr_cnt;
    uint64_t rd_cnt;
    uint64_t wr_resp_cnt;
    uint64_t rd_resp_cnt;
    uint64_t rd_pass_cnt;
    uint64_t rd_fail_cnt;
};


}  // namespace dramsim3
#endif
