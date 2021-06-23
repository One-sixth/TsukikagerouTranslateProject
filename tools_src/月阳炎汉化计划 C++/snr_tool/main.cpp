#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include "../format_lib/file_utils.hpp"
#include "../format_lib/SnrFile.h"
#include <filesystem>
#include <format>
#include "../deps/cxxopts.hpp"


#pragma comment(lib, "format_lib.lib")


/*
--type 0 --action decrypt --in "G:\月陽炎DVD\tmp3" --out "G:\月陽炎DVD\tmp4"
--type 0 --action encrypt --in "G:\月陽炎DVD\tmp4" --out "G:\月陽炎DVD\tmp5"

--type 1 --action decrypt --in "G:\月陽炎DVD\tkmp1" --out "G:\月陽炎DVD\tkmp2"
--type 1 --action encrypt --in "G:\月陽炎DVD\tkmp2" --out "G:\月陽炎DVD\tkmp3"
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


	auto options = cxxopts::Options(argv[0], "月阳炎 Snr Tool");
	options.add_options()
		("type", "which game. \"0\" is 月阳炎, \"1\" is 月阳炎-千秋恋歌-. Only allow \"0\" or \"1\"", cxxopts::value<int>())
		("action", "which operator. Only allow \"encrypt\" or \"decrypt\"", cxxopts::value<std::string>())
		("in", "input file or input dir", cxxopts::value<std::string>())
		("out", "output dir", cxxopts::value<std::string>());

	auto result = options.parse(argc, argv);

	string action, in, out;
	int type;

	try
	{
		type = result["type"].as<int>();
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

	if (action == "encrypt")
	{
		auto files = make_file_list(in);

		for (auto p : files)
		{
			string out_path = format((string)"{0}/{1}", out, filesystem::path(p).filename().string());

			SnrFileWriter writer;
			writer.load_dec_file(p, type);
			writer.save_enc_file(out_path);
		}

		cout << "Success" << endl;
	}
	else if (action == "decrypt")
	{
		auto files = make_file_list(in);

		for (auto p : files)
		{
			string out_path = format((string)"{0}/{1}", out, filesystem::path(p).filename().string());

			SnrFileReader reader;
			reader.load_enc_file(p, type);
			reader.save_dec_file(out_path);
		}

		cout << "Success" << endl;
	}
	else
	{
		cout << "Error! actionn param only allow \"encrypt\" or \"decrypt\"" << endl;;
	}
}