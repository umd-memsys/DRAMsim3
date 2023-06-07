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

namespace dramsim3 {

class CUSTOM_CPU { 
   public:
    typedef enum {ROCO, BKCO} addr_io_t; 
    struct Address_IO {
        Address_IO()
            : addr_io(ROCO), addr(Address()) {}
        Address_IO(addr_io_t addr_io, Address addr)
            : addr_io(addr_io),
              addr(addr) {
                // assert(addr_io == ROCO);
              }
        Address_IO(const Address_IO& addr_io)
            : addr(addr_io.addr) {}

        addr_io_t addr_io;
        Address addr;
        // int channel;
        // int rank;
        // int bankgroup;
        // int bank;
        // int row;
        // int column;    

        static uint64_t ch_mask, ra_mask, bg_mask, ba_mask, ro_mask, co_mask;        

        Address_IO& operator++() {
            if(addr_io == ROCO) {
                addr.column++;
                if(addr.column==static_cast<int>(co_mask)) {
                  addr.column = 0;
                  addr.row++;
                }
            }
            else if(addr_io == BKCO) {
                addr.column++;
                if(addr.column == static_cast<int>(co_mask)) {
                  addr.column = 0;
                  addr.bank++;
                  if(addr.bank == static_cast<int>(ba_mask)) {
                    addr.bank = 0;
                    addr.bankgroup++;
                    if(addr.bankgroup == static_cast<int>(bg_mask)) {
                      addr.bankgroup = 0;
                      addr.rank++;
                      if(addr.rank == static_cast<int>(ra_mask)) {
                        addr.rank = 0;
                        addr.row++;
                      }
                    }
                  }
                }
            }
            else {
              std::cerr<<"NOT SUPPORTED ADDRESS INCREMENTAL ORDER"<<std::endl;
              exit(1);
            }
            return *this;
        }
        Address_IO operator++(int) {
            Address_IO temp = *this;
            ++*this;
            return temp;
        }

        friend std::ostream& operator<<(std::ostream& os, const Address_IO& io) {
          os<<"CH["<<io.addr.channel<<"]RA["<<io.addr.rank<<"]BG["<<io.addr.bankgroup;
          os<<"]BK["<<io.addr.bank<<"]RO["<<io.addr.row<<"]COL["<<io.addr.column<<"]";
          return os;
        }

    };

    CUSTOM_CPU(const std::string& config_file, const std::string& output_dir,
               const std::string& gen_type)
        : memory_system_(
              config_file, output_dir,
              std::bind(&CUSTOM_CPU::ReadCallBack, this, std::placeholders::_1),
              std::bind(&CUSTOM_CPU::WriteCallBack, this, std::placeholders::_1)),
          clk_(0), gen_type_(gen_type), address_table(1) {
            trans_cnt   = 0;
            hex_addr    = 0;
            rd_pass_cnt = 0;
            rd_fail_cnt = 0;
            wr_cnt      = 0;
            rd_cnt      = 0;
            wr_resp_cnt = 0;
            rd_resp_cnt = 0;
            ndp_pass_cnt = 0;
            ndp_fail_cnt = 0;
            use_data = memory_system_.isLRDIMM();
            config_ = memory_system_.GetConfig();

            std::cout<<"================================"<<std::endl;
            std::cout<<"======= DRAMSim3 Start ========="<<std::endl;
            std::cout<<"================================"<<std::endl;  
            std::cout<<std::endl;            

            std::cout<<"Configure File  : "<<config_file<<std::endl;
            // std::cout<<"Output Path     :"<<output_dir<<std::endl;
            std::cout<<"Generation Mode : "<<gen_type_<<std::endl;
            std::cout<<"LRDIMM Mode     : "<<std::boolalpha<<use_data<<std::endl;
            std::cout<<"# of DIMMs      : "<<config_->dimms<<std::endl;
            std::cout<<"# of Ranks      : "<<config_->ranks<<
                      " ( "<<config_->ranks_per_dimm<<" ranks per DIMM)"<<std::endl;
            std::cout<<"Total Cap.      : "<<config_->channel_size/1024<<" GB"<<std::endl;                      

            // Address Mask
            Address_IO tmp;
            tmp.co_mask = config_->co_mask;
            tmp.ro_mask = config_->ro_mask;
            tmp.ba_mask = config_->ba_mask;
            tmp.bg_mask = config_->bg_mask;
            tmp.ra_mask = config_->ra_mask;
            tmp.ch_mask = config_->ch_mask;      
            
            // address_table[0].print_table();
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
    void genRefData(const std::string& kernal_type);    // Generate Refererece Data for Tatget kernel Type
    void genNDPInst(const std::string& kernal_type);    // Generate NDP Instruction for Tatget kernel Type
    void genNDPData(const std::string& kernal_type);    // Generate NDP Data for Tatget kernel Type
    void genNDPConfig(const std::string& kernal_type);  // Generate NDP Configuration Request
    void genNDPExec(const std::string& kernal_type);    // Generate NDP Execution Memory Request
    void genNDPReadResult(const std::string& kernal_type);    // Generate NDP Result Read Request
    void checkNDPResult(const std::string& kernal_type);
    bool simDone();
    bool NoTransInSim();
    //DQ mapping function
    std::vector<uint64_t> wr_DQMapping(std::vector<uint64_t> &payload, uint64_t rank_address);
    std::vector<uint64_t> rd_DQMapping(std::vector<uint64_t> &payload, uint64_t rank_address);

    std::vector<uint64_t> access_history;
    std::vector<Transaction> trans_vec;

    std::vector<uint64_t> convertFloatToUint64(std::vector<float> &f_payload);
    std::vector<float> convertUint64ToFloat(std::vector<uint64_t> &payload);

    uint32_t FloattoUint32(float float_value);
    float Uint32ToFloat(uint32_t uint_value);

    void NDPData_FloatVecToTrans(Address_IO addr_io,std::vector<float> f_vec);

   private:
    // To check error
    std::unordered_map<uint64_t,std::vector<uint64_t>> wr_req;         

    // Transation Vector for Write NDP Instruction and Data 
    std::vector<Transaction> ndp_config;
    std::vector<Transaction> ndp_insts;
    std::vector<Transaction> ndp_data;
    std::vector<Transaction> ndp_exec;
    std::vector<Transaction> ndp_read_result;

    // Reference Data Format (support BLAS-1 function) z=axpy 
    float scalar_alpha;
    std::vector<float> vector_x,vector_y,vector_z;

    std::vector<uint64_t> resp_data;

    // Simulation State
    bool run_state_ndp_config = false;
    bool run_state_ndp_preloading_data = false;
    bool run_state_ndp_preloading_inst = false;
    bool run_state_ndp_kernel_exec = false;
    bool run_state_ndp_read_result = false;
    bool sim_done = false;

    Config *config_;    

   protected:
    MemorySystem memory_system_;
    uint64_t clk_;
    std::mt19937_64 gen;    
    bool get_next_ = true;
    bool use_data;
    Transaction trans;
    std::string gen_type_; 
    All_Ch_Addrss_Table address_table;

    uint32_t trans_cnt;
    uint32_t hex_addr;

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
