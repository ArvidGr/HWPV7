#ifndef ERROR_INJECTOR_H
#define ERROR_INJECTOR_H

#include <cstdint>

// Simulierte Fehler-Injektion
class ErrorInjector
{
private:
    int error_rate_percent; // 0-100

public:
    ErrorInjector(int rate = 0);
    void set_error_rate(int rate);
    uint8_t inject_error(uint8_t data);
};

extern ErrorInjector error_injector;

#endif // ERROR_INJECTOR_H
