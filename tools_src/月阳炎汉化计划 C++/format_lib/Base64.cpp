#include "Base64.h"


namespace
{

	//定义编码字典
	uint8_t alphabet_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	//定义解码字典
	uint8_t reverse_map[] =
	{
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62, 255, 255, 255, 63,
			52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255, 255, 255, 255, 255,
			255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
			15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 255, 255, 255, 255, 255,
			255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
			41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 255, 255, 255, 255, 255
	};

	/*
	*  编码
	*  传入需要编码的数据地址和数据长度
	*  返回:解码后的数据
	*/
	vector<uint8_t> base64_encode(const uint8_t* text, uint32_t text_len)
	{
		//计算解码后的数据长度
		//由以上可知  Base64就是将3个字节的数据(24位)，拆成4个6位的数据，然后前两位补零
		//将其转化为0-63的数据  然后根据编码字典进行编码
		int encode_length = text_len / 3 * 4;
		if (text_len % 3 > 0)
		{
			encode_length += 4;
		}

		//为编码后数据存放地址申请内存
		//uint8_t* encode = (uint8_t*)malloc(encode_length);
		//uint8_t* encode = new uint8_t[encode_length];
		vector<uint8_t> encode(encode_length, 0);

		//编码
		uint32_t i, j;
		for (i = 0, j = 0; i + 3 <= text_len; i+=3)
		{
			encode[j++] = alphabet_map[text[i] >> 2];                             //取出第一个字符的前6位并找出对应的结果字符
			encode[j++] = alphabet_map[((text[i] << 4) & 0x30) | (text[i + 1] >> 4)];     //将第一个字符的后2位与第二个字符的前4位进行组合并找到对应的结果字符
			encode[j++] = alphabet_map[((text[i + 1] << 2) & 0x3c) | (text[i + 2] >> 6)];   //将第二个字符的后4位与第三个字符的前2位组合并找出对应的结果字符
			encode[j++] = alphabet_map[text[i + 2] & 0x3f];                         //取出第三个字符的后6位并找出结果字符
		}

		//对于最后不够3个字节的  进行填充
		if (i < text_len)
		{
			uint32_t tail = text_len - i;
			if (tail == 1)
			{
				encode[j++] = alphabet_map[text[i] >> 2];
				encode[j++] = alphabet_map[(text[i] << 4) & 0x30];
				encode[j++] = '=';
				encode[j++] = '=';
			}
			else //tail==2
			{
				encode[j++] = alphabet_map[text[i] >> 2];
				encode[j++] = alphabet_map[((text[i] << 4) & 0x30) | (text[i + 1] >> 4)];
				encode[j++] = alphabet_map[(text[i + 1] << 2) & 0x3c];
				encode[j++] = '=';
			}
		}
		return encode;
	}

	vector<uint8_t> base64_decode(const uint8_t* code, uint32_t code_len)
	{
		//由编码处可知，编码后的base64数据一定是4的倍数个字节
		if ((code_len & 0x03) != 0)  //如果它的条件返回错误，则终止程序执行。4的倍数。
		{
			throw runtime_error("Error! Base64 code is bad");
		}

		//为解码后的数据地址申请内存
		//uint8_t* plain = (uint8_t*)malloc(code_len / 4 * 3);
		//uint8_t* plain = new uint8_t[code_len / 4 * 3];
		vector<uint8_t> plain(code_len / 4 * 3, 0);

		//开始解码
		uint32_t i, j = 0;
		uint8_t quad[4];
		for (i = 0; i < code_len; i+=4)
		{
			for (uint32_t k = 0; k < 4; k++)
			{
				quad[k] = reverse_map[code[i + k]];//分组，每组四个分别依次转换为base64表内的十进制数
			}

			if(!(quad[0] < 64 && quad[1] < 64))
				throw runtime_error("Error! Base64 code is bad");

			plain[j++] = (quad[0] << 2) | (quad[1] >> 4); //取出第一个字符对应base64表的十进制数的前6位与第二个字符对应base64表的十进制数的前2位进行组合

			if (quad[2] >= 64)
				break;
			else if (quad[3] >= 64)
			{
				plain[j++] = (quad[1] << 4) | (quad[2] >> 2); //取出第二个字符对应base64表的十进制数的后4位与第三个字符对应base64表的十进制数的前4位进行组合
				break;
			}
			else
			{
				plain[j++] = (quad[1] << 4) | (quad[2] >> 2);
				plain[j++] = (quad[2] << 6) | quad[3];//取出第三个字符对应base64表的十进制数的后2位与第4个字符进行组合
			}
		}
		plain.resize(j);
		return plain;
	}

}


string Base64::encode(vector<uint8_t> d)
{
	auto d2 = base64_encode(d.data(), d.size());
	string s((char*)d2.data(), d2.size());
	//s.assign((char*)d2.data(), d2.size());
	return s;
}

vector<uint8_t> Base64::decode(string s)
{
	vector<uint8_t> s2(s.size());
	memcpy_s(s2.data(), s2.size(), s.data(), s.size());
	auto d = base64_decode(s2.data(), s2.size());
	return d;
}
