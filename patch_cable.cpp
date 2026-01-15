#include "patch_cable.h"
#include <fstream>
#include <iostream>
#include <cstdio>

using namespace std;

PatchCableFile::PatchCableFile(const string &fname) : filename(fname)
{
    ifstream check(filename);
    if (!check.good())
    {
        ofstream file(filename, ios::binary);
        uint8_t zero = 0;
        file.write((char *)&zero, 1);
        file.close();
        cout << "[CABLE] Patchkabel-Datei erstellt: " << filename << endl;
    }
    else
    {
        cout << "[CABLE] Patchkabel-Datei gefunden: " << filename << endl;
        check.close();
    }
}

PatchCableFile::~PatchCableFile()
{
    remove(filename.c_str());
}

void PatchCableFile::write(uint8_t value)
{
    ofstream file(filename, ios::binary | ios::in | ios::out);
    file.seekp(0);
    file.write((char *)&value, 1);
    file.close();
}

uint8_t PatchCableFile::read()
{
    ifstream file(filename, ios::binary);
    uint8_t value = 0;
    if (file.is_open())
    {
        file.read((char *)&value, 1);
        file.close();
    }
    return value;
}

void PatchCableFile::write_bits_a(uint8_t data)
{
    uint8_t current = read();
    uint8_t new_val = (current & 0x0F) | ((data & 0x0F) << 4);
    write(new_val);
}

void PatchCableFile::write_bits_b(uint8_t data)
{
    uint8_t current = read();
    uint8_t new_val = (current & 0xF0) | (data & 0x0F);
    write(new_val);
}

uint8_t PatchCableFile::read_bits_a()
{
    return (read() >> 4) & 0x0F;
}

uint8_t PatchCableFile::read_bits_b()
{
    return read() & 0x0F;
}
