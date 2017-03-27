#ifndef __CONFIG_H
#define __CONFIG_H

#include <string>

unsigned int LogBase2(unsigned int power_of_two);


class Config {
    public:
        Config();

    //Simulation Parameters
    unsigned int cycles = 100000;

    //DRAM physical structure
    unsigned int channels = 1;
    unsigned int ranks = 2;
    unsigned int bankgroups = 2;
    unsigned int banks_per_group = 2;
    unsigned int rows = 1 << 16;
    unsigned int columns = 1 << 10;

    unsigned int queue_size = 16;

    //DRAM timing parameters
    unsigned int tBurst = 8; //tBL
    unsigned int tCCDL = 6;
    unsigned int tCCDS = 4;
    unsigned int tRTRS = 2;
    unsigned int tRTP = 5;
    unsigned int tCAS = 3; //tCL
    unsigned int tCWD = 3; // =tCAS
    unsigned int tWTR = 5;
    unsigned int tWR = 10;
    unsigned int tRP = 10;
    unsigned int tRRD = 4;
    unsigned int tRAS = 24;
    unsigned int tRCD = 10;
    unsigned int tRFC = 74;
    unsigned int tRC = tRAS + tRP;
    unsigned int tCKESR = 50;
    unsigned int tXS = 10;
    unsigned int tRFCb = 20;
    unsigned int tRREFD = 5;

    unsigned int tREFI = 7800;
    unsigned int tREFIb = 1950;

    unsigned int tFAW = 50;

    unsigned int activation_window_depth = 4;

    std::string address_mapping = "rorababgchcl"; //Rank-Bank-Bankgroup-Channel-Column-Row
    std::string queue_structure = "PER_BANK";


    //Computed parameters
    unsigned int channel_width_, rank_width_, bankgroup_width_, bank_width_, row_width_, column_width_;
};

#endif