#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <vector>
#include "bankstate.h"

class Controller {
    public:
        Controller(int ranks, int bankgroups, int banks_per_group);
    private:
        int ranks_, bankgroups_, banks_per_group_;
        std::vector< std::vector< std::vector<BankState*> > > bank_states_;
};

#endif