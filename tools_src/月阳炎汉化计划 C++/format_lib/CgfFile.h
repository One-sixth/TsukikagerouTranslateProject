#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <bit>
#include "file_utils.hpp"
#include <stdexcept>


using namespace std;


class CgfFile
{
public:
    static vector<uint8_t> decrypt(vector<uint8_t> data);
    static vector<uint8_t> encrypt(vector<uint8_t> data);

};

