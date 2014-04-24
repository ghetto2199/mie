#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <mie/string.hpp>
#include "util.hpp"
#include "benchmark.hpp"
#ifdef USE_MISCHASAN
#include "mischasan_strstr.hpp"
#endif
#ifdef __GNUC__
#include <strings.h>
#endif
//#define USE_BOOST_BM

typedef std::basic_string<MIE_WCHAR_T> Wcstring;

// strcasestr(key must not have capital character)
const char *strcasestr_C(const char *str, const char *key)
{
#ifdef _GNU_SOURCE
	return strcasestr(str, key);
#else
	const size_t keySize = strlen(key);
	while (*str) {
#ifdef _WIN32
		if (_memicmp(str, key, keySize) == 0) return str;
#else
		if (strncasecmp(str, key, keySize) == 0) return str;
#endif
		str++;
	}
	return 0;
#endif
}
int memicmp_C(const char *p, const char *q, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		char c = p[i];
		if ('A' <= c && c <= 'Z') c += 'a' - 'A';
		if (c != q[i]) return (int)c - (int)q[i];
	}
	return 0;
}
const char *findCaseStr_C(const char *begin, const char *end, const char *key, size_t keySize)
{
	while (begin + keySize <= end) {
#ifdef _WIN32
		if (_memicmp(begin, key, keySize) == 0) return begin;
#else
		if (memicmp_C(begin, key, keySize) == 0) return begin;
#endif
		begin++;
	}
	return end;
}

// std::string.find()
struct Fstr_find {
	const std::string *str_;
	const std::string *key_;
	typedef size_t type;
	void set(const std::string& str, const std::string& key)
	{
		str_ = &str;
		key_ = &key;
	}
	Fstr_find() : str_(0), key_(0) { }
	size_t begin() const { return 0; }
	size_t end() const { return std::string::npos; }
	size_t find(size_t p) const { return str_->find(*key_, p); }
};

#ifdef USE_BOOST_BM
#include <boost/algorithm/searching/boyer_moore.hpp>
struct Frange_boost_bm_find {
	boost::algorithm::boyer_moore<const char*> *bm_;
	~Frange_boost_bm_find()
	{
		delete bm_;
	}
	const char *str_;
	const char *end_;
	const char *key_;
	size_t keySize_;
	typedef const char* type;
	void set(const std::string& str, const std::string& key)
	{
		str_ = &str[0];
		end_ = str_ + str.size();
		key_ = &key[0];
		keySize_ = key.size();
		bm_ = new boost::algorithm::boyer_moore<const char*>(key_, key_ + keySize_);
	}
	Frange_boost_bm_find() : bm_(0), str_(0), end_(0), key_(0), keySize_(0) { }
	const char *begin() const { return str_; }
	const char *end() const { return end_; }
	const char *find(const char *p) const { return (*bm_)(p, end_); }
};
#endif

#ifdef __linux__
struct Fmemmem {
	const char *str_;
	const char *strEnd_;
	const char *key_;
	size_t keyLen_;
	typedef const char* type;
	void set(const std::string& str, const std::string& key)
	{
		str_ = &str[0];
		strEnd_ = str_ + str.size();
		key_ = &key[0];
		keyLen_ = key.size();
	}
	Fmemmem() : str_(0), strEnd_(0), key_(0), keyLen_(0) { }
	const char *begin() const { return str_; }
	const char *end() const { return 0; }
	const char *find(const char *p) const { return (const char*)memmem((const void*)p, strEnd_ - p, (const void*)key_, keyLen_); }
};

#endif

// std::string.find_first_of()
struct Fstr_find_first_of {
	const std::string *str_;
	const std::string *key_;
	typedef size_t type;
	void set(const std::string& str, const std::string& key)
	{
		str_ = &str;
		key_ = &key;
	}
	Fstr_find_first_of() : str_(0), key_(0) { }
	size_t begin() const { return 0; }
	size_t end() const { return std::string::npos; }
	size_t find(size_t p) const { return str_->find_first_of(*key_, p); }
};

