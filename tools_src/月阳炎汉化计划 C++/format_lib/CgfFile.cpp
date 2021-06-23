#include "CgfFile.h"


vector<uint8_t> CgfFile::decrypt(vector<uint8_t> data)
{
    int dec_size = data.size();
    if (data.size() % 4 != 0)
    {
        int pad = 4 - data.size() % 4;
        data.resize(data.size() + pad, 0);
    }

    uint32_t* data32 = (uint32_t*)data.data();
    const uint32_t seed = 0x3977141B;
    uint32_t key = seed;
    for (int i = 0; i < data.size(); i += 4)
    {
        key = rotl(key, 3);
        *data32++ ^= key;
        key += seed;
    }
    data.resize(dec_size);
    return data;
}

vector<uint8_t> CgfFile::encrypt(vector<uint8_t> data)
{
    return decrypt(data);
}