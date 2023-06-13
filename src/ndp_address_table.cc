#include "ndp_address_table.h"

namespace dramsim3 {
    Address_Table::Address_Table(uint64_t channel) : channel(channel) {
        /*
            Insert New Address and Configuration 
        */
        ADDR_REGION_START_NDP_DRAM = Address(channel,0,0,0,START_RO_BOTH_NDP_DRAM,0);
        ADDR_REGION_START_NDP      = Address(channel,0,0,0,START_RO_ONLY_NDP,0);
        ADDR_NDP_CONFIG            = Address(channel,0,0,0,START_RO_ONLY_NDP,0);
        ADDR_NDP_DB_CONFIG         = Address(channel,0,0,0,START_RO_ONLY_NDP,0);
        ADDR_NDP_RCD_CONFIG        = Address(channel,0,2,0,START_RO_ONLY_NDP,0);
        ADDR_NDP_INST_MEM          = Address(channel,1,0,0,START_RO_ONLY_NDP,0);
        ADDR_NDP_DATA_MEM          = Address(channel,2,0,0,START_RO_ONLY_NDP,0);

        address_map.insert({"ADDR_REGION_START_NDP_DRAM",ADDR_REGION_START_NDP_DRAM});
        address_map.insert({"ADDR_REGION_START_NDP",ADDR_REGION_START_NDP});        
        address_map.insert({"ADDR_NDP_CONFIG",ADDR_NDP_CONFIG});    
        address_map.insert({"ADDR_NDP_DB_CONFIG",ADDR_NDP_DB_CONFIG});
        address_map.insert({"ADDR_NDP_RCD_CONFIG",ADDR_NDP_RCD_CONFIG});
        address_map.insert({"ADDR_NDP_INST_MEM",ADDR_NDP_INST_MEM});
        address_map.insert({"ADDR_NDP_DATA_MEM",ADDR_NDP_DATA_MEM});
    }

    Address Address_Table::get_address(std::string address) {
        if(address_map.find(address)!=address_map.end()) {
            return address_map[address];
        }
        else {
            std::cerr<<"WRONG ADDRESS"<<std::endl;
            exit(1);
        }
    }

    Address Address_Table::operator[](std::string address) {
        return get_address(address);
    }

    void Address_Table::print_table() {
        for(std::pair<std::string,Address> element: address_map) {
            std::cout<<std::left<<std::setw(30)<<element.first<<" / ";
            element.second.display();
        }
    }

    All_Ch_Addrss_Table::All_Ch_Addrss_Table(uint64_t num_channel) : 
        num_channel(num_channel) {
        for(uint64_t ch_idx=0;ch_idx<num_channel;ch_idx++)
            address_tables.push_back(Address_Table(ch_idx));
    }

    Address_Table All_Ch_Addrss_Table::operator[](uint64_t ch_idx) {
        if(ch_idx < num_channel) {
            return address_tables[ch_idx];
        }
        else {
            std::cerr<<"Channel Index Over number of channel"<<std::endl;
            exit(1);
        }
    }
}  // namespace dramsim3

