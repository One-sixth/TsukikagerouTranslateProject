#include "../format_lib/ArcFile.h"
#include "../deps/cxxopts.hpp"
#include <filesystem>
#include <iostream>

#pragma comment(lib, "format_lib.lib")


/*
--action extract --in "G:\月陽炎DVD\scenario.arc" --out "G:\月陽炎DVD\tmp3"
--action archive --in "G:\月陽炎DVD\tmp3" --out "G:\月陽炎DVD\tmp3.arc"
*/


using namespace std;


int main(int argc, const char* argv[])
{
	auto options = cxxopts::Options(argv[0], "月阳炎 Arc Tool");
	options.add_options()
		("action", "which operator. Only allow \"extract\" or \"archive\"", cxxopts::value<std::string>())
		("in", "extract: input file ; archive: input dir", cxxopts::value<std::string>())
		("out", "extract: output dir ; archive: output file", cxxopts::value<std::string>());

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

	if (action == "archive")
	{
		if (!filesystem::directory_entry(in).is_directory())
			throw runtime_error("Error! in param is not a valid dir.");

		ArcFileWriter writer;
		writer.open_dir(in);
		writer.write(out);

		cout << "Success" << endl;
	}
	else if (action == "extract")
	{
		if (!filesystem::directory_entry(in).is_regular_file())
			throw runtime_error("Error! in param is not a valid file.");

		if (!filesystem::directory_entry(out).exists())
			filesystem::create_directories(out);

		ArcFileReader reader;
		reader.open(in);
		reader.extract(out);

		cout << "Success" << endl;
	}
	else
	{
		cout << "Error! actionn param only allow \"extract\" and \"archive\"" << endl;
	}
}