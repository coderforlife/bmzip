#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "GetBootmanagerVersion.h"
#include "ms-compression\compression.h"

typedef unsigned char byte;
typedef byte* bytes;
typedef const byte const_byte;
typedef const_byte* const_bytes;

bytes read_all(const TCHAR* filepath, size_t* size)
{
	size_t l, len;
	bytes b, b0;
	FILE* f = _tfopen(filepath, _T("rb"));
	if (f == NULL) { /* error! */ return NULL; }
	if (fseek(f, 0L, SEEK_END) || (*size = len = ftell(f)) == -1 || fseek(f, 0L, SEEK_SET) || (b0 = b = (bytes)malloc(len)) == NULL) { /* error! */ fclose(f); return NULL; }
	while ((l = fread(b, sizeof(byte), len, f)) > 0 && (len -= l) > 0) { b += l; }
	fclose(f);
	if (len > 0) { /* error! */ free(b0); b0 = NULL; }
	return b0;
}

int write_all(const TCHAR* filepath, bytes b, size_t size)
{
	size_t l;
	FILE* f = _tfopen(filepath, _T("wb"));
	if (f == NULL) { /* error! */ return 0; }
	while ((l = fwrite(b, sizeof(byte), size, f)) > 0 && (size-=l) > 0) { b += l; }
	fclose(f);
	if (size > 0) { /* error! */ return 0; }
	return 1;
}

typedef struct _BM_DATA
{
	unsigned int signature;
	unsigned int compressed_size;
	unsigned int uncompressed_size;
	unsigned int offset;
} BM_DATA;

const unsigned int bmLZNT1      = 0x49434D42; // BMCI
const unsigned int bmXpressHuff = 0x48584D42; // BMXH
const unsigned int MZ90         = 0x00905A4D; // MZ\x90\x00


int bm_decomp(const TCHAR* in_file, const TCHAR* out_file)
{
	size_t in_size, out_size;
	bytes in_start = read_all(in_file, &in_size), in_end = in_start + in_size, in = in_start, out;
	BM_DATA *bm_data;
	CompressionFormat format;
	
	if (in == NULL)
	{
		// error!
		_ftprintf(stderr, _T("Failed to read file '%s'\n"), in_file);
		return -1;
	}

	while ((in = (bytes)memchr(in, 'B', in_end - in - 3)) != NULL && *(unsigned int*)in != bmLZNT1 && *(unsigned int*)in != bmXpressHuff) { ++in; }
	if (in == NULL)
	{
		// error! not found
		_ftprintf(stderr, _T("Failed to find the boot manager compression signature in file '%s'\n"), in_file);
		return -2;
	}

	bm_data = (BM_DATA*)in;

	if (*(unsigned int*)(in+sizeof(BM_DATA)) == MZ90)
	{
		// There is a mini program between here and the compressed data
		Version v = GetBootmanagerVersion(in + sizeof(BM_DATA));
		if (v.Major != 0 || v.Minor != 0 || v.Build != 0 || v.Revision != 0)
			_tprintf(_T("Version: %hu.%hu.%hu.%hu\n"), v.Major, v.Minor, v.Build, v.Revision);
		else
			_tprintf(_T("Version: Unknown\n"));
	}
	else
		_tprintf(_T("Version: Unknown\n"));

	in += bm_data->offset;
	out = (bytes)malloc(bm_data->uncompressed_size);
	if (out == NULL)
	{
		// error!
		_ftprintf(stderr, _T("Failed to allocate %u bytes for decompression\n"), bm_data->uncompressed_size);
		return -3;
	}

	switch (bm_data->signature)
	{
	case bmLZNT1:      _tprintf(_T("Compression Method: LZNT1\n"));          format = COMPRESSION_LZNT1; break;
	case bmXpressHuff: _tprintf(_T("Compression Method: XPRESS Huffman\n")); format = COMPRESSION_XPRESS_HUFF; break;
	}

	out_size = decompress(format, in, bm_data->compressed_size, out, bm_data->uncompressed_size);
	if (out_size == 0)
	{
		// error!
		_ftprintf(stderr, _T("Failed to decompress data: %u\n"), errno);
		return -4;
	}
	if (!write_all(out_file, out, out_size))
	{
		_ftprintf(stderr, _T("Failed to save decompress file to '%s'\n"), out_file);
		return -5;
	}

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR *in_file = _T("win8-bootmgr"), *out_file = _T("win8-bootmgr.exe");
	return bm_decomp(in_file, out_file);
}