// findChar_range(begin, end, "azAZ<>")
struct FfindChar_range_emu {
	const char *begin_;
	const char *end_;
	char tbl_[256];
	typedef const char* type;
	void set(const std::string& str, const std::string&)
	{
		begin_ = &str[0];
		end_ = begin_ + str.size();
		for (unsigned int i = 0; i < 256; i++) {
			tbl_[i] = ('a' <= i && i <= 'z') || ('A' <= i && i <= 'Z') || ('<' <= i && i <= '>');
		}
	}
	FfindChar_range_emu() : begin_(0), end_(0) { }
	const char *begin() const { return begin_; }
	const char *end() const { return end_; }
	const char *find(const char *p) const
	{
		while (p < end_) {
			if (tbl_[(unsigned char)*p]) return p;
			p++;
		}
		return end_;
	}
};

template<class C, class UC>
const C *strchr_range_T(const C *str, const C *key)
{
	while (*str) {
		UC  c = (UC)*str;
		for (const UC *p = (const UC *)key; *p; p += 2) {
			if (p[0] <= c && c <= p[1]) {
				return str;
			}
		}
		str++;
	}
	return 0;
}
const char *strchr_range_C(const char *str, const char *key)
{
	return strchr_range_T<char, unsigned char>(str, key);
}

const MIE_WCHAR_T *wcschr_range_C(const MIE_WCHAR_T *str, const MIE_WCHAR_T *key)
{
	return strchr_range_T<MIE_WCHAR_T, MIE_WCHAR_T>(str, key);
}

const char *mystrstr_C(const char *str, const char *key)
{
	size_t len = strlen(key);
//	if (len == 1) return strchr(str, key[0]);
	while (*str) {
		const char *p = strchr(str, key[0]);
		if (p == 0) return 0;
		if (memcmp(p + 1, key + 1, len - 1) == 0) return p;
		str = p + 1;
	}
	return 0;
}
template<class C>
size_t myWcslen(const C* str)
{
	size_t i = 0;
	while (str[i]) {
		i++;
	}
	return i;
}
template<class C>
const C *myWcschr(const C *str, C c)
{
	while (*str) {
		if (*str == c) return str;
		str++;
	}
	return 0;
}
const MIE_WCHAR_T *mywcsstr_C(const MIE_WCHAR_T *str, const MIE_WCHAR_T *key)
{
	size_t len = myWcslen(key);
//	if (len == 1) return strchr(str, key[0]);
	while (*str) {
		const MIE_WCHAR_T *p = myWcschr(str, key[0]);
		if (p == 0) return 0;
		if (memcmp(p + 1, key + 1, (len - 1) * 2) == 0) return p;
		str = p + 1;
	}
	return 0;
}

template<class C>
const C *strchr_any_T(const C *str, const C *key)
{
	while (*str) {
		const C c = *str;
		for (const C *p = key; *p; p++) {
			if (c == *p) {
				return str;
			}
		}
		str++;
	}
	return 0;
}
const char *strchr_any_C(const char *str, const char *key)
{
	return strchr_any_T<char>(str, key);
}
const MIE_WCHAR_T *wcschr_any_C(const MIE_WCHAR_T *str, const MIE_WCHAR_T *key)
{
	return strchr_any_T<MIE_WCHAR_T>(str, key);
}

const char *findChar_C(const char *begin, const char *end, char c)
{
	return std::find(begin, end, c);
}

const MIE_WCHAR_T *findWchar_C(const MIE_WCHAR_T *begin, const MIE_WCHAR_T *end, MIE_WCHAR_T c)
{
	return std::find(begin, end, c);
}

template<class C>
const C *findChar_any_T(const C *begin, const C *end, const C *key, size_t keySize)
{
	while (begin != end) {
		const C c = *begin;
		for (size_t i = 0; i < keySize; i++) {
			if (c == key[i]) {
				return begin;
			}
		}
		begin++;
	}
	return end;
}

const char *findChar_any_C(const char *begin, const char *end, const char *key, size_t keySize)
{
	return findChar_any_T<char>(begin, end, key, keySize);
}

const MIE_WCHAR_T *findWchar_any_C(const MIE_WCHAR_T *begin, const MIE_WCHAR_T *end, const MIE_WCHAR_T *key, size_t keySize)
{
	return findChar_any_T<MIE_WCHAR_T>(begin, end, key, keySize);
}

