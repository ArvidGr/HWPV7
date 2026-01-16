#include "../include/error_injector.h"
#include <iostream>
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
    lock_guard<mutex> lock(inject_mutex);
    error_rate_percent = rate;
    if (rate > 0)
    {
        cout << "[WARNING] Fehler-Injektion nur in Software-Simulation verfuegbar!" << endl;
    }
}

uint8_t ErrorInjector::inject_error(uint8_t data)
{
    // Bei echter Hardware: keine Fehler-Injektion
    return data;
}
