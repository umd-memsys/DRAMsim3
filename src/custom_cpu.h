#ifndef __CCPU_H
#define __CCPU_H

#include <fstream>
#include <functional>
#include <random>
#include <string>
#include "memory_system.h"
#include <unordered_map>
#include "common.h"
#include "configuration.h"
#include <iostream>
#include "ndp_address_table.h"
#include "half.hpp"

typedef half_float::half fp16;

namespace dramsim3 {

class CUSTOM_CPU { 
   public:
    CUSTOM_CPU(const std::string& config_file, const std::string& output_dir,
               const std::string& gen_type)
        : memory_system_(
              config_file, output_dir,
              std::bind(&CUSTOM_CPU::ReadCallBack, this, std::placeholders::_1),
              std::bind(&CUSTOM_CPU::WriteCallBack, this, std::placeholders::_1)),
          clk_(0), gen_type_(gen_type), address_table(memory_system_.GetNumChannel()) {

          use_data = memory_system_.isLRDIMM();
          config_ = memory_system_.GetConfig();

          std::cout<<"================================"<<std::endl;
          std::cout<<"======= DRAMSim3 Start ========="<<std::endl;
          std::cout<<"================================"<<std::endl;  
          std::cout<<std::endl;            

          std::cout<<"Configure File  : "<<config_file<<std::endl;
          std::cout<<"Generation Mode : "<<gen_type_<<std::endl;
          std::cout<<"LRDIMM Mode     : "<<std::boolalpha<<use_data<<std::endl;
          std::cout<<"# of DIMMs      : "<<config_->dimms<<std::endl;
          std::cout<<"# of Ranks      : "<<config_->ranks<<
                      " ( "<<config_->ranks_per_dimm<<" ranks per DIMM)"<<std::endl;
          std::cout<<"Total Cap.      : "<<config_->channel_size/1024<<" GB"<<std::endl;                      

          address_table[0].print_table();
          }

    void initialize();
    void ClockTick();
    void ReadCallBack(uint64_t addr);
    void WriteCallBack(uint64_t addr);
    void PrintStats() { memory_system_.PrintStats(); }
    Transaction genTransaction(); // Generation Memory Trasaction (Stream, Random, Kernel, etc. )

    bool NoTransInSim();                                      // Check All RD/WR Response is Done  
    void StoreWRTrans(uint64_t hex_addr, std::vector<uint64_t> &payload); // Store Write Transaction to Compare Read Data
    bool CheckRD(uint64_t hex_addr, std::vector<uint64_t> &payload);      // Check RD Response Data
    void printResult();

    //DQ mapping function
    std::vector<uint64_t> wr_DQMapping(std::vector<uint64_t> &payload, uint64_t rank_address);
    std::vector<uint64_t> rd_DQMapping(std::vector<uint64_t> &payload, uint64_t rank_address);

    // NDP-related Function
    void genRefData(const std::string& kernal_type);          // Generate Refererece Data for Tatget kernel Type
    void genNDPInst(const std::string& kernal_type);          // Generate NDP Instruction for Tatget kernel Type
    void genNDPData(const std::string& kernal_type);          // Generate NDP Data for Tatget kernel Type
    void genNDPConfig(const std::string& kernal_type);        // Generate NDP Configuration Request
    void genNDPExec(const std::string& kernal_type);          // Generate NDP Execution Memory Request
    void genNDPReadResult(const std::string& kernal_type);    // Generate NDP Result Read Request
    void checkNDPResult(const std::string& kernal_type);      // Check NDP Result with Reference Data
    bool simDone();                                           // Check Simulation for NDP is Done

    // convert Data format (FP32 -> FP16 -> UINT64)
    uint32_t FloattoUint32(float float_value);
    float Uint32ToFloat(uint32_t uint_value);
    fp16 FloattoFP16(float float_value);
    float FP16toFloat(fp16 fp16_value);
    std::vector<uint64_t> convertFloatToUint64(std::vector<float> &f_payload);
    std::vector<float> convertUint64ToFloat(std::vector<uint64_t> &payload);

    // Data Convert (only Using NDP Data Phase)
    void NDPData_FloatVecToTrans(Address_IO addr_io,std::vector<float> f_vec);
    
   private:
    //Store Access History
    std::vector<uint64_t> access_history;

    // To check Read Response Data, Store WR Data
    std::unordered_map<uint64_t,std::vector<uint64_t>> wr_req;         

    // Transation Vector for Write NDP Instruction and Data 
    std::vector<Transaction> ndp_config;
    std::vector<Transaction> ndp_insts;
    std::vector<Transaction> ndp_data;
    std::vector<Transaction> ndp_exec;
    std::vector<Transaction> ndp_read_result;

    // Reference Data Format (support BLAS-1 function) z=axpy 
    float scalar_alpha;
    fp16 fp16_scalar_alpha;
    std::vector<float> vector_x,vector_y, vector_z;
    std::vector<fp16> fp16_vector_x, fp16_vector_y, fp16_vector_z;

    // NDP RD Result 
    std::vector<uint64_t> resp_data;

    // Simulation State
    bool run_state_ndp_config = false;
    bool run_state_ndp_preloading_data = false;
    bool run_state_ndp_preloading_inst = false;
    bool run_state_ndp_kernel_exec = false;
    bool run_state_ndp_read_result = false;
    bool sim_done = false;

    // DRAMSim3 Configuration Object
    Config *config_;    

   protected:
    MemorySystem memory_system_;
    uint64_t clk_;
    std::string gen_type_; 
    All_Ch_Addrss_Table address_table;

    
    std::mt19937_64 gen;    
    bool get_next_ = true;
    bool use_data;
    Transaction trans;

    // Use Memory Transaction Generation (Stream and Random)
    uint32_t trans_cnt;
    uint32_t hex_addr;

    // Counters for Memory Transactions 
    uint64_t wr_cnt;
    uint64_t rd_cnt;
    uint64_t wr_resp_cnt;
    uint64_t rd_resp_cnt;
    uint64_t rd_pass_cnt;
    uint64_t rd_fail_cnt;
    uint64_t ndp_pass_cnt;
    uint64_t ndp_fail_cnt;
    
 
};


}  // namespace dramsim3
#endif