template<class C, class UC>
const C *findChar_range_T(const C *begin, const C *end, const C *key, size_t keySize)
{
	while (begin != end) {
		const UC c = (UC)*begin;
		for (size_t i = 0; i < keySize; i += 2) {
			if (key[i] <= c && c <= key[i + 1]) {
				return begin;
			}
		}
		begin++;
	}
	return end;
}
const char *findChar_range_C(const char *begin, const char *end, const char *key, size_t keySize)
{
	return findChar_range_T<char, unsigned char>(begin, end, key, keySize);
}

const MIE_WCHAR_T *findWchar_range_C(const MIE_WCHAR_T *begin, const MIE_WCHAR_T *end, const MIE_WCHAR_T *key, size_t keySize)
{
	return findChar_range_T<MIE_WCHAR_T, MIE_WCHAR_T>(begin, end, key, keySize);
}

template<class C>
const C *findStr_T(const C *begin, const C *end, const C *key, size_t keySize)
{
	while (begin + keySize <= end) {
		if (memcmp(begin, key, keySize * sizeof(C)) == 0) {
			return begin;
		}
		begin++;
	}
	return end;
}
const char *findStr_C(const char *begin, const char *end, const char *key, size_t keySize)
{
	return findStr_T<char>(begin, end, key, keySize);
}
const MIE_WCHAR_T *findWstr_C(const MIE_WCHAR_T *begin, const MIE_WCHAR_T *end, const MIE_WCHAR_T *key, size_t keySize)
{
	return findStr_T<MIE_WCHAR_T>(begin, end, key, keySize);
}

const char *findStr2_C(const char *begin, const char *end, const char *key, size_t keySize)
{
	while (begin + keySize <= end) {
		const char *p = (const char*)memchr(begin, key[0], end - begin);
		if (p == 0) break;
		if (memcmp(p + 1, key + 1, keySize - 1) == 0) {
			return p;
		}
		begin = p + 1;
	}
	return end;
}

/////////////////////////////////////////////////
void strlen_test()
{
	puts("strlen_test");
	{
		std::string str;
		for (int i = 0; i < 33; i++) {
			size_t a = strlen(str.c_str());
			size_t b = mie::strlen(str.c_str());
			TEST_EQUAL(a, b);
			str += 'a';
		}
		str = "0123456789abcdefghijklmn\0";
		for (int i = 0; i < 16; i++) {
			size_t a = strlen(&str[i]);
			size_t b = mie::strlen(&str[i]);
			TEST_EQUAL(a, b);
		}
	}
	{
		Wcstring str;
		for (int i = 0; i < 33; i++) {
			size_t a = myWcslen(str.c_str());
			size_t b = mie::wcslen(str.c_str());
			TEST_EQUAL(a, b);
			str += (MIE_WCHAR_T)'a';
		}
		str.clear();
		const std::string cs = "0123456789abcdefghijklmn";
		for (size_t i = 0; i < cs.size(); i++) {
			str += cs[i];
		}
		str += (MIE_WCHAR_T)0;
		for (int i = 0; i < 16; i++) {
			size_t a = myWcslen(&str[i]);
			size_t b = mie::wcslen(&str[i]);
			TEST_EQUAL(a, b);
		}
	}
	puts("ok");
}

void strchr_test(const std::string& text)
{
	const int MaxChar = 254;
	puts("strchr_test");
	std::string str;
	str.resize(MaxChar + 1);
	for (int i = 1; i < MaxChar; i++) {
		str[i - 1] = (char)i;
	}
	str[MaxChar] = '\0';
	const std::string *pstr = text.empty() ? &str : &text;
	for (int c = '0'; c <= '9'; c++) {
		benchmark("strchr_C", Fstrchr<STRCHR>(), "strchr", Fstrchr<mie::strchr>(), *pstr, std::string(1, (char)c));
	}
	{
		const MIE_WCHAR_T s[] = { 2, 3, 0 };
		TEST_EQUAL(mie::wcschr(s, 5), (const MIE_WCHAR_T*)0);
	}
	{
		const int len = 1024;
		Wcstring str;
		str.resize(len + 1);
		for (int i = 0; i < len; i++) {
			str[i] = (MIE_WCHAR_T)(i + 1);
		}
		for (MIE_WCHAR_T c = 1; c < len; c++) {
			TEST_EQUAL(mie::wcschr(&str[0], c), &str[c - 1]);
		}
	}
}

