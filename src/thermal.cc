#include "thermal.h"

using namespace dramcore;
using namespace std;

extern "C" double *steady_thermal_solver(double ***powerM, double W, double Lc, int numP, int dimX, int dimZ, double **Midx, int count, double Tamb_);
extern "C" double *transient_thermal_solver(double ***powerM, double W, double L, int numP, int dimX, int dimZ, double **Midx, int MidxSize, double *Cap, int CapSize, double time, int iter, double *T_trans, double Tamb_);
extern "C" double **calculate_Midx_array(double W, double Lc, int numP, int dimX, int dimZ, int *MidxSize, double Tamb_);
extern "C" double *calculate_Cap_array(double W, double Lc, int numP, int dimX, int dimZ, int *CapSize);
extern "C" double *initialize_Temperature(double W, double Lc, int numP, int dimX, int dimZ, double Tamb_);


std::function<Address(const Address& addr)> dramcore::GetPhyAddress;

ThermalCalculator::ThermalCalculator(const Config &config, Statistics &stats) : time_iter0(10),
                                                                                config_(config),
                                                                                stats_(stats),
                                                                                sample_id(0),
                                                                                save_clk(0)
{
    // Initialize dimX, dimY, numP
    // The dimension of the chip is determined such that the floorplan is
    // as square as possilbe. If a square floorplan cannot be reached, 
    // x-dimension is larger
    if (config_.IsHMC())
    {
        double xd, yd; 
        numP = config_.num_dies;
        bank_x = 1; bank_y = 2;

        xd = bank_x * config_.bank_asr; 
        yd = bank_y * 1.0; 
        vault_x = determineXY(xd, yd, config_.channels); 
        vault_y = config_.channels / vault_x; 

        dimX = vault_x * bank_x * config_.numXgrids;
        dimY = vault_y * bank_y * config_.numYgrids;

        num_case = 1;
    }
    else if (config_.IsHBM())
    {
        numP = config_.num_dies; 
        bank_x = 8; bank_y = 2; 
        vault_x = 1; vault_y = 2; 
        dimX = vault_x * bank_x * config_.numXgrids; 
        dimY = vault_y * bank_y * config_.numYgrids; 

        num_case = 1; 

    }
    else
    {
        numP = 1;
        bank_x = determineXY(config_.bank_asr, 1.0, config_.banks);
        bank_y = config_.banks / bank_x;

        dimX = bank_x * config_.numXgrids;
        dimY = bank_y * config_.numYgrids;

        num_case = config_.ranks * config_.channels;
    }

    Tamb = config_.Tamb0 + T0;

    cout << "bank aspect ratio = " << config_.bank_asr << endl;
    //cout << "#rows = " << config_.rows << "; #columns = " << config_.rows / config_.bank_asr << endl;
    cout << "numXgrids = " << config_.numXgrids << "; numYgrids = " << config_.numYgrids << endl;
    cout << "vault_x = " << vault_x << "; vault_y = " << vault_y << endl;
    cout << "bank_x = " << bank_x << "; bank_y = " << bank_y << endl;
    cout << "dimX = " << dimX << "; dimY = " << dimY << "; numP = " << numP << endl;
    cout << "number of devices is " << config_.devices_per_rank << endl;

    SetPhyAddressMapping();

    // Initialize the vectors
    accu_Pmap = vector<vector<double>>(numP * dimX * dimY, vector<double>(num_case, 0));
    cur_Pmap = vector<vector<double>>(numP * dimX * dimY, vector<double>(num_case, 0));
    T_trans = vector<vector<double>>((numP * 3 + 1) * (dimX+num_dummy) * (dimY+num_dummy), vector<double>(num_case, 0));
    T_final = vector<vector<double>>((numP * 3 + 1) * (dimX+num_dummy) * (dimY+num_dummy), vector<double>(num_case, 0));

    sref_energy_prev = vector<double> (num_case, 0); 
    pre_stb_energy_prev = vector<double> (num_case, 0);
    act_stb_energy_prev = vector<double> (num_case, 0);
    pre_pd_energy_prev = vector<double> (num_case, 0); 

    InitialParameters();

    refresh_count = vector<vector<uint32_t> > (config_.channels * config_.ranks, vector<uint32_t> (config_.banks, 0));

    // Initialize the output file
    final_temperature_file_csv_.open(config_.final_temperature_file_csv);
    PrintCSVHeader_final(final_temperature_file_csv_);
    
    // print bank position
    bank_position_csv_.open(config_.bank_position_csv);
    PrintCSV_bank(bank_position_csv_); 

    epoch_max_temp_file_csv_.open(config_.epoch_max_temp_file_csv);
    PrintCSVHeader_trans(epoch_max_temp_file_csv_);
    
    // print header to csv files
    if (config_.output_level >= 2) {
        epoch_temperature_file_csv_.open(config_.epoch_temperature_file_csv);
        PrintCSVHeader_trans(epoch_temperature_file_csv_);
    }
}

ThermalCalculator::~ThermalCalculator()
{}

