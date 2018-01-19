#ifndef __THERMAL_H
#define __THERMAL_H

#include <functional>
#include <vector>
#include <iostream>
#include <time.h>
#include <cmath>
#include <fstream>
#include "common.h"
#include "bankstate.h"
#include "timing.h"
#include "configuration.h"
#include "thermal_config.h"

using namespace std;

namespace dramcore
{

extern std::function<Address(const Address& addr)> GetPhyAddress;

class ThermalCalculator
{
    int time_iter0, time_iter;
    double Tamb; // The ambient temperature in Kelvin

    const int num_dummy = 2; 

    const Config &config_;
    Statistics &stats_;

    int dimX, dimY, numP;  // Dimension of the memory
    double **Midx;		   // Midx storing thermal conductance
    double *Cap;		   // Cap storing the thermal capacitance
    int MidxSize, CapSize; // first dimension size of Midx and Cap
    vector<vector<double>> T_trans, T_final;
    //double *T_trans; // T_trans storing the transient temperature array
    //double *T_final; // T_final storing the final temperature array

    int sample_id;	 // index of the sampling power
    uint64_t save_clk; // saved clk

    vector<vector<double>> accu_Pmap; // accumulative power map
    vector<vector<double>> cur_Pmap;  // current power map

    vector<vector<uint32_t>> refresh_count;

    // previous background energy
    vector<double> sref_energy_prev;
    vector<double> pre_stb_energy_prev;
    vector<double> act_stb_energy_prev;
    vector<double> pre_pd_energy_prev;

    // other intermediate parameters
    // not need to be defined here but it will be easy to use if it is defined
    int vault_x, vault_y, bank_x, bank_y;
    int num_case; // number of different cases where the thermal simulation is performed
    vector<int> layerP;

    // Output files
    ofstream epoch_temperature_file_csv_;
    ofstream final_temperature_file_csv_;
    ofstream bank_position_csv_;

    /* private methods */
    void SetPhyAddressMapping();
    void LocationMappingANDaddEnergy_RF(const Command &cmd, int bank0, int row0, int caseID_, double add_energy);
    void LocationMappingANDaddEnergy(const Command &cmd, int bank0, int row0, int caseID_, double add_energy);

    void CalcTransT(int case_id);
    void CalcFinalT(int case_id, uint64_t clk);
    void InitialParameters();
    int square_array(int total_grids_);
    int determineXY(double xd, double yd, int total_grids_);
    double GetMaxT(vector<vector<double>> T_, int case_id);
    void calculate_time_step();
    // print to csv-files
    void PrintCSV_trans(ofstream &csvfile, vector<vector<double>> P_, vector<vector<double>> T_, int id, uint64_t scale);
    void PrintCSV_final(ofstream &csvfile, vector<vector<double>> P_, vector<vector<double>> T_, int id, uint64_t scale);
    void PrintCSVHeader_trans(ofstream &csvfile);
    void PrintCSVHeader_final(ofstream &csvfile);
    void PrintCSV_bank(ofstream &csvfile);

  public:
    ThermalCalculator() {}
    ThermalCalculator(const Config &config, Statistics &stats);
    ~ThermalCalculator();
    void UpdatePower(const Command &cmd, uint64_t clk);

    void PrintTransPT(uint64_t clk);
    void PrintFinalPT(uint64_t clk);
};
}

#endif