void strchr_any_test(const std::string& text)
{
	puts("strchr_any_test");
	std::string str = "123a456abcdefghijklmnob123aa3vnrabcdefghijklmnopaw3nabcdevra";
	for (int i = 1; i < 256; i++) {
		str += (char)i;
	}
	str += '\0';
	const char tbl[][17] = {
		"a",
		"ab",
		"abc",
		"abcd",
		"abcde",
		"abcdef",
		"abcdefg",
		"abcdefgh",
		"abcdefghi",
		"abcdefghij",
		"abcdefghijk",
		"abcdefghijkl",
		"abcdefghijklm",
		"abcdefghijklmn",
		"abcdefghijklmno",
		"abcdefghijklmnop",
	};
	const std::string *pstr = text.empty() ? &str : &text;
	for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
		const std::string key = tbl[i];
		benchmark("strchr_any_C", Fstrstr<strchr_any_C>(), "strchr_any", Fstrstr<mie::strchr_any>(), *pstr, key);
//		benchmark("find_first_of", Fstr_find_first_of(), "strchr_any", Fstrstr<mie::strchr_any>(), *pstr, key);
	}
	{
		Wcstring str;
		for (MIE_WCHAR_T c = 1; c < 65535; c++) {
			str += c;
		}
		const MIE_WCHAR_T tbl[][8] = {
			{ 1 },
			{ 128 },
			{ 1, 2 },
			{ 5, 10, 20, 32 },
			{ 65534, 2, 3, 4, 25, 123, 23 },
			{ 65535 },
		};
		for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
			TEST_EQUAL(mie::wcschr_any(&str[0], tbl[i]), wcschr_any_C(&str[0], tbl[i]));
		}
	}
	puts("ok");
}

void strchr_range_test(const std::string& text)
{
	puts("strchr_range_test");
	std::string str = "123a456abcdefghijklmnob123aa3vnraw3nabcdevra";
	for (int i = 1; i < 256; i++) {
		str += (char)i;
	}
	str += '\0';
	const char tbl[][17] = {
		"aa",
		"az",
		"09",
		"az09AZ",
		"acefgixz",
		"acefgixz29",
		"acefgixz29XY",
		"acefgixz29XYAB",
		"acefgixz29XYABRU",
	};
	const std::string *pstr = text.empty() ? &str : &text;
	for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
		const std::string key = tbl[i];
		benchmark("strchr_range_C", Fstrstr<strchr_range_C>(), "strchr_range", Fstrstr<mie::strchr_range>(), *pstr, key);
	}
	puts("ok");
	{
		Wcstring str;
		for (MIE_WCHAR_T c = 1; c < 65535; c++) {
			str += c;
		}
		const MIE_WCHAR_T tbl[][8] = {
			{ 1, 1 },
			{ '0', '9' },
			{ 'a', 'z', '0', '9', 'A', 'Z' },
			{ 'a', 'c', 'e', 'f', 'g', 'i', 'x', 'z' },
		};
		for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
			TEST_EQUAL(mie::wcschr_range(&str[0], tbl[i]), wcschr_range_C(&str[0], tbl[i]));
		}
	}
}

void strstr_test()
{
	puts("strstr_test");
	const int SIZE = 1024 * 1024 * 10;
	struct {
		const char *str;
		const char *key;
	} tbl[] = {
		{ "abcdefghijklmn", "fghi" },
		{ "abcdefghijklmn", "i" },
		{ "abcdefghijklmn", "ij" },
		{ "abcdefghijklmn", "abcdefghijklm" },
		{ "0123456789abcdefghijkl", "0123456789abcdefghijklm" },
		{ "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTU", "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTU@" },
	};
	for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
		std::string str;
		str.reserve(SIZE);
		const size_t len = strlen(tbl[i].str);
		for (size_t j = 0; j < SIZE / len; j++) {
			str.append(tbl[i].str, len);
		}
		std::string key = tbl[i].key;
		benchmark("strstr_C", Fstrstr<STRSTR>(), "strstr", Fstrstr<mie::strstr>(), str, key);
		benchmark("strstr2_C", Fstrstr<mystrstr_C>(), "strstr", Fstrstr<mie::strstr>(), str, key);
	}
	{
		Wcstring str;
		for (MIE_WCHAR_T c = 1; c < 65534; c++) {
			str += c;
		}
		str += 'a';
		str += 'b';
		str += 'c';
		str += 'z';
		const MIE_WCHAR_T tbl[][9] = {
			{ 'x', 0 },
			{ 5, 6, 7, 8, 9, 10, 11, 12, 0 },
			{ 10000, 10001, 10002, 0 },
			{ 200, 201, 203, 0 },
			{ 'a', 'b', 'c', 'z', 0 },
		};
		for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
			const MIE_WCHAR_T *q1 = mywcsstr_C(str.c_str(), tbl[i]);
			const MIE_WCHAR_T *q2 = mie::wcsstr(str.c_str(), tbl[i]);
			TEST_EQUAL(q1, q2);
		}
	}
}

