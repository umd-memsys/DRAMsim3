#ifndef __CONFIG_H
#define __CONFIG_H

class Config {
    public:
        Config();

    //Simulation Parameters
    int cycles = 10000;

    //DRAM physical structure
    int channels = 1;
    int ranks = 1;
    int bankgroups = 1;
    int banks_per_group = 1;
    int rows = 1024;

    int queue_size = 16;


    //DRAM timing parameters
    int tBurst = 8; //tBL
    int tCCDL = 6;
    int tCCDS = 4;
    int tRTRS = 2;
    int tRTP = 5;
    int tCAS = 3; //tCL
    int tCWD = 3; // =tCAS
    int tWTR = 5;
    int tWR = 10;
    int tRP = 10;
    int tRRD = 4;
    int tRAS = 24;
    int tRCD = 10;
    int tRFC = 74;
    int tRC = tRAS + tRP;
    int tCKESR = 50;
    int tXS = 10;
    int tRFCb = 20;
    int tRREFD = 5;

    int tREFI = 7800;
    int tREFIb = 1950;

    int tFAW = 50;

    int activation_window_depth = 4;



};

#endif