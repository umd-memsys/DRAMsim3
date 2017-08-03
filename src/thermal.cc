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

	cout << "vault_x = " << vault_x << "; vault_y = " << vault_y << endl;
	cout << "bank_x = " << bank_x << "; bank_y = " << bank_y << endl;
	cout << "dimX = " << dimX << "; dimY = " << dimY << "; numP = " << numP << endl;

	// Initialize the vectors
	accu_Pmap = vector<vector<double> > (numP * dimX * dimY, vector<double> (num_case, 0)); 
	cur_Pmap = vector<vector<double> > (numP * dimX * dimY, vector<double> (num_case, 0)); 
	T_trans = vector<vector<double> > ((numP*3+1) * dimX * dimY, vector<double> (num_case, 0));
	T_final = vector<vector<double> > ((numP*3+1) * dimX * dimY, vector<double> (num_case, 0));


	InitialParameters(); 

	refresh_count = vector<uint32_t> (config_.channels * config_.ranks, 0);
	cout << "size of refresh_count is " << refresh_count.size() << endl;

	// Initialize the output file
    epoch_temperature_file_csv_.open(config_.epoch_temperature_file_csv);
    final_temperature_file_csv_.open(config_.final_temperature_file_csv);

    // print header to csv files 
    PrintCSVHeader_trans(epoch_temperature_file_csv_);
    PrintCSVHeader_final(final_temperature_file_csv_);

	//cout << "done thermal calculator\n";
}

ThermalCalculator::~ThermalCalculator()
{
	cout << "Print the final temperature \n";
}

void ThermalCalculator::LocationMapping(const Command& cmd, int bank0, int row0, int *x, int *y, int *z)
{
	//cout << "Enter LocationMapping\n";
	int row_id, bank_id; 
	if (row0 > -1)
		row_id = row0; 
	else
		row_id = cmd.Row();

	if (bank0 > -1)
		bank_id = bank0; 
	else
		bank_id = cmd.Bank();

	if (config_.IsHMC() || config_.IsHBM())
	{
		int vault_id = cmd.Channel();
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
		*z = 0; 
		int bank_id_x = bank_id / bank_y; 
		int bank_id_y = bank_id % bank_y;
		int grid_step = config_.rows / (config_.numXgrids * config_.numYgrids); 
		int grid_id = row_id / grid_step; 
		int grid_id_x = grid_id / config_.numYgrids; 
		int grid_id_y = grid_id % config_.numYgrids; 
		*x = bank_id_x * config_.numXgrids + grid_id_x;
		*y = bank_id_y * config_.numYgrids + grid_id_y;
		//cout << "bank_id = " << bank_id << "; row_id = " << row_id << endl;
	}
	//cout << "Finish LocationMapping ...";

	//cout << "x = " << *x << "; y = " << *y << "; z = " << *z << endl;
	//cout << "--------------------------------------\n";
}

void ThermalCalculator::DummyFunc(const Command& cmd, uint64_t clk)
{
	// this function is to test other functions
}