void findChar_test(const std::string& text)
{
	puts("findChar_test");
	std::string str = "123a456abcdefghijklmnob123aa3vnraw3nabcdevra";
	str += "abcdefghijklmaskjfalksjdflaksjflakesoirua93va3vnopasdfasdfaserxdf";
	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < 256; i++) {
			str += (char)i;
		}
	}
	str += "abcdefghijklmn";

	const std::string *pstr = text.empty() ? &str : &text;
	for (int c = '0'; c <= '9'; c++) {
		benchmark("findChar_C", Frange_char<findChar_C>(), "findChar", Frange_char<mie::findChar>(), *pstr, std::string(1, (char)c));
	}
	{
		const char *tt = NULL;
		const char *q1 = findChar_C(tt, tt, 'a');
		const char *q2 = mie::findChar(tt, tt, 'a');
		TEST_EQUAL((int)(q1 - tt), 0);
		TEST_EQUAL((int)(q2 - tt), 0);
	}
	puts("ok");
	{
		Wcstring str;
		for (MIE_WCHAR_T c = 1; c < 65534; c++) {
			str += c;
		}
		for (MIE_WCHAR_T c = 1; c < 65535; c++) {
			const MIE_WCHAR_T *begin = str.c_str();
			const MIE_WCHAR_T *end = str.c_str() + str.size();
			const MIE_WCHAR_T *q1 = mie::findWchar(begin, end, c);
			const MIE_WCHAR_T *q2 = findWchar_C(begin, end, c);
			TEST_EQUAL(q1, q2);
		}
	}
}

void findChar_any_test(const std::string& text)
{
	puts("findChar_any_test");
	std::string str = "123a456abcdefghijklmnob123aa3vnraw3nXbcdevra";
	str += "abcdefghijklmaskjfalksjdflaksjflakesoiruXa93va3vnopasdfasdfaserxdf";
	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < 256; i++) {
			str += (char)i;
		}
	}
	str += "abcdefghYjklmn";
	for (int i = 0; i < 3; i++) {
		str += str;
	}

	const char tbl[][17] = {
		"z",
		"XYZ",
		"ax035ZU",
		"0123456789abcdef"
	};

	const std::string *pstr = text.empty() ? &str : &text;
	for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
		const std::string key = tbl[i];
		benchmark("findChar_any_C", Frange<findChar_any_C>(), "findChar_any", Frange<mie::findChar_any>(), *pstr, key);
	}
	{
		const char *tt = NULL;
		const char *q1 = findChar_any_C(tt, tt, "abcd", 4);
		const char *q2 = mie::findChar_any(tt, tt, "abcd", 4);
		TEST_EQUAL((int)(q1 - tt), 0);
		TEST_EQUAL((int)(q2 - tt), 0);
	}
	puts("ok");
	{
		Wcstring str;
		for (MIE_WCHAR_T c = 1; c < 65535; c++) {
			str += c;
		}
		const MIE_WCHAR_T tbl[][9] = {
			{ 'z' },
			{ 'X', 'Y', 'Z' },
			{ 'a', 'x', '0', '3', 'Z', 'U', 1234, 9999 },
			{ '0', '1', '2', '3' },
		};
		for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
			const MIE_WCHAR_T *begin = str.c_str();
			const MIE_WCHAR_T *end = str.c_str() + str.size();
			const MIE_WCHAR_T *key = tbl[i];
			const size_t keySize =  myWcslen(tbl[i]);
			const MIE_WCHAR_T *q1 = mie::findWchar_any(begin, end, key, keySize);
			const MIE_WCHAR_T *q2 = findWchar_any_C(begin, end, key, keySize);
			TEST_EQUAL(q1, q2);
		}
	}
}

