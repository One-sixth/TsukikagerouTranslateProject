#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>
#include <format>
#include <filesystem>
#include <stdexcept>
#include "file_utils.hpp"


using namespace std;


const vector<uint8_t> const_arc_header{ '\x44', '\x41', '\x46', '\x1A' };


class ArcFileReader
{
public:
	struct Item
	{
		uint32_t begin;
		uint32_t size;
		char     name[24];
	};

public:

	void open(string file_path);
	void extract(string out_dir);

public:
	ifstream f;
	vector<Item> item_list;

};


class ArcFileWriter
{
public:
	struct Item
	{
		uint32_t begin;
		uint32_t size;
		char name[24];
	};

	struct Index
	{
		uint32_t size;
		string path;
		string name;
	};

public:

	void open_dir(string dir_path);
	void write(string file_path);

public:
	ifstream f;
	vector<Index> index_list;

};

