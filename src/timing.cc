#include "timing.h"

using namespace std;

Timing::Timing() :
    same_bank(int(CommandType::SIZE)),
    other_banks_same_bankgroup(int(CommandType::SIZE)),
    other_bankgroups_same_rank(int(CommandType::SIZE)),
    other_ranks(int(CommandType::SIZE))
{
    same_bank[int(CommandType::READ)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, max(tBurst, tCCDL) },
        { CommandType::WRITE, max(tBurst, tCCDL) },
        { CommandType::READ_PRECHARGE, max(tBurst, tCCDL) },
        { CommandType::WRITE_PRECHARGE, max(tBurst, tCCDL) },
        { CommandType::PRECHARGE, tRTP }
    };

    other_banks_same_bankgroup[int(CommandType::READ)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, max(tBurst, tCCDL) },
        { CommandType::WRITE, max(tBurst, tCCDL) },
        { CommandType::READ_PRECHARGE, max(tBurst, tCCDL) },
        { CommandType::WRITE_PRECHARGE, max(tBurst, tCCDL) },
        { CommandType::PRECHARGE, tRTP }
    };

    other_bankgroups_same_rank[int(CommandType::READ)] = list< pair<CommandType, int> >
    {
        { CommandType::READ, max(tBurst, tCCDS) },
        { CommandType::WRITE, max(tBurst, tCCDS) },
        { CommandType::READ_PRECHARGE, max(tBurst, tCCDS) },
        { CommandType::WRITE_PRECHARGE, max(tBurst, tCCDS) }
    };

    other_ranks[int(CommandType::READ)] = list < pair<CommandType, int> >
    { 
        { CommandType::READ, tBurst + tRTRS },
        { CommandType::WRITE, tBurst + tRTRS },
        { CommandType::READ_PRECHARGE, tBurst + tRTRS },
        { CommandType::WRITE_PRECHARGE, tBurst + tRTRS }
    };
}