// Author: Tommy Jung

/*
  OUTPUT KEYS:
  # nop       = no request in this bank.
  # closed    = there is a request in this bank, but the row is closed.
  # act       = activate
  # rd        = read
  # wr        = write
  # pre       = precharge
  # row_miss  = there is a request but row miss. 
  # arb       = there is a row hit, but other bank is accessing (arbitrated)
  # ref       = refresh
  # conf      = there is a row hit, but can't access due to various timing constraints (tWTR, tCCD_S, etc)
*/

#include "blood_graph.h"

namespace dramsim3 {

// default constructor
BloodGraph::BloodGraph(int channel_id, const Config &config)
  : config_(config)
{
  channel_id_ = channel_id; 
  clk_ = 0;

  is_in_ref_ = false;
  ref_count_ = 0;

  bank_count_ = config_.ranks * config_.banks;
  read_issued_.reserve(bank_count_);
  write_issued_.reserve(bank_count_);
  pre_count_.reserve(bank_count_);
  act_count_.reserve(bank_count_);
  for (int i = 0; i < bank_count_; i++) {
    read_issued_[i] = false;
    write_issued_[i] = false;
    pre_count_[i] = 0;
    act_count_[i] = 0;
  }

  std::string trace_file_name = "blood_graph_ch" + std::to_string(channel_id_) + ".log";
  trace_.open(trace_file_name, std::ofstream::out);
#ifdef BLOOD_GRAPH
  trace_ << "time,bank,state" << std::endl;
#endif
}


void BloodGraph::IsInRefresh(bool ref)
{
  is_in_ref_ = ref;
}


void BloodGraph::IssueCommand(const Command &cmd) {
  if (!is_in_ref_) {
    int bank_id = cmd_queue_->GetQueueIndex(cmd.Rank(), cmd.Bankgroup(), cmd.Bank());

    if (cmd.IsRefresh()) {
      ref_count_ = config_.tRFC;
    } else if (cmd.IsRead()) {
      read_issued_[bank_id] = true;
    } else if (cmd.IsWrite()) {
      write_issued_[bank_id] = true;
    } else if (cmd.cmd_type == CommandType::ACTIVATE) {
      act_count_[bank_id] = config_.tRCDRD;
    } else if (cmd.cmd_type == CommandType::PRECHARGE) {
      pre_count_[bank_id] = config_.tRP;
    }
  }
}


void BloodGraph::ClockTick() {
  if (is_in_ref_) {
    for (int i = 0; i < bank_count_; i++) {
      PrintTrace(i, "ref");
      act_count_[i] = 0;
      pre_count_[i] = 0;
    }
  } else if (ref_count_ > 0) {
    for (int i = 0; i < bank_count_; i++) {
      PrintTrace(i, "ref");
    }
    ref_count_--;
  } else {

    bool read_issued_found;
    bool write_issued_found;

    for (int i = 0; i < bank_count_; i++) {
      if (read_issued_[i]) {
        read_issued_found = true;
        break;
      }
    }
    for (int i = 0; i < bank_count_; i++) {
      if (write_issued_[i]) {
        write_issued_found = true;
        break;
      }
    }

    for (int i = 0; i < bank_count_; i++) {
      if (act_count_[i] > 0) {
        PrintTrace(i, "act");
        act_count_[i]--;
      } else if (pre_count_[i] > 0) {
        PrintTrace(i, "pre");
        pre_count_[i]--;
      } else if (read_issued_[i]) {
        PrintTrace(i, "rd");
      } else if (write_issued_[i]) {
        PrintTrace(i, "wr");
      } else {
        if (cmd_queue_->QueueEmpty(i)) {
          PrintTrace(i, "nop");
        } else {
          int ra = i / config_.banks;
          int bg = (i % config_.banks) / config_.banks_per_group;
          int ba = (i % config_.banks) % config_.banks_per_group;
          if (channel_state_->IsRowOpen(ra,bg,ba)) {
            // check if there is any row hit.
            int open_row = channel_state_->OpenRow(ra,bg,ba);
            bool read_row_hit_found = false;
            bool write_row_hit_found = false;
            auto queue = cmd_queue_->GetQueue(ra,bg,ba);

            for (auto cmd_it = queue.begin(); cmd_it != queue.end(); cmd_it++) {
                if (open_row == cmd_it->Row() && cmd_it->IsRead()) {
                  read_row_hit_found = true;
                } else if (open_row == cmd_it->Row() && cmd_it->IsWrite()) {
                  write_row_hit_found = true;
                }
            }

            if (read_row_hit_found) {
              if (read_issued_found) {
                PrintTrace(i, "arb");
              } else {
                PrintTrace(i, "conf");
              }
            } else if (write_row_hit_found) {
              if (write_issued_found) {
                PrintTrace(i, "arb");
              } else {
                PrintTrace(i, "conf");
              }
            } else {
              PrintTrace(i, "row_miss");
            }
          } else {
            PrintTrace(i, "closed");
          }
        }
      }
    }
  }

  is_in_ref_ = false;
  for (int i = 0; i < bank_count_; i++) {
    read_issued_[i] = false;
    write_issued_[i] = false;
  }
  clk_++;
}

void BloodGraph::PrintTrace(int bank_id, const std::string &str)
{
#ifdef BLOOD_GRAPH
  trace_ << clk_ << "," << bank_id << "," << str << std::endl;
#endif
}


} // namespace dramsim3
