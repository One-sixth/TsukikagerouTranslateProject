#pragma once
#define _CRT_SECURE_NO_WARNINGS 1

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <format>
#include "file_utils.hpp"
#include "CgfFile.h"
#include "DeflateUtils.h"

using namespace std;


const vector<uint8_t> const_snr_header ={ 0x53, 0x4E, 0x52, 0x1A };
const vector<uint8_t> const_z_header ={ 0x78, 0xDA };


class SnrFileReader
{
public:
	// type == 0 ���������ױ���
	// type == 1 ���������� fandisk ǧ������

	void load_enc_file(string file, int type);
	void save_dec_file(string file);

	void decrypt_and_decompress(int type);

	vector<uint8_t> decrypt(vector<uint8_t> data);
	vector<uint8_t> decrypt_2(vector<uint8_t> data);

public:
	// �����
	uint32_t enc_size;
	uint32_t dec_size;
	vector<uint8_t> enc_data;

	// enc_data ���ܺ��
	vector<uint8_t> z_check_code;
	vector<uint8_t> z_header;
	vector<uint8_t> z_data;

	// z_data ��ѹ�����
	vector<uint8_t> dec_data;
};


class SnrFileWriter
{
public:
	void load_dec_file(string file, int type);
	void save_enc_file(string file);

	void encrypt_and_compress(int type);

	vector<uint8_t> encrypt(vector<uint8_t> data);
	vector<uint8_t> encrypt_2(vector<uint8_t> data);

	vector<uint8_t> make_check_sum(vector<uint8_t> data);

public:

	// Ҫд����
	uint32_t enc_size;
	uint32_t dec_size;
	vector<uint8_t> enc_data;

	// enc_data ���ܺ��
	vector<uint8_t> z_check_code;	// �㷨δ֪��Ŀǰ���̶�Ϊ 4��0
	vector<uint8_t> z_header;
	vector<uint8_t> z_data;

	// z_data ��ѹ�����
	vector<uint8_t> dec_data;
};

