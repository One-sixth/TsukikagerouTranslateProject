#include "SnrTextFile.h"
#include "../deps/json.hpp"

using json = nlohmann::json;

namespace
{
	//void erase_all_empty_char(string& s)
	//{
	//	const vector<char> empty_char{0x20, 0x0A, 0x0D};
	//	for (char c : empty_char)
	//		s.erase(std::remove(s.begin(), s.end(), c), s.end());
	//}
}


// -----------------------SnrTextFileReader-----------------------


void SnrTextFileReader::load_pack_file(string file)
{
	block_list.clear();

	ifstream f = file_utils::OpenBinFileRead(file);

	header = file_utils::Read(f, 2);
	
	// 有一些snr文件不包含文本，那么将它们原样保存
	//if (header == const_snr_text_header)
	// snr_text 没有文件头，开头两个字节似乎是分支号
	if (true)
	{
		// snr_text 类型
		file_type = "snr_text";

		// 先读取所有块的内容
		vector<vector<uint8_t>>	bytes_blocks;
		vector<int32_t>	bytes_blocks_begin_offset;

		while (true)
		{
			unsigned short block_size;
			file_utils::ReadType(f, block_size);

			bytes_blocks_begin_offset.push_back(f.tellg());

			if (block_size == 0xFFFF)
			{
				end_data = file_utils::ReadAll(f);
				break;
			}
			else
			{
				vector<uint8_t> block = file_utils::Read(f, block_size - 2);
				bytes_blocks.push_back(block);
			}
		}

		// 检查是否为主分支块
		auto try_main_blance_block = [this](vector<uint8_t> bb, Block& block, bool& success) -> void
		{
			success = false;
			
			bool b1 = bb.size() >= 18;

			if (!b1)
				return;

			uint16_t m;
			memcpy_s(&m, 2, bb.data() + 8, 2);

			if (m == 0xC8 || m == 0xC9)
			{
				// 成功发现分支块
				found_blance_block += 1;

				// 分支块应该只有一个，单个snr文件内不会有多个
				if (found_blance_block >= 2)
					throw runtime_error("Error! Found multi blance block in single snr file.");

				uint16_t n_blance;
				memcpy_s(&n_blance, 2, bb.data() + 6, 2);

				// 如果分支数小于2个或大于5个，需要进一步检查文件结构，这不应该出现
				if (n_blance < 2 || n_blance > 5)
					throw runtime_error("Error! Found abnormal blance num.");

				vector<uint32_t> null_pos;
				auto before_data = vector<uint8_t>(bb.begin(), bb.begin()+14);
				for (int nn = 0; nn < n_blance; ++nn)
				{
					if (null_pos.size() == 0)
					{
						auto p = find(bb.begin() + 14, bb.end(), 0);
						if (p == bb.end())
							throw runtime_error("Error! Wrong blance block str.");
						null_pos.push_back(p - bb.begin());
					}
					else
					{
						auto p = find(bb.begin() + null_pos.back() + 1, bb.end(), 0);
						if (p == bb.end())
							throw runtime_error("Error! Wrong blance block str.");
						null_pos.push_back(p - bb.begin());
					}
				}

				// 因为分支块可以有多个语句，所以这里的字符串是特殊构造，用0字节分隔
				// python那边记得处理
				block = Block();
				block.is_branch_block = true;
				block.before_data = vector<uint8_t>(bb.begin(), bb.begin()+14);
				block.text_data = vector<uint8_t>(bb.begin()+14, bb.begin()+null_pos.back());
				block.after_data = vector<uint8_t>(bb.begin()+null_pos.back(), bb.end());

				success = true;
			}
		};

		// 检查是否为偏移块
		auto try_offset_block = [this](vector<uint8_t> bb, int32_t bb_offset, Block& block, bool& success) -> void
		{
			success = false;

			// 偏移块前面不一定有分支块，啧。。。继续尝试更改条件
			//if (found_blance_block > 0 && bb.size() == 0x13-2)
			if (bb.size() == 0x13-2 || bb.size() == 0x12 - 2)
			{
				// 可能确认了，这个值固定是长跳转值
				int16_t g_off = *(int16_t*)(bb.data() + 2);
				if (g_off == 0)
					return;

				if (g_off <= (int16_t)bb_offset)
					throw runtime_error("Error! Offset is abnormal.");

				block = Block();
				block.before_data = bb;
				block.is_offset_block = true;
				block.rel_offset = g_off - (int16_t)bb_offset;

				success = true;
			}
		};

		// 检查是否为文本块
		auto try_text_block = [this](vector<uint8_t> bb, int32_t bb_offset, Block& block, bool& success) -> void
		{
			success = false;

			bool b1 = bb.size() >= 18;
			if (!b1)
				return;

			uint32_t a = 0;
			for (int i = 0; i < 14; ++i)
				a+=bb[i];

			if (a != 0)
				return;

			vector<uint8_t>::iterator pos;
			// 寻找0字节
			pos = find(bb.begin() + 14, bb.end(), 0);
			if (pos == bb.end())
				return;

			block = Block();
			block.before_data.assign(bb.begin(), bb.begin() + 14);
			block.text_data.assign(bb.begin() + 14, pos);
			block.after_data.assign(pos, bb.end());

			success = true;
		};

		// 解析块循环
		for (auto b_i = 0; b_i < bytes_blocks.size(); ++b_i)
		{
			bool success = false;
			Block block;

			auto& bb = bytes_blocks[b_i];
			auto& bb_offset = bytes_blocks_begin_offset[b_i];

			if (!success)
				try_offset_block(bb, bb_offset, block, success);

			if (!success)
				try_main_blance_block(bb, block, success);

			if (!success)
				try_text_block(bb, bb_offset, block, success);

			if (!success)
			{
				block = Block();
				block.before_data = bb;
			}

			block_list.push_back(block);
		}
	}
	else
	{
		// 未知类型
		file_type = "unknow";

		f.seekg(0);
		end_data = file_utils::ReadAll(f);
	}

	

	// 文件格式
	// 2字节头
	// 2字节块大小，设为N
	// 块：(N-2)字节块内容，2字节下一次读取大小
	// 循环直到2字节块大小变成FF
	// 剩下内容未完全弄懂，仅知道是文件结束内容

	// 进一步提取数据
	// 如果是文本块，其内容格式为
	// 14个00字节+shiftjis文本+1个00字节+2个未知用途字节

	// 当大小后面有连续14个00字节，并且大小大于等于18个字节时，才可能包含文本
	// 目前策略，检测大小大于等于20个字节，并且大小后面有14个00字节，
	// 然后获取一个00结尾字符串，里面要求要有shiftjis编码的字符，才认为是文本
	// 算了，判断为shiftjis字符还是放在另外一个程序里面识别，这里就不判断了。

	/*
	* 以下说明，从0开始开始计数，第0个字节代表块内偏移为0的字节的位置
	*
	* 分支块，一个分支块后面会携带几组辅助块
	* 分支块特征，第6-7字节为分支数（可能，因为常见 02 00），第8-9字节固定为 C8 00 代表是分支块
	* 每组里面有2个辅助块，第一个辅助块的第2-3字节为全文件偏移，指向第二个辅助块第14个字节处
	*
	* 有个方法比较轻松处理这个问题，记下偏移内容减去块开始的坐标，到时加回新的块开始坐标，这样就比较方便
	* 因为那个偏移指向的块似乎是比较固定的，并且对辅助块，我这里不会做任何打乱顺序的事情
	*
	*/

	// TODO
	/*
	* 新的处理，分支块前面，有时会有长跳转块，长跳转会跳到分支块文本最后一个文本的后面，这里要特殊处理了。
	* 如果分支块前是一个长跳转块，修改该长跳转块的跳转地址
	*/
}

