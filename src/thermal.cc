#include "thermal.h"


using namespace dramcore; 
using namespace std; 

extern "C" double *steady_thermal_solver(double ***powerM, double W, double Lc, int numP, int dimX, int dimZ, double **Midx, int count, double Tamb_);
extern "C" double *transient_thermal_solver(double ***powerM, double W, double L, int numP, int dimX, int dimZ, double **Midx, int MidxSize, double *Cap, int CapSize, double time, int iter, double *T_trans, double Tamb_);
extern "C" double **calculate_Midx_array(double W, double Lc, int numP, int dimX, int dimZ, int* MidxSize, double Tamb_);
extern "C" double *calculate_Cap_array(double W, double Lc, int numP, int dimX, int dimZ, int* CapSize); 
extern "C" double *initialize_Temperature(double W, double Lc, int numP, int dimX, int dimZ, double Tamb_); 


ThermalCalculator::ThermalCalculator(const Config &config):
	config_(config),
	sample_id(0),
	save_clk(0),
	time_iter0(10)
{

	// Initialize dimX, dimY, numP
	if (config_.IsHMC() || config_.IsHBM())
	{
		numP = config_.num_dies;
		int num_bank_per_layer = config_.banks / numP; 
		vault_x = square_array(config_.channels); 
		bank_y = square_array(num_bank_per_layer);

		vault_y = config_.channels / vault_x; 
		bank_x = num_bank_per_layer / bank_y; 

		dimX = vault_x * bank_x * config_.numXgrids; 
		dimY = vault_y * bank_y * config_.numYgrids; 

		num_case = 1; 
	}
	else
	{
		numP = 1; 
		bank_y = square_array(config_.banks);
		bank_x = config_.banks / bank_y; 

		dimX = bank_x * config_.numXgrids;
		dimY = bank_y * config_.numYgrids;

		num_case = config_.ranks * config_.channels; 
	}

	Tamb = config_.Tamb0 + T0;

	// Initialize the vectors
	accu_Pmap = vector<vector<double> > (numP * dimX * dimY, vector<double> (num_case, 0)); 
	cur_Pmap = vector<vector<double> > (numP * dimX * dimY, vector<double> (num_case, 0)); 
	T_trans = vector<vector<double> > ((numP*3+1) * dimX * dimY, vector<double> (num_case, 0));
	T_final = vector<vector<double> > ((numP*3+1) * dimX * dimY, vector<double> (num_case, 0));


	InitialParameters(); 

	refresh_count = vector<uint32_t> (config_.channels * config_.ranks, 0);
}

ThermalCalculator::~ThermalCalculator()
{

}

void ThermalCalculator::LocationMapping(const Command& cmd, int row0, int *x, int *y, int *z)
{
	int row_id; 
	if (row0 > -1)
		row_id = row0; 
	else
		row_id = cmd.Row();

	if (config_.IsHMC() || config_.IsHBM())
	{
		int vault_id = cmd.Channel();
		int bank_id = cmd.Bank(); 
		int vault_id_x = vault_id / vault_y; 
		int vault_id_y = vault_id % vault_y; 
		int num_bank_per_layer = config_.banks / config_.num_dies; 
		*z = bank_id / num_bank_per_layer; 
		int bank_same_layer = bank_id % num_bank_per_layer; 
		int bank_id_x = bank_same_layer / bank_y; 
		int bank_id_y = bank_same_layer % bank_y; 
		int grid_step = config_.rows / (config_.numXgrids * config_.numYgrids); 
		int grid_id = row_id / grid_step; 
		int grid_id_x = grid_id / config_.numYgrids; 
		int grid_id_y = grid_id % config_.numYgrids; 
		*x = vault_id_x * (bank_x * config_.numXgrids) + bank_id_x * config_.numXgrids + grid_id_x; 
		*y = vault_id_y * (bank_y * config_.numYgrids) + bank_id_y * config_.numYgrids + grid_id_y;
	}
	else
	{
		*z = 1; 
		int bank_id = cmd.Bank();
		int bank_id_x = bank_id / bank_y; 
		int bank_id_y = bank_id % bank_y;
		int grid_step = config_.rows / (config_.numXgrids * config_.numYgrids); 
		int grid_id = row_id / grid_step; 
		int grid_id_x = grid_id / config_.numYgrids; 
		int grid_id_y = grid_id % config_.numYgrids; 
		*x = bank_id_x * config_.numXgrids + grid_id_x;
		*y = bank_id_y * config_.numYgrids + grid_id_y;
	}
}


