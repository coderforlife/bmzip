// bmzip: program for compressing and decompressing the bootmgr file
// Copyright (C) 2012  Jeffrey Bush  jeff@coderforlife.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

static bytes read_all(const TCHAR* filepath, size_t* size)
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

static int write_all(const TCHAR* filepath, bytes b, size_t size)
{
	size_t l;
	FILE* f = _tfopen(filepath, _T("wb"));
	if (f == NULL) { /* error! */ return 0; }
	while ((l = fwrite(b, sizeof(byte), size, f)) > 0 && (size-=l) > 0) { b += l; }
	fflush(f);
	fclose(f);
	if (size > 0) { /* error! */ return 0; }
	return 1;
}

static int append_all(const TCHAR* filepath, bytes b, size_t size)
{
	size_t l;
	FILE* f = _tfopen(filepath, _T("ab"));
	if (f == NULL) { /* error! */ return 0; }
	while ((l = fwrite(b, sizeof(byte), size, f)) > 0 && (size-=l) > 0) { b += l; }
	fflush(f);
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

static BM_DATA* find_bm_data(bytes in, bytes in_end)
{
	BM_DATA* bm_data;
	while ((in = (bytes)memchr(in, 'B', in_end - in - 3)) != NULL && *(unsigned int*)in != bmLZNT1 && *(unsigned int*)in != bmXpressHuff) { ++in; }
	bm_data = (BM_DATA*)in;
	return in == NULL || (in + bm_data->offset + bm_data->compressed_size > in_end) ? NULL : bm_data;
}
static void print_version(BM_DATA* bm_data)
{
	// Check the mini program between here and the compressed data that contains version information
	Version v = GetBootmanagerVersion((void*)(bm_data+1));
	if (v.Major != 0 || v.Minor != 0 || v.Build != 0 || v.Revision != 0)
		_tprintf(_T("Version: %hu.%hu.%hu.%hu\n"), v.Major, v.Minor, v.Build, v.Revision);
	else
		_tprintf(_T("Version: Unknown\n"));
}
static CompressionFormat get_format(BM_DATA* bm_data)
{
	switch (bm_data->signature)
	{
	case bmLZNT1:      _tprintf(_T("Compression Method: LZNT1\n"));          return COMPRESSION_LZNT1;
	case bmXpressHuff: _tprintf(_T("Compression Method: XPRESS Huffman\n")); return COMPRESSION_XPRESS_HUFF;
	}
	return (CompressionFormat)-1;
}

static int bm_decomp(const TCHAR* in_file, const TCHAR* out_file)
{
	size_t in_size, out_size;
	bytes in, out;
	BM_DATA *bm_data;
	
	if ((in = read_all(in_file, &in_size)) == NULL) { /* error! */ _ftprintf(stderr, _T("Failed to read input file '%s'\n"), in_file); return -1; }
	if ((bm_data = find_bm_data(in, in+in_size)) == NULL) { /* error! not found */ _ftprintf(stderr, _T("Failed to find the boot manager compression signature in file '%s'\n"), in_file); return -2; }
	print_version(bm_data);
	if ((out = (bytes)malloc(bm_data->uncompressed_size)) == NULL) { /* error! */ _ftprintf(stderr, _T("Failed to allocate %u bytes for decompression\n"), bm_data->uncompressed_size); return -3; }
	if ((out_size = decompress(get_format(bm_data), ((bytes)bm_data) + bm_data->offset, bm_data->compressed_size, out, bm_data->uncompressed_size)) != bm_data->uncompressed_size) { /* error! */ _ftprintf(stderr, _T("Failed to decompress data: %u\n"), errno); return -4; }
	if (!write_all(out_file, out, out_size)) { /* error! */ _ftprintf(stderr, _T("Failed to save decompressed file to '%s'\n"), out_file); return -5; }

	// TODO: free()

	return 0;
}

static int bm_comp(const TCHAR* in_file, const TCHAR* out_file)
{
	size_t in_size, out_size;
	bytes in, out, out_data;
	BM_DATA *bm_data;

	if ((in = read_all(in_file, &in_size)) == NULL) { /* error! */ _ftprintf(stderr, _T("Failed to read input file '%s'\n"), in_file); return -1; }
	if ((out = read_all(out_file, &out_size)) == NULL) { /* error! */ _ftprintf(stderr, _T("Failed to read output file '%s' - it must be a bootmgr file\n"), out_file); return -1; }
	if ((bm_data = find_bm_data(out, out+out_size)) == NULL) { /* error! not found */ _ftprintf(stderr, _T("Failed to find the boot manager compression signature in file '%s' - it must be a bootmgr file\n"), out_file); return -2; }
	print_version(bm_data);
	bm_data->uncompressed_size = in_size;
	if ((out_data = (bytes)malloc(2*in_size)) == NULL) { /* error! */ _ftprintf(stderr, _T("Failed to allocate %u bytes for compression\n"), 2*in_size); return -3; }
	if ((out_size = compress(get_format(bm_data), in, in_size, out_data, 2*in_size)) == 0) { /* error! */ _ftprintf(stderr, _T("Failed to compress data: %u\n"), errno); return -4; }
	bm_data->compressed_size = out_size;
	out_data[out_size++] = 0;
	if (!write_all(out_file, out, ((bytes)bm_data) + bm_data->offset - out)) { /* error! */ _ftprintf(stderr, _T("Failed to save compressed file to '%s'\n"), out_file); return -5; }
	if (!append_all(out_file, out_data, out_size)) { /* error! */ _ftprintf(stderr, _T("Failed to save compressed file to '%s'\n"), out_file); return -5; }
	
	// TODO: free()

	return 0;
}

static void usage(_TCHAR* prog)
{
	_tprintf(_T("To extract bootmgr.exe:\n"));
	_tprintf(_T("  %s bootmgr bootmgr.exe\n"), prog);
	_tprintf(_T("  The bootmgr file must be a complete bootmgr file and bootmgr.exe is overwritten\n"));
	_tprintf(_T("\n"));
	_tprintf(_T("To compress bootmgr.exe\n"));
	_tprintf(_T("  %s /c bootmgr.exe bootmgr\n"), prog);
	_tprintf(_T("  The bootmgr file must be a complete bootmgr file and is modified with the input file\n"));
	_tprintf(_T("\n"));
}

int _tmain(int argc, _TCHAR* argv[])
{
	bool compress;

	_tprintf(_T("bmzip Copyright (C) 2012  Jeffrey Bush <jeff@coderforlife.com>\n"));
	_tprintf(_T("This program comes with ABSOLUTELY NO WARRANTY;\n"));
	_tprintf(_T("This is free software, and you are welcome to redistribute it\n"));
	_tprintf(_T("under certain conditions;\n"));
	_tprintf(_T("See http://www.gnu.org/licenses/gpl.html for more details.\n"));
	_tprintf(_T("\n"));

	compress = argc > 1 && _tcslen(argv[1]) == 2 && (argv[1][0] == _T('-') || argv[1][0] == _T('/')) && (argv[1][1] == _T('c') || argv[1][1] == _T('C'));
	if (!compress && argc == 3)		return bm_decomp(argv[1], argv[2]);
	else if (compress && argc == 4)	return bm_comp(argv[2], argv[3]);
	usage(argv[0]);
	return 1;
}
