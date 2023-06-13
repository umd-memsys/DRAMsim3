#include "custom_cpu.h"
#include <vector>
#include <cstdlib> 
#include <ctime>   


// #define _PRINT_TRANS
#define NUM_TRANSACTION 1024
namespace dramsim3 {

Address Address_IO::mask_addr;

void CUSTOM_CPU::initialize() {

    //Initializte Counters
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

    // Mask Addrses of Address IO must be set to some mask value
    Address_IO tmp;
    tmp.mask_addr = Address(config_->ch_mask, config_->ra_mask, config_->bg_mask,
                            config_->ba_mask, config_->ro_mask, config_->co_mask);

    // address_table[0].print_table();
}

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

            if(use_data && trans.is_write) StoreWRTrans(trans.addr,trans.payload);            
        }
    }
    clk_++;
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

        if(run_state_ndp_kernel_exec && !run_state_ndp_read_result)
            for(auto element : rd_data) resp_data.push_back(element);        
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

void CUSTOM_CPU::WriteCallBack(uint64_t addr) {
    wr_resp_cnt++;
    #ifdef _PRINT_TRANS
    std::cout<<"["<<std::setw(10)<<clk_<<"] == ";
    std::cout<<"MC -> Host [WR] ["<<std::hex<<std::setw(8)<<addr<<"]"<<std::endl;
    #endif
    return;    
}

