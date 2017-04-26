#ifndef __CONFIG_H
#define __CONFIG_H

#include <string>

namespace dramcore {

class Config {
public:
    Config(std::string config_file);

    //DRAM physical structure
    unsigned int channel_size;
    unsigned int channels;
    unsigned int ranks;
    unsigned int banks;
    unsigned int bankgroups;
    unsigned int banks_per_group;
    unsigned int rows;
    unsigned int columns;
    unsigned int device_width;
    unsigned int bus_width;
    unsigned int queue_size;
    unsigned int burst_len;

    //DRAM timing parameters
    unsigned int tCCDL;
    unsigned int tCCDS;
    unsigned int tRTRS;
    unsigned int tRTP;
    unsigned int tCAS; //tCL
    unsigned int tCWD; // =tCAS
    unsigned int tWTR;
    unsigned int tWR;
    unsigned int tRP;
    unsigned int tRRD;
    unsigned int tRAS;
    unsigned int tRCD;
    unsigned int tRFC;
    unsigned int tRC;
    unsigned int tCKESR;
    unsigned int tXS;
    unsigned int tRFCb;
    unsigned int tRREFD;

    unsigned int tREFI;
    unsigned int tREFIb;

    unsigned int tFAW;

    unsigned int activation_window_depth = 4;

    std::string address_mapping;
    std::string queue_structure;
    unsigned int req_buffer_size;

    std::string validation_output_file;

    unsigned int epoch_period;

    //Computed parameters
    unsigned int channel_width_, rank_width_, bankgroup_width_, 
        bank_width_, row_width_, column_width_, throwaway_bits;

private:
    void CalculateSize();
};

}
#endif
