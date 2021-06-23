#include "Base64.h"


namespace
{

	//��������ֵ�
	uint8_t alphabet_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	//��������ֵ�
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
	*  ����
	*  ������Ҫ��������ݵ�ַ�����ݳ���
	*  ����:����������
	*/
	vector<uint8_t> base64_encode(const uint8_t* text, uint32_t text_len)
	{
		//������������ݳ���
		//�����Ͽ�֪  Base64���ǽ�3���ֽڵ�����(24λ)�����4��6λ�����ݣ�Ȼ��ǰ��λ����
		//����ת��Ϊ0-63������  Ȼ����ݱ����ֵ���б���
		int encode_length = text_len / 3 * 4;
		if (text_len % 3 > 0)
		{
			encode_length += 4;
		}

		//Ϊ��������ݴ�ŵ�ַ�����ڴ�
		//uint8_t* encode = (uint8_t*)malloc(encode_length);
		//uint8_t* encode = new uint8_t[encode_length];
		vector<uint8_t> encode(encode_length, 0);

		//����
		uint32_t i, j;
		for (i = 0, j = 0; i + 3 <= text_len; i+=3)
		{
			encode[j++] = alphabet_map[text[i] >> 2];                             //ȡ����һ���ַ���ǰ6λ���ҳ���Ӧ�Ľ���ַ�
			encode[j++] = alphabet_map[((text[i] << 4) & 0x30) | (text[i + 1] >> 4)];     //����һ���ַ��ĺ�2λ��ڶ����ַ���ǰ4λ������ϲ��ҵ���Ӧ�Ľ���ַ�
			encode[j++] = alphabet_map[((text[i + 1] << 2) & 0x3c) | (text[i + 2] >> 6)];   //���ڶ����ַ��ĺ�4λ��������ַ���ǰ2λ��ϲ��ҳ���Ӧ�Ľ���ַ�
			encode[j++] = alphabet_map[text[i + 2] & 0x3f];                         //ȡ���������ַ��ĺ�6λ���ҳ�����ַ�
		}

		//������󲻹�3���ֽڵ�  �������
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
		//�ɱ��봦��֪��������base64����һ����4�ı������ֽ�
		if ((code_len & 0x03) != 0)  //��������������ش�������ֹ����ִ�С�4�ı�����
		{
			throw runtime_error("Error! Base64 code is bad");
		}

		//Ϊ���������ݵ�ַ�����ڴ�
		//uint8_t* plain = (uint8_t*)malloc(code_len / 4 * 3);
		//uint8_t* plain = new uint8_t[code_len / 4 * 3];
		vector<uint8_t> plain(code_len / 4 * 3, 0);

		//��ʼ����
		uint32_t i, j = 0;
		uint8_t quad[4];
		for (i = 0; i < code_len; i+=4)
		{
			for (uint32_t k = 0; k < 4; k++)
			{
				quad[k] = reverse_map[code[i + k]];//���飬ÿ���ĸ��ֱ�����ת��Ϊbase64���ڵ�ʮ������
			}

			if(!(quad[0] < 64 && quad[1] < 64))
				throw runtime_error("Error! Base64 code is bad");

			plain[j++] = (quad[0] << 2) | (quad[1] >> 4); //ȡ����һ���ַ���Ӧbase64���ʮ��������ǰ6λ��ڶ����ַ���Ӧbase64���ʮ��������ǰ2λ�������

			if (quad[2] >= 64)
				break;
			else if (quad[3] >= 64)
			{
				plain[j++] = (quad[1] << 4) | (quad[2] >> 2); //ȡ���ڶ����ַ���Ӧbase64���ʮ�������ĺ�4λ��������ַ���Ӧbase64���ʮ��������ǰ4λ�������
				break;
			}
			else
			{
				plain[j++] = (quad[1] << 4) | (quad[2] >> 2);
				plain[j++] = (quad[2] << 6) | quad[3];//ȡ���������ַ���Ӧbase64���ʮ�������ĺ�2λ���4���ַ��������
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
