#include "DeflateUtils.h"
#include "sdefl.h"
#include "sinfl.h"


//vector<uint8_t> DeflateUtils::compress(vector<uint8_t> data)
//{
//	sdefl sdefl2 ={};
//	vector<uint8_t> out;
//	out.resize(data.size() * 10);
//	int n = sdeflate(&sdefl2, out.data(), data.data(), data.size(), 8);
//	if (n > out.size())
//		throw runtime_error("Error! Overflow!");
//	out.resize(n);
//	return out;
//}

vector<uint8_t> DeflateUtils::compress(vector<uint8_t> data)
{
	sdefl sdefl2 ={};
	vector<uint8_t> out;
	out.resize(data.size() * 10);
	int n = zsdeflate(&sdefl2, out.data(), data.data(), data.size(), 8);
	if (n > out.size())
		throw runtime_error("Error! Overflow!");
	out.resize(n);
	return out;
}

//vector<uint8_t> DeflateUtils::decompress(vector<uint8_t> data)
//{
//	vector<uint8_t> out;
//	out.resize(data.size() * 10);
//	int n = sinflate(out.data(), data.data(), data.size());
//	if (n > out.size())
//		throw runtime_error("Error! Overflow!");
//	out.resize(n);
//	return out;
//}

vector<uint8_t> DeflateUtils::decompress(vector<uint8_t> data)
{
	vector<uint8_t> out;
	out.resize(data.size() * 10);
	//int adl = 0;
	//int n = zsinflate(out.data(), data.data(), data.size(), adl);
	// 程序用的adler32算法与这里的不一样，校验码会失败，要绕过
	int n = sinflate(out.data(), data.data()+2, data.size());
	if (n > out.size())
		throw runtime_error("Error! Overflow!");
	else if (n == -1)
		throw runtime_error("Error! Decompress fail!");
	out.resize(n);
	return out;
}
