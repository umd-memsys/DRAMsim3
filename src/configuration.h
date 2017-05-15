#ifndef __CONFIG_H
#define __CONFIG_H

#include <string>

namespace dramcore {

class Config {
public:
    Config(std::string config_file);

    //DRAM physical structure
    std::string protocol;
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
    unsigned int BL;

    //Generic DRAM timing parameters
    unsigned int burst_cycle;   // seperate BL with timing since fot GDDRx it's not BL/2
    unsigned int AL;
    unsigned int CL;
    unsigned int CWL;
    unsigned int RL;
    unsigned int WL;
    unsigned int tCCD_L;
    unsigned int tCCD_S;
    unsigned int tRTRS;
    unsigned int tRTP;
    unsigned int tWTR_L;
    unsigned int tWTR_S;
    unsigned int tWR;
    unsigned int tRP;
    unsigned int tRRD_L;
    unsigned int tRRD_S;
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
    unsigned int tRPRE;  // read preamble and write preamble are important
    unsigned int tWPRE; 
    unsigned int read_delay;
    unsigned int write_delay;

    // GDDR5 
    unsigned int tRCDRD;
    unsigned int tRCDWR;

    unsigned int activation_window_depth = 4;

    std::string address_mapping;
    std::string queue_structure;
    unsigned int queue_size;
    bool req_buffering_enabled;

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
