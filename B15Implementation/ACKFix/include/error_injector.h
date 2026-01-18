#ifndef ERROR_INJECTOR_H
#define ERROR_INJECTOR_H

#include <cstdint>
#include <mutex>

// Fehler-Injektion für Software-Simulation
// Hinweis: Beim echten B15F gibt es keine Fehler-Injektion
class ErrorInjector
{
private:
    int error_rate_percent;
    std::mutex inject_mutex;

public:
    ErrorInjector(int rate = 0);
    void set_error_rate(int rate);
    uint8_t inject_error(uint8_t data);
};

// Globale ErrorInjector-Instanz
extern ErrorInjector error_injector;

#endif // ERROR_INJECTOR_H
