#include "error_injector.h"
#include <iostream>
#include <bitset>
#include <cstdlib>
#include <ctime>

using namespace std;

ErrorInjector error_injector(0);

ErrorInjector::ErrorInjector(int rate) : error_rate_percent(rate)
{
    srand(time(NULL));
}

void ErrorInjector::set_error_rate(int rate)
{
    error_rate_percent = rate;
    cout << "[ERROR-INJECTOR] Fehlerrate gesetzt auf " << rate << "%" << endl;
}

uint8_t ErrorInjector::inject_error(uint8_t data)
{
    if (error_rate_percent > 0 && (rand() % 100) < error_rate_percent)
    {
        int bit_to_flip = rand() % 8;
        uint8_t corrupted = data ^ (1 << bit_to_flip);
        cout << "  [ERROR!] Bit " << bit_to_flip << " geflippt: "
             << bitset<8>(data) << " -> " << bitset<8>(corrupted) << endl;
        return corrupted;
    }
    return data;
}
