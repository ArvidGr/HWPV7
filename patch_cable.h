#ifndef PATCH_CABLE_H
#define PATCH_CABLE_H

#include <string>
#include <cstdint>

// Simuliertes Patchkabel als Datei
class PatchCableFile
{
private:
    std::string filename;

public:
    PatchCableFile(const std::string &fname = "patchcable.bin");
    ~PatchCableFile();

    void write(uint8_t value);
    uint8_t read();

    void write_bits_a(uint8_t data);
    void write_bits_b(uint8_t data);
    uint8_t read_bits_a();
    uint8_t read_bits_b();
};

#endif // PATCH_CABLE_H