void findChar_range_test(const std::string& text)
{
	puts("findChar_range_test");
#if 1
    std::string str = "123a456abcdefghijklmnob123aa3vnraw3nXbcdevra";
	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < 256; i++) {
			str += (char)i;
		}
	}
	str += "............!!f..!!!!!!!$$$$$$$$$$""!!!0()........A......../....Z";
	str += "abcdefghYjklmn";
	for (int i = 0; i < 3; i++) {
		str += str;
	}

	const char tbl[][16] = {
		"zz",
		"09",
		"az09",
		"0-9a-fA-F",
		"09afAF//..",
	};
	const std::string *pstr = text.empty() ? &str : &text;
	for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
		const std::string key = tbl[i];
		benchmark("findChar_range_C", Frange<findChar_range_C>(), "findChar_range", Frange<mie::findChar_range>(), *pstr, key);
	}
	std::string key = "azAZ<>"; // QQQ:same as in FfindChar_range_emu
	benchmark("findChar_range_emu", FfindChar_range_emu(), "findChar_range", Frange<mie::findChar_range>(), text, key);
	{
		const char *tt = NULL;
		const char *q1 = findChar_range_C(tt, tt, "abcd", 4);
		const char *q2 = mie::findChar_range(tt, tt, "abcd", 4);
		TEST_EQUAL((int)(q1 - tt), 0);
		TEST_EQUAL((int)(q2 - tt), 0);
	}
	puts("ok");
#endif
    {
		Wcstring str;
		for (MIE_WCHAR_T c = 1; c < 65534; c++) {
			str += c;
		}
		const MIE_WCHAR_T tbl[][9] = {
			{ 'z', 'z' },
			{ '0', '9' },
			{ 1234, 5678, 'a', 'f', 'A', 'F', 10000, 20000 },
			{ '0', '9', 'a', 'z', '/', '/' },
			{ 65534, 65535 },
		};
		for (size_t i = 1; i < NUM_OF_ARRAY(tbl); i++) {
#if 0
            printf("i=%d\n", (int)i);
            benchmarkW("findWchar_range_C", Fwrange<findWchar_range_C>(), "findWchar_range", Fwrange<mie::findWchar_range>(), str, tbl[i]);
            benchmarkW("findWchar_range_C", Fwrange<findWchar_range_C>(), "findWchar_range", Fwrange<mie::findWchar_range>(), str, tbl[i]);
#else
			const MIE_WCHAR_T *begin = str.c_str();
			const MIE_WCHAR_T *end = str.c_str() + str.size();
			const MIE_WCHAR_T *key = tbl[i];
			const size_t keySize =  myWcslen(tbl[i]);
			const MIE_WCHAR_T *q1 = mie::findWchar_range(begin, end, key, keySize);
			const MIE_WCHAR_T *q2 = findWchar_range_C(begin, end, key, keySize);
			TEST_EQUAL(q1, q2);
#endif
		}
	}
}

