#ifndef __REFRESH_H
#define __REFRESH_H

#include <vector>

class Refresh {
    public:
        Refresh();
    private:
        //Keep track of the last time when a refresh command was issued to this bank 
        std::vector< std::vector<long> > last_bank_refresh_;

        //Last time when a refresh command was issued to the entire rank
        //Also updated when an epoch of bank level refreshed is done as well
        long last_rank_refresh_;
        
};

#endif