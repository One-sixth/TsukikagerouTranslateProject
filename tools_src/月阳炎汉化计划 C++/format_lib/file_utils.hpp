/*
文件读取写入辅助函数库
*/

#pragma once

#include <vector>
#include <fstream>
#include <string>
#include <exception>
#include <concepts>


namespace file_utils
{
	using namespace std;

	/*
	获得文件大小
	*/
	inline size_t GetFileSize(ifstream& f)
	{
		size_t save_pos = f.tellg();
		f.seekg(0, ios::end);
		size_t size = f.tellg();
		f.seekg(save_pos, ios::beg);
		return size;
	}
	inline size_t GetFileSize(ofstream& f)
	{
		size_t save_pos = f.tellp();
		f.seekp(0, ios::end);
		size_t size = f.tellp();
		f.seekp(save_pos, ios::beg);
		return size;
	}
	//inline size_t GetFileSize(fstream& f)
	//{
	//	size_t save_pos = f.tellg();
	//	f.seekg(0, ios::end);
	//	size_t size = f.tellg();
	//	f.seekg(save_pos, ios::beg);
	//	return size;
	//}

	/*
	打开和保存文件，快捷
	*/
	inline ifstream OpenBinFileRead(const string& path)
	{
		ifstream f(path, ios::binary);
		if (!f.is_open())
		{
			throw runtime_error("Error! File can not open.");
		}
		return move(f);
	}
	inline ofstream OpenBinFileWrite(const string& path)
	{
		ofstream f(path, ios::binary);
		if (!f.is_open())
		{
			throw runtime_error("Error! File can not open.");
		}
		return move(f);
	}

	/*
	从文件流里读入一些字节
	*/
	inline void ReadAll(ifstream& f, vector<char>& buff, size_t size_limit=-1)
	{
		size_t pos_begin = f.tellg();
		f.seekg(0, ios::end);
		size_t size = size_t(f.tellg()) - pos_begin;
		f.seekg(pos_begin);
		if (size > size_limit)
			throw runtime_error("ReadAll read size bigger than size_limit.");
		buff.resize(size);
		f.read((char*)buff.data(), size);
		if (f.gcount() != size)
			throw runtime_error("read data len small than size.");
	}

	inline void ReadAll(ifstream& f, vector<uint8_t>& buff, size_t size_limit=-1)
	{
		size_t pos_begin = f.tellg();
		f.seekg(0, ios::end);
		size_t size = size_t(f.tellg()) - pos_begin;
		f.seekg(pos_begin);
		if (size > size_limit)
			throw runtime_error("ReadAll read size bigger than size_limit.");
		buff.resize(size);
		f.read((char*)buff.data(), size);
		if (f.gcount() != size)
			throw runtime_error("read data len small than size.");
	}

	inline vector<uint8_t> ReadAll(ifstream& f, size_t size_limit=-1)
	{
		vector<uint8_t> buf;
		size_t pos_begin = f.tellg();
		f.seekg(0, ios::end);
		size_t size = size_t(f.tellg()) - pos_begin;
		f.seekg(pos_begin);
		if (size > size_limit)
			throw runtime_error("ReadAll read size bigger than size_limit.");
		buf.resize(size);
		f.read((char*)buf.data(), size);
		if (f.gcount() != size)
			throw runtime_error("read data len small than size.");
		return buf;
	}

	/*
	从文件流里读入一些字节
	*/
	inline void Read(ifstream& f, uint32_t size, void* buff)
	{
		f.read((char*)buff, size);
		if (f.gcount() != size)
			throw runtime_error("read data len small than size.");
	}

	/*
	从文件流里读入一些字节
	*/
	inline vector<uint8_t> Read(ifstream& f, uint32_t size, bool err_with_short=true)
	{
		vector<uint8_t> buf;
		buf.resize(size);
		f.read((char*)buf.data(), size);
		if (err_with_short && f.gcount() != size)
			throw runtime_error("read data len small than size.");
		buf.resize(f.gcount());
		return buf;
	}

	/*
	往文件流里写入一些字节
	*/
	inline void Write(ofstream& f, uint32_t size, const void* buff)
	{
		f.write((const char*)buff, size);
	}

