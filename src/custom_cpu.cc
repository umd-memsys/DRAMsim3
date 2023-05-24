#include "custom_cpu.h"
#include <vector>
#include <cstdlib> 
#include <ctime>   

// #define _PRINT_TRANS
#define NUM_TRANSACTION 1024
namespace dramsim3 {

void CUSTOM_CPU::ClockTick() {
    memory_system_.ClockTick();

    if(get_next_) trans = genTransaction();
    if(trans.is_valid) {
        get_next_ = memory_system_.WillAcceptTransaction(trans.addr, trans.is_write, 
                                                         trans.is_MRS);
        if(get_next_) {
            if(trans.is_write) wr_cnt++;
            else               rd_cnt++;

            #ifdef _PRINT_TRANS
            if(trans.is_write) {
                std::cout<<"["<<std::setw(10)<<clk_<<"] == ";
                std::cout<<"Host -> MC [WR] ["<<std::hex<<std::setw(8)<<trans.addr<<"] Data ";
                for(auto value : trans.payload) std::cout<<"["<<std::hex<<std::setw(8)<<value<<"]";
                std::cout<<std::endl;
            }
            else {
                std::cout<<"["<<std::setw(10)<<clk_<<"] == ";
                std::cout<<"Host -> MC [RD] ["<<std::hex<<std::setw(8)<<trans.addr<<"]";
                std::cout<<std::endl;                     
            }
            #endif
            if(use_data)    memory_system_.AddTransaction(trans.addr, trans.is_write, 
                                                          trans.is_MRS, trans.payload);
            else             memory_system_.AddTransaction(trans.addr, trans.is_write, 
                                          trans.is_MRS);            
        }
    }
    clk_++;
    return;
}

Transaction CUSTOM_CPU::genTransaction() {

    if(gen_type_ == "RANDOM" || gen_type_ == "random") {
        if((gen()%2 == 0)) {
            if((gen()%2 == 0)) {
                hex_addr = gen();
                if(use_data) {
                    std::vector<u_int64_t> payload;
                    payload.resize(8);
                    for(auto &a : payload) a = ((u_int64_t)(std::rand()) << 32) | (u_int64_t)(std::rand());
                    StoreWRTrans(hex_addr,payload);
                    return Transaction(hex_addr,true,false,payload);            
                }
                else {
                    return Transaction(hex_addr,true,false);            
                }
            }
            else {
                if(access_history.size()!=0) {
                    hex_addr = access_history[gen()%access_history.size()];
                    return Transaction(hex_addr,false,false);
                }
                else return Transaction();
            }

        }
    }
    else {
        if(trans_cnt < NUM_TRANSACTION) {
            uint64_t addr = hex_addr;
            trans_cnt++;
            hex_addr++;
            if(hex_addr==NUM_TRANSACTION) hex_addr = 0;        
            if(use_data) {
                std::vector<u_int64_t> payload;
                payload.resize(8);
                for(auto &a : payload) a = addr;
                StoreWRTrans(addr,payload);
                return Transaction(addr,true,false,payload);
            }
            else {
                return Transaction(addr,true,false);
            }
        }
        else if(trans_cnt < (NUM_TRANSACTION*2)) {
            uint64_t addr = hex_addr;
            hex_addr++;
            trans_cnt++;
            return Transaction(addr,false,false);
        }
    }

    return Transaction();
}

void CUSTOM_CPU::genTransactionVector() {
 //TBD
}

void CUSTOM_CPU::WriteCallBack(uint64_t addr) {
    wr_resp_cnt++;
    #ifdef _PRINT_TRANS
    std::cout<<"["<<std::setw(10)<<clk_<<"] == ";
    std::cout<<"MC -> Host [RD] ["<<std::hex<<std::setw(8)<<addr<<"]"<<std::endl;
    #endif
    return;    
}

void CUSTOM_CPU::ReadCallBack(uint64_t addr) {
    rd_resp_cnt++;
    if(use_data) {
        std::vector<uint64_t> rd_data = memory_system_.GetRespData(addr);
        assert(rd_data.size()!=0);
        #ifdef _PRINT_TRANS    
            std::cout<<"["<<std::setw(10)<<clk_<<"] == ";
            std::cout<<"MC -> Host [RD] ["<<std::hex<<std::setw(8)<<addr<<"] Data ";
            for(auto value : rd_data) std::cout<<"["<<std::hex<<std::setw(8)<<value<<"]";
            std::cout<<std::endl;
        #endif        
        if(CheckRD(addr,rd_data)) rd_pass_cnt++;
        else                      rd_fail_cnt++;        
    }
    else {
        #ifdef _PRINT_TRANS
        std::cout<<"["<<std::setw(10)<<clk_<<"] == ";
        std::cout<<"MC -> Host [RD] ["<<std::hex<<std::setw(8)<<addr<<"]"<<std::endl;
        #endif
        return;            
    }
    return;
}

void CUSTOM_CPU::StoreWRTrans(uint64_t hex_addr, std::vector<uint64_t> &payload) {
    //To check the consistency of the read response data, Store WR Data with Hex Address
    if(wr_req.find(hex_addr) != wr_req.end()) {
        auto pre_payload = wr_req[hex_addr];
        assert(pre_payload.size() == payload.size());
        for(uint32_t i=0;i<pre_payload.size();i++) 
            pre_payload[i] = payload[i];            
    }
    else {
        access_history.push_back(hex_addr);
        std::vector<uint64_t> new_payload;
        for(uint32_t i=0;i<payload.size();i++) new_payload.push_back(payload[i]);
        wr_req.insert(std::make_pair(hex_addr,new_payload));
    }
}

bool CUSTOM_CPU::CheckRD(uint64_t hex_addr, std::vector<uint64_t> &payload) {
    if(wr_req.find(hex_addr) != wr_req.end()) {  
        assert(wr_req[hex_addr].size() == payload.size());
        bool error = false;
        for(uint32_t i=0;i<payload.size();i++) 
            if(wr_req[hex_addr][i] != payload[i]) error=true;

        if(error) std::cerr<<"READ DATA FAIL"<<std::endl;
        
        if(error) return false;
        else      return true;
    }
    else {
        // Read unwritten address
        std::cerr<<"(WARNING) ACCESS UNWRITTEN MEMORY"<<std::endl;                        
        return false;
    }
}


std::vector<uint64_t> CUSTOM_CPU::DataReshape(std::vector<uint64_t> &payload){

    std::unordered_map<int, int> DqMapping_U1 = {
        {0, 1}, {1, 3}, {2, 2}, {3, 0}, {4, 5}, {5, 7}, {6, 6}, {7, 4}
    };
    std::unordered_map<int, int> DqMapping_U2_U3 = {
        {0, 3}, {1, 1}, {2, 0}, {3, 2}, {4, 7}, {5, 5}, {6, 4}, {7, 6}
    };
    std::unordered_map<int, int> DqMapping_U4_U8 = {
        {0, 1}, {1, 3}, {2, 0}, {3, 2}, {4, 5}, {5, 7}, {6, 6}, {7, 4}
    };
    std::unordered_map<int, int> DqMapping_U7_U9 = {
        {0, 1}, {1, 3}, {2, 0}, {3, 2}, {4, 7}, {5, 5}, {6, 4}, {7, 6}
    };
    std::unordered_map<int, int> DqMapping_U10 = {
        {0, 3}, {1, 1}, {2, 0}, {3, 2}, {4, 7}, {5, 5}, {6, 6}, {7, 4}
    };


    std::vector<uint64_t> payload_Reshape;
    payload_Reshape.resize(8);

    uint8_t RemappingValues[8] = {0};
    int count = 0;
    int vector_index;
    uint64_t combinedValue = 0;



    for(auto& a : payload) {
        
        
  
         for(int x = 0; x < 8; x++) {
            if(count == 0){
                if(DqMapping_U1.find(x) != DqMapping_U1.end()) vector_index = DqMapping_U1.find(x) -> second;
            }
            else if(count == 1 || count == 2){
                if(DqMapping_U2_U3.find(x) != DqMapping_U2_U3.end()) vector_index = DqMapping_U2_U3.find(x) -> second;
            }
            else if(count == 3 || count == 5){
                if(DqMapping_U4_U8.find(x) != DqMapping_U4_U8.end()) vector_index = DqMapping_U4_U8.find(x) -> second;
            }
            else if(count == 4 || count == 6){
                if(DqMapping_U7_U9.find(x) != DqMapping_U7_U9.end()) vector_index = DqMapping_U7_U9.find(x) -> second;
            }
            else {
                if(DqMapping_U10.find(x) != DqMapping_U10.end()) vector_index = DqMapping_U10.find(x) -> second;
            }
               

                RemappingValues[vector_index] = (a >> (8*x)) & 0xFF;
                
            
         }
        

        for (int i = 7; i >= 0; i--) {
            
            combinedValue |= static_cast<int64_t>(RemappingValues[i]) << (8*i);
        }
        
        payload_Reshape[count] = combinedValue;
        count++;

    }
    
    return payload_Reshape;

}


void CUSTOM_CPU::printResult() {
    std::cout<<std::endl;
    std::cout<<"================================"<<std::endl;
    std::cout<<"======= DRAMSim3 Result ========"<<std::endl;
    std::cout<<"================================"<<std::endl;
    std::cout<<std::endl;
    std::cout<<"# of Write (to MC)    : "<<std::dec<<wr_cnt<<std::endl;
    std::cout<<"# of Read (to MC)     : "<<std::dec<<wr_cnt<<std::endl;    
    std::cout<<"# of Write (from MC)  : "<<std::dec<<wr_resp_cnt<<std::endl;
    std::cout<<"# of Read (from MC)   : "<<std::dec<<rd_resp_cnt<<std::endl;
    std::cout<<"# of Pass Read (data) : "<<std::dec<<(rd_pass_cnt)<<std::endl;
    std::cout<<"# of Fail Read (data) : "<<std::dec<<(rd_fail_cnt)<<std::endl;
}

}  // namespace dramsim3
