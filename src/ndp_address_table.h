#ifndef __NDP_ADDRESS_TABLE_H
#define __NDP_ADDRESS_TABLE_H

#include <iostream>
#include <assert.h>
#include "common.h"
#include <unordered_map>
#include <vector>

#define START_RK_ONLY_NDP 0
#define START_BG_ONLY_NDP 0
#define START_BK_ONLY_NDP 0
#define START_RO_ONLY_NDP 0x3FFFF
#define NDP_CONFIG_COL 0
#define START_RO_BOTH_NDP_DRAM 0x20000

namespace dramsim3 {
class Address_Table {
    public:
        Address_Table(uint64_t channel);  
        void print_table();

        Address get_address(std::string address);
        Address operator[](std::string address);
    
    private:
        // Each DIMM has unique channel index
        uint64_t channel;
        std::unordered_map<std::string,Address> address_map;         

        // Address Space
        Address ADDR_REGION_START_NDP_DRAM;
        Address ADDR_REGION_START_NDP;
        Address ADDR_NDP_CONFIG;
        Address ADDR_NDP_DB_CONFIG;
        Address ADDR_NDP_RCD_CONFIG;
        Address ADDR_NDP_INST_MEM;
        Address ADDR_NDP_DATA_MEM;            
};

class All_Ch_Addrss_Table {
    public:
        All_Ch_Addrss_Table(uint64_t num_channel);  
        // Address get_address(std::string address);
    
        Address_Table operator[](uint64_t ch_idx);
    private:
        uint64_t num_channel;
        std::vector<Address_Table> address_tables;
};

}  // namespace dramsim3
#endif
