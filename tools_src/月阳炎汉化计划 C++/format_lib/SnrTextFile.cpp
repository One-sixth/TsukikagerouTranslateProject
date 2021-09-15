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
	
	// ��һЩsnr�ļ��������ı�����ô������ԭ������
	//if (header == const_snr_text_header)
	// snr_text û���ļ�ͷ����ͷ�����ֽ��ƺ��Ƿ�֧��
	if (true)
	{
		// snr_text ����
		file_type = "snr_text";

		// �ȶ�ȡ���п������
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

		// ����Ƿ�Ϊ����֧��
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
				// �ɹ����ַ�֧��
				found_blance_block += 1;

				// ��֧��Ӧ��ֻ��һ��������snr�ļ��ڲ����ж��
				if (found_blance_block >= 2)
					throw runtime_error("Error! Found multi blance block in single snr file.");

				uint16_t n_blance;
				memcpy_s(&n_blance, 2, bb.data() + 6, 2);

				// �����֧��С��2�������5������Ҫ��һ������ļ��ṹ���ⲻӦ�ó���
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

				// ��Ϊ��֧������ж����䣬����������ַ��������⹹�죬��0�ֽڷָ�
				// python�Ǳ߼ǵô���
				block = Block();
				block.is_branch_block = true;
				block.before_data = vector<uint8_t>(bb.begin(), bb.begin()+14);
				block.text_data = vector<uint8_t>(bb.begin()+14, bb.begin()+null_pos.back());
				block.after_data = vector<uint8_t>(bb.begin()+null_pos.back(), bb.end());

				success = true;
			}
		};

		// ����Ƿ�Ϊƫ�ƿ�
		auto try_offset_block = [this](vector<uint8_t> bb, int32_t bb_offset, Block& block, bool& success) -> void
		{
			success = false;

			// ƫ�ƿ�ǰ�治һ���з�֧�飬���������������Ը�������
			//if (found_blance_block > 0 && bb.size() == 0x13-2)
			if (bb.size() == 0x13-2 || bb.size() == 0x12 - 2)
			{
				// ����ȷ���ˣ����ֵ�̶��ǳ���תֵ
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

		// ����Ƿ�Ϊ�ı���
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
			// Ѱ��0�ֽ�
			pos = find(bb.begin() + 14, bb.end(), 0);
			if (pos == bb.end())
				return;

			block = Block();
			block.before_data.assign(bb.begin(), bb.begin() + 14);
			block.text_data.assign(bb.begin() + 14, pos);
			block.after_data.assign(pos, bb.end());

			success = true;
		};

		// ������ѭ��
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
		// δ֪����
		file_type = "unknow";

		f.seekg(0);
		end_data = file_utils::ReadAll(f);
	}

	

	// �ļ���ʽ
	// 2�ֽ�ͷ
	// 2�ֽڿ��С����ΪN
	// �飺(N-2)�ֽڿ����ݣ�2�ֽ���һ�ζ�ȡ��С
	// ѭ��ֱ��2�ֽڿ��С���FF
	// ʣ������δ��ȫŪ������֪�����ļ���������

	// ��һ����ȡ����
	// ������ı��飬�����ݸ�ʽΪ
	// 14��00�ֽ�+shiftjis�ı�+1��00�ֽ�+2��δ֪��;�ֽ�

	// ����С����������14��00�ֽڣ����Ҵ�С���ڵ���18���ֽ�ʱ���ſ��ܰ����ı�
	// Ŀǰ���ԣ�����С���ڵ���20���ֽڣ����Ҵ�С������14��00�ֽڣ�
	// Ȼ���ȡһ��00��β�ַ���������Ҫ��Ҫ��shiftjis������ַ�������Ϊ���ı�
	// ���ˣ��ж�Ϊshiftjis�ַ����Ƿ�������һ����������ʶ������Ͳ��ж��ˡ�

	/*
	* ����˵������0��ʼ��ʼ��������0���ֽڴ������ƫ��Ϊ0���ֽڵ�λ��
	*
	* ��֧�飬һ����֧������Я�����鸨����
	* ��֧����������6-7�ֽ�Ϊ��֧�������ܣ���Ϊ���� 02 00������8-9�ֽڹ̶�Ϊ C8 00 �����Ƿ�֧��
	* ÿ��������2�������飬��һ��������ĵ�2-3�ֽ�Ϊȫ�ļ�ƫ�ƣ�ָ��ڶ����������14���ֽڴ�
	*
	* �и������Ƚ����ɴ���������⣬����ƫ�����ݼ�ȥ�鿪ʼ�����꣬��ʱ�ӻ��µĿ鿪ʼ���꣬�����ͱȽϷ���
	* ��Ϊ�Ǹ�ƫ��ָ��Ŀ��ƺ��ǱȽϹ̶��ģ����ҶԸ����飬�����ﲻ�����κδ���˳�������
	*
	*/

	// TODO
	/*
	* �µĴ�����֧��ǰ�棬��ʱ���г���ת�飬����ת��������֧���ı����һ���ı��ĺ��棬����Ҫ���⴦���ˡ�
	* �����֧��ǰ��һ������ת�飬�޸ĸó���ת�����ת��ַ
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

		// �ֶ������¿��ܶ���Ŀհ��ַ���
		// ��Ϊjson�����ڲ�����ֶ���հ���
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

	// �����ҵ���֧��͵õ���֧�����ʼλ��
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

			// ��Ҫ����+2��+2����һ�ζ�ȡ�Ĵ�С
			auto ori_block_size = b.size() + 2;

			if (ori_block_size >= 0xffff)
				throw runtime_error("Error! Block is bigger than 0xffff.");

			unsigned short block_size = ori_block_size;
			file_utils::WriteType(f, block_size);

			auto tmp_before_data = b.before_data;

			// ������ض��ķ�֧�����飬���⴦��
			if (b.is_offset_block)
			{
				int16_t cur_block_begin = (int16_t)f.tellp();
				int16_t& target_offset = *(int16_t*)(tmp_before_data.data() + 2);

				int out_branch_bid = -1;
				int out_branch_block_begin_offset = -1;
				find_branch_block(cur_block_id, cur_block_begin, 3, out_branch_bid, out_branch_block_begin_offset);

				if (out_branch_bid == -1 || found_branch || b.rel_offset < 40)
				{
					// �󲿷�ʱ���ڷ�֧�����
					// ����ʱ���������
					// cout << "Found offset block not need with branch! " << file << " " << cur_block_id << endl;
					target_offset = b.rel_offset + cur_block_begin;
				}
				else
				{
					// ��������з�֧��
					target_offset = out_branch_block_begin_offset + block_list[out_branch_bid].size() - 2;
				}
			}

			found_branch = found_branch || b.is_branch_block;

			file_utils::Write(f, tmp_before_data);
			file_utils::Write(f, b.text_data);
			file_utils::Write(f, b.after_data);
		}
		// д��FFFF�����ļ�����
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
