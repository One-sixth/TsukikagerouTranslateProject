#include "ArcFile.h"


// ------------------------ ArcFileReader ------------------------


void ArcFileReader::open(string file_path)
{
	f.close();

	f.open(file_path, ios::binary);
	if (!f.is_open())
		throw runtime_error("Error! Can not open file.");

	vector<uint8_t> header = file_utils::Read(f, 4);

	if (header != const_arc_header)
		throw runtime_error("Error! Not a valid ARC file.");

	int file_count;
	file_utils::ReadType(f, file_count);

	item_list.reserve(file_count);

	for (int i = 0; i < file_count; ++i)
	{
		Item it ={0};
		file_utils::ReadType(f, it);
		item_list.push_back(it);
	}

}

void ArcFileReader::extract(string out_dir)
{
	if (!f.is_open())
		throw runtime_error("Error!");

	for (auto item : item_list)
	{
		f.seekg(item.begin, ios::beg);
		auto data = file_utils::Read(f, item.size);
		
		auto out_name = format((string)"{0}/{1}", out_dir, (string)item.name);
		ofstream of(out_name, ios::binary);
		if (!of.is_open())
			throw runtime_error("Error!");
		file_utils::Write(of, data);
		of.close();
	}
}


// ------------------------ ArcFileWriter ------------------------


void ArcFileWriter::open_dir(string dir_path)
{
	if (!filesystem::directory_entry(dir_path).is_directory())
		throw runtime_error("Error! Target is not a directory.");

	for (auto it : filesystem::directory_iterator(dir_path))
	{
		if (!it.is_regular_file())
			throw runtime_error("Error! Found a item is not a file.");

		if (it.path().filename().string().size() > 23)
			throw runtime_error("Error! Filename is too long.");

		auto f = ifstream(it.path().string(), ios::binary);
		Index item2 ={};
		item2.size = file_utils::GetFileSize(f);
		item2.path = it.path().string();
		item2.name = it.path().filename().string();
		//item2.begin = 0;
		f.close();
		if (it.file_size() != item2.size)
			cout << "Warning! File size is not match." << endl;
		index_list.push_back(item2);
	}
}

void ArcFileWriter::write(string file_path)
{
	vector<Item> arc_index_list;
	arc_index_list.reserve(index_list.size());
	
	int header_size = 4;
	int file_count_size = 4;
	int arc_index_seg_size = sizeof(Item) * index_list.size();

	int cur_start_pos = header_size + file_count_size + arc_index_seg_size;

	for (Index item2 : index_list)
	{
		Item item ={};
		item.begin = cur_start_pos;
		item.size = item2.size;
		memcpy_s(item.name, sizeof(item.name), item2.name.data(), item2.name.size());
		arc_index_list.push_back(item);

		cur_start_pos += item2.size;
	}

	ofstream of(file_path, ios::binary);
	if (!of.is_open())
		throw runtime_error("Error! Can not open file.");

	file_utils::Write(of, const_arc_header);
	uint32_t file_count = index_list.size();
	file_utils::WriteType(of, file_count);
	file_utils::Write(of, arc_index_list);
	for (Index item2 : index_list)
	{
		ifstream f(item2.path, ios::binary);
		auto data = file_utils::ReadAll(f);
		file_utils::Write(of, data);
	}
	of.close();
}