void findStr_test(const std::string& text)
{
	puts("findStr_test");
	struct {
		const char *str;
		const char *key;
	} tbl[] = {
		{ "abcdefghijklmn", "fghi" },
		{ "abcdefghijklmn", "x" },
		{ "abcdefghijklmn", "i" },
		{ "abcdefghijklmn", "ij" },
		{ "abcdefghijklmn", "lmn" },
		{ "abcdefghijklmn", "abcdefghijklm" },
		{ "0123456789abcdefghijkl", "0123456789abcdef" },
		{ "0123456789abcdefghijkl", "0123456789abcdefghijklm" },
		{ "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTU", "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTU@" },
	};
	for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
		std::string str = tbl[i].str;
		for (int j = 0; j < 4; j++) {
			str += str;
		}
		const std::string key = tbl[i].key;
		const std::string *pstr = text.empty() ? &str : &text;
		benchmark("findStr_C", Frange<findStr_C>(), "findStr", Frange<mie::findStr>(), *pstr, key);
		benchmark("findStr2_C", Frange<findStr2_C>(), "findStr", Frange<mie::findStr>(), *pstr, key);
#ifdef __linux__
		benchmark("memmem", Fmemmem(), "findStr", Frange<mie::findStr>(), *pstr, key);
#endif
	}
	{
		MIE_ALIGN(16) const char tt[]="\0a\0bAbc\0ef123";
		const char *key = "bc\0ef12";
		const char *q1 = findStr_C(tt, tt + 13, key, 7);
		const char *q2 = mie::findStr(tt, tt + 13, key, 7);
#ifdef __linux__
		const char *q3 = (const char*)memmem((const void*)tt, 13, (const void*)key, 7);
#endif
		TEST_EQUAL((int)(q1 - tt), 5);
		TEST_EQUAL((int)(q2 - tt), 5);
#ifdef __linux__
		TEST_EQUAL((int)(q3 - tt), 5);
#endif
	}
	{
		const char *tt = NULL;
		const char *q1 = findStr_C(tt, tt, "test", 4);
		const char *q2 = mie::findStr(tt, tt, "test", 4);
		TEST_EQUAL((int)(q1 - tt), 0);
		TEST_EQUAL((int)(q2 - tt), 0);
	}
	puts("ok");
}

void strcasestr_test(const std::string& text)
{
	puts("strcasestr_test");
	std::string str;
	for (int i = 1; i < 256; i++) {
		str += (char)i;
	}
	for (int i = 0; i < 3; i++) {
		str += str;
	}
	for (int i = 1; i < 256; i++) {
		if ('A' <= i && i <= 'Z') continue;
		std::string key(1, (char)i);
		verify(Fstrstr<strcasestr_C>(), Fstrstr<mie::strcasestr>(), str, key);
	}
	str = "@AZ[`az{";
	for (int i = 0; i < 7; i++) {
		str += str;
	}
	const char tbl[][48] = {
		"@",
		"a",
		"z",
		"@az[",
		"`az{",
		"z[`",
		"@az[`az{",
		"@az[`az{@az[`az",
		"@az[`az{@az[`az{@az[`az{@az[`az{",
	};
	for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
		verify(Fstrstr<strcasestr_C>(), Fstrstr<mie::strcasestr>(), str, tbl[i]);
	}
	if (!text.empty()) {
		const char tbl[][32] = {
			"ssl",
			"cybozu",
			"xyz",
			"@",
			"}",
			"["
			"`",
			"abc",
			"xyz",
			"openssl",
			"0123456789",
			"00000000000",
			"abcdefghijklmnopqrstuvwxyz",
		};
		for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
			benchmark("strcasestr", Fstrstr<strcasestr_C>(), "mie::strcasestr", Fstrstr<mie::strcasestr>(), text, tbl[i]);
		}
	}
	puts("ok");
}

void findCaseStr_test(const std::string& text)
{
	puts("findCaseStr_test");
	std::string str;
	for (int i = 0; i < 256; i++) {
		str += (char)i;
	}
	for (int i = 0; i < 3; i++) {
		str += str;
	}
	for (int i = 0; i < 256; i++) {
		if ('A' <= i && i <= 'Z') continue;
		std::string key(1, (char)i);
		verify(Frange<findCaseStr_C>(), Frange<mie::findCaseStr>(), str, key);
	}
	str = "@AZ[`az{";
	for (int i = 0; i < 7; i++) {
		str += str;
	}
	const char tbl[][48] = {
		"@",
		"a",
		"z",
		"@az[",
		"`az{",
		"z[`",
		"@az[`az{",
		"@az[`az{@az[`az",
		"@az[`az{@az[`az{@az[`az{@az[`az{",
	};
	for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
		verify(Frange<findCaseStr_C>(), Frange<mie::findCaseStr>(), str, tbl[i]);
	}
	if (!text.empty()) {
		const char tbl[][32] = {
			"ssl",
			"cybozu",
			"xyz",
			"@",
			"}",
			"["
			"`",
			"abc",
			"xyz",
			"openssl",
			"0123456789",
			"00000000000",
			"abcdefghijklmnopqrstuvwxyz",
		};
		for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
			benchmark("findCaseStr_C", Frange<findCaseStr_C>(), "mie::findCaseStr", Frange<mie::findCaseStr>(), text, tbl[i]);
		}
	}
	{
		MIE_ALIGN(16) const char tt[]="\0a\0bABc\0Ef123";
		const char *q1 = findCaseStr_C(tt, tt + 13, "bc\0ef12", 7);
		const char *q2 = mie::findCaseStr(tt, tt + 13, "bc\0ef12", 7);
		TEST_EQUAL((int)(q1 - tt), 5);
		TEST_EQUAL((int)(q2 - tt), 5);
	}
	{
		const char *tt = NULL;
		const char *q1 = findCaseStr_C(tt, tt, "test", 4);
		const char *q2 = mie::findCaseStr(tt, tt, "test", 4);
		TEST_EQUAL((int)(q1 - tt), 0);
		TEST_EQUAL((int)(q2 - tt), 0);
	}
	puts("ok");
}