	/*
	往文件流里写入一些字节
	*/
	template <typename T>
	inline void Write(ofstream& f, const vector<T> buf)
	{
		f.write((const char*)buf.data(), buf.size() * sizeof(T));
	}

	/*
	读入一个T类的模版函数，注意T类必须为简单类型，不能包含指针或引用之类的东西，间接包含也不行。
	*/
	template <typename T>
	inline void ReadType(ifstream& f, T& t)
	{
		Read(f, sizeof(t), (char*)&t);
	}

	/*
	保存一个T类的模版函数，注意T类必须为简单类型，不能包含指针或引用之类的东西，间接包含也不行。
	*/
	template <typename T>
	inline void WriteType(ofstream& f, const T& t)
	{
		Write(f, sizeof(t), (char*)&t);
	}

	/*
	读入一个string
	*/
	inline void ReadString(ifstream& f, string& s)
	{
		uint32_t size;
		ReadType(f, size);
		s.resize(size);
		Read(f, size, (char*)s.data());
	}

	inline void ReadString(ifstream& f, u8string& s)
	{
		uint32_t size;
		ReadType(f, size);
		s.resize(size);
		Read(f, size, (char*)s.data());
	}

	/*
	保存一个string
	*/
	inline void WriteString(ofstream& f, const string& s)
	{
		if (s.size() > UINT32_MAX)
			throw runtime_error("Error! Data is too big!");

		uint32_t size = static_cast<uint32_t>(s.size());
		WriteType(f, size);
		Write(f, size, (char*)s.data());
	}

	inline void WriteString(ofstream& f, const u8string& s)
	{
		if (s.size() > UINT32_MAX)
			throw runtime_error("Error! Data is too big!");

		uint32_t size = static_cast<uint32_t>(s.size());
		WriteType(f, size);
		Write(f, size, (char*)s.data());
	}

	/*
	读入一个vector的模版函数，注意T类必须为简单类型，不能包含指针或引用之类的东西，间接包含也不行。
	如果需要用此函数读入复杂类型，请特化此模板函数
	*/
	template <typename T>
	inline void ReadVector(ifstream& f, vector<T>& buff)
	{
		uint32_t size;
		ReadType(f, size);
		buff.resize(size);
		Read(f, sizeof(T) * size, (char*)buff.data());
	}

	/*
	保存一个vector的模版函数，注意T类必须为简单类型，不能包含指针或引用之类的东西，间接包含也不行。
	如果需要用此函数写入复杂类型，请特化此模板函数
	*/
	template <typename T>
	inline void WriteVector(ofstream& f, const vector<T>& buf)
	{
		if (buf.size() * sizeof(T) > UINT32_MAX)
			throw runtime_error("Error! Data is too big!");

		uint32_t size = static_cast<uint32_t>(buf.size());
		WriteType(f, size);
		Write(f, sizeof(T) * size, (char*)buf.data());
	}

	/*
	特化vector<string>的模板
	*/
	template <>
	inline void ReadVector<string>(ifstream& f, vector<string>& buf)
	{
		uint32_t size;
		ReadType(f, size);
		buf.resize(size);
		for (auto& i : buf)
			ReadString(f, i);
	}

	/*
	特化vector<string>的模板
	*/
	template <>
	inline void WriteVector<string>(ofstream& f, const vector<string>& buf)
	{
		if (buf.size() > UINT32_MAX)
			throw runtime_error("Error! Data is too big!");

		uint32_t size = static_cast<uint32_t>(buf.size());
		WriteType(f, size);
		for (const auto& i : buf)
			WriteString(f, i);
	}


	/*
	读入一个vector的模版函数，要求T类必须实现和可见Load，Save函数
	*/
	template <typename T>
	inline void ReadClassVector(ifstream& f, vector<T>& data)
	{
		uint32_t size;
		ReadType(f, size);
		data.resize(size);
		for (size_t i = 0; i < size; ++i)
			data[i].Load(f);
	}

	/*
	写入一个vector的模版函数，要求T类必须实现和可见Load，Save函数
	*/
	template <typename T>
	inline void WriteClassVector(ofstream& f, vector<T>& data)
	{
		if (data.size() > UINT32_MAX)
			throw runtime_error("Error! Data is too big!");

		uint32_t size = data.size();
		WriteType(f, size);
		for (size_t i = 0; i < size; ++i)
			data[i].Save(f);
	}

};

