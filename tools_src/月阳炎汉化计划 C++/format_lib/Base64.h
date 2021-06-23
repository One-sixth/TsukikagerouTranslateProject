#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>


using namespace std;


class Base64
{
public:
	static string encode(vector<uint8_t> d);
	static vector<uint8_t> decode(string s);

};