Transaction CUSTOM_CPU::genTransaction() {
    if(gen_type_ == "kernel" || gen_type_ == "KERNEL" ) {
        if(!run_state_ndp_config) {
            if(!ndp_config.empty()) {
                auto tran = ndp_config[0];
                ndp_config.erase(ndp_config.begin());
                return tran;
            }
            else {
                if(NoTransInSim()) {
                    run_state_ndp_config = true;
                    std::cout<<"Run Simulation: NDP Config -> Preloading Data"<<std::endl;
                }
                return Transaction();
            }
        }
        else if(!run_state_ndp_preloading_data) {
            if(!ndp_data.empty()) {
                auto tran = ndp_data[0];
                ndp_data.erase(ndp_data.begin());
                return tran;
            }
            else {
                if(NoTransInSim()) {
                    run_state_ndp_preloading_data = true;
                    std::cout<<"Run Simulation: Preloading Data -> Preloading Inst."<<std::endl;                 
                }
                return Transaction();
            }
        }
        else if(!run_state_ndp_preloading_inst) {
            if(!ndp_insts.empty()) {
                auto tran = ndp_insts[0];
                ndp_insts.erase(ndp_insts.begin());
                return tran;
            }            
            else {
                if(NoTransInSim()) {                
                    run_state_ndp_preloading_inst = true;
                    std::cout<<"Run Simulation: Preloading Inst. -> Kernel Exec"<<std::endl;                                      
                }
                return Transaction();            
            }            
        }
        else if(!run_state_ndp_kernel_exec) {
            if(!ndp_exec.empty()) {
                auto tran = ndp_exec[0];
                ndp_exec.erase(ndp_exec.begin());
                return tran;
            }            
            else {
                if(NoTransInSim()) {                
                    run_state_ndp_kernel_exec = true;
                    std::cout<<"Run Simulation: Kernel Exec -> Read Result"<<std::endl;                  
                }
                return Transaction();            
            }       
        }
        else if(!run_state_ndp_read_result) { 
            if(!ndp_read_result.empty()) {
                auto tran = ndp_read_result[0];
                ndp_read_result.erase(ndp_read_result.begin());
                return tran;
            }            
            else {
                if(NoTransInSim()) {                
                    run_state_ndp_read_result = true;
                    std::cout<<"Run Simulation: Read Result Done"<<std::endl;
                }
                return Transaction();            
            }    
        }
        else {
            sim_done = true;
            return Transaction();
        }
    }
    else if(gen_type_ == "RANDOM" || gen_type_ == "random") {
        if((gen()%2 == 0)) {
            if((gen()%2 == 0)) {
                hex_addr = gen();
                if(use_data) {
                    std::vector<u_int64_t> payload;
                    payload.resize(8);
                    for(auto &a : payload) a = ((u_int64_t)(std::rand()) << 32) | (u_int64_t)(std::rand());
                    // StoreWRTrans(hex_addr,payload);
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
                // StoreWRTrans(addr,payload);
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

bool CUSTOM_CPU::NoTransInSim() {
    return (rd_resp_cnt == rd_cnt) && (wr_resp_cnt == wr_cnt);
}

void CUSTOM_CPU::StoreWRTrans(uint64_t hex_addr, std::vector<uint64_t> &payload) {
    //To check the consistency of the read response data, Store WR Data with Hex Address
    if(wr_req.find(hex_addr) != wr_req.end()) {
        assert(wr_req[hex_addr].size() == payload.size());            
        for(uint32_t i=0;i<wr_req[hex_addr].size();i++) 
            wr_req[hex_addr][i] = payload[i];                      
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

        if(error) {
            std::cerr<<"READ DATA FAIL (0x"<<std::hex<<hex_addr<<")"<<std::endl;                        
            for(uint32_t i=0;i<payload.size();i++) {
                std::cerr<<" Written Data: 0x"<<wr_req[hex_addr][i]<<" / Read Data: 0x"<<payload[i]<<std::endl;
            }
        }
        
        if(error) return false;
        else      return true;
    }
    else {
        // Read unwritten address
        std::cerr<<"(WARNING) "<<__func__<<" : ACCESS UNWRITTEN MEMORY (0x"<<std::hex<<hex_addr<<")"<<std::endl;                        
        return false;
    }
}

void CUSTOM_CPU::printResult() {
    std::cout<<std::endl;
    std::cout<<"================================"<<std::endl;
    std::cout<<"======= DRAMSim3 Result ========"<<std::endl;
    std::cout<<"================================"<<std::endl;
    std::cout<<std::endl;
    std::cout<<"# of Write (to MC)    : "<<std::dec<<wr_cnt<<std::endl;
    std::cout<<"# of Read (to MC)     : "<<std::dec<<rd_cnt<<std::endl;    
    std::cout<<"# of Write (from MC)  : "<<std::dec<<wr_resp_cnt<<std::endl;
    std::cout<<"# of Read (from MC)   : "<<std::dec<<rd_resp_cnt<<std::endl;
    std::cout<<"# of Pass Read (data) : "<<std::dec<<(rd_pass_cnt)<<std::endl;
    std::cout<<"# of Fail Read (data) : "<<std::dec<<(rd_fail_cnt)<<std::endl;
    std::cout<<"# of Pass NDP Element : "<<std::dec<<(ndp_pass_cnt)<<std::endl;
    std::cout<<"# of Fail NDP Element : "<<std::dec<<(ndp_fail_cnt)<<std::endl;    
}

std::vector<uint64_t> wr_DQMapping(std::vector<uint64_t> &payload, uint64_t rank_address){

    uint8_t DataRemapping;
    uint64_t ResultData;
    uint8_t Burst_num = 0;
    uint8_t OneByte = 0; //64bit -> 8bit    
    std::vector<uint64_t> MergedData;
    MergedData.resize(8);
    for(auto &a : payload){
        ResultData = 0;
        for(int DB_num = 0; DB_num < 8; DB_num++){
                DataRemapping = 0;
                OneByte = (a >> (8*DB_num)) & 0xFF; 

                    for(int i = 0; i <8; i++){
                         DataRemapping |= (((OneByte >> wr_dq_map_per_db[rank_address%2][DB_num][7-i]) & 0x01) << i); //DB_num
                    }
                ResultData |= static_cast<int64_t>(DataRemapping) << (8*DB_num);  
            }
    MergedData[Burst_num] = ResultData;
    Burst_num++;
    }
    return MergedData;
}

std::vector<uint64_t> rd_DQMapping(std::vector<uint64_t> &payload, uint64_t rank_address){
    uint8_t DataRemapping;
    uint8_t Burst_num = 0;
    uint8_t OneByte = 0; //64bit -> 8bit
    std::vector<uint64_t> MergedData;
    MergedData.resize(8);
    for(auto &a : payload){
        for(int DB_num = 0; DB_num < 8; DB_num++){
                DataRemapping = 0;
                OneByte = (a >> (8*DB_num)) & 0xFF;     //OneByte = 8bit
                    for(int i = 0; i <8; i++){
                         DataRemapping |= (((OneByte >> rd_dq_map_per_db[rank_address%2][DB_num][7-i]) & 0x01) << i);
                    }
                MergedData[DB_num] |= (static_cast<int64_t>(DataRemapping) << (8*Burst_num));
            }
    Burst_num++;
    }
    return MergedData;
}

void CUSTOM_CPU::genRefData(const std::string& kernal_type) {
    std::cout<<"Generating Reference Data "<<std::endl;
    uint32_t vector_length = 1024;//gen();
    std::uniform_real_distribution<float> real_dist(0.0f, 100.0f);
    if(kernal_type == "EWA") {        
        std::cout<<"  Kernel Type: Element-Wise Add"<<std::endl;
        std::cout<<"  Vector Length : "<<vector_length<<std::endl;
        scalar_alpha = real_dist(gen);
        vector_x.resize(vector_length);
        vector_y.resize(vector_length);
        vector_z.resize(vector_length);
        for(uint32_t i=0;i<vector_length;i++) {
            vector_x[i] = real_dist(gen);
            vector_y[i] = real_dist(gen);
            vector_z[i] = scalar_alpha*vector_x[i] + vector_y[i];
        }
    }
    else if(kernal_type == "EWM") {
        std::cout<<"  Kernel Type: Element-Wise Multiply"<<std::endl;
        std::cout<<"  Vector Length : "<<vector_length<<std::endl;
        scalar_alpha = real_dist(gen);
        vector_x.resize(vector_length);
        vector_y.resize(vector_length);
        vector_z.resize(vector_length);
        for(uint32_t i=0;i<vector_length;i++) {
            vector_x[i] = real_dist(gen);
            vector_y[i] = real_dist(gen);
            vector_z[i] = scalar_alpha*vector_x[i] * vector_y[i];
        }
    }
    else {
        std::cerr<<"NOT SUPPORTED KERNEL TYPE"<<std::endl;
        exit(1);
    }
}

void CUSTOM_CPU::genNDPInst(const std::string& kernal_type) {
    std::cout<<"Generating NDP Instruction and Memory Transactions "<<std::endl;
    // Generating NDP Instruction 
    // Not Yet Defiend NDP Instruction, So use random number 
    std::vector<uint32_t> ndp_inst_list;
    if(kernal_type == "EWA") {        
        for(uint32_t i=0;i<32;i++) 
            ndp_inst_list.push_back(gen());
            /*            
            TBD
            */
    }
    else if(kernal_type == "EWM") {
        for(uint32_t i=0;i<16;i++) 
            ndp_inst_list.push_back(gen());
            /*            
            TBD
            */            
    }
    else {
        std::cerr<<"NOT SUPPORTED KERNEL TYPE"<<std::endl;
        exit(1);
    }

    // Convert NDP Instruction to Memory Transactions
    // All NDP Instruction should be broasdcasted to all DB 
    if(ndp_inst_list.size()%2!=0) ndp_inst_list.push_back(0);
    uint64_t data_per_db;
    assert(ndp_inst_list.size()%2==0);
    assert(config_->dbs_per_dimm == 8); 

    Address_IO addr_io = Address_IO(ROCO,address_table[0]["ADDR_NDP_INST_MEM"]);
    for(uint32_t i=0;i<ndp_inst_list.size();i+=2) {        
        std::vector<uint64_t> new_payload;        
        data_per_db = (static_cast<int64_t>(ndp_inst_list[i+1]) << 32) | ndp_inst_list[i];
        for(int j=0;j<config_->dbs_per_dimm;j++) new_payload.push_back(data_per_db);        
        ndp_insts.push_back(Transaction(config_->MergedAddress(addr_io.addr),true,false,new_payload));
        addr_io++;
    }    

    // Dummpy Write 
    Address_IO dummpy_address = Address_IO(BKCO,Address(0,0,0,0,0x200,0));      
    for(uint32_t i=0;i<1024;i++) {        
        std::vector<uint64_t> new_payload;        
        for(int j=0;j<config_->dbs_per_dimm;j++) new_payload.push_back(0);        
        ndp_insts.push_back(Transaction(config_->MergedAddress(dummpy_address.addr),true,false,new_payload));
        dummpy_address++;
    }    

}

void CUSTOM_CPU::genNDPData(const std::string& kernal_type) {
    std::cout<<"Generating NDP Data and Memory Transactions "<<std::endl;
    if(kernal_type == "EWA" || kernal_type == "EWM") {      
        //Convert Vector to Memory Transcations 
        //Convert FP32 to FP16,Fixed,...
        
        assert(vector_x.size() == vector_y.size());    
        Address_IO vec_x_io = Address_IO(BKCO,Address(0,0,0,0,0x101,0));  
        Address_IO vec_y_io = Address_IO(BKCO,Address(0,1,0,0,0x102,0));


        NDPData_FloatVecToTrans(vec_x_io,vector_x);
        NDPData_FloatVecToTrans(vec_y_io,vector_y);

        // Only Test of ReadResult Phase 
        Address_IO vec_z_io = Address_IO(BKCO,address_table[0]["ADDR_NDP_DATA_MEM"]); 
        NDPData_FloatVecToTrans(vec_z_io,vector_z); 
        
    }
    else {
        std::cerr<<"NOT SUPPORTED KERNEL TYPE"<<std::endl;
        exit(1);
    }
}

void CUSTOM_CPU::genNDPConfig(const std::string& kernal_type) {
    std::cout<<"Generating NDP Configuration Request "<<std::endl;

    std::vector<uint64_t> new_payload;        
    for(int j=0;j<config_->dbs_per_dimm;j++) new_payload.push_back(0);        
    ndp_config.push_back(Transaction(config_->MergedAddress(address_table[0]["ADDR_REGION_START_NDP_DRAM"]),true,false,new_payload));
    ndp_config.push_back(Transaction(config_->MergedAddress(address_table[0]["ADDR_REGION_START_NDP"]),true,false,new_payload));
    ndp_config.push_back(Transaction(config_->MergedAddress(address_table[0]["ADDR_NDP_CONFIG"]),true,false,new_payload));
    ndp_config.push_back(Transaction(config_->MergedAddress(address_table[0]["ADDR_NDP_DB_CONFIG"]),true,false,new_payload));
    ndp_config.push_back(Transaction(config_->MergedAddress(address_table[0]["ADDR_NDP_RCD_CONFIG"]),true,false,new_payload));
    ndp_config.push_back(Transaction(config_->MergedAddress(address_table[0]["ADDR_NDP_INST_MEM"]),true,false,new_payload));
    ndp_config.push_back(Transaction(config_->MergedAddress(address_table[0]["ADDR_NDP_DATA_MEM"]),true,false,new_payload));

    if(kernal_type == "EWA" || kernal_type == "EWM") {      
        // Some Configuration depending on kernel type         
    }
    else {
        std::cerr<<"NOT SUPPORTED KERNEL TYPE"<<std::endl;
        exit(1);
    }
}

void CUSTOM_CPU::genNDPExec(const std::string& kernal_type) {
    std::cout<<"Generating NDP Execution Request "<<std::endl;

    if(kernal_type == "EWA" || kernal_type == "EWM") {      
        // Currently, Read Operand Data..
        Address_IO vec_x_io = Address_IO(BKCO,Address(0,0,0,0,0x101,0));  
        Address_IO vec_y_io = Address_IO(BKCO,Address(0,1,0,0,0x102,0));        

        for(uint32_t i=0;i<vector_x.size()/16;i++) {
            ndp_exec.push_back(Transaction(config_->MergedAddress(vec_x_io.addr),false,false));
            vec_x_io++;
        }
        for(uint32_t i=0;i<vector_y.size()/16;i++) {
            ndp_exec.push_back(Transaction(config_->MergedAddress(vec_y_io.addr),false,false));
            vec_y_io++;
        }        
        
    }
    else {
        std::cerr<<"NOT SUPPORTED KERNEL TYPE"<<std::endl;
        exit(1);
    }
}

void CUSTOM_CPU::genNDPReadResult(const std::string& kernal_type) {
    std::cout<<"Generating NDP Result Read Request "<<std::endl;

    if(kernal_type == "EWA" || kernal_type == "EWM") {      
        // Currently, Read Operand Data..
        Address_IO vec_z_io = Address_IO(BKCO,address_table[0]["ADDR_NDP_DATA_MEM"]);

        for(uint32_t i=0;i<vector_z.size()/16;i++) {
            ndp_read_result.push_back(Transaction(config_->MergedAddress(vec_z_io.addr),false,false));
            vec_z_io++;
        }   
                 
    }
    else {
        std::cerr<<"NOT SUPPORTED KERNEL TYPE"<<std::endl;
        exit(1);
    }
}

void CUSTOM_CPU::checkNDPResult(const std::string& kernal_type) {
    std::cout<<"Check NDP Result with Reference Data "<<std::endl;
    
    if(kernal_type == "EWA" || kernal_type == "EWM") {      
        std::vector<float> f_resp_data = convertUint64ToFloat(resp_data);
        for(uint64_t i=0;i<vector_z.size();i++)
            if(vector_z[i] == f_resp_data[i]) ndp_pass_cnt++;
            else                              ndp_fail_cnt++;
                 
    }
    else {
        std::cerr<<"NOT SUPPORTED KERNEL TYPE"<<std::endl;
        exit(1);
    }
}

bool CUSTOM_CPU::simDone() {
    return sim_done;
}

uint32_t CUSTOM_CPU::FloattoUint32(float float_value) {
    uint32_t converted_value;
    memcpy(&converted_value, &float_value, sizeof(converted_value));
    return converted_value;
    // return reinterpret_cast<uint&>(float_value);
}

float CUSTOM_CPU::Uint32ToFloat(uint32_t uint_value) {
    float converted_value;
    memcpy(&converted_value, &uint_value, sizeof(converted_value));    
    return converted_value;
    // return reinterpret_cast<float&>(uint_value);
}

std::vector<uint64_t> CUSTOM_CPU::convertFloatToUint64(std::vector<float> &f_payload) {
    std::vector<uint64_t> payload;
    for(uint32_t i=0;i<f_payload.size();i++) {
        uint64_t value_64b = static_cast<uint64_t>(FloattoUint32(f_payload[i]));
        if(i%2==0) payload.push_back(value_64b);
        else payload[i/2] = payload[i/2] | (value_64b << 32);
    }
    return payload;
}

std::vector<float> CUSTOM_CPU::convertUint64ToFloat(std::vector<uint64_t> &payload) {
    std::vector<float> f_payload;
    for(auto value : payload) {
        float f0 = Uint32ToFloat(static_cast<uint32_t>(value & 0xFFFFFFFF));
        float f1 = Uint32ToFloat(static_cast<uint32_t>((value >> 32 )& 0xFFFFFFFF));
        f_payload.push_back(f0);
        f_payload.push_back(f1);
    }
    return f_payload;
}

void CUSTOM_CPU::NDPData_FloatVecToTrans(Address_IO addr_io,std::vector<float> f_vec) {
    std::vector<uint64_t> payload_vec = convertFloatToUint64(f_vec);

    while(payload_vec.size()%8 != 0) {
        payload_vec.push_back(0);
    }
 
    assert(payload_vec.size()%8 == 0);
    
    for(uint32_t i=0;i<payload_vec.size()/8;i++) {
        std::vector<uint64_t> payload;
        for(uint32_t j=0;j<8;j++) payload.push_back(payload_vec[i*8+j]);
        ndp_data.push_back(Transaction(config_->MergedAddress(addr_io.addr),true,false,payload));
        addr_io++;
    }    
}

}  // namespace dramsim3
