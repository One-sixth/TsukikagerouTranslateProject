#include "SnrFile.h"


//------------------------ SnrFileReader ------------------------


void SnrFileReader::load_enc_file(string file, int type)
{
	auto f = ifstream(file, ios::binary);

	vector<uint8_t> header;
	header = file_utils::Read(f, 4);
	if (header != const_snr_header)
		throw runtime_error("Error! Not a SNR file.");

	file_utils::ReadType(f, enc_size);
	file_utils::ReadType(f, dec_size);
	file_utils::ReadAll(f, enc_data);
	if (enc_data.size() != enc_size)
		cout << "Warning! enc_data.size != enc_size, the file may be broken." << endl;

	decrypt_and_decompress(type);

	if (dec_data.size() != dec_size)
		cout << "Warning! dec_data.size != dec_size, the file may be broken." << endl;
}

void SnrFileReader::save_dec_file(string file)
{
	ofstream of(file, ios::binary);
	if (!of.is_open())
		throw runtime_error("Errro! Output file can not open.");

	file_utils::Write(of, dec_data);
	of.close();
}

void SnrFileReader::decrypt_and_decompress(int type)
{
	// 解密
	vector<uint8_t> tmp_data = enc_data;
	if (type == 0)
		tmp_data = decrypt(tmp_data);
	else if (type == 1)
		tmp_data = decrypt_2(tmp_data);
	else
		throw runtime_error(format((string)"Error! Unknow type {0}", type));
	tmp_data = CgfFile::decrypt(tmp_data);

	z_check_code = vector<uint8_t>(tmp_data.begin(), tmp_data.begin() + 4);
	z_header = vector<uint8_t>(tmp_data.begin() + 4, tmp_data.begin() + 6);

	//if (z_header != const_z_header)
	if (z_header[0] != const_z_header[0])
		//cout << "Warning! z_header maybe is not valid." << endl;
		throw runtime_error("Errro! z_header is not valid.");

	//z_data = vector<uint8_t>(tmp_data.begin() + 6, tmp_data.end());
	z_data = vector<uint8_t>(tmp_data.begin() + 4, tmp_data.end());

	tmp_data = DeflateUtils::decompress(z_data);

	dec_data = tmp_data;
}

vector<uint8_t> SnrFileReader::decrypt(vector<uint8_t> data)
{
	uint8_t key = 0x84;
	for (int i = 0; i < data.size(); ++i)
	{
		data[i] -= key;
		for (int count = ((i & 0xF) + 2) / 3; count > 0; --count)
		{
			key += 0x99;
		}
	}
	return data;
}

vector<uint8_t> SnrFileReader::decrypt_2(vector<uint8_t> data)
{
	uint8_t* ecx = data.data();
	unsigned int edi = data.size();
	unsigned int esi = 0;
	unsigned int edx = 0;
	unsigned int eax = 0;
	unsigned int ebx = 0;
	unsigned long long tmp64 = 0;

	*(char*)&ebx = 0x71;

loop1:
	*(char*)&eax = *(char*)ecx;
	*(char*)&eax = *(char*)&eax - *(char*)&ebx;
	*(char*)ecx = *(char*)&eax;
	eax = esi;
	eax = eax & 0xF;
	if (eax > 0)
	{
		edx = eax + 0x4;
		eax = 0xCCCCCCCD;
		tmp64 = (unsigned long long)eax * (unsigned long long)edx;
		eax = tmp64;
		edx = tmp64 >> 32;
		eax = edx;
		*(char*)&edx = 0x47;
		eax = eax >> 0x2;
		*(short*)&eax = (short)*(char*)&eax * (short)*(char*)&edx;
		*(char*)&ebx += *(char*)&eax;
	}
	ecx += 1;
	esi += 1;
	if (esi < edi)
		goto loop1;

	return data;
}


//------------------------ SnrFileWriter ------------------------


void SnrFileWriter::load_dec_file(string file, int type)
{
	ifstream f(file, ios::binary);
	if (!f.is_open())
		throw runtime_error("Error! File can not open.");

	dec_data = file_utils::ReadAll(f);

	encrypt_and_compress(type);
}


