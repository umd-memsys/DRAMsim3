#ifndef __CONFIG_H
#define __CONFIG_H

#include <string>

namespace dramcore {

class Config {
public:
    Config(std::string config_file);

    //DRAM physical structure
    unsigned int channels;
    unsigned int ranks;
    unsigned int bankgroups;
    unsigned int banks_per_group;
    unsigned int rows;
    unsigned int columns;

    unsigned int queue_size;

    //DRAM timing parameters
    unsigned int tBurst; //tBL
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
    std::string verification_output_file;
    bool verification_output;

    //Computed parameters
    unsigned int channel_width_, rank_width_, bankgroup_width_, bank_width_, row_width_, column_width_;
};

}
#endif