void ThermalCalculator::UpdatePower(const Command& cmd, uint64_t clk)
{
	int x, y, z; // 3D-dimension of the current cell 
	uint32_t rank, channel;
	double energy; 
	int row_s, ir; // for refresh
	int case_id;
	
	rank = cmd.Rank();
	channel = cmd.Channel(); 

	if (config_.IsHMC() || config_.IsHBM())
		case_id = 0;
	else
		case_id = channel * config_.ranks + rank; 

	save_clk = clk;

	if (cmd.cmd_type_ == CommandType::REFRESH)
	{
		// update refresh_count 
		row_s = refresh_count[channel * config_.ranks + rank] * config_.numRowRefresh; 
		refresh_count[channel * config_.ranks + rank] ++; 
		if (refresh_count[channel * config_.ranks + rank] * config_.numRowRefresh == config_.rows)
			refresh_count[channel * config_.ranks + rank] = 0;
		energy = config_.ref_energy_inc / config_.numRowRefresh; 
		for (ir = row_s; ir < row_s + config_.numRowRefresh; ir ++)
		{
			LocationMapping(cmd, ir, &x, &y, &z); 
			accu_Pmap[z*(dimX*dimY) + y*dimX + x][case_id] += energy / 1000; 
			cur_Pmap[z*(dimX*dimY) + y*dimX + x][case_id] += energy / 1000; 
		}

	}
	else
	{
		switch (cmd.cmd_type_){
			case CommandType::ACTIVATE:
				energy = config_.act_energy_inc; 
				break;
			case CommandType::READ:
			case CommandType::READ_PRECHARGE:
				energy = config_.read_energy_inc;
				break;
			case CommandType::WRITE:
			case CommandType::WRITE_PRECHARGE:
				energy = config_.write_energy_inc;
				break;
			default:
				printf("Error!"); 
				break;
		}
		LocationMapping(cmd, -1, &x, &y, &z); 
		accu_Pmap[z*(dimX*dimY) + y*dimX + x][case_id] += energy / 1000; 
		cur_Pmap[z*(dimX*dimY) + y*dimX + x][case_id] += energy / 1000; 
	}




	// print transient power and temperature 
	if (clk > (sample_id+1) * config_.power_epoch_period)
	{
		// add the background energy
		double extra_energy = (config_.act_stb_energy_inc + config_.pre_stb_energy_inc + config_.pre_pd_energy_inc + config_.sref_energy_inc) * config_.power_epoch_period / (dimX * dimY * numP); 
		for (int i = 0; i < dimX * dimY * numP; i ++)
			for (int j = 0; j < num_case; j ++)
				cur_Pmap[i][j] += extra_energy / 1000; 

		PrintTransPT(); 
		cur_Pmap = vector<vector<double> > (numP * dimX * dimY, vector<double> (config_.ranks, 0)); 
		sample_id ++;
	}
} 

void ThermalCalculator::PrintTransPT()
{
	double maxT; 
	for (int ir = 0; ir < num_case; ir ++){
		CalcTransT(ir); 
		maxT = GetMaxT(T_trans, ir); 
		cout << "MaxT of case " << ir << " is " << maxT - T0 << " [C]\n";
	}
}

void ThermalCalculator::PrintFinalPT(uint64_t clk)
{
	// first add the background energy 
	double extra_energy = (config_.act_stb_energy_inc + config_.pre_stb_energy_inc + config_.pre_pd_energy_inc + config_.sref_energy_inc) * clk / (dimX * dimY * numP); 
		for (int i = 0; i < dimX * dimY * numP; i ++)
			for (int j = 0; j < num_case; j ++)
				accu_Pmap[i][j] += extra_energy / 1000; 

	// calculate the final temperature for each case 
	double maxT; 
	for (int ir = 0; ir < num_case; ir ++){
		CalcFinalT(ir);
		maxT = GetMaxT(T_final, ir);
		cout << "MaxT of case " << ir << " is " << maxT - T0 << " [C]\n";
	}
}

