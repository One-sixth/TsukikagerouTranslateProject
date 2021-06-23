#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include "../format_lib/file_utils.hpp"
#include "../format_lib/SnrTextFile.h"
#include <filesystem>
#include <format>
#include "../deps/cxxopts.hpp"


#pragma comment(lib, "format_lib.lib")


/*
--action unpack --in "G:\月陽炎DVD\tkmp2" --out "G:\月陽炎DVD\tkmp2_unpack"
--action repack --in "G:\月陽炎DVD\tkmp2_unpack" --out "G:\月陽炎DVD\tkmp2_repack"
--action repack --in "G:\月陽炎DVD\tkmp2_unpack_trans" --out "G:\月陽炎DVD\tkmp2_repack"

--action unpack --in "G:\月陽炎DVD\tmp4" --out "G:\月陽炎DVD\tmp4_unpack"
--action repack --in "G:\月陽炎DVD\tmp4_unpack" --out "G:\月陽炎DVD\tmp4_repack"
--action repack --in "G:\月陽炎DVD\tmp4_unpack_trans" --out "G:\月陽炎DVD\tmp4_repack"
*/


using namespace std;


vector<string> make_file_list(string dir_path_or_file_path)
{
	if (!filesystem::directory_entry(dir_path_or_file_path).exists())
		throw runtime_error("Error! Target is not exist.");

	if (filesystem::is_regular_file(dir_path_or_file_path))
		return { dir_path_or_file_path };

	if (filesystem::is_directory(dir_path_or_file_path))
	{
		vector<string> files;
		for (auto p : filesystem::directory_iterator(dir_path_or_file_path))
		{
			if (p.is_regular_file())
				files.push_back(p.path().string());
			else
				cout << format((string)"Warnin. {0} a normal file. Will ignore.", p.path().string()) << endl;
		}
		return files;
	}

	throw runtime_error("Error! Target is not file or dir.");
}


int main(int argc, const char* argv[])
{
	//{
	//	SnrFileReader reader;
	//	reader.load_enc_file("G:\\月陽炎DVD\\tmp3\\h_04_01.snr");
	//	vector<uint8_t> tmp_data;
	//	tmp_data.insert(tmp_data.end(), reader.z_header.begin(), reader.z_header.end());
	//	tmp_data.insert(tmp_data.end(), reader.z_data.begin(), reader.z_data.end());
	//	SnrFileWriter writer;
	//	auto chksum = writer.make_check_sum(tmp_data);
	//	cout << chksum[0] << endl;
	//}

	//// 特别处理
	//return 0;


	auto options = cxxopts::Options(argv[0], "月阳炎 Snr Text Tool");
	options.add_options()
		("action", "which operator. Only allow \"repack\" or \"unpack\"", cxxopts::value<std::string>())
		("in", "input file or input dir", cxxopts::value<std::string>())
		("out", "output dir", cxxopts::value<std::string>());

	auto result = options.parse(argc, argv);

	string action, in, out;

	try
	{
		action = result["action"].as<std::string>();
		in = result["in"].as<std::string>();
		out = result["out"].as<std::string>();
	}
	catch (const std::exception& e)
	{
		cout << options.help() << endl;
		cout << "Fail, because " << e.what() << endl;
		return -1;
	}

	cout << "Cur action = " << action << endl;

	if (!filesystem::exists(out))
		filesystem::create_directories(out);
	else if (!filesystem::is_directory(out))
	{
		cout << format((string)"Error! out param {0} is not a dir.", out);
	}

	if (action == "repack")
	{
		auto files = make_file_list(in);

		for (auto p : files)
		{
			string out_path = format((string)"{0}/{1}", out, filesystem::path(p).filename().string());

			SnrTextFileWriter writer;
			writer.load_unpack_file(p);
			writer.save_pack_file(out_path);
		}

		cout << "Success" << endl;
	}
	else if (action == "unpack")
	{
		auto files = make_file_list(in);

		for (auto p : files)
		{
			string out_path = format((string)"{0}/{1}", out, filesystem::path(p).filename().string());

			SnrTextFileReader reader;
			reader.load_pack_file(p);
			reader.save_unpack_file(out_path);
		}

		cout << "Success" << endl;
	}
	else
	{
		cout << "Error! actionn param only allow \"repack\" or \"unpack\"" << endl;;
	}
}