void ThermalCalculator::UpdatePower(const Command& cmd, uint64_t clk)
{
	int x, y, z; // 3D-dimension of the current cell 
	uint32_t rank, channel;
	double energy; 
	int row_s, ir, ib; // for refresh
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
		//cout << "rank = " << rank << "; channel = " << channel << endl;
		// update refresh_count 
		row_s = refresh_count[channel * config_.ranks + rank] * config_.numRowRefresh; 
		refresh_count[channel * config_.ranks + rank] ++; 
		if (refresh_count[channel * config_.ranks + rank] * config_.numRowRefresh == config_.rows)
			refresh_count[channel * config_.ranks + rank] = 0;
		energy = config_.ref_energy_inc / config_.numRowRefresh; 
		for (ib = 0; ib < config_.banks; ib ++){
			for (ir = row_s; ir < row_s + config_.numRowRefresh; ir ++){
				//cout << "ib = " << ib << "; ir = " << ir << endl;
				LocationMapping(cmd, ib, ir, &x, &y, &z); 
				//cout << "x = " << x << "; y = " << y << "; z = " << z << endl;
				accu_Pmap[z*(dimX*dimY) + y*dimX + x][case_id] += energy / 1000; 
				cur_Pmap[z*(dimX*dimY) + y*dimX + x][case_id] += energy / 1000; 
			}
		}

	}
	else
	{
		//cout << "normal\n";
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
				//cout << "Error: CommandType is " << cmd.cmd_type_ << endl;
				break;
		}
		LocationMapping(cmd, -1, -1, &x, &y, &z); 
		accu_Pmap[z*(dimX*dimY) + y*dimX + x][case_id] += energy / 1000; 
		cur_Pmap[z*(dimX*dimY) + y*dimX + x][case_id] += energy / 1000; 
	}




	// print transient power and temperature 
	if (clk > (sample_id+1) * config_.power_epoch_period)
	{
		//cout << "begin sampling!\n";
		// add the background energy
		double extra_energy = (config_.act_stb_energy_inc + config_.pre_stb_energy_inc + config_.pre_pd_energy_inc + config_.sref_energy_inc) * config_.power_epoch_period / (dimX * dimY * numP); 
		for (int i = 0; i < dimX * dimY * numP; i ++)
			for (int j = 0; j < num_case; j ++)
				cur_Pmap[i][j] += extra_energy / 1000; 

		PrintTransPT(clk); 
		cur_Pmap = vector<vector<double> > (numP * dimX * dimY, vector<double> (config_.ranks, 0)); 
		sample_id ++;
	}
} 

void ThermalCalculator::PrintTransPT(uint64_t clk)
{
	cout << "============== At " << clk * config_.tCK * 1e-6 << "[ms] =============\n"; 
	double maxT; 
	for (int ir = 0; ir < num_case; ir ++){
		CalcTransT(ir); 
		maxT = GetMaxT(T_trans, ir); 
		cout << "MaxT of case " << ir << " is " << maxT - T0 << " [C]\n";
		// print to file 
		PrintCSV_trans(epoch_temperature_file_csv_, cur_Pmap, T_trans, ir, config_.power_epoch_period);
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
		CalcFinalT(ir, clk);
		maxT = GetMaxT(T_final, ir);
		cout << "MaxT of case " << ir << " is " << maxT << " [C]\n";
		// print to file
		PrintCSV_final(final_temperature_file_csv_, accu_Pmap, T_final, ir, clk);
	}

	// close all the csv files 
	final_temperature_file_csv_.close(); 
	//final_power_file_csv_.close();
	epoch_temperature_file_csv_.close(); 
	//epoch_power_file_csv_.close();
}

void ThermalCalculator::CalcTransT(int case_id)
{
	double ***powerM; 
	double *T; 
	double time = config_.power_epoch_period * config_.tCK * 1e-9; 
	int i, j, l;
	double totP = 0; 

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

    for (i = 0; i < dimX; i ++){
    	for (j = 0; j < dimY; j ++){
    		for (l = 0; l < numP; l ++){
    			powerM[i][j][l] = cur_Pmap[l*(dimX*dimY) + j*dimX + i][case_id] / (double) config_.power_epoch_period; 
    			totP += powerM[i][j][l];
    		}
    	}
    }
    cout << "total Power is " << totP * 1000 << " [mW]\n";

    // fill in T
    if ( !(T = (double *)malloc(((numP*3+1)*dimX*dimY) * sizeof(double))) ) printf("Malloc fails for T.\n");
    for (i = 0; i < T_trans.size(); i ++)
    	T[i] = T_trans[i][case_id];

    T = transient_thermal_solver(powerM, config_.ChipX, config_.ChipY, numP, dimX, dimY, Midx, MidxSize, Cap, CapSize, time, time_iter, T, Tamb);

    for (i = 0; i < T_trans.size(); i ++)
    	T_trans[i][case_id] = T[i]; 

    free(T);
}