void ThermalCalculator::CalcTransT(int case_id)
{
	double ***powerM; 
	double *T; 
	double time = config_.power_epoch_period * config_.tCK * 1e-9; 
	int i, j, l;

	// fill in powerM
	if ( !(powerM = (double ***)malloc(dimX * sizeof(double **))) ) printf("Malloc fails for powerM[].\n");
    for (i = 0; i < dimX; i++)
    {
        if ( !(powerM[i] = (double **)malloc(dimY * sizeof(double *))) ) printf("Malloc fails for powerM[%d][].\n", i);
        for (j = 0; j < dimY; j++)
        {
            if ( !(powerM[i][j] = (double *)malloc(numP * sizeof(double))) ) printf("Malloc fails for powerM[%d][%d][].\n", i,j);
        }
    }

    for (i = 0; i < dimX; i ++)
    	for (j = 0; j < dimY; j ++)
    		for (l = 0; l < numP; l ++)
    			powerM[i][j][l] = cur_Pmap[l*(dimX*dimY) + j*dimX + i][case_id] / (double) config_.power_epoch_period; 

    // fill in T
    if ( !(T = (double *)malloc(((numP*3+1)*dimX*dimY) * sizeof(double))) ) printf("Malloc fails for T.\n");
    for (i = 0; i < T_trans.size(); i ++)
    	T[i] = T_trans[i][case_id];

    T = transient_thermal_solver(powerM, config_.ChipX, config_.ChipY, numP, dimX, dimY, Midx, MidxSize, Cap, CapSize, time, time_iter, T, Tamb);

    for (i = 0; i < T_trans.size(); i ++)
    	T_trans[i][case_id] = T[i]; 

    free(T);
}

void ThermalCalculator::CalcFinalT(int case_id)
{
	double ***powerM; 
	int i, j, l;
	// fill in powerM
	if ( !(powerM = (double ***)malloc(dimX * sizeof(double **))) ) printf("Malloc fails for powerM[].\n");
    for (i = 0; i < dimX; i++)
    {
        if ( !(powerM[i] = (double **)malloc(dimY * sizeof(double *))) ) printf("Malloc fails for powerM[%d][].\n", i);
        for (j = 0; j < dimY; j++)
        {
            if ( !(powerM[i][j] = (double *)malloc(numP * sizeof(double))) ) printf("Malloc fails for powerM[%d][%d][].\n", i,j);
        }
    }

    for (i = 0; i < dimX; i ++)
    	for (j = 0; j < dimY; j ++)
    		for (l = 0; l < numP; l ++)
    			powerM[i][j][l] = accu_Pmap[l*(dimX*dimY) + j*dimX + i][case_id] / (double) config_.power_epoch_period; 

    double *T; 
    T = steady_thermal_solver(powerM, config_.ChipX, config_.ChipY, numP, dimX, dimY, Midx, MidxSize, Tamb);

    // assign the value to T_final 
    for (int i = 0; i < T_final.size(); i ++)
    	T_final[i][case_id] = T[i];

    free(T);

}

void ThermalCalculator::InitialParameters()
{
	layerP = vector<int> (numP, 0);
	for (int l = 0; l < numP; l++)
		layerP[l] = l * 3; 
	Midx = calculate_Midx_array(config_.ChipX, config_.ChipY, numP, dimX, dimY, &MidxSize, Tamb);
	Cap = calculate_Cap_array(config_.ChipX, config_.ChipY, numP, dimX, dimY, &CapSize); 
	calculate_time_step(); 

	double *T;
	for (int ir = 0; ir < num_case; ir ++){
		T = initialize_Temperature(config_.ChipX, config_.ChipY, numP, dimX, dimY, Tamb); 
		for (int i = 0; i < T_trans.size(); i++)
			T_trans[i][ir] = T[i];
	}
	free(T);

}

int ThermalCalculator::square_array(int total_grids_)
{
	int x, y, x_re = 1; 
	for (x = 1; x <= sqrt(total_grids_); x ++)
	{
		y = total_grids_ / x; 
		if (x * y == total_grids_)
			x_re = x; 
	}
	return x_re; 
}

void ThermalCalculator::calculate_time_step()
{
	double dt = 100.0; 
	int layer_dim = dimX * dimY; 
	double c, g; 

	for (int l = 0; l < CapSize; l ++)
	{
		c = Cap[l]; 
		for (int i = 0; i < layer_dim; i ++)
		{
			if (Midx[l*layer_dim + i][0] == Midx[l*layer_dim + i][1])
			{
				g = Midx[l*layer_dim + i][2]; 
				if (c/g < dt) 
					dt = c/g; 
			}
		}
	}

	cout << "maximum dt is " << dt << endl;

	// calculate time_iter
	double power_epoch_time = config_.power_epoch_period * config_.tCK * 1e-9; // [s]
	time_iter = time_iter0; 
	while (power_epoch_time / time_iter >= dt)
		time_iter ++;

}

double ThermalCalculator::GetMaxT(vector<vector<double> > T_, int case_id)
{
	double maxT = 0; 
	for (int i = 0; i < T_.size(); i ++)
	{
		if (T_[i][case_id] > maxT)
		{
			maxT = T_[i][case_id]; 
		} 
	}
	return maxT;
}