int main(int argc, char *argv[])
{
	if (!mie::isAvailableSSE42()) {
		fprintf(stderr, "SSE4.2 is not supported\n");
		return 0;
	}
	argc--, argv++;
	const std::string text = (argc == 1) ? LoadFile(argv[0]) : "";
	std::vector<std::string> keyTbl;

	const char tbl[][32] = {
		"a", // dummy for cache
		"a", "b", "c", "d",
		"ab", "xy", "ex",
		"std", "jit", "asm",
		"atoi", "1234",
		"File", "?????",
		"patch", "56789",
		"\xE3\x81\xA7\xE3\x81\x99", /* de-su */
		"cybozu",
		"openssl",
		"namespace",
		"\xe3\x81\x93\xe3\x82\x8c\xe3\x81\xaf", /* ko-re-wa */
		"cybozu::ssl",
		"asdfasdfasdf",
		"static_assert",
		"const_iterator",
		"000000000000000",
		"WARIXDFSKVJWSVFDVWESVF",
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
	};

	for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
		keyTbl.push_back(tbl[i]);
	}

	try {
//		findChar_range_test(text);
//return 0;
//		benchmarkTbl("memmem", Fmemmem(), "findStr", Frange<mie::findStr>(), text, keyTbl);
//		return 0;
#ifdef USE_BOOST_BM
		benchmarkTbl("boost", Frange_boost_bm_find(), "mie::strstr", Fstrstr<mie::strstr>(), text, keyTbl);
		return 0;
#endif
#ifdef USE_MISCHASAN
		benchmarkTbl("mischasan", Fmischasan_strstr(), "strstr", Fstrstr<STRSTR>(), text, keyTbl);
		benchmarkTbl("mischasan", Fmischasan_strstr(), "mie::strstr", Fstrstr<mie::strstr>(), text, keyTbl);
		return 0;
#endif
		strcasestr_test(text);
		findCaseStr_test(text);
#if 1
		/*
			compare strstr with strcasestr
			strcasestr speed is about 0.66~0.70 times speed of strstr
		*/
		if (!text.empty()) {
			std::string itext = text;
			for (size_t i = 0; i < itext.size(); i++) {
				char c = itext[i];
				if ('A' <= c && c <= 'Z') itext[i] = c + 'a' - 'A';
			}
			benchmarkTbl("mie::strstr", Fstrstr<mie::strstr>(), "mie::strcasestr", Fstrstr<mie::strcasestr>(), itext, keyTbl);
		}
#endif
		strlen_test();
		strchr_test(text);
		strchr_any_test(text);
		strchr_range_test(text);

		strstr_test();
		if (!text.empty()) {
			benchmarkTbl("strstr_C", Fstrstr<STRSTR>(), "strstr", Fstrstr<mie::strstr>(), text, keyTbl);
			benchmarkTbl("mystrstr_C", Fstrstr<mystrstr_C>(), "strstr", Fstrstr<mie::strstr>(), text, keyTbl);
			benchmarkTbl("string::find", Fstr_find(), "findStr", Frange<mie::findStr>(), text, keyTbl);
		}

		findChar_test(text);
		findChar_any_test(text);
		findChar_range_test(text);
		findStr_test(text);
		return 0;
	} catch (std::exception& e) {
		printf("ERR:%s\n", e.what());
	} catch (...) {
		printf("unknown error\n");
	}
	return 1;
}
