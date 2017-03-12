#include <iostream>
#include <cstdlib>
#include "controller.h"
#include "timing.h"
#include "common.h"

using namespace std;

int main(int argc, char **argv)
{
    int ranks = 1;
    int bank_groups = 1;
    int banks_per_group = 1;
    int rows = 1024;
    
    long clk = 0;
    int cycles = 100000;

    Timing timing;
    Controller ctrl(ranks, bank_groups, banks_per_group, timing);

    // Create random CPU requests at random time intervals
    // With random row buffer hits
    // And insert them into the controller
    auto last_row = 0;
    for(auto i = 0; i < cycles; i++) {
        //CPU Clock Tick
        if ( rand() % 4 == 0) {
            auto rank = rand() % ranks;
            auto bankgroup = rand() % bank_groups;
            auto bank = rand() % banks_per_group;
            auto row = rand() % 2 == 0 ? last_row : rand() % rows;
            last_row = row;
            auto cmd_type = rand() % 3 == 0 ? CommandType::WRITE : CommandType::READ;

            Request* req = new Request(cmd_type, rank, bankgroup, bank, row);
            req->arrival_time_ = clk;
            if(ctrl.InsertReq(req)) {
                /*cout << "----------------------------------------------------------------------\n";
                cout << "Request Inserted at clk = " << clk << " " << *req << endl;
                cout << "----------------------------------------------------------------------\n";*/
            }
        }
        //Memory Controller Clock Tick
        ctrl.ClockTick();
        clk++;
    }
    return 0;
}