void SnrTextFileReader::save_unpack_file(string file)
{
	ofstream f = file_utils::OpenBinFileWrite(file);

	json d;

	d["file_type"] = file_type;
	d["header"] = Base64::encode(header);
	d["end_data"] = Base64::encode(end_data);

	for (auto b : block_list)
	{
		json node;

		auto tmp_before_data = Base64::encode(b.before_data);
		auto tmp_text_data = Base64::encode(b.text_data);
		auto tmp_after_data = Base64::encode(b.after_data);

		node["before_data"] = tmp_before_data;
		node["text_data"] = tmp_text_data;
		node["after_data"] = tmp_after_data;
		node["is_branch_block"] = b.is_branch_block;
		node["is_offset_block"] = b.is_offset_block;
		node["rel_offset"] = b.rel_offset;

		d["block_list"].push_back(node);
	}

	string json_text = d.dump(2, 32, true);
	file_utils::Write(f, json_text.size(), json_text.data());

	f.close();
}


// -----------------------SnrTextFileWriter-----------------------


void SnrTextFileWriter::load_unpack_file(string file)
{
	block_list.clear();

	ifstream f = file_utils::OpenBinFileRead(file);
	auto d_data = file_utils::ReadAll(f);
	string d_text((char*)d_data.data(), d_data.size());

	json d = json::parse(d_text.c_str());

	file_type = d["file_type"].get<string>();

	header = Base64::decode(d["header"].get<string>());

	for (auto it=d["block_list"].begin(); it != d["block_list"].end(); ++it)
	{
		Block new_block;

		// 手动处理下可能多出的空白字符串
		// 改为json后，现在不会出现多余空白了
		auto tmp_s_before_data = (*it)["before_data"].get<string>();
		auto tmp_s_text_data = (*it)["text_data"].get<string>();
		auto tmp_s_after_data = (*it)["after_data"].get<string>();

		auto tmp_before_data = Base64::decode(tmp_s_before_data);
		auto tmp_text_data = Base64::decode(tmp_s_text_data);
		auto tmp_after_data = Base64::decode(tmp_s_after_data);

		auto tmp_is_offset_block = (*it)["is_offset_block"].get<bool>();
		auto tmp_is_branch_block = (*it)["is_branch_block"].get<bool>();
		auto tmp_rel_offset = (*it)["rel_offset"].get<int>();

		new_block.before_data = tmp_before_data;
		new_block.text_data = tmp_text_data;
		new_block.after_data = tmp_after_data;
		new_block.is_branch_block = tmp_is_branch_block;
		new_block.is_offset_block = tmp_is_offset_block;
		new_block.rel_offset = tmp_rel_offset;
		block_list.push_back(new_block);
	}

	auto tmp_end_data = Base64::decode(d["end_data"].get<string>());
	end_data = tmp_end_data;

}

