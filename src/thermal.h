#ifndef __THERMAL_H
#define __THERMAL_H

#include <vector>
#include <iostream>
#include <time.h> 
#include "common.h"
#include "bankstate.h"
#include "timing.h"
#include "configuration.h"
#include "ThermalConfig.h"

using namespace std; 

namespace dramcore{

	class ThermalCalculator
	{
		int DimX, DimY, DimZ; // Dimension of the memory 
		double **Midx; // Midx storing thermal conductance 
		double *Cap; // Cap storing the thermal capacitance 
		int MidSize, CapSize; // first dimension size of Midx and Cap
		double *T_trans; // T_trans storing the transient temperature array 
		double *T_final; // T_final storing the final temperature array 

		uint64_t power_epoch; // number of DRAM cycles for transient simulation 
		int sample_id; // index of the sampling power 
		uint64_t save_clk; // saved clk

		vector<vector<vector<double> > > accu_Pmap; // accumulative power map  
		vector<vector<vector<double> > > cur_Pmap; // current power map 

		/* private methods */
		void LocationMapping(const Command& cmd, int *x, int *y, int *z); 
		double EnergyCalculating(const Command& cmd); 
		void CalcTransT(); 
		void CalcFinalT();
		void InitialParameters();


	public:
		ThermalCalculator();
		~ThermalCalculator(); 
		void UpdatePower(const Command& cmd, uint64_t clk); 

		void PrintTransPT(); 
		void PrintFinalPT(); 




	};


}



#endif 