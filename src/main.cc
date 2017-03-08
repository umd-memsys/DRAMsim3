#include <iostream>
#include "controller.h"
#include "timing.h"

using namespace std;

int main(int argc, char **argv)
{
    int ranks = 2;
    int bank_groups = 2;
    int banks_per_group = 2;

    Timing timing;
    Controller ctrl(ranks, bank_groups, banks_per_group, timing);

    ctrl.ClockTick();

    return 0;
}