void SnrTextFileWriter::save_pack_file(string file)
{
	ofstream f = file_utils::OpenBinFileWrite(file);

	// 尝试找到分支块和得到分支块的起始位置
	auto find_branch_block = [this](int cur_block_id, int cur_block_begin_offset, int max_try, int& out_branch_block_id, int& out_branch_block_begin_offset) -> void
	{
		int off = cur_block_begin_offset + block_list[cur_block_id].size();
		int branch_block_id = -1;
		for (int bid = cur_block_id+1; bid < min(cur_block_id + 1 + max_try, (int)block_list.size()); ++bid)
		{
			off += 2;
			if (block_list[bid].is_branch_block)
			{
				branch_block_id = bid;
				break;
			}
			else
			{
				off += block_list[bid].size();
			}
		}

		out_branch_block_id = branch_block_id;
		out_branch_block_begin_offset = off;

	};

	if (file_type == "snr_text")
	{
		file_utils::Write(f, header);

		bool found_branch = false;

		for (int cur_block_id = 0; cur_block_id < block_list.size(); ++cur_block_id)
		{
			const Block& b = block_list[cur_block_id];

			// 不要忘记+2，+2是下一次读取的大小
			auto ori_block_size = b.size() + 2;

			if (ori_block_size >= 0xffff)
				throw runtime_error("Error! Block is bigger than 0xffff.");

			unsigned short block_size = ori_block_size;
			file_utils::WriteType(f, block_size);

			auto tmp_before_data = b.before_data;

			// 如果是特定的分支辅助块，特殊处理
			if (b.is_offset_block)
			{
				int16_t cur_block_begin = (int16_t)f.tellp();
				int16_t& target_offset = *(int16_t*)(tmp_before_data.data() + 2);

				int out_branch_bid = -1;
				int out_branch_block_begin_offset = -1;
				find_branch_block(cur_block_id, cur_block_begin, 3, out_branch_bid, out_branch_block_begin_offset);

				if (out_branch_bid == -1 || found_branch || b.rel_offset < 40)
				{
					// 大部分时候都在分支块后面
					// 部分时候独立存在
					// cout << "Found offset block not need with branch! " << file << " " << cur_block_id << endl;
					target_offset = b.rel_offset + cur_block_begin;
				}
				else
				{
					// 如果后面有分支块
					target_offset = out_branch_block_begin_offset + block_list[out_branch_bid].size() - 2;
				}
			}

			found_branch = found_branch || b.is_branch_block;

			file_utils::Write(f, tmp_before_data);
			file_utils::Write(f, b.text_data);
			file_utils::Write(f, b.after_data);
		}
		// 写入FFFF代表文件结束
		file_utils::Write(f, vector<uint8_t>{0xff, 0xff});
		file_utils::Write(f, end_data);
	}
	else if (file_type == "unknow")
	{
		file_utils::Write(f, end_data);
	}
	else
		throw runtime_error("Error! Wrong file type.");
}

//void SnrTextFileWriter::update_select_blance_offset()
//{
//}
