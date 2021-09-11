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
		// 提取后的
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

	// 分支块计数
	int found_blance_block = 0;
	// 分支块似乎只出现在倒数几个块里面，分支块，每个snr文件也只有一个
	// --因为偏移块似乎只会出现在分支块后面，所以偏移块仅在发现分支块后才允许发现
	// 否决上一句话，偏移块在没有分支块时也被找到了。不过也是在末尾，但是长度只有0x12，不是之前发现的0x13，要注意

};


class SnrTextFileWriter
{
public:
	struct Block
	{
		// 提取后的
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

	// 这个分支偏移很重要，不更新会导致月阳炎本体游戏分支时闪退。
	//void update_select_blance_offset();

public:
	string file_type;
	vector<uint8_t> header;
	vector<Block> block_list;
	vector<uint8_t> end_data;
};

