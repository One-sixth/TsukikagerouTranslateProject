#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>


using namespace std;


class DeflateUtils
{
public:
	static vector<uint8_t> compress(vector<uint8_t> data);
	static vector<uint8_t> decompress(vector<uint8_t> data);
};

