#include <iostream>
#include <cstdlib>
#include <fstream>
#include "controller.h"
#include "timing.h"
#include "common.h"
#include "config.h"
#include "cpu.h"

using namespace std;

int main(int argc, char **argv)
{
    ofstream req_log("requests.log");

    Config config;
    TraceBasedCPU cpu;
    Timing timing(config);
    Controller ctrl(config, timing);

    // Create random CPU requests at random time intervals
    // With random row buffer hits
    // And insert them into the controller
    auto last_row = 0;
    auto id = 0;
    for(auto clk = 0; clk < config.cycles; clk++) {
        //CPU Clock Tick
        if ( rand() % 4 == 0) {
            auto rank = rand() % config.ranks;
            auto bankgroup = rand() % config.bankgroups;
            auto bank = rand() % config.banks_per_group;
            auto row = rand() % 3 == 0 ? rand() % config.rows : last_row;
            last_row = row;
            auto cmd_type = rand() % 3 == 0 ? CommandType::WRITE : CommandType::READ;

            Request* req = new Request(cmd_type, rank, bankgroup, bank, row, id);
            req->arrival_time_ = clk;
            if(ctrl.InsertReq(req)) {
                id++;
                req_log << "Request Inserted at clk = " << clk << " " << *req << endl;
            }
        }
        //Memory Controller Clock Tick
        ctrl.ClockTick();
    }

    req_log.close();
    return 0;
}