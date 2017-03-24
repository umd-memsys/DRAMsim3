#include "timing.h"
#include <utility>

using namespace std;

Timing::Timing(const Config& config) :
    config_(config),
    same_bank(static_cast<int>(CommandType::SIZE)),
    other_banks_same_bankgroup(static_cast<int>(CommandType::SIZE)),
    other_bankgroups_same_rank(static_cast<int>(CommandType::SIZE)),
    other_ranks(static_cast<int>(CommandType::SIZE)),
    same_rank(static_cast<int>(CommandType::SIZE))
{
    read_to_read_l = std::max(config_.tBurst, config_.tCCDL);
    read_to_read_s = std::max(config_.tBurst, config_.tCCDS);
    read_to_read_o = config_.tBurst + config_.tRTRS;
    read_to_write = std::max(config_.tBurst, config_.tCCDL) + config_.tCAS - config_.tCWD; // What if (tCAS - tCWD) < 0? Would that help issue an early write?
    read_to_write_o = config_.tBurst + config_.tRTRS + config_.tCAS - config_.tCWD; // What if (tCAS - tCWD) < 0? Would that help issue an early write?
    read_to_precharge = config_.tRTP + config_.tBurst - config_.tCCDL; // What if (tBurst - tCCD) < 0? Would that help issue an early precharge?

    write_to_read = config_.tCWD + config_.tBurst + config_.tWTR; //Why doesn't tCCD come into the picture?
    write_to_read_o = config_.tCWD + config_.tBurst + config_.tRTRS - config_.tCAS; // What if (tCWD - tCAS) < 0? Would that help issue an early read?
    write_to_write_l = std::max(config_.tBurst, config_.tCCDL);
    write_to_write_s = std::max(config_.tBurst, config_.tCCDS);
    write_to_write_o = config_.tBurst + config_.tRTRS; // Let's say tRTRS == tOST
    write_to_precharge = config_.tCWD + config_.tBurst + config_.tWR;

    precharge_to_activate = config_.tRP;
    read_to_activate = read_to_precharge + precharge_to_activate;
    write_to_activate = write_to_precharge + precharge_to_activate;

    activate_to_activate = config_.tRC;
    activate_to_activate_o = config_.tRRD;
    activate_to_precharge = config_.tRAS;
    activate_to_read_write = config_.tRCD;
    activate_to_refresh = config_.tRRD;
    refresh_to_refresh = config_.tRREFD;
    refresh_to_activate = refresh_to_refresh;

    refresh_cycle = config_.tRFC;

    refresh_cycle_bank = config_.tRFCb;

    self_refresh_entry_to_exit = config_.tCKESR;
    self_refresh_exit = config_.tXS;

    // command READ
    same_bank[static_cast<int>(CommandType::READ)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_l },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_l },
        { CommandType::WRITE_PRECHARGE, read_to_write },
        { CommandType::PRECHARGE, read_to_precharge } 
    };
    other_banks_same_bankgroup[static_cast<int>(CommandType::READ)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_l },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_l },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_bankgroups_same_rank[static_cast<int>(CommandType::READ)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_s },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_s },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_ranks[static_cast<int>(CommandType::READ)] = list < pair<CommandType, int> >
    { 
        { CommandType::READ, read_to_read_o },
        { CommandType::WRITE, read_to_write_o }, 
        { CommandType::READ_PRECHARGE, read_to_read_o },
        { CommandType::WRITE_PRECHARGE, read_to_write_o }
    };

    //command WRITE
    same_bank[static_cast<int>(CommandType::WRITE)] = list< pair<CommandType, int> > 
    {
        { CommandType::READ, write_to_read },
        { CommandType::WRITE, write_to_write_l },
        { CommandType::READ_PRECHARGE, write_to_read },
        { CommandType::WRITE_PRECHARGE, write_to_write_l },
        { CommandType::PRECHARGE, write_to_precharge }
    };
    other_banks_same_bankgroup[static_cast<int>(CommandType::WRITE)] = list< pair<CommandType, int> > 
    {
        { CommandType::READ, write_to_read },
        { CommandType::WRITE, write_to_write_l },
        { CommandType::READ_PRECHARGE, write_to_read },
        { CommandType::WRITE_PRECHARGE, write_to_write_l }
    };
    other_bankgroups_same_rank[static_cast<int>(CommandType::WRITE)] = list< pair<CommandType, int> > 
    {
        { CommandType::READ, write_to_read },
        { CommandType::WRITE, write_to_write_s },
        { CommandType::READ_PRECHARGE, write_to_read },
        { CommandType::WRITE_PRECHARGE, write_to_write_s }
    };
    other_ranks[static_cast<int>(CommandType::WRITE)] = list< pair<CommandType, int> > 
    {
        { CommandType::READ, write_to_read_o },
        { CommandType::WRITE, write_to_write_o },
        { CommandType::READ_PRECHARGE, write_to_read_o },
        { CommandType::WRITE_PRECHARGE, write_to_write_o }
    };

    //command READ_PRECHARGE
    same_bank[static_cast<int>(CommandType::READ_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, read_to_activate },
        { CommandType::REFRESH, read_to_activate },
        { CommandType::REFRESH_BANK, read_to_activate },
        { CommandType::SELF_REFRESH_ENTER, read_to_activate }
    };
    other_banks_same_bankgroup[static_cast<int>(CommandType::READ_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_l },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_l },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_bankgroups_same_rank[static_cast<int>(CommandType::READ_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_s },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_s },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_ranks[static_cast<int>(CommandType::READ_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_o },
        { CommandType::WRITE, read_to_write_o }, 
        { CommandType::READ_PRECHARGE, read_to_read_o },
        { CommandType::WRITE_PRECHARGE, read_to_write_o }
    };

    //command WRITE_PRECHARGE
    same_bank[static_cast<int>(CommandType::WRITE_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, write_to_activate },
        { CommandType::REFRESH, write_to_activate },
        { CommandType::REFRESH_BANK, write_to_activate },
        { CommandType::SELF_REFRESH_ENTER, write_to_activate }
    };
    other_banks_same_bankgroup[static_cast<int>(CommandType::WRITE_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, write_to_read },
        { CommandType::WRITE, write_to_write_l },
        { CommandType::READ_PRECHARGE, write_to_read },
        { CommandType::WRITE_PRECHARGE, write_to_write_l }
    };
    other_bankgroups_same_rank[static_cast<int>(CommandType::WRITE_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, write_to_read },
        { CommandType::WRITE, write_to_write_s },
        { CommandType::READ_PRECHARGE, write_to_read },
        { CommandType::WRITE_PRECHARGE, write_to_write_s }
    };
    other_ranks[static_cast<int>(CommandType::WRITE_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, write_to_read_o },
        { CommandType::WRITE, write_to_write_o },
        { CommandType::READ_PRECHARGE, write_to_read_o },
        { CommandType::WRITE_PRECHARGE, write_to_write_o }
    };

    //command ACTIVATE
    same_bank[static_cast<int>(CommandType::ACTIVATE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, activate_to_read_write },
        { CommandType::WRITE, activate_to_read_write },
        { CommandType::READ_PRECHARGE, activate_to_read_write },
        { CommandType::WRITE_PRECHARGE, activate_to_read_write },
        { CommandType::PRECHARGE, activate_to_precharge },
        //{ CommandType::ACTIVATE, activate_to_activate }
    };

    other_banks_same_bankgroup[static_cast<int>(CommandType::ACTIVATE)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, activate_to_activate_o },
        { CommandType::REFRESH_BANK, activate_to_refresh }
    };

    other_bankgroups_same_rank[static_cast<int>(CommandType::ACTIVATE)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, activate_to_activate_o },
        { CommandType::REFRESH_BANK, activate_to_refresh }
    };

    //command PRECHARGE
    same_bank[static_cast<int>(CommandType::PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, precharge_to_activate },
        { CommandType::REFRESH, precharge_to_activate },
        { CommandType::REFRESH_BANK, precharge_to_activate },
        { CommandType::SELF_REFRESH_ENTER, precharge_to_activate }
    };

    //command REFRESH_BANK
    same_rank[static_cast<int>(CommandType::REFRESH_BANK)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, refresh_cycle_bank },
        { CommandType::REFRESH,  refresh_cycle_bank },
        { CommandType::REFRESH_BANK, refresh_cycle_bank },
        { CommandType::SELF_REFRESH_ENTER, refresh_cycle_bank }
    };

    other_banks_same_bankgroup[static_cast<int>(CommandType::REFRESH_BANK)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, refresh_to_activate },
        { CommandType::REFRESH_BANK, refresh_to_refresh },
    };

    other_bankgroups_same_rank[static_cast<int>(CommandType::REFRESH_BANK)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, refresh_to_activate },
        { CommandType::REFRESH_BANK, refresh_to_refresh },
    };

    //REFRESH, SELF_REFRESH_ENTER and SELF_REFRESH_EXIT are isued to the entire rank
    //command REFRESH
    same_rank[static_cast<int>(CommandType::REFRESH)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, refresh_cycle },
        { CommandType::REFRESH,  refresh_cycle },
        { CommandType::SELF_REFRESH_ENTER, refresh_cycle }
    };

    //command SELF_REFRESH_ENTER
    same_rank[static_cast<int>(CommandType::SELF_REFRESH_ENTER)] = list< pair<CommandType, int> >
    {
        { CommandType::SELF_REFRESH_EXIT,  self_refresh_entry_to_exit}
    };

    //command SELF_REFRESH_EXIT 
    same_rank[static_cast<int>(CommandType::SELF_REFRESH_EXIT)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, self_refresh_exit },
        { CommandType::REFRESH, self_refresh_exit },
        { CommandType::SELF_REFRESH_ENTER, self_refresh_exit }
    };
}
