#include "thermal.h"


using namespace dramcore; 
using namespace std; 

extern "C" double ***steady_thermal_solver(double ***powerM, double W, double Lc, int numP, int dimX, int dimZ, double **Midx, int count, double *Tt);
extern "C" double *transient_thermal_solver(double ***powerM, double W, double L, int numP, int dimX, int dimZ, double **Midx, int MidxSize, double *Cap, int CapSize, double time, int iter, double *T_trans);
extern "C" double **calculate_Midx_array(double W, double Lc, int numP, int dimX, int dimZ, int* MidxSize);
extern "C" double *calculate_Cap_array(double W, double Lc, int numP, int dimX, int dimZ, int* CapSize); 
extern "C" double *initialize_Temperature(double W, double Lc, int numP, int dimX, int dimZ); 


ThermalCalculator::ThermalCalculator():
	sample_id(0),
	save_clk(0)
{
	// Initialize power_epoch

	// Initialize DimX, DimY, DimZ


	// Initialize the vectors
	accu_Pmap = vector<vector<vector<double> > > (DimX, vector<vector<double> > (DimY, vector<double> (DimZ, 0)));
	cur_Pmap = vector<vector<vector<double> > > (DimX, vector<vector<double> > (DimY, vector<double> (DimZ, 0)));

	InitialParamters(); 
}

ThermalCalculator::~ThermalCalculator()
{

}

void ThermalCalculator::LocationMapping(const Command& cmd int *x, int *y, int *z)
{

}

double ThermalCalculator::EnergyCalculating(const Command& cmd)
{

} 

void ThermalCalculator::UpdatePower(const Command& cmd, uint64_t clk)
{
	int x, y, z; // 3D-dimension of the current cell 
	EnergyCalculating(cmd);
	LocationMapping(cmd, &x, &y, &z); 
	save_clk = clk;

	// update the power

	// print transient power and temperature 
	if (clk > (sample_id+1) * power_epoch)
	{
		PrintTransPT(); 
		cur_Pmap = vector<vector<vector<double> > > (DimX, vector<vector<double> > (DimY, vector<double> (DimZ, 0)));
		sample_id ++;
	}
} 

void ThermalCalculator::PrintTransPT()
{

}

void ThermalCalculator::PrintFinalPT()
{

}

void ThermalCalculator::CalcTransT()
{

}

void ThermalCalculator::CalcFinalT()
{

}

void ThermalCalculator::InitialParamters()
{
	
}
