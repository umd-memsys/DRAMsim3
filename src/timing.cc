#include "timing.h"
#include <utility>

using namespace std;
using namespace dramcore;

Timing::Timing(const Config& config) :
    same_bank(static_cast<int>(CommandType::SIZE)),
    other_banks_same_bankgroup(static_cast<int>(CommandType::SIZE)),
    other_bankgroups_same_rank(static_cast<int>(CommandType::SIZE)),
    other_ranks(static_cast<int>(CommandType::SIZE)),
    same_rank(static_cast<int>(CommandType::SIZE))
{
    uint32_t read_to_read_l = std::max(config.burst_cycle, config.tCCD_L);
    uint32_t read_to_read_s = std::max(config.burst_cycle, config.tCCD_S);
    uint32_t read_to_read_o = config.burst_cycle + config.tRTRS;
    uint32_t read_to_write = config.RL + config.burst_cycle - config.WL + config.tRPRE + config.tRTRS;  // refer page 94 of DDR4 spec
    uint32_t read_to_write_o = config.read_delay + config.burst_cycle + config.tRTRS - config.write_delay;
    uint32_t read_to_precharge = config.AL + config.tRTP;
    uint32_t readp_to_act = config.AL + config.burst_cycle + config.tRTP + config.tRP;
    
    uint32_t write_to_read_l = config.write_delay + config.tWTR_L;
    uint32_t write_to_read_s = config.write_delay + config.tWTR_S;
    uint32_t write_to_read_o = config.write_delay + config.burst_cycle + config.tRTRS - config.read_delay;
    uint32_t write_to_write_l = std::max(config.burst_cycle, config.tCCD_L);
    uint32_t write_to_write_s = std::max(config.burst_cycle, config.tCCD_S);
    uint32_t write_to_write_o = config.burst_cycle + config.tWPRE; 
    uint32_t write_to_precharge = config.WL + config.burst_cycle + config.tWR;

    uint32_t precharge_to_activate = config.tRP;
    uint32_t precharge_to_precharge = config.tPPD;
    uint32_t read_to_activate = read_to_precharge + precharge_to_activate;
    uint32_t write_to_activate = write_to_precharge + precharge_to_activate;

    uint32_t activate_to_activate = config.tRC;
    uint32_t activate_to_activate_l = config.tRRD_L;
    uint32_t activate_to_activate_s = config.tRRD_S;
    uint32_t activate_to_precharge = config.tRAS;
    uint32_t activate_to_read, activate_to_write;
    if (config.IsGDDR() || config.IsHBM()){
        activate_to_read = config.tRCDRD;
        activate_to_write = config.tRCDWR;   
    } else {
        activate_to_read = config.tRCD - config.AL;
        activate_to_write = config.tRCD - config.AL;
    }
    uint32_t activate_to_refresh = config.tRC;  // need to precharge before ref, so it's tRC

    // TODO: deal with different refresh rate
    uint32_t refresh_to_refresh = config.tREFI;  // refresh intervals (per rank level)
    uint32_t refresh_to_activate = config.tRFC;  // tRFC is defined as ref to act
    uint32_t refresh_to_activate_bank = config.tRFCb;


    uint32_t self_refresh_entry_to_exit = config.tCKESR;
    uint32_t self_refresh_exit = config.tXS;
    uint32_t powerdown_to_exit = config.tCKE;
    uint32_t powerdown_exit = config.tXP;


    if (config.bankgroups == 1) {  
        // for GDDR5 bankgroup can be disabled, in that case 
        // the value of tXXX_S should be used instead of tXXX_L 
        // (because now the device is running at a lower freq)
        // we overwrite the following values so that we don't have 
        // to change the assignement of the vectors
        read_to_read_l = std::max(config.burst_cycle, config.tCCD_S);
        write_to_read_l = config.write_delay + config.tWTR_S;
        write_to_write_l = std::max(config.burst_cycle, config.tCCD_S);
        activate_to_activate_l = config.tRRD_S;
    }

    // command READ
    same_bank[static_cast<int>(CommandType::READ)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, read_to_read_l },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_l },
        { CommandType::WRITE_PRECHARGE, read_to_write },
        { CommandType::PRECHARGE, read_to_precharge } 
    };
    other_banks_same_bankgroup[static_cast<int>(CommandType::READ)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, read_to_read_l },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_l },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_bankgroups_same_rank[static_cast<int>(CommandType::READ)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, read_to_read_s },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_s },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_ranks[static_cast<int>(CommandType::READ)] = std::list < std::pair<CommandType, uint32_t> >
    { 
        { CommandType::READ, read_to_read_o },
        { CommandType::WRITE, read_to_write_o }, 
        { CommandType::READ_PRECHARGE, read_to_read_o },
        { CommandType::WRITE_PRECHARGE, read_to_write_o }
    };

    //command WRITE
    same_bank[static_cast<int>(CommandType::WRITE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, write_to_read_l },
        { CommandType::WRITE, write_to_write_l },
        { CommandType::READ_PRECHARGE, write_to_read_l },
        { CommandType::WRITE_PRECHARGE, write_to_write_l },
        { CommandType::PRECHARGE, write_to_precharge }
    };
    other_banks_same_bankgroup[static_cast<int>(CommandType::WRITE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, write_to_read_l },
        { CommandType::WRITE, write_to_write_l },
        { CommandType::READ_PRECHARGE, write_to_read_l },
        { CommandType::WRITE_PRECHARGE, write_to_write_l }
    };
    other_bankgroups_same_rank[static_cast<int>(CommandType::WRITE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, write_to_read_s },
        { CommandType::WRITE, write_to_write_s },
        { CommandType::READ_PRECHARGE, write_to_read_s },
        { CommandType::WRITE_PRECHARGE, write_to_write_s }
    };
    other_ranks[static_cast<int>(CommandType::WRITE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, write_to_read_o },
        { CommandType::WRITE, write_to_write_o },
        { CommandType::READ_PRECHARGE, write_to_read_o },
        { CommandType::WRITE_PRECHARGE, write_to_write_o }
    };

    //command READ_PRECHARGE
    same_bank[static_cast<int>(CommandType::READ_PRECHARGE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, readp_to_act },
        { CommandType::REFRESH, read_to_activate },
        { CommandType::REFRESH_BANK, read_to_activate },
        { CommandType::SELF_REFRESH_ENTER, read_to_activate }
    };
    other_banks_same_bankgroup[static_cast<int>(CommandType::READ_PRECHARGE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, read_to_read_l },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_l },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_bankgroups_same_rank[static_cast<int>(CommandType::READ_PRECHARGE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, read_to_read_s },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_s },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_ranks[static_cast<int>(CommandType::READ_PRECHARGE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, read_to_read_o },
        { CommandType::WRITE, read_to_write_o }, 
        { CommandType::READ_PRECHARGE, read_to_read_o },
        { CommandType::WRITE_PRECHARGE, read_to_write_o }
    };

    //command WRITE_PRECHARGE
    same_bank[static_cast<int>(CommandType::WRITE_PRECHARGE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, write_to_activate },
        { CommandType::REFRESH, write_to_activate },
        { CommandType::REFRESH_BANK, write_to_activate },
        { CommandType::SELF_REFRESH_ENTER, write_to_activate }
    };
    other_banks_same_bankgroup[static_cast<int>(CommandType::WRITE_PRECHARGE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, write_to_read_l },
        { CommandType::WRITE, write_to_write_l },
        { CommandType::READ_PRECHARGE, write_to_read_l },
        { CommandType::WRITE_PRECHARGE, write_to_write_l }
    };
    other_bankgroups_same_rank[static_cast<int>(CommandType::WRITE_PRECHARGE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, write_to_read_s },
        { CommandType::WRITE, write_to_write_s },
        { CommandType::READ_PRECHARGE, write_to_read_s },
        { CommandType::WRITE_PRECHARGE, write_to_write_s }
    };
    other_ranks[static_cast<int>(CommandType::WRITE_PRECHARGE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::READ, write_to_read_o },
        { CommandType::WRITE, write_to_write_o },
        { CommandType::READ_PRECHARGE, write_to_read_o },
        { CommandType::WRITE_PRECHARGE, write_to_write_o }
    };

    //command ACTIVATE
    same_bank[static_cast<int>(CommandType::ACTIVATE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, activate_to_activate },
        { CommandType::READ, activate_to_read },
        { CommandType::WRITE, activate_to_write },
        { CommandType::READ_PRECHARGE, activate_to_read },
        { CommandType::WRITE_PRECHARGE, activate_to_write },
        { CommandType::PRECHARGE, activate_to_precharge },
    };

    other_banks_same_bankgroup[static_cast<int>(CommandType::ACTIVATE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, activate_to_activate_l },
        { CommandType::REFRESH_BANK, activate_to_refresh }
    };

    other_bankgroups_same_rank[static_cast<int>(CommandType::ACTIVATE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, activate_to_activate_s },
        { CommandType::REFRESH_BANK, activate_to_refresh }
    };

    //command PRECHARGE
    same_bank[static_cast<int>(CommandType::PRECHARGE)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, precharge_to_activate },
        { CommandType::REFRESH, precharge_to_activate },
        { CommandType::REFRESH_BANK, precharge_to_activate },
        { CommandType::SELF_REFRESH_ENTER, precharge_to_activate }
    };

    // for those who need tPPD
    if (config.IsGDDR() or config.protocol == DRAMProtocol::LPDDR4 ) {
        other_banks_same_bankgroup[static_cast<int>(CommandType::PRECHARGE)] = std::list< std::pair<CommandType, uint32_t> >
        {
            { CommandType::PRECHARGE, precharge_to_precharge },
        };

        other_bankgroups_same_rank[static_cast<int>(CommandType::PRECHARGE)] = std::list< std::pair<CommandType, uint32_t> >
        {
            { CommandType::PRECHARGE, precharge_to_precharge },
        };
    }

    //command REFRESH_BANK
    same_rank[static_cast<int>(CommandType::REFRESH_BANK)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, refresh_to_activate_bank },
        { CommandType::REFRESH,  refresh_to_activate_bank },
        { CommandType::REFRESH_BANK, refresh_to_activate_bank },
        { CommandType::SELF_REFRESH_ENTER, refresh_to_activate_bank }
    };

    other_banks_same_bankgroup[static_cast<int>(CommandType::REFRESH_BANK)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, refresh_to_activate },
        { CommandType::REFRESH_BANK, refresh_to_refresh },
    };

    other_bankgroups_same_rank[static_cast<int>(CommandType::REFRESH_BANK)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, refresh_to_activate },
        { CommandType::REFRESH_BANK, refresh_to_refresh },
    };

    //REFRESH, SELF_REFRESH_ENTER and SELF_REFRESH_EXIT are isued to the entire rank
    //command REFRESH
    same_rank[static_cast<int>(CommandType::REFRESH)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, refresh_to_activate },
        { CommandType::REFRESH,  refresh_to_activate },
        { CommandType::SELF_REFRESH_ENTER, refresh_to_activate }
    };

    //command SELF_REFRESH_ENTER
    // TODO: add power down commands
    same_rank[static_cast<int>(CommandType::SELF_REFRESH_ENTER)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::SELF_REFRESH_EXIT,  self_refresh_entry_to_exit}
    };

    //command SELF_REFRESH_EXIT 
    same_rank[static_cast<int>(CommandType::SELF_REFRESH_EXIT)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, self_refresh_exit },
        { CommandType::REFRESH, self_refresh_exit },
        { CommandType::REFRESH_BANK, self_refresh_exit },
        { CommandType::SELF_REFRESH_ENTER, self_refresh_exit }
    };
}
