#include "timing.h"
#include <utility>

using namespace std;
using namespace dramcore;

Timing::Timing(const Config& config) :
    config_(config),
    same_bank(static_cast<int>(CommandType::SIZE)),
    other_banks_same_bankgroup(static_cast<int>(CommandType::SIZE)),
    other_bankgroups_same_rank(static_cast<int>(CommandType::SIZE)),
    other_ranks(static_cast<int>(CommandType::SIZE)),
    same_rank(static_cast<int>(CommandType::SIZE))
{
    read_to_read_l = std::max(config_.burst_cycle, config_.tCCD_L);
    read_to_read_s = std::max(config_.burst_cycle, config_.tCCD_S);
    read_to_read_o = config_.burst_cycle + config_.tRTRS;
    read_to_write = config_.RL + config_.burst_cycle - config_.WL + config_.tRPRE + config_.tRTRS;  // refer page 94 of DDR4 spec
    read_to_write_o = config_.read_delay + config_.burst_cycle + config_.tRTRS - config_.write_delay;
    read_to_precharge = config_.AL + config_.tRTP;
    readp_to_act = config_.AL + config_.burst_cycle + config_.tRTP + config_.tRP;
    
    write_to_read_l = config_.write_delay + config_.tWTR_L;
    write_to_read_s = config_.write_delay + config_.tWTR_S;
    write_to_read_o = config_.write_delay + config_.burst_cycle + config_.tRTRS - config_.read_delay;
    write_to_write_l = std::max(config_.burst_cycle, config_.tCCD_L);
    write_to_write_s = std::max(config_.burst_cycle, config_.tCCD_S);
    write_to_write_o = config_.burst_cycle + config_.tWPRE; 
    write_to_precharge = config_.WL + config_.burst_cycle + config_.tWR;

    precharge_to_activate = config_.tRP;
    precharge_to_precharge = config_.tPPD;
    read_to_activate = read_to_precharge + precharge_to_activate;
    write_to_activate = write_to_precharge + precharge_to_activate;

    activate_to_activate = config_.tRC;
    activate_to_activate_l = config_.tRRD_L;
    activate_to_activate_s = config_.tRRD_S;
    activate_to_precharge = config_.tRAS;
    if (config_.IsGDDR() || config_.IsHBM()){
        activate_to_read = config_.tRCDRD;
        activate_to_write = config_.tRCDWR;   
    } else {
        activate_to_read = config_.tRCD - config_.AL;
        activate_to_write = config_.tRCD - config_.AL;
    }
    activate_to_refresh = config_.tRC;  // need to precharge before ref, so it's tRC

    // TODO the following ref timings need to be fixed
    refresh_to_refresh = config_.tRREFD;
    refresh_to_activate = refresh_to_refresh;

    refresh_cycle = config_.tRFC;

    refresh_cycle_bank = config_.tRFCb;

    self_refresh_entry_to_exit = config_.tCKESR;
    self_refresh_exit = config_.tXS;

    if (config_.bankgroups == 1) {  
        // for GDDR5 bankgroup can be disabled, in that case 
        // the value of tXXX_S should be used instead of tXXX_L 
        // (because now the device is running at a lower freq)
        // we overwrite the following values so that we don't have 
        // to change the assignement of the vectors
        read_to_read_l = std::max(config_.burst_cycle, config_.tCCD_S);
        write_to_read_l = config_.write_delay + config_.tWTR_S;
        write_to_write_l = std::max(config_.burst_cycle, config_.tCCD_S);
        activate_to_activate_l = config_.tRRD_S;
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
    if (config_.IsGDDR() or config_.protocol == DRAMProtocol::LPDDR4 ) {
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
        { CommandType::ACTIVATE, refresh_cycle_bank },
        { CommandType::REFRESH,  refresh_cycle_bank },
        { CommandType::REFRESH_BANK, refresh_cycle_bank },
        { CommandType::SELF_REFRESH_ENTER, refresh_cycle_bank }
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
        { CommandType::ACTIVATE, refresh_cycle },
        { CommandType::REFRESH,  refresh_cycle },
        { CommandType::SELF_REFRESH_ENTER, refresh_cycle }
    };

    //command SELF_REFRESH_ENTER
    same_rank[static_cast<int>(CommandType::SELF_REFRESH_ENTER)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::SELF_REFRESH_EXIT,  self_refresh_entry_to_exit}
    };

    //command SELF_REFRESH_EXIT 
    same_rank[static_cast<int>(CommandType::SELF_REFRESH_EXIT)] = std::list< std::pair<CommandType, uint32_t> >
    {
        { CommandType::ACTIVATE, self_refresh_exit },
        { CommandType::REFRESH, self_refresh_exit },
        { CommandType::SELF_REFRESH_ENTER, self_refresh_exit }
    };
}