void ThermalCalculator::CalcFinalT(int case_id, uint64_t clk)
{
	double ***powerM; 
	int i, j, l;
	double totP;
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

    for (i = 0; i < dimX; i ++){
    	for (j = 0; j < dimY; j ++){
    		for (l = 0; l < numP; l ++){
    			powerM[i][j][l] = accu_Pmap[l*(dimX*dimY) + j*dimX + i][case_id] / (double) clk; 
    			totP += powerM[i][j][l]; 
    			//cout << "powerM[" << i << "][" << j << "][" << l << "] = " << powerM[i][j][l] << endl;
    		}
    	}
    }

    cout << "total Power is " << totP * 1000 << " [mW]\n";

    double *T; 
    T = steady_thermal_solver(powerM, config_.ChipX, config_.ChipY, numP, dimX, dimY, Midx, MidxSize, Tamb);

    // assign the value to T_final 
    for (int i = 0; i < T_final.size(); i ++){
    	T_final[i][case_id] = T[i];
    	//cout << "T_final[" << i << "][" << case_id << "] = " << T_final[i][case_id] << endl;
    }

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
	int idx0, idx1, idxC; 

	//cout << "CapSize = " << CapSize << "; MidxSize = " << MidxSize << "; layer_dim = " << layer_dim << endl;

	for (int j = 0; j < MidxSize; j ++)
	{
        idx0 = (int) (Midx[j][0] + 0.01); 
        idx1 = (int) (Midx[j][1] + 0.01); 
        idxC = idx0 / layer_dim;

        if (idx0 == idx1){
        	g = Midx[j][2]; 
            c = Cap[idxC]; 
            if (c/g < dt)
            	dt = c/g;
        }
    }


	cout << "maximum dt is " << dt << endl;

	// calculate time_iter
	double power_epoch_time = config_.power_epoch_period * config_.tCK * 1e-9; // [s]
	cout << "power_epoch_time = " << power_epoch_time << endl;
	time_iter = time_iter0; 
	while (power_epoch_time / time_iter >= dt)
		time_iter ++;
	//time_iter += 10;
	cout << "time_iter = " << time_iter << endl;

}

double ThermalCalculator::GetMaxT(vector<vector<double> > T_, int case_id)
{
	double maxT = 0; 
	for (int i = 0; i < T_.size(); i ++)
	{
		///cout << T_[i][case_id] << endl;
		if (T_[i][case_id] > maxT)
		{
			maxT = T_[i][case_id]; 
		} 
	}
	return maxT;
}


void ThermalCalculator::PrintCSV_trans(ofstream& csvfile, vector<vector<double> > P_, vector<vector<double> > T_, int id, uint64_t scale)
{
	double pw, tm; 
	for (int l = 0; l < numP; l ++){
		for (int j = 0; j < dimY; j ++){
			for (int i = 0; i < dimX; i ++){
				pw = P_[l*(dimX*dimY) + j*dimX + i][id] / (double) scale; 
				tm = T_[(layerP[l]+1)*(dimX*dimY) + j*dimX + i][id] - T0;
				csvfile << id << "," << i << "," << j << "," << l << "," << pw << "," << tm << "," << sample_id << endl;  
			}
		}
	}

}

void ThermalCalculator::PrintCSV_final(ofstream& csvfile, vector<vector<double> > P_, vector<vector<double> > T_, int id, uint64_t scale)
{
	double pw, tm; 
	for (int l = 0; l < numP; l ++){
		for (int j = 0; j < dimY; j ++){
			for (int i = 0; i < dimX; i ++){
				pw = P_[l*(dimX*dimY) + j*dimX + i][id] / (double) scale; 
				tm = T_[(layerP[l]+1)*(dimX*dimY) + j*dimX + i][id];
				csvfile << id << "," << i << "," << j << "," << l << "," << pw << "," << tm << endl;  
			}
		}
	}

}

void ThermalCalculator::PrintCSVHeader_trans(ofstream& csvfile)
{
	csvfile << "rank_channel_index,x,y,z,power,temperature,epoch" << endl;
}

void ThermalCalculator::PrintCSVHeader_final(ofstream& csvfile)
{
	csvfile << "rank_channel_index,x,y,z,power,temperature" << endl;
}