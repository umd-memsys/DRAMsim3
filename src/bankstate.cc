#include "bankstate.h"
#include "statistics.h"

using namespace std;
using namespace dramcore;

BankState::BankState(Statistics &stats) :
    stats_(stats),
    state_(State::CLOSED),
    cmd_timing_(static_cast<int>(CommandType::SIZE)),
    open_row_(-1),
    row_hit_count_(0),
    refresh_waiting_(false)
{
    cmd_timing_[static_cast<int>(CommandType::READ)] = 0;
    cmd_timing_[static_cast<int>(CommandType::READ_PRECHARGE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::WRITE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::WRITE_PRECHARGE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::ACTIVATE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::PRECHARGE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::REFRESH)] = 0;
    cmd_timing_[static_cast<int>(CommandType::SELF_REFRESH_ENTER)] = 0;
    cmd_timing_[static_cast<int>(CommandType::SELF_REFRESH_EXIT)] = 0;

}

CommandType BankState::GetRequiredCommandType(const Command& cmd) {
    switch(cmd.cmd_type_) {
        case CommandType::READ:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return cmd.Row() == open_row_ ? CommandType::READ : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    cout << "In unknown state" << endl;
                    AbruptExit(__FILE__, __LINE__);
            }
            case CommandType::READ_PRECHARGE:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return cmd.Row() == open_row_ ? CommandType::READ_PRECHARGE : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    cout << "In unknown state" << endl;
                    AbruptExit(__FILE__, __LINE__);
            }
        case CommandType::WRITE:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return cmd.Row() == open_row_ ? CommandType::WRITE : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    cout << "In unknown state" << endl;
                    AbruptExit(__FILE__, __LINE__);
            }
            case CommandType::WRITE_PRECHARGE:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return cmd.Row() == open_row_ ? CommandType::WRITE_PRECHARGE : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    cout << "In unknown state" << endl;
                    AbruptExit(__FILE__, __LINE__);
            }
            case CommandType::REFRESH_BANK:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::REFRESH_BANK;
                case State::OPEN:
                    return CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    cout << "In unknown state" << endl;
                    AbruptExit(__FILE__, __LINE__);
            }
        case CommandType::REFRESH:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::REFRESH;
                case State::OPEN:
                    return CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    cout << "In unknown state" << endl;
                    AbruptExit(__FILE__, __LINE__);
            }
        case CommandType::SELF_REFRESH_ENTER:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::SELF_REFRESH_ENTER;
                case State::OPEN:
                    return CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                default:
                    cout << "In unknown state" << endl;
                    AbruptExit(__FILE__, __LINE__);
            }
        case CommandType::SELF_REFRESH_EXIT:
            switch(state_) {
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                case State::CLOSED:
                case State::OPEN:
                default:
                    cout << "In unknown state" << endl;
                    AbruptExit(__FILE__, __LINE__);
            }
        // ACTIVATE and PRECHARGE are on demand commands
        // They will not be scheduled on their own.
        case CommandType::ACTIVATE:
        case CommandType::PRECHARGE:
        default:
            AbruptExit(__FILE__, __LINE__);
            return CommandType::SIZE;
    }
}

void BankState::UpdateState(const Command& cmd) {
    switch(state_) {
        case State::OPEN:
            switch(cmd.cmd_type_) {
                case CommandType::READ:
                case CommandType::WRITE:
                    if(row_hit_count_ != 0) {
                        stats_.numb_row_hits++;
                        if (cmd.cmd_type_ == CommandType::READ) stats_.numb_read_row_hits++;
                        if (cmd.cmd_type_ == CommandType::WRITE) stats_.numb_write_row_hits++;
                    }
                    row_hit_count_++;
                    break;
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::PRECHARGE:
                    state_ = State::CLOSED;
                    open_row_ = -1;
                    row_hit_count_ = 0;
                    break;
                case CommandType::ACTIVATE:
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                case CommandType::SELF_REFRESH_ENTER:
                case CommandType::SELF_REFRESH_EXIT:
                default:
                    AbruptExit(__FILE__, __LINE__);
            }
            break;
        case State::CLOSED:
            switch(cmd.cmd_type_) {
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                    break;
                case CommandType::ACTIVATE:
                    state_ = State::OPEN;
                    open_row_ = cmd.Row();
                    break;
                case CommandType::SELF_REFRESH_ENTER:
                    state_ = State::SELF_REFRESH;
                    break;
                case CommandType::READ:
                case CommandType::WRITE:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::PRECHARGE:
                case CommandType::SELF_REFRESH_EXIT:
                default:
                    AbruptExit(__FILE__, __LINE__);
            }
            break;
        case State::SELF_REFRESH:
            switch(cmd.cmd_type_) {
                case CommandType::SELF_REFRESH_EXIT:
                    state_ = State::CLOSED;
                    break;
                case CommandType::READ:
                case CommandType::WRITE:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::ACTIVATE:
                case CommandType::PRECHARGE:
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                case CommandType::SELF_REFRESH_ENTER:
                default:
                    AbruptExit(__FILE__, __LINE__);
            }
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
    }
    return;
}

void BankState::UpdateTiming(CommandType cmd_type, uint64_t time) {
    cmd_timing_[static_cast<int>(cmd_type)] = max(cmd_timing_[static_cast<int>(cmd_type)], time);
    return;
}
