#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <format>
#include <algorithm>
#include "file_utils.hpp"
#include "CgfFile.h"
#include "DeflateUtils.h"
#include "Base64.h"


using namespace std;


//const vector<uint8_t> const_snr_text_header{ 1, 0 };


class SnrTextFileReader
{
public:
	struct Block
	{
		// ��ȡ���
		vector<uint8_t>	before_data;
		vector<uint8_t>	text_data;
		vector<uint8_t>	after_data;

		bool	is_offset_block = false;
		bool	is_branch_block = false;
		int16_t	rel_offset = 0;
	};

public:
	void load_pack_file(string file);
	void save_unpack_file(string file);

public:
	string file_type;
	vector<uint8_t> header;
	vector<Block> block_list;
	vector<uint8_t> end_data;

	// ��֧�����
	int found_blance_block = 0;
	// ��֧���ƺ�ֻ�����ڵ������������棬��֧�飬ÿ��snr�ļ�Ҳֻ��һ��
	// --��Ϊƫ�ƿ��ƺ�ֻ������ڷ�֧����棬����ƫ�ƿ���ڷ��ַ�֧����������
	// �����һ�仰��ƫ�ƿ���û�з�֧��ʱҲ���ҵ��ˡ�����Ҳ����ĩβ�����ǳ���ֻ��0x12������֮ǰ���ֵ�0x13��Ҫע��

};


class SnrTextFileWriter
{
public:
	struct Block
	{
		// ��ȡ���
		vector<uint8_t>	before_data;
		vector<uint8_t>	text_data;
		vector<uint8_t>	after_data;

		bool	is_branch_block = false;
		bool	is_offset_block = false;
		int16_t	rel_offset = 0;

	public:
		inline int size() const
		{
			return before_data.size() + text_data.size() + after_data.size();
		}
	};

public:
	void load_unpack_file(string file);
	void save_pack_file(string file);

	// �����֧ƫ�ƺ���Ҫ�������»ᵼ�������ױ�����Ϸ��֧ʱ���ˡ�
	//void update_select_blance_offset();

public:
	string file_type;
	vector<uint8_t> header;
	vector<Block> block_list;
	vector<uint8_t> end_data;
};

