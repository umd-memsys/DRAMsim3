#include "bankstate.h"
#include <iostream>
#include <algorithm>

using namespace std;

BankState::BankState(int rank, int bankgroup, int bank) :
    state_(State::CLOSED),
    open_row_(-1),
    rank_(rank),
    bankgroup_(bankgroup),
    bank_(bank)
{
    timing_[CommandType::READ] = 0;
    timing_[CommandType::READ_PRECHARGE] = 0;
    timing_[CommandType::WRITE] = 0;
    timing_[CommandType::WRITE_PRECHARGE] = 0;
    timing_[CommandType::ACTIVATE] = 0;
    timing_[CommandType::PRECHARGE] = 0;
    timing_[CommandType::REFRESH] = 0;
    timing_[CommandType::SELF_REFRESH_ENTER] = 0;
    timing_[CommandType::SELF_REFRESH_EXIT] = 0; 
}

CommandType BankState::GetRequiredCommand(const Request& req) {
    switch(req.request_type_) {
        case RequestType::READ:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return req.row_ == open_row_ ? CommandType::READ : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
            }
            case RequestType::READ_PRECHARGE:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return req.row_ == open_row_ ? CommandType::READ_PRECHARGE : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
            }
        case RequestType::WRITE:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return req.row_ == open_row_ ? CommandType::WRITE : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
            }
            case RequestType::WRITE_PRECHARGE:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return req.row_ == open_row_ ? CommandType::WRITE_PRECHARGE : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
            }
        case RequestType::REFRESH:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::REFRESH;
                case State::OPEN:
                    return CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
            }
        case RequestType::SELF_REFRESH_ENTER:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::SELF_REFRESH_ENTER;
                case State::OPEN:
                    return CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    exit(-1);
            }
        default:
            exit(-1);
    }
}

void BankState::UpdateState(const Command& cmd) {
    switch(state_) {
        case State::OPEN:
            switch(cmd.cmd_type_) {
                case CommandType::READ:
                case CommandType::WRITE:
                    break;
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::PRECHARGE:
                    state_ = State::CLOSED;
                    open_row_ = -1;
                    break;
                case CommandType::ACTIVATE:
                case CommandType::REFRESH:
                case CommandType::SELF_REFRESH_ENTER:
                case CommandType::SELF_REFRESH_EXIT:
                    exit(-1);
            }
            break;
        case State::CLOSED:
            switch(cmd.cmd_type_) {
                case CommandType::REFRESH:
                    break;
                case CommandType::ACTIVATE:
                    state_ = State::OPEN;
                    open_row_ = cmd.row_;
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
                    exit(-1);
            }
        case State::SELF_REFRESH:
            switch(cmd.cmd_type_) {
                case CommandType::SELF_REFRESH_EXIT:
                    state_ = State::CLOSED;
                case CommandType::READ:
                case CommandType::WRITE:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::ACTIVATE:
                case CommandType::PRECHARGE:
                case CommandType::REFRESH:
                case CommandType::SELF_REFRESH_ENTER:
                    exit(-1);
            }
        default:
            exit(-1);
    }
    return;
}

void BankState::UpdateTiming(const Command& cmd, long time) {
    timing_[cmd.cmd_type_] = max(timing_[cmd.cmd_type_], time);
    return;
}

bool BankState::IsReady(const Command& cmd, long time) {
    return time >= timing_[cmd.cmd_type_];
}