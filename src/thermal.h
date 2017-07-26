#ifndef __THERMAL_H
#define __THERMAL_H

#include <vector>
#include <iostream>
#include <time.h> 
#include <cmath>
#include "common.h"
#include "bankstate.h"
#include "timing.h"
#include "configuration.h"

using namespace std; 

namespace dramcore{

	class ThermalCalculator
	{
		// thermal simulation parameters 
		double T0;

		double R_TSV;

		double Ksi;
		double Kcu;
		double Kin;
		double Khs; 

		double Csi;
		double Ccu;
		double Cin;
		double Chs;

		double Hsi;
		double Hcu;
		double Hin;
		double Hhs;

		int time_iter0, time_iter; 

		const Config& config_;

		int dimX, dimY, numP; // Dimension of the memory 
		double **Midx; // Midx storing thermal conductance 
		double *Cap; // Cap storing the thermal capacitance 
		int MidSize, CapSize; // first dimension size of Midx and Cap
		vector<vector<double> > T_trans, T_final;
		//double *T_trans; // T_trans storing the transient temperature array 
		//double *T_final; // T_final storing the final temperature array 

		int sample_id; // index of the sampling power 
		uint64_t save_clk; // saved clk

		vector<vector<double> > accu_Pmap; // accumulative power map  
		vector<vector<double> > cur_Pmap; // current power map 

		vector<uint32_t> refresh_count; 

		// other intermediate parameters
		// not need to be defined here but it will be easy to use if it is defined 
		int vault_x, vault_y, bank_x, bank_y;
		int num_case; // number of different cases where the thermal simulation is performed
		vector<int> layerP; 


		/* private methods */
		void LocationMapping(const Command& cmd, int row0, int *x, int *y, int *z); 
		void CalcTransT(int case_id); 
		void CalcFinalT(int case_id);
		void InitialParameters();
		int square_array(int total_grids_)
		double GetMaxT(vector<vector<double> > T_, int case_id); 


	public:
		ThermalCalculator(const Config &config);
		~ThermalCalculator(); 
		void UpdatePower(const Command& cmd, uint64_t clk); 

		void PrintTransPT(); 
		void PrintFinalPT(uint64_t clk); 




	};


}



#endif 