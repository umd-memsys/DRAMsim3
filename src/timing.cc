#include "timing.h"

using namespace std;

Timing::Timing() :
    same_bank(int(CommandType::SIZE)),
    other_banks_same_bankgroup(int(CommandType::SIZE)),
    other_bankgroups_same_rank(int(CommandType::SIZE)),
    other_ranks(int(CommandType::SIZE)),
    same_rank(int(CommandType::SIZE))
{
    // command READ
    same_bank[int(CommandType::READ)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_l },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_l },
        { CommandType::WRITE_PRECHARGE, read_to_write },
        { CommandType::PRECHARGE, read_to_precharge } 
    };
    other_banks_same_bankgroup[int(CommandType::READ)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_l },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_l },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_bankgroups_same_rank[int(CommandType::READ)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_s },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_s },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_ranks[int(CommandType::READ)] = list < pair<CommandType, int> >
    { 
        { CommandType::READ, read_to_read_o },
        { CommandType::WRITE, read_to_write_o }, 
        { CommandType::READ_PRECHARGE, read_to_read_o },
        { CommandType::WRITE_PRECHARGE, read_to_write_o }
    };

    //command WRITE
    same_bank[int(CommandType::WRITE)] = list< pair<CommandType, int> > 
    {
        { CommandType::READ, write_to_read },
        { CommandType::WRITE, write_to_write_l },
        { CommandType::READ_PRECHARGE, write_to_read },
        { CommandType::WRITE_PRECHARGE, write_to_write_l },
        { CommandType::PRECHARGE, write_to_precharge }
    };
    other_banks_same_bankgroup[int(CommandType::WRITE)] = list< pair<CommandType, int> > 
    {
        { CommandType::READ, write_to_read },
        { CommandType::WRITE, write_to_write_l },
        { CommandType::READ_PRECHARGE, write_to_read },
        { CommandType::WRITE_PRECHARGE, write_to_write_l }
    };
    other_bankgroups_same_rank[int(CommandType::WRITE)] = list< pair<CommandType, int> > 
    {
        { CommandType::READ, write_to_read },
        { CommandType::WRITE, write_to_write_s },
        { CommandType::READ_PRECHARGE, write_to_read },
        { CommandType::WRITE_PRECHARGE, write_to_write_s }
    };
    other_ranks[int(CommandType::WRITE)] = list< pair<CommandType, int> > 
    {
        { CommandType::READ, write_to_read_o },
        { CommandType::WRITE, write_to_write_o },
        { CommandType::READ_PRECHARGE, write_to_read_o },
        { CommandType::WRITE_PRECHARGE, write_to_write_o }
    };

    //command READ_PRECHARGE
    same_bank[int(CommandType::READ_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, read_to_activate },
        { CommandType::REFRESH, read_to_activate },
        { CommandType::REFRESH_BANK, read_to_activate },
        { CommandType::SELF_REFRESH_ENTER, read_to_activate }
    };
    other_banks_same_bankgroup[int(CommandType::READ_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_l },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_l },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_bankgroups_same_rank[int(CommandType::READ_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_s },
        { CommandType::WRITE, read_to_write },
        { CommandType::READ_PRECHARGE, read_to_read_s },
        { CommandType::WRITE_PRECHARGE, read_to_write }
    };
    other_ranks[int(CommandType::READ_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, read_to_read_o },
        { CommandType::WRITE, read_to_write_o }, 
        { CommandType::READ_PRECHARGE, read_to_read_o },
        { CommandType::WRITE_PRECHARGE, read_to_write_o }
    };

    //command WRITE_PRECHARGE
    same_bank[int(CommandType::WRITE_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, write_to_activate },
        { CommandType::REFRESH, write_to_activate },
        { CommandType::REFRESH_BANK, write_to_activate },
        { CommandType::SELF_REFRESH_ENTER, write_to_activate }
    };
    other_banks_same_bankgroup[int(CommandType::WRITE_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, write_to_read },
        { CommandType::WRITE, write_to_write_l },
        { CommandType::READ_PRECHARGE, write_to_read },
        { CommandType::WRITE_PRECHARGE, write_to_write_l }
    };
    other_bankgroups_same_rank[int(CommandType::WRITE_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, write_to_read },
        { CommandType::WRITE, write_to_write_s },
        { CommandType::READ_PRECHARGE, write_to_read },
        { CommandType::WRITE_PRECHARGE, write_to_write_s }
    };
    other_ranks[int(CommandType::WRITE_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, write_to_read_o },
        { CommandType::WRITE, write_to_write_o },
        { CommandType::READ_PRECHARGE, write_to_read_o },
        { CommandType::WRITE_PRECHARGE, write_to_write_o }
    };

    //command ACTIVATE
    same_bank[int(CommandType::ACTIVATE)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, activate_to_read_write },
        { CommandType::WRITE, activate_to_read_write },
        { CommandType::READ_PRECHARGE, activate_to_read_write },
        { CommandType::WRITE_PRECHARGE, activate_to_read_write },
        { CommandType::PRECHARGE, activate_to_precharge },
        //{ CommandType::ACTIVATE, activate_to_activate }
    };

    other_banks_same_bankgroup[int(CommandType::WRITE_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, activate_to_activate_o },
        { CommandType::REFRESH_BANK, activate_to_refresh }
    };

    other_bankgroups_same_rank[int(CommandType::WRITE_PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, activate_to_activate_o },
        { CommandType::REFRESH_BANK, activate_to_refresh }
    };

    //command PRECHARGE
    same_bank[int(CommandType::PRECHARGE)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, precharge_to_activate },
        { CommandType::REFRESH, precharge_to_activate },
        { CommandType::REFRESH_BANK, precharge_to_activate },
        { CommandType::SELF_REFRESH_ENTER, precharge_to_activate }
    };

    //command REFRESH_BANK
    same_rank[int(CommandType::REFRESH_BANK)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, refresh_cycle_bank },
        { CommandType::REFRESH,  refresh_cycle_bank },
        { CommandType::REFRESH_BANK, refresh_cycle_bank },
        { CommandType::SELF_REFRESH_ENTER, refresh_cycle_bank }
    };

    other_banks_same_bankgroup[int(CommandType::REFRESH_BANK)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, refresh_to_activate },
        { CommandType::REFRESH_BANK, refresh_to_refresh },
    };

    other_bankgroups_same_rank[int(CommandType::REFRESH_BANK)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, refresh_to_activate },
        { CommandType::REFRESH_BANK, refresh_to_refresh },
    };

    //REFRESH, SELF_REFRESH_ENTER and SELF_REFRESH_EXIT are isued to the entire rank
    //command REFRESH
    same_rank[int(CommandType::REFRESH)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, refresh_cycle },
        { CommandType::REFRESH,  refresh_cycle },
        { CommandType::SELF_REFRESH_ENTER, refresh_cycle }
    };

    //command SELF_REFRESH_ENTER
    same_rank[int(CommandType::SELF_REFRESH_ENTER)] = list< pair<CommandType, int> >
    {
        { CommandType::SELF_REFRESH_EXIT,  self_refresh_entry_to_exit}
    };

    //command SELF_REFRESH_EXIT 
    same_rank[int(CommandType::SELF_REFRESH_EXIT)] = list< pair<CommandType, int> >
    {
        { CommandType::ACTIVATE, self_refresh_exit },
        { CommandType::REFRESH, self_refresh_exit },
        { CommandType::SELF_REFRESH_ENTER, self_refresh_exit }
    };
}