void ThermalCalculator::SetPhyAddressMapping() {
    std::string mapping_string = config_.loc_mapping;
    if (mapping_string.empty()) {
        // if no location mapping specified, then do not map and use default mapping...
        GetPhyAddress = [](const Address& addr) {
            return Address(addr);
        };
        return;
    }
    std::vector<std::string> bit_fields = StringSplit(mapping_string, ',');
    if (bit_fields.size() != 6) {
        cerr << "loc_mapping should have 6 fields!" << endl;
        exit(1);
    }
    std::vector<std::vector<int>> mapped_pos(bit_fields.size(), std::vector<int>());
    for (unsigned i = 0; i < bit_fields.size(); i++) {
        std::vector<std::string> bit_pos = StringSplit(bit_fields[i], '-');
        for (unsigned j = 0; j < bit_pos.size(); j++) {
            if (!bit_pos[j].empty()){
                int colon_pos = bit_pos[j].find(":");
                if (colon_pos == std::string::npos) {  // no "start:end" short cuts
                    int pos = std::stoi(bit_pos[j]);
                    mapped_pos[i].push_back(pos);
                } else {
                    // for string like start:end (both inclusive), push all numbers in between into mapped_pos
                    int start_pos = std::stoi(bit_pos[j].substr(0, colon_pos));
                    int end_pos = std::stoi(bit_pos[j].substr(colon_pos + 1, std::string::npos));
                    if (start_pos > end_pos) {  // seriously there is no smart way in c++ to do this?
                        for (int k = start_pos; k >= end_pos; k--) {
                            mapped_pos[i].push_back(k);
                        }
                    } else {
                        for (int k = start_pos; k <= end_pos; k++) {
                            mapped_pos[i].push_back(k);
                        }
                    }
                }
            }
        }
    }

#ifdef DEBUG_LOC_MAPPING
    cout << "final mapped pos:";
    for (unsigned i = 0; i < mapped_pos.size(); i++) {
        for (unsigned j = 0; j < mapped_pos[i].size(); j++) {
            cout << mapped_pos[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
#endif // DEBUG_LOC_MAPPING

    const int column_offset = 3; 

    GetPhyAddress = [mapped_pos, column_offset](const Address& addr) {
        uint64_t new_hex = 0;
        // ch - ra - bg - ba - ro - co
        int origin_pos[] = {addr.channel_, addr.rank_, addr.bankgroup_, addr.bank_, addr.row_, addr.column_};
        int new_pos[] = {0, 0, 0, 0, 0, 0};
        for (unsigned i = 0; i < mapped_pos.size(); i++) {
            int field_width = mapped_pos[i].size();
            for (int j = 0; j < field_width; j++){
                uint64_t this_bit = GetBitInPos(origin_pos[i], field_width - j -1);
                uint64_t new_bit = (this_bit << mapped_pos[i][j]);
                new_hex |= new_bit;
#ifdef DEBUG_LOC_MAPPING
                cout << "mapping " << this_bit << " to " << mapped_pos[i][j] << ", result:"
                     << std::hex << new_hex << std::dec << endl;
#endif  // DEBUG_LOC_MAPPING
            }
        }

        int pos = column_offset;
        for (int i = mapped_pos.size() - 1; i >= 0; i--) {
            new_pos[i] = ModuloWidth(new_hex, mapped_pos[i].size(), pos);
            pos += mapped_pos[i].size();
        }

#ifdef DEBUG_LOC_MAPPING
        cout << "new channel " << new_pos[0] << " vs old channel " << addr.channel_ << endl;;
        cout << "new rank " << new_pos[1] << " vs old rank " << addr.rank_ << endl;
        cout << "new bg " << new_pos[2] << " vs old bg " << addr.bankgroup_ << endl;
        cout << "new bank " << new_pos[3] << " vs old bank " << addr.bank_ << endl;
        cout << "new row " << new_pos[4] << " vs old row " << addr.row_ << endl;
        cout << "new col " << new_pos[5] << " vs old col " << addr.column_ << endl;
        cout << std::dec;
#endif

        return Address(new_pos[0], new_pos[1], new_pos[2], 
                       new_pos[3], new_pos[4], new_pos[5]);
    };
}

void ThermalCalculator::LocationMappingANDaddEnergy(const Command &cmd, int bank0, int row0, int caseID_, double add_energy)
{
    int vault_id, bank_id, row_id, col_id;
    Address temp_addr = Address(cmd.addr_);
    Address new_loc;
    int vault_id_x, vault_id_y, bank_id_x, bank_id_y, grid_id_x, grid_id_y;
    int x, y, z; 
    int col_tile_id; 

    row_id = cmd.Row();
    bank_id = cmd.Bank();
    
    // modify the bank-id if there exists bank groups 
    if (config_.bankgroups > 1)
    {
        bank_id += cmd.Bankgroup() * config_.banks_per_group;
    }

    //cout << "bank_order = " << config_.bank_order << endl;

    if (config_.bank_order == 1)
    {
        // y-direction priority
        if (config_.IsHMC() || config_.IsHBM())
        {
            if (config_.IsHMC())
            {
                vault_id = cmd.Channel();
                vault_id_x = vault_id / vault_y;
                vault_id_y = vault_id % vault_y;
                int num_bank_per_layer = config_.banks / config_.num_dies;
                if (config_.bank_layer_order == 0)
                    z = bank_id / num_bank_per_layer;
                else
                    z = numP - bank_id / num_bank_per_layer - 1; 
                int bank_same_layer = bank_id % num_bank_per_layer;
                bank_id_x = bank_same_layer / bank_y;
                bank_id_y = bank_same_layer % bank_y;
            }
            else
            {
                vault_id = cmd.Channel(); 
                int bank_group_id = bank_id / config_.banks_per_group; 
                int sub_bank_id = bank_id % config_.banks_per_group; 
                bank_id_x = bank_group_id * 2 + sub_bank_id / 2; 
                bank_id_y = sub_bank_id % 2; 

                z = vault_id / 2; // each layer has two channels (vaults)
                vault_id_y = vault_id % 2; 
                vault_id_x = 0; 

                //cout << "vault_id_y = " << vault_id_y << endl;
            }
            
            grid_id_x = row_id / config_.matX / config_.RowTile; 

            for (int i = 0; i < config_.BL; i ++)
            {
                new_loc = GetPhyAddress(temp_addr);
                col_id = new_loc.column_ * config_.device_width;
                col_tile_id = row_id / config_.TileRowNum;  
                for (int j = 0; j < config_.device_width; j ++){
                    grid_id_y = col_id / config_.matY + col_tile_id * (config_.numYgrids / config_.RowTile);
                    x = vault_id_x * (bank_x * config_.numXgrids) + bank_id_x * config_.numXgrids + grid_id_x;
                    y = vault_id_y * (bank_y * config_.numYgrids) + bank_id_y * config_.numYgrids + grid_id_y; 

                    accu_Pmap[z * (dimX * dimY) + y * dimX + x][caseID_] += add_energy / config_.device_width;
                    cur_Pmap[z * (dimX * dimY) + y * dimX + x][caseID_] += add_energy / config_.device_width;
                    col_id ++;
                }
                
                temp_addr.column_ ++; 
            }
        }
        else
        {
            z = 0;
            if (config_.bankgroups > 1)
            {
                int bank_group_id = bank_id / config_.banks_per_group; 
                int sub_bank_id = bank_id % config_.banks_per_group; 
                // banks in a group always form like a square
                // bank_groups are arranged in a line -- either in x or y direction
                bank_id_x = sub_bank_id / 2; 
                bank_id_y = sub_bank_id % 2; 
                if (bank_x <= bank_y)
                    bank_id_y += bank_group_id * 2; 
                else
                    bank_id_x += bank_group_id * 2; 
            }
            else
            {
                bank_id_x = bank_id / bank_y;
                bank_id_y = bank_id % bank_y;
            }
            
            grid_id_x = row_id / config_.matX / config_.RowTile; 

            for (int i = 0; i < config_.BL; i ++)
            {
                new_loc = GetPhyAddress(temp_addr);
                col_id = new_loc.column_ * config_.device_width;
                col_tile_id = row_id / config_.TileRowNum;  
                for (int j = 0; j < config_.device_width; j ++){
                    grid_id_y = col_id / config_.matY + col_tile_id * (config_.numYgrids / config_.RowTile);
                    x = bank_id_x * config_.numXgrids + grid_id_x;
                    y = bank_id_y * config_.numYgrids + grid_id_y; 

                    accu_Pmap[z * (dimX * dimY) + y * dimX + x][caseID_] += add_energy / config_.device_width;
                    cur_Pmap[z * (dimX * dimY) + y * dimX + x][caseID_] += add_energy / config_.device_width;
                    col_id ++;
                }
                
                temp_addr.column_ ++; 
            }
        }
    }
    else
    {
        // x-direction priority
        if (config_.IsHMC() || config_.IsHBM())
        {
            if (config_.IsHMC())
            {
                vault_id = cmd.Channel();
                vault_id_y = vault_id / vault_x;
                vault_id_x = vault_id % vault_x;
                int num_bank_per_layer = config_.banks / config_.num_dies;
                if (config_.bank_layer_order == 0)
                    z = bank_id / num_bank_per_layer;
                else
                    z = numP - bank_id / num_bank_per_layer - 1; 
                int bank_same_layer = bank_id % num_bank_per_layer;
                bank_id_y = bank_same_layer / bank_x;
                bank_id_x = bank_same_layer % bank_x;
            }
            else
            {
                vault_id = cmd.Channel(); 
                int bank_group_id = bank_id / config_.banks_per_group; 
                int sub_bank_id = bank_id % config_.banks_per_group; 
                bank_id_x = bank_group_id * 2 + sub_bank_id / 2; 
                bank_id_y = sub_bank_id % 2; 

                z = vault_id / 2; // each layer has two channels (vaults)
                vault_id_y = vault_id % 2; 
                vault_id_x = 0; 
            }
            
            grid_id_x = row_id / config_.matX / config_.RowTile; 

            for (int i = 0; i < config_.BL; i ++)
            {
                new_loc = GetPhyAddress(temp_addr);
                col_id = new_loc.column_ * config_.device_width;
                col_tile_id = row_id / config_.TileRowNum; 
                for (int j = 0; j < config_.device_width; j ++){
                    grid_id_y = col_id / config_.matY + col_tile_id * (config_.numYgrids / config_.RowTile);
                    x = vault_id_x * (bank_x * config_.numXgrids) + bank_id_x * config_.numXgrids + grid_id_x;
                    y = vault_id_y * (bank_y * config_.numYgrids) + bank_id_y * config_.numYgrids + grid_id_y; 

                    accu_Pmap[z * (dimX * dimY) + y * dimX + x][caseID_] += add_energy / config_.device_width;
                    cur_Pmap[z * (dimX * dimY) + y * dimX + x][caseID_] += add_energy / config_.device_width;
                    col_id ++;
                }
                
                temp_addr.column_ ++; 
            }
        }
        else
        {
            z = 0;
            if (config_.bankgroups > 1)
            {
                int bank_group_id = bank_id / config_.banks_per_group; 
                int sub_bank_id = bank_id % config_.banks_per_group; 
                bank_id_y = sub_bank_id / 2; 
                bank_id_x = sub_bank_id % 2; 
                if (bank_x <= bank_y){
                    bank_id_y += bank_group_id * 2; 
                }
                else{
                    bank_id_x += bank_group_id * 2; 
                }
            }
            else
            {
                bank_id_y = bank_id / bank_x;
                bank_id_x = bank_id % bank_x;
            }
            grid_id_x = row_id / config_.matX / config_.RowTile; 


            for (int i = 0; i < config_.BL; i ++)
            {
                new_loc = GetPhyAddress(temp_addr);
                col_id = new_loc.column_ * config_.device_width;
                col_tile_id = row_id / config_.TileRowNum; 
                for (int j = 0; j < config_.device_width; j ++){
                    grid_id_y = col_id / config_.matY + col_tile_id * (config_.numYgrids / config_.RowTile);
                    x = bank_id_x * config_.numXgrids + grid_id_x;
                    y = bank_id_y * config_.numYgrids + grid_id_y;

                    accu_Pmap[z * (dimX * dimY) + y * dimX + x][caseID_] += add_energy / config_.device_width;
                    cur_Pmap[z * (dimX * dimY) + y * dimX + x][caseID_] += add_energy / config_.device_width;
                    col_id ++;
                }
                
                temp_addr.column_ ++; 
            }
        }
    }
}


void ThermalCalculator::LocationMappingANDaddEnergy_RF(const Command &cmd, int bank0, int row0, int caseID_, double add_energy)
{
    int vault_id, bank_id, row_id;
    int col_id = 0;
    int vault_id_x, vault_id_y, bank_id_x, bank_id_y, grid_id_x, grid_id_y;
    int x, y, z;
    int col_tile_id;

    // remap the row (if set in ini file) so that they don't concentrate in one area
    Address temp_addr = Address(cmd.addr_);
    temp_addr.row_ = row0;
    temp_addr.bank_ = bank0;
    temp_addr = GetPhyAddress(temp_addr);
    row_id = temp_addr.row_;
    bank_id = bank0;

    if (config_.bank_order == 1)
    {
        // y-direction priority
        
        if (config_.IsHMC() || config_.IsHBM())
        {
            if (config_.IsHMC())
            {
                vault_id = cmd.Channel();
                vault_id_x = vault_id / vault_y;
                vault_id_y = vault_id % vault_y;
                int num_bank_per_layer = config_.banks / config_.num_dies;
                if (config_.bank_layer_order == 0)
                    z = bank_id / num_bank_per_layer;
                else
                    z = numP - bank_id / num_bank_per_layer - 1; 
                int bank_same_layer = bank_id % num_bank_per_layer;
                bank_id_x = bank_same_layer / bank_y;
                bank_id_y = bank_same_layer % bank_y;
            }
            else
            {
                vault_id = cmd.Channel(); 
                int bank_group_id = bank_id / config_.banks_per_group; 
                int sub_bank_id = bank_id % config_.banks_per_group; 
                bank_id_x = bank_group_id * 2 + sub_bank_id / 2; 
                bank_id_y = sub_bank_id % 2; 

                z = vault_id / 2; // each layer has two channels (vaults)
                vault_id_y = vault_id % 2; 
                vault_id_x = 0; 
            }
            grid_id_x = row_id / config_.matX / config_.RowTile; 
            col_tile_id = row_id / config_.TileRowNum; 
            grid_id_y = col_id / config_.matY + col_tile_id * (config_.numYgrids / config_.RowTile);
            x = vault_id_x * (bank_x * config_.numXgrids) + bank_id_x * config_.numXgrids + grid_id_x;
            y = vault_id_y * (bank_y * config_.numYgrids) + bank_id_y * config_.numYgrids + grid_id_y; 

        }
        else
        {
            z = 0;
            if (config_.bankgroups > 1)
            {
                int bank_group_id = bank_id / config_.banks_per_group; 
                int sub_bank_id = bank_id % config_.banks_per_group; 
                bank_id_x = sub_bank_id / 2; 
                bank_id_y = sub_bank_id % 2; 
                if (bank_x <= bank_y)
                    bank_id_y += bank_group_id * 2; 
                else
                    bank_id_x += bank_group_id * 2; 
            }
            else
            {
                bank_id_x = bank_id / bank_y;
                bank_id_y = bank_id % bank_y;
            }
            
            grid_id_x = row_id / config_.matX / config_.RowTile; 
            col_tile_id = row_id / config_.TileRowNum; 
            grid_id_y = col_id / config_.matY + col_tile_id * (config_.numYgrids / config_.RowTile);
            x = bank_id_x * config_.numXgrids + grid_id_x;
            y = bank_id_y * config_.numYgrids + grid_id_y;
        }

    }
    else
    {
        // x-direction priority
        if (config_.IsHMC() || config_.IsHBM())
        {
            if (config_.IsHMC())
            {
                vault_id = cmd.Channel();
                vault_id_y = vault_id / vault_x;
                vault_id_x = vault_id % vault_x;
                int num_bank_per_layer = config_.banks / config_.num_dies;
                if (config_.bank_layer_order == 0)
                    z = bank_id / num_bank_per_layer;
                else
                    z = numP - bank_id / num_bank_per_layer - 1; 
                int bank_same_layer = bank_id % num_bank_per_layer;
                bank_id_y = bank_same_layer / bank_x;
                bank_id_x = bank_same_layer % bank_x;
            }
            else
            {
                vault_id = cmd.Channel(); 
                int bank_group_id = bank_id / config_.banks_per_group; 
                int sub_bank_id = bank_id % config_.banks_per_group; 
                bank_id_x = bank_group_id * 2 + sub_bank_id / 2; 
                bank_id_y = sub_bank_id % 2; 

                z = vault_id / 2; // each layer has two channels (vaults)
                vault_id_y = vault_id % 2; 
                vault_id_x = 0; 
            }
            grid_id_x = row_id / config_.matX / config_.RowTile; 
            col_tile_id = row_id / config_.TileRowNum; 
            grid_id_y = col_id / config_.matY + col_tile_id * (config_.numYgrids / config_.RowTile);
            x = vault_id_x * (bank_x * config_.numXgrids) + bank_id_x * config_.numXgrids + grid_id_x;
            y = vault_id_y * (bank_y * config_.numYgrids) + bank_id_y * config_.numYgrids + grid_id_y; 

        }
        else
        {
            z = 0;
            if (config_.bankgroups > 1)
            {
                int bank_group_id = bank_id / config_.banks_per_group; 
                int sub_bank_id = bank_id % config_.banks_per_group; 
                bank_id_y = sub_bank_id / 2; 
                bank_id_x = sub_bank_id % 2; 
                if (bank_x <= bank_y){
                    bank_id_y += bank_group_id * 2; 
                }
                else{
                    bank_id_x += bank_group_id * 2; 
                }
            }
            else
            {
                bank_id_y = bank_id / bank_x;
                bank_id_x = bank_id % bank_x;
            }
            grid_id_x = row_id / config_.matX / config_.RowTile; 
            col_tile_id = row_id / config_.TileRowNum; 
            grid_id_y = col_id / config_.matY + col_tile_id * (config_.numYgrids / config_.RowTile);
            x = bank_id_x * config_.numXgrids + grid_id_x;
            y = bank_id_y * config_.numYgrids + grid_id_y;
        }
    }

    // y is the baseline y
    for (int i = 0; i < config_.numYgrids; i ++)
    {
        accu_Pmap[z * (dimX * dimY) + y * dimX + x][caseID_] += add_energy;
        cur_Pmap[z * (dimX * dimY) + y * dimX + x][caseID_] += add_energy;
        y ++;
    }

}





void ThermalCalculator::UpdatePower(const Command &cmd, uint64_t clk)
{
    //cout << "Enter UpdatePower()\n";
    double energy = 0.0;
    int row_s, ir, ib; // for refresh
    int case_id;
    double device_scale;

    uint32_t rank = cmd.Rank();
    uint32_t channel = cmd.Channel();

    if (config_.IsHMC() || config_.IsHBM()){
        device_scale = 1; 
        case_id = 0;
    }
    else{
        device_scale = (double)config_.devices_per_rank;
        case_id = channel * config_.ranks + rank;
    }

    save_clk = clk;

    if (cmd.cmd_type_ == CommandType::REFRESH)
    {
        for (ib = 0; ib < config_.banks; ib ++){
            row_s = refresh_count[channel * config_.ranks + rank][ib] * config_.numRowRefresh;
            refresh_count[channel * config_.ranks + rank][ib] ++; 
            if (refresh_count[channel * config_.ranks + rank][ib] * config_.numRowRefresh == config_.rows)
                refresh_count[channel * config_.ranks + rank][ib] = 0;
            energy = config_.ref_energy_inc / config_.numRowRefresh / config_.banks / config_.numYgrids; 
            for (ir = row_s; ir < row_s + config_.numRowRefresh; ir ++){
                LocationMappingANDaddEnergy_RF(cmd, ib, ir, case_id, energy / 1000.0 / device_scale);
            }
        }
    }
    else if (cmd.cmd_type_ == CommandType::REFRESH_BANK)
    {
        ib = cmd.Bank(); 
        row_s = refresh_count[channel * config_.ranks + rank][ib] * config_.numRowRefresh;
        refresh_count[channel * config_.ranks + rank][ib] ++; 
        if (refresh_count[channel * config_.ranks + rank][ib] * config_.numRowRefresh == config_.rows)
            refresh_count[channel * config_.ranks + rank][ib] = 0;
        energy = config_.refb_energy_inc / config_.numRowRefresh / config_.numYgrids; 
        for (ir = row_s; ir < row_s + config_.numRowRefresh; ir ++){
            LocationMappingANDaddEnergy_RF(cmd, ib, ir, case_id, energy / 1000.0 / device_scale);
        }
    }
    else
    {
        switch (cmd.cmd_type_)
        {
        case CommandType::ACTIVATE:
            //cout << "ACTIVATE\n";
            energy = config_.act_energy_inc;
            break;
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
            //cout << "READ\n";
            energy = config_.read_energy_inc;
            break;
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
            //cout << "WRITE\n";
            energy = config_.write_energy_inc;
            break;
        default:
            energy = 0.0;
            //cout << "Error: CommandType is " << cmd.cmd_type_ << endl;
            break;
        }
        if (energy > 0){
            energy /= config_.BL;
            LocationMappingANDaddEnergy(cmd, -1, -1, case_id, energy / 1000.0 / device_scale);
        }
    }


    // print transient power and temperature
    if (clk > (sample_id + 1) * config_.power_epoch_period)
    {
        cout << "begin sampling!\n";
        // add the background energy
        if (config_.IsHMC() || config_.IsHBM()){
            double pre_stb_sum = Statistics::Stats2DCumuSum(stats_.pre_stb_energy);
            double act_stb_sum = Statistics::Stats2DCumuSum(stats_.act_stb_energy);
            double pre_pd_sum = Statistics::Stats2DCumuSum(stats_.pre_pd_energy);
            double sref_sum = Statistics::Stats2DCumuSum(stats_.sref_energy);
            double extra_energy = sref_sum + pre_stb_sum + act_stb_sum + pre_pd_sum - sref_energy_prev[0] - pre_stb_energy_prev[0] - act_stb_energy_prev[0] - pre_pd_energy_prev[0];
            extra_energy = extra_energy / (dimX * dimY * numP);
            sref_energy_prev[0] = sref_sum;
            pre_stb_energy_prev[0] = pre_stb_sum;
            act_stb_energy_prev[0] = act_stb_sum;
            pre_pd_energy_prev[0] = pre_pd_sum;

            for (int i = 0; i < dimX * dimY * numP; i++)
                for (int j = 0; j < num_case; j++)
                    cur_Pmap[i][j] += extra_energy / 1000 / device_scale;
        }
        else{
            int case_id;  
            for (int jch = 0; jch < config_.channels; jch ++){
                for (int jrk = 0; jrk < config_.ranks; jrk ++){
                    case_id = jch*config_.ranks+jrk; // case_id is determined by the channel and rank index 
                    double extra_energy = stats_.sref_energy[jch][jrk].cumulative_value + stats_.pre_stb_energy[jch][jrk].cumulative_value + stats_.act_stb_energy[jch][jrk].cumulative_value + stats_.pre_pd_energy[jch][jrk].cumulative_value - sref_energy_prev[case_id] - pre_stb_energy_prev[case_id] - act_stb_energy_prev[case_id] - pre_pd_energy_prev[case_id];
                    extra_energy = extra_energy / (dimX * dimY * numP);
                    sref_energy_prev[case_id] = stats_.sref_energy[jch][jrk].cumulative_value;
                    pre_stb_energy_prev[case_id] = stats_.pre_stb_energy[jch][jrk].cumulative_value;
                    act_stb_energy_prev[case_id] = stats_.act_stb_energy[jch][jrk].cumulative_value;
                    pre_pd_energy_prev[case_id] = stats_.pre_pd_energy[jch][jrk].cumulative_value;
                    for (int i = 0; i < dimX * dimY * numP; i ++){
                        cur_Pmap[i][case_id] += extra_energy / 1000 / device_scale; 
                    }
                }
            }
        }
        
        PrintTransPT(clk);
        cur_Pmap = vector<vector<double>>(numP * dimX * dimY, vector<double>(num_case, 0));
        sample_id++;
    }
}

void ThermalCalculator::PrintTransPT(uint64_t clk)
{
    cout << "============== At " << clk * config_.tCK * 1e-6 << "[ms] =============\n";
    for (int ir = 0; ir < num_case; ir++)
    {
        CalcTransT(ir);
        double maxT = GetMaxT(T_trans, ir);
        cout << "MaxT of case " << ir << " is " << maxT - T0 << " [C]\n";
        
        // write MaxT of the epoch to save time & space
        epoch_max_temp_file_csv_ << "-,-,-,-," << maxT - T0 << "," << ir << endl;
        // only outputs full file when output level >= 2
        if (config_.output_level >= 2) {
            PrintCSV_trans(epoch_temperature_file_csv_, cur_Pmap, T_trans, ir, config_.power_epoch_period);
        }
    }
}

void ThermalCalculator::PrintFinalPT(uint64_t clk)
{
    double device_scale = (double)config_.devices_per_rank;

    if (config_.IsHMC() || config_.IsHBM())
        device_scale = 1;

    
    // first add the background energy
    if (config_.IsHMC() || config_.IsHBM()){
        double extra_energy = (Statistics::Stats2DCumuSum(stats_.act_stb_energy) + Statistics::Stats2DCumuSum(stats_.pre_stb_energy) + Statistics::Stats2DCumuSum(stats_.sref_energy) + Statistics::Stats2DCumuSum(stats_.pre_pd_energy)) / (dimX * dimY * numP);
        cout << "background energy " << extra_energy * (dimX * dimY * numP) << endl;
        for (int i = 0; i < dimX * dimY * numP; i++)
            for (int j = 0; j < num_case; j++)
                accu_Pmap[i][j] += extra_energy / 1000 / device_scale;
    }
    else{
        double extra_energy; 
        int case_id; 
        for (int jch = 0; jch < config_.channels; jch ++){
            for (int jrk = 0; jrk < config_.ranks; jrk ++){
                case_id = jch*config_.ranks+jrk; // case_id is determined by the channel and rank index 
                extra_energy = (stats_.sref_energy[jch][jrk].cumulative_value + stats_.pre_stb_energy[jch][jrk].cumulative_value + stats_.act_stb_energy[jch][jrk].cumulative_value + stats_.pre_pd_energy[jch][jrk].cumulative_value) / (dimX * dimY * numP);
                cout << "background energy " << extra_energy * (dimX * dimY * numP) << endl;
                double sum_energy = 0.0;
                for (int i = 0; i < dimX * dimY * numP; i++) {
                    sum_energy += accu_Pmap[i][case_id];
                }
                cout << "other energy " << sum_energy * 1000.0 * device_scale << endl;
                for (int i = 0; i < dimX * dimY * numP; i ++){
                    accu_Pmap[i][case_id] += extra_energy / 1000 / device_scale; 
                }
            }
        }
    }
    
    
    // calculate the final temperature for each case
    for (int ir = 0; ir < num_case; ir++)
    {
        CalcFinalT(ir, clk);
        double maxT = GetMaxT(T_final, ir);
        cout << "MaxT of case " << ir << " is " << maxT << " [C]\n";
        // print to file
        PrintCSV_final(final_temperature_file_csv_, accu_Pmap, T_final, ir, clk);
    }

    // close all the csv files
    final_temperature_file_csv_.close();
    
    epoch_max_temp_file_csv_.close();

    if (config_.output_level >= 2) {
        epoch_temperature_file_csv_.close();
    }
}

void ThermalCalculator::CalcTransT(int case_id)
{
    double ***powerM;
    double *T;
    double time = config_.power_epoch_period * config_.tCK * 1e-9;
    int i, j, l;

    // assert in powerM
    if (!(powerM = (double ***)malloc((dimX+num_dummy) * sizeof(double **))))
        printf("Malloc fails for powerM[].\n");
    for (i = 0; i < dimX+num_dummy; i++)
    {
        if (!(powerM[i] = (double **)malloc((dimY+num_dummy) * sizeof(double *))))
            printf("Malloc fails for powerM[%d][].\n", i);
        for (j = 0; j < dimY+num_dummy; j++)
        {
            if (!(powerM[i][j] = (double *)malloc(numP * sizeof(double))))
                printf("Malloc fails for powerM[%d][%d][].\n", i, j);
        }
    }
    // initialize powerM
    for (i = 0; i < dimX+num_dummy; i ++)
        for (j = 0; j < dimY+num_dummy; j++)
            for (l = 0; l < numP; l ++)
                powerM[i][j][l] = 0;

    // fill in powerM
    double totP = 0.0;
    for (i = 0; i < dimX; i++)
    {
        for (j = 0; j < dimY; j++)
        {
            for (l = 0; l < numP; l++)
            {
                powerM[i+num_dummy/2][j+num_dummy/2][l] = cur_Pmap[l * (dimX * dimY) + j * dimX + i][case_id] / (double)config_.power_epoch_period;
                totP += powerM[i+num_dummy/2][j+num_dummy/2][l];
            }
        }
    }
    cout << "total Power is " << totP * 1000 << " [mW]\n";

    // fill in T
    if (!(T = (double *)malloc(((numP * 3 + 1) * (dimX+num_dummy) * (dimY+num_dummy)) * sizeof(double))))
        printf("Malloc fails for T.\n");
    for (i = 0; i < T_trans.size(); i++)
        T[i] = T_trans[i][case_id];

    T = transient_thermal_solver(powerM, config_.ChipX, config_.ChipY, numP, dimX+num_dummy, dimY+num_dummy, Midx, MidxSize, Cap, CapSize, time, time_iter, T, Tamb);

    for (i = 0; i < T_trans.size(); i++)
        T_trans[i][case_id] = T[i];

    free(T);
}

void ThermalCalculator::CalcFinalT(int case_id, uint64_t clk)
{
    double ***powerM;
    int i, j, l;
    // assert in powerM
    if (!(powerM = (double ***)malloc((dimX+num_dummy) * sizeof(double **))))
        printf("Malloc fails for powerM[].\n");
    for (i = 0; i < dimX+num_dummy; i++)
    {
        if (!(powerM[i] = (double **)malloc((dimY+num_dummy) * sizeof(double *))))
            printf("Malloc fails for powerM[%d][].\n", i);
        for (j = 0; j < dimY+num_dummy; j++)
        {
            if (!(powerM[i][j] = (double *)malloc(numP * sizeof(double))))
                printf("Malloc fails for powerM[%d][%d][].\n", i, j);
        }
    }

    // initialize powerM
    for (i = 0; i < dimX+num_dummy; i ++)
        for (j = 0; j < dimY+num_dummy; j++)
            for (l = 0; l < numP; l ++)
                powerM[i][j][l] = 0;
    // fill in powerM
    double totP = 0.0;
    for (i = 0; i < dimX; i++)
    {
        for (j = 0; j < dimY; j++)
        {
            for (l = 0; l < numP; l++)
            {
                powerM[i+num_dummy/2][j+num_dummy/2][l] = accu_Pmap[l * (dimX * dimY) + j * dimX + i][case_id] / (double)clk;
                totP += powerM[i+num_dummy/2][j+num_dummy/2][l];
                //cout << "powerM[" << i << "][" << j << "][" << l << "] = " << powerM[i][j][l] << endl;
            }
        }
    }

    cout << "total Power is " << totP * 1000 << " [mW]\n";

    double *T;
    T = steady_thermal_solver(powerM, config_.ChipX, config_.ChipY, numP, dimX+num_dummy, dimY+num_dummy, Midx, MidxSize, Tamb);

    // assign the value to T_final
    for (int i = 0; i < T_final.size(); i++)
    {
        T_final[i][case_id] = T[i];
        //cout << "T_final[" << i << "][" << case_id << "] = " << T_final[i][case_id] << endl;
    }

    free(T);
}

void ThermalCalculator::InitialParameters()
{
    layerP = vector<int>(numP, 0);
    for (int l = 0; l < numP; l++)
        layerP[l] = l * 3;
    Midx = calculate_Midx_array(config_.ChipX, config_.ChipY, numP, dimX+num_dummy, dimY+num_dummy, &MidxSize, Tamb);
    Cap = calculate_Cap_array(config_.ChipX, config_.ChipY, numP, dimX+num_dummy, dimY+num_dummy, &CapSize);
    calculate_time_step();

    double *T;
    for (int ir = 0; ir < num_case; ir++)
    {
        T = initialize_Temperature(config_.ChipX, config_.ChipY, numP, dimX+num_dummy, dimY+num_dummy, Tamb);
        for (int i = 0; i < T_trans.size(); i++)
            T_trans[i][ir] = T[i];
    }
    free(T);
}

int ThermalCalculator::square_array(int total_grids_)
{
    int x, y, x_re = 1;
    for (x = 1; x <= sqrt(total_grids_); x++)
    {
        y = total_grids_ / x;
        if (x * y == total_grids_)
            x_re = x;
    }
    return x_re;
}

int ThermalCalculator::determineXY(double xd, double yd, int total_grids_)
{
    int x, y, x_re = 1; 
    double asr, asr_re = 1000; 
    for (y = 1; y <= total_grids_; y ++)
    {
        x = total_grids_ / y; 
        if (x * y == total_grids_)
        {
            // total_grids_ can be factored by x and y
            asr = (x*xd >= y*yd) ? (x*xd/y/yd) : (y*yd/x/xd); 
            if (asr < asr_re)
            {
                cout << "asr = " << asr << "; x = " << x << "; y = " << y << "; xd = " << xd << endl;
                x_re = total_grids_ / y;   
                asr_re = asr; 
            }
        }
    }
    return x_re;
}


void ThermalCalculator::calculate_time_step()
{
    double dt = 100.0;
    int layer_dim = (dimX+num_dummy) * (dimY + num_dummy);

    for (int j = 0; j < MidxSize; j++)
    {
        int idx0 = (int)(Midx[j][0] + 0.01);
        int idx1 = (int)(Midx[j][1] + 0.01);
        int idxC = idx0 / layer_dim;

        if (idx0 == idx1)
        {
            double g = Midx[j][2];
            double c = Cap[idxC];
            if (c / g < dt)
                dt = c / g;
        }
    }

    cout << "maximum dt is " << dt << endl;

    // calculate time_iter
    double power_epoch_time = config_.power_epoch_period * config_.tCK * 1e-9; // [s]
    cout << "power_epoch_time = " << power_epoch_time << endl;
    time_iter = time_iter0;
    while (power_epoch_time / time_iter >= dt)
        time_iter++;
    //time_iter += 10;
    cout << "time_iter = " << time_iter << endl;
}

double ThermalCalculator::GetMaxT(vector<vector<double>> T_, int case_id)
{
    double maxT = 0;
    for (int i = 0; i < T_.size(); i++)
    {
        ///cout << T_[i][case_id] << endl;
        if (T_[i][case_id] > maxT)
        {
            maxT = T_[i][case_id];
        }
    }
    return maxT;
}

void ThermalCalculator::PrintCSV_trans(ofstream &csvfile, vector<vector<double>> P_, vector<vector<double>> T_, int id, uint64_t scale)
{
    for (int l = 0; l < numP; l++)
    {
        for (int j = 0; j < dimY+num_dummy; j++)
        {
            //cout << "j = " << j << endl; 
            if (j < num_dummy/2 || j >= dimY+num_dummy/2)
                continue; 
            for (int i = 0; i < dimX+num_dummy; i++)
            { 
                //cout << "\ti = " << i << "; (dimX, dimY) = (" << dimX << "," << dimY << ")\n";
                if (i < num_dummy/2 || i >= dimX+num_dummy/2)
                    continue; 
                double pw = P_[l * ((dimX) * (dimY)) + (j-num_dummy/2) * (dimX) + (i-num_dummy/2)][id] / (double)scale;
                double tm = T_[(layerP[l] + 1) * ((dimX+num_dummy) * (dimY+num_dummy)) + j * (dimX+num_dummy) + i][id] - T0;
                csvfile << id << "," << i-num_dummy/2 << "," << j-num_dummy/2 << "," << l << "," << pw << "," << tm << "," << sample_id << endl;
            }
        }
    }
}

void ThermalCalculator::PrintCSV_final(ofstream &csvfile, vector<vector<double>> P_, vector<vector<double>> T_, int id, uint64_t scale)
{
    for (int l = 0; l < numP; l++)
    {
        for (int j = 0; j < dimY+num_dummy; j++)
        {
            if (j < num_dummy/2 || j >= dimY+num_dummy/2)
                continue; 
            for (int i = 0; i < dimX+num_dummy; i++)
            {
                if (i < num_dummy/2 || i >= dimX + num_dummy/2)
                    continue; 
                double pw = P_[l * (dimX * dimY) + (j-num_dummy/2) * dimX + (i-num_dummy/2)][id] / (double)scale;
                double tm = T_[(layerP[l] + 1) * ((dimX+num_dummy) * (dimY+num_dummy)) + j * (dimX+num_dummy) + i][id];
                csvfile << id << "," << i-num_dummy/2 << "," << j-num_dummy/2 << "," << l << "," << pw << "," << tm << endl;
            }
        }
    }
}

void ThermalCalculator::PrintCSV_bank(ofstream &csvfile)
{
    
    int bank_id, bank_same_layer, bank_id_x, bank_id_y; 
    int bank_group_id, sub_bank_id;
    int vault_id, vault_id_x, vault_id_y, num_bank_per_layer; 
    int start_x, end_x, start_y, end_y, z;
    if (config_.bank_order == 1)
    {
        
        if (config_.IsHMC() || config_.IsHBM())
        {
            csvfile << "vault_id,bank_id,start_x,end_x,start_y,end_y,z" << endl;
            if (config_.IsHMC())
            {
                for (vault_id = 0; vault_id < config_.channels; vault_id ++){
                    vault_id_x = vault_id / vault_y;
                    vault_id_y = vault_id % vault_y;
                    num_bank_per_layer = config_.banks / config_.num_dies;
                    for (bank_id = 0; bank_id < config_.banks; bank_id ++){
                        if (config_.bank_layer_order == 0)
                            z = bank_id / num_bank_per_layer;
                        else
                            z = numP - bank_id / num_bank_per_layer - 1; 
                        bank_same_layer = bank_id % num_bank_per_layer;
                        bank_id_x = bank_same_layer / bank_y;
                        bank_id_y = bank_same_layer % bank_y;

                        start_x = vault_id_x * (bank_x * config_.numXgrids) + bank_id_x * config_.numXgrids; 
                        end_x = vault_id_x * (bank_x * config_.numXgrids) + (bank_id_x + 1) * config_.numXgrids - 1;
                        start_y =  vault_id_y * (bank_y * config_.numYgrids) + bank_id_y * config_.numYgrids;
                        end_y = vault_id_y * (bank_y * config_.numYgrids) + (bank_id_y + 1) * config_.numYgrids - 1;
                        csvfile << vault_id << "," << bank_id << "," << start_x << "," << end_x << "," << start_y << "," << end_y << "," << z << endl;
                    }
                }
            }
            else
            {
                for (vault_id = 0; vault_id < config_.channels; vault_id ++){
                    z = vault_id / 2; // each layer has two channels (vaults)
                    vault_id_y = vault_id % 2; 
                    vault_id_x = 0; 
                    for (bank_id = 0; bank_id < config_.banks; bank_id ++){
                        bank_group_id = bank_id / config_.banks_per_group; 
                        sub_bank_id = bank_id % config_.banks_per_group; 
                        bank_id_x = bank_group_id * 2 + sub_bank_id / 2; 
                        bank_id_y = sub_bank_id % 2;

                        start_x = vault_id_x * (bank_x * config_.numXgrids) + bank_id_x * config_.numXgrids; 
                        end_x = vault_id_x * (bank_x * config_.numXgrids) + (bank_id_x + 1) * config_.numXgrids - 1;
                        start_y =  vault_id_y * (bank_y * config_.numYgrids) + bank_id_y * config_.numYgrids;
                        end_y = vault_id_y * (bank_y * config_.numYgrids) + (bank_id_y + 1) * config_.numYgrids - 1;
                        csvfile << vault_id << "," << bank_id << "," << start_x << "," << end_x << "," << start_y << "," << end_y << "," << z << endl;
                    }  
                }
            }
            

        }
        else
        {
            csvfile << "vault_id,bank_id,start_x,end_x,start_y,end_y,z" << endl;
            z = 0;
            for (bank_id = 0; bank_id < config_.banks; bank_id ++){
                if (config_.bankgroups > 1)
                {
                    bank_group_id = bank_id / config_.banks_per_group; 
                    sub_bank_id = bank_id % config_.banks_per_group; 
                    bank_id_x = sub_bank_id / 2; 
                    bank_id_y = sub_bank_id % 2; 
                    if (bank_x <= bank_y)
                        bank_id_y += bank_group_id * 2; 
                    else
                        bank_id_x += bank_group_id * 2; 
                }
                else
                {
                    bank_id_x = bank_id / bank_y;
                    bank_id_y = bank_id % bank_y;
                }

                start_x = bank_id_x * config_.numXgrids; 
                end_x = (bank_id_x + 1) * config_.numXgrids - 1; 
                start_y = bank_id_y * config_.numYgrids;
                end_y = (bank_id_y + 1) * config_.numYgrids - 1;
                csvfile << "0," << bank_id << "," << start_x << "," << end_x << "," << start_y << "," << end_y << "," << z << endl;
            }
        }

    }
    else
    {
        // x-direction priority
        if (config_.IsHMC() || config_.IsHBM())
        {
            csvfile << "vault_id,bank_id,start_x,end_x,start_y,end_y,z" << endl;
            if (config_.IsHMC())
            {
                for (vault_id = 0; vault_id < config_.channels; vault_id ++){
                    vault_id_y = vault_id / vault_x;
                    vault_id_x = vault_id % vault_x;
                    num_bank_per_layer = config_.banks / config_.num_dies;
                    for (bank_id = 0; bank_id < config_.banks; bank_id ++){
                        if (config_.bank_layer_order == 0)
                            z = bank_id / num_bank_per_layer;
                        else
                            z = numP - bank_id / num_bank_per_layer - 1; 
                        bank_same_layer = bank_id % num_bank_per_layer;
                        bank_id_y = bank_same_layer / bank_x;
                        bank_id_x = bank_same_layer % bank_x;

                        start_x = vault_id_x * (bank_x * config_.numXgrids) + bank_id_x * config_.numXgrids; 
                        end_x = vault_id_x * (bank_x * config_.numXgrids) + (bank_id_x + 1) * config_.numXgrids - 1;
                        start_y =  vault_id_y * (bank_y * config_.numYgrids) + bank_id_y * config_.numYgrids;
                        end_y = vault_id_y * (bank_y * config_.numYgrids) + (bank_id_y + 1) * config_.numYgrids - 1;
                        csvfile << vault_id << "," << bank_id << "," << start_x << "," << end_x << "," << start_y << "," << end_y << "," << z << endl;
                    }
                }
            }
            else
            {
                for (vault_id = 0; vault_id < config_.channels; vault_id ++){
                    z = vault_id / 2; // each layer has two channels (vaults)
                    vault_id_y = vault_id % 2; 
                    vault_id_x = 0; 
                    for (bank_id = 0; bank_id < config_.banks; bank_id ++){
                        bank_group_id = bank_id / config_.banks_per_group; 
                        sub_bank_id = bank_id % config_.banks_per_group; 
                        bank_id_x = bank_group_id * 2 + sub_bank_id / 2; 
                        bank_id_y = sub_bank_id % 2;

                        start_x = vault_id_x * (bank_x * config_.numXgrids) + bank_id_x * config_.numXgrids; 
                        end_x = vault_id_x * (bank_x * config_.numXgrids) + (bank_id_x + 1) * config_.numXgrids - 1;
                        start_y =  vault_id_y * (bank_y * config_.numYgrids) + bank_id_y * config_.numYgrids;
                        end_y = vault_id_y * (bank_y * config_.numYgrids) + (bank_id_y + 1) * config_.numYgrids - 1;
                        csvfile << vault_id << "," << bank_id << "," << start_x << "," << end_x << "," << start_y << "," << end_y << "," << z << endl;
                    }  
                }
            }
        }
        else
        {
            csvfile << "vault_id,bank_id,start_x,end_x,start_y,end_y,z" << endl;
            z = 0;
            for (bank_id = 0; bank_id < config_.banks; bank_id ++){
                if (config_.bankgroups > 1)
                {
                    bank_group_id = bank_id / config_.banks_per_group; 
                    sub_bank_id = bank_id % config_.banks_per_group; 
                    bank_id_y = sub_bank_id / 2; 
                    bank_id_x = sub_bank_id % 2; 
                    if (bank_x <= bank_y){
                        bank_id_y += bank_group_id * 2; 
                    }
                    else{
                        bank_id_x += bank_group_id * 2; 
                    }
                }
                else
                {
                    bank_id_y = bank_id / bank_x;
                    bank_id_x = bank_id % bank_x;
                }

                start_x = bank_id_x * config_.numXgrids; 
                end_x = (bank_id_x + 1) * config_.numXgrids - 1; 
                start_y = bank_id_y * config_.numYgrids;
                end_y = (bank_id_y + 1) * config_.numYgrids - 1;
                csvfile << "0," << bank_id << "," << start_x << "," << end_x << "," << start_y << "," << end_y << "," << z << endl;
            }
        }
    }
}


void ThermalCalculator::PrintCSVHeader_trans(ofstream &csvfile)
{
    csvfile << "rank_channel_index,x,y,z,power,temperature,epoch" << endl;
}

void ThermalCalculator::PrintCSVHeader_final(ofstream &csvfile)
{
    csvfile << "rank_channel_index,x,y,z,power,temperature" << endl;
}