void SnrFileWriter::save_enc_file(string file)
{
	ofstream of(file, ios::binary);
	if (!of.is_open())
		throw runtime_error("Errro! Output file can not open.");

	file_utils::Write(of, const_snr_header);
	file_utils::WriteType(of, enc_size);
	file_utils::WriteType(of, dec_size);
	file_utils::Write(of, enc_data);
	of.close();
}

void SnrFileWriter::encrypt_and_compress(int type)
{
	dec_size = dec_data.size();

	z_header = const_z_header;
	z_data = DeflateUtils::compress(dec_data);
	z_check_code.assign(4, 0);	// 校验码算法未知，将固定为0
	//z_check_code = make_check_sum(z_data);

	vector<uint8_t> tmp_data;
	tmp_data.insert(tmp_data.end(), z_check_code.begin(), z_check_code.end());
	//tmp_data.insert(tmp_data.end(), z_header.begin(), z_header.end());
	// 经过进一步研究，snr包里的 数据流 格式为 2字节压缩头 + deflate数据 + 4字节特制的adler32校验码
	tmp_data.insert(tmp_data.end(), z_data.begin(), z_data.end());

	tmp_data = CgfFile::encrypt(tmp_data);

	if (type == 0)
		enc_data = encrypt(tmp_data);
	else if (type == 1)
		enc_data = encrypt_2(tmp_data);
	else
		throw runtime_error(format((string)"Errro! Unknow type {0}.", type));

	enc_size = enc_data.size();
}

vector<uint8_t> SnrFileWriter::encrypt(vector<uint8_t> data)
{
	uint8_t key = 0x84;
	for (int i = 0; i < data.size(); ++i)
	{
		data[i] += key;
		for (int count = ((i & 0xF) + 2) / 3; count > 0; --count)
		{
			key += 0x99;
		}
	}
	return data;
}

vector<uint8_t> SnrFileWriter::encrypt_2(vector<uint8_t> data)
{
	uint8_t* ecx = data.data();
	unsigned int edi = data.size();
	unsigned int esi = 0;
	unsigned int edx = 0;
	unsigned int eax = 0;
	unsigned int ebx = 0;
	unsigned long long tmp64 = 0;

	*(char*)&ebx = 0x71;

loop1:
	*(char*)&eax = *(char*)ecx;
	*(char*)&eax = *(char*)&eax + *(char*)&ebx;
	*(char*)ecx = *(char*)&eax;
	eax = esi;
	eax = eax & 0xF;
	if (eax > 0)
	{
		edx = eax + 0x4;
		eax = 0xCCCCCCCD;
		tmp64 = (unsigned long long)eax * (unsigned long long)edx;
		eax = tmp64;
		edx = tmp64 >> 32;
		eax = edx;
		*(char*)&edx = 0x47;
		eax = eax >> 0x2;
		*(short*)&eax = (short)*(char*)&eax * (short)*(char*)&edx;
		*(char*)&ebx += *(char*)&eax;
	}
	ecx += 1;
	esi += 1;
	if (esi < edi)
		goto loop1;

	return data;
}

vector<uint8_t> SnrFileWriter::make_check_sum(vector<uint8_t> data)
{
	throw runtime_error("Error! 算法未完成.");

	//int key = 0x0046EE38;
	int key = 0;
	// key 是一个指向常量表的指针
	// 目前该算法不爬了。

	int ebx = 0;
	int edi = 0;

	uint8_t* edx = data.data();
	// int ecx = key;
	int esi = key;
	int eax = -1;
	int ecx = data.size();
	ecx -= 1;
	if (ecx < 0)
	{
		eax = ~eax;
		goto end;
	}
	edi = ecx + 1;
loop:
	ecx = eax;
	ebx = 0;
	ebx = *(char*)edx;
	ecx >> 18;
	ecx = ecx ^ ebx;
	eax << 8;
	ecx = *(int*)(esi + ecx * 4 + 330);
	eax = eax ^ ecx;
	edx += 1;
	edi -= 1;
	if (edi > 0)
		goto loop;

end:
	eax = ~eax;

	vector<uint8_t> check_sum(4);
	memcpy_s(check_sum.data(), 4, &eax, 4);
	return check_sum;
}
