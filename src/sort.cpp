/*
 * Sort for Win32
 * Created by LoRd_MuldeR <mulder2@gmx.de>.
 *
 * This work is licensed under the CC0 1.0 Universal License.
 * To view a copy of the license, visit:
 * https://creativecommons.org/publicdomain/zero/1.0/legalcode
 */

#include <cstdio>
#include <set>
#include <vector>
#include <clocale>
#include <algorithm>
#include <memory>
#include <fcntl.h>
#include <io.h>
#include <stdexcept>
#include <random>
#include "strnatcmp.h"

#define BUFFER_SIZE 131072U

// ==========================================================================
// Utility functions
// ==========================================================================

static __forceinline bool is_whitesapce(const wchar_t c)
{
	return iswspace(c) || iswcntrl(c);
}

static __forceinline wchar_t *trim_str(wchar_t *str)
{
	while ((*str) && is_whitesapce(*str))
	{
		++str;
	}
	size_t len = wcslen(str);
	while (len > 0U)
	{
		if(is_whitesapce(str[--len]))
		{
			str[len] = L'\0';
			continue;
		}
		break;
	}
	return str;
}

static __forceinline bool is_blank(const wchar_t *str)
{
	while (*str)
	{
		if (!is_whitesapce(*(str++)))
		{
			return false;
		}
	}
	return true;
}

static __forceinline wchar_t *read_line(wchar_t *const buffer, const int max_count, FILE *const stream, const bool trim, bool &truncated)
{
	truncated = true;
	wchar_t *const line = fgetws(buffer, max_count, stream);
	if (line)
	{
		size_t len = wcslen(line);
		while (len > 0U)
		{
			if(line[--len] == L'\n')
			{
				line[len] = L'\0';
				truncated = false;
			}
		}
		return trim ? trim_str(line) : line;
	}
	return NULL;
}

// ==========================================================================
// Random engine
// ==========================================================================

static std::unique_ptr<std::mt19937_64> g_random;

static __forceinline void mt_init(void)
{
	std::random_device rd;
	g_random.reset(new std::mt19937_64());
	g_random->seed(rd());
}

static __forceinline size_t mt_next(const size_t max)
{
	if (!g_random)
	{
		throw std::logic_error("Random number generator not initialized!");
	}
	return (*g_random)() % max;
}

// ==========================================================================
// CompareStr classes
// ==========================================================================

struct CompareStr
{
	bool operator() (const std::wstring& lhs, const std::wstring& rhs) const
	{
		return (wcscmp(lhs.c_str(), rhs.c_str()) < 0);
	}
};

struct CompareStrR
{
	bool operator() (const std::wstring& lhs, const std::wstring& rhs) const
	{
		return (wcscmp(lhs.c_str(), rhs.c_str()) > 0);
	}
};

struct CompareStrI
{
	bool operator() (const std::wstring& lhs, const std::wstring& rhs) const
	{
		return (_wcsicmp(lhs.c_str(), rhs.c_str()) < 0);
	}
};

struct CompareStrIR
{
	bool operator() (const std::wstring& lhs, const std::wstring& rhs) const
	{
		return (_wcsicmp(lhs.c_str(), rhs.c_str()) > 0);
	}
};

struct CompareStrN
{
	bool operator() (const std::wstring& lhs, const std::wstring& rhs) const
	{
		return (strnatcmp(lhs.c_str(), rhs.c_str()) < 0);
	}
};

struct CompareStrNR
{
	bool operator() (const std::wstring& lhs, const std::wstring& rhs) const
	{
		return (strnatcmp(lhs.c_str(), rhs.c_str()) > 0);
	}
};

struct CompareStrNI
{
	bool operator() (const std::wstring& lhs, const std::wstring& rhs) const
	{
		return (strnatcasecmp(lhs.c_str(), rhs.c_str()) < 0);
	}
};

struct CompareStrNIR
{
	bool operator() (const std::wstring& lhs, const std::wstring& rhs) const
	{
		return (strnatcasecmp(lhs.c_str(), rhs.c_str()) > 0);
	}
};

// ==========================================================================
// Sort class
// ==========================================================================

#define CREATE_SORT(SET,CMP) (static_cast<IStore*>(new Store<std::SET<std::wstring,CMP>>()))

class IStore
{
public:
	virtual bool read(const wchar_t *const file_name, const bool utf16, const bool trim, const bool skip_blank) = 0;
	virtual void shuffle(void) = 0;
	virtual bool write(const bool flush) = 0;
};

template <typename ContainerType> class Store : public IStore
{
public:
	virtual bool read(const wchar_t *const file_name, const bool utf16, const bool trim, const bool skip_blank)
	{
		FILE *const file = file_name ? _wfopen(file_name, utf16 ? L"rt,ccs=UNICODE" : L"rt,ccs=UTF-8") : stdin;
		if (!file)
		{
			fwprintf(stderr, L"Failed to open input file: %s\n", file_name);
			return false;
		}

		bool truncated_this = false, truncated_last = false;
		wchar_t *line;
		while (line = read_line(m_buffer, BUFFER_SIZE, file, trim, truncated_this))
		{
			if (!(truncated_last || (skip_blank && is_blank(line))))
			{
				add(m_store, line);
			}
			truncated_last = truncated_this;
		}

		if (file_name)
		{
			fclose(file);
		}

		return true;
	}

	virtual void shuffle(void)
	{
		shuffle(m_store);
	}

	virtual bool write(const bool flush)
	{
		for (auto iter = m_store.cbegin(); iter != m_store.cend(); ++iter)
		{
			if (fwprintf(stdout, L"%s\n", iter->c_str()) < 0)
			{
				return false;
			}
			if (flush)
			{
				fflush(stdout); /*force flush*/
			}
		}

		return true;
	}

protected:
	template <typename T>
	static void add(std::set<std::wstring, T> &store, const wchar_t *const line)
	{
		store.insert(line);
	}

	template <typename T>
	static void add(std::multiset<std::wstring, T> &store, const wchar_t *const line)
	{
		store.insert(line);
	}

	static void add(std::vector<std::wstring> &store, const wchar_t *const line)
	{
		store.push_back(line);
	}

	template <typename T>
	static void shuffle(std::set<std::wstring, T> &store)
	{
		throw std::logic_error("Shuffle not supported by this type of storage!");
	}

	template <typename T>
	static void shuffle(std::multiset<std::wstring, T> &store)
	{
		throw std::logic_error("Shuffle not supported by this type of storage!");
	}

	static void shuffle(std::vector<std::wstring> &store)
	{
		std::random_shuffle(store.begin(), store.end(), mt_next);
	}

private:
	ContainerType m_store;
	wchar_t m_buffer[BUFFER_SIZE];
};

// ==========================================================================
// Helper functions
// ==========================================================================

typedef struct _param_t
{
	bool reverse;
	bool ignore_case;
	bool unique;
	bool natural;
	bool trim;
	bool skip_blank;
	bool utf16;
	bool shuffle;
	bool flush;
	bool keep_going;
}
param_t;

static void print_logo(void)
{
	fwprintf(stderr, L"Sort for Win32 [%S], created by LoRd_MuldeR <mulder2@gmx.de>\n", __DATE__);
	fputws(L"This work is licensed under the CC0 1.0 Universal License.\n\n", stderr);
}

static void print_manpage(void)
{
	print_logo();
	fputws(L"Reads lines from the stdin, sorts these lines, and prints them to the stdout.\n", stderr);
	fputws(L"Optionally, lines can be read from one or multiple files instead of stdin.\n\n", stderr);
	fputws(L"Usage:\n", stderr);
	fputws(L"   sort.exe [OPTIONS] [<FILE_1> [<FILE_2> ... ]]\n\n", stderr);
	fputws(L"Sorting options:\n", stderr);
	fputws(L"   --reverse       Sort the lines descending, default is ascending.\n", stderr);
	fputws(L"   --ignore-case   Ignore the character casing while sorting the lines.\n", stderr);
	fputws(L"   --unique        Discard any duplicate lines from the result set.\n", stderr);
	fputws(L"   --natural       Sort the lines using 'natural order' string comparison.\n\n", stderr);
	fputws(L"Input options:\n", stderr);
	fputws(L"   --trim          Remove leading/trailing whitespace characters.\n", stderr);
	fputws(L"   --skip-blank    Discard any lines consisting solely of whitespaces.\n", stderr);
	fputws(L"   --utf16         Process input lines as UTF-16, default is UTF-8.\n\n", stderr);
	fputws(L"Other options:\n", stderr);
	fputws(L"   --shuffle       Shuffle the lines randomly, instead of sorting.\n", stderr);
	fputws(L"   --flush         Force flush of the stdout after each line was printed.\n", stderr);
	fputws(L"   --keep-going    Do not abort, if processing an input file failed.\n", stderr);
}

static bool parse_option(const wchar_t *const arg_name, param_t &params)
{
	if (!_wcsicmp(arg_name, L"reverse"))
	{
		params.reverse = true;
	}
	else if (!_wcsicmp(arg_name, L"ignore-case"))
	{
		params.ignore_case = true;
	}
	else if (!_wcsicmp(arg_name, L"unique"))
	{
		params.unique = true;
	}
	else if (!_wcsicmp(arg_name, L"natural"))
	{
		params.natural = true;
	}
	else if (!_wcsicmp(arg_name, L"trim"))
	{
		params.trim = true;
	}
	else if (!_wcsicmp(arg_name, L"skip-blank"))
	{
		params.skip_blank = true;
	}
	else if (!_wcsicmp(arg_name, L"utf16"))
	{
		params.utf16 = true;
	}
	else if (!_wcsicmp(arg_name, L"shuffle"))
	{
		params.shuffle = true;
	}
	else if (!_wcsicmp(arg_name, L"flush"))
	{
		params.flush = true;
	}
	else if (!_wcsicmp(arg_name, L"keep-going"))
	{
		params.keep_going = true;
	}
	else if (!_wcsicmp(arg_name, L"help"))
	{
		print_manpage();
		exit(EXIT_SUCCESS);
	}
	else
	{
		print_logo();
		fwprintf(stderr, L"Error: Specified option \"--%s\" is unknown or misspelled!\n", arg_name);
		fputws(L"Please type \"sort.exe --help\" for details.\n", stderr);
		return false;
	}
	return true;
}

static bool parse_all_options(const int argc, const wchar_t *const argv[], int &arg_off, param_t &params)
{
	memset(&params, 0, sizeof(param_t));
	while ((arg_off < argc) && (!_wcsnicmp(argv[arg_off], L"--", 2U)))
	{
		const wchar_t *const arg_name = argv[arg_off++] + 2U;
		if (*arg_name)
		{
			if (!parse_option(arg_name, params))
			{
				return false;
			}
			continue;
		}
		break;
	}
	if (params.shuffle && (params.ignore_case || params.reverse || params.unique || params.natural))
	{
		fputws(L"Error: Option \"--shuffle\" can not be combined with any of the sorting options!\n", stderr);
		fputws(L"Please type \"sort.exe --help\" for details.\n", stderr);
		return false;
	}
	return true;
}

static IStore *create_store(const param_t &params)
{
	if (params.shuffle)
	{
		return new Store<std::vector<std::wstring>>();
	}
	else
	{
		if (!params.natural)
		{
			if (params.unique)
			{
				if (params.ignore_case)
				{
					return (params.reverse) ? CREATE_SORT(set, CompareStrIR) : CREATE_SORT(set, CompareStrI);
				}
				else
				{
					return (params.reverse) ? CREATE_SORT(set, CompareStrR) : CREATE_SORT(set, CompareStr);
				}
			}
			else
			{
				if (params.ignore_case)
				{
					return (params.reverse) ? CREATE_SORT(multiset, CompareStrIR) : CREATE_SORT(multiset, CompareStrI);
				}
				else
				{
					return (params.reverse) ? CREATE_SORT(multiset, CompareStrR) : CREATE_SORT(multiset, CompareStr);
				}
			}
		}
		else
		{
			if (params.unique)
			{
				if (params.ignore_case)
				{
					return (params.reverse) ? CREATE_SORT(set, CompareStrNIR) : CREATE_SORT(set, CompareStrNI);
				}
				else
				{
					return (params.reverse) ? CREATE_SORT(set, CompareStrNR) : CREATE_SORT(set, CompareStrN);
				}
			}
			else
			{
				if (params.ignore_case)
				{
					return (params.reverse) ? CREATE_SORT(multiset, CompareStrNIR) : CREATE_SORT(multiset, CompareStrNI);
				}
				else
				{
					return (params.reverse) ? CREATE_SORT(multiset, CompareStrNR) : CREATE_SORT(multiset, CompareStrN);
				}
			}
		}
	}
}

// ==========================================================================
// MAIN
// ==========================================================================

extern "C"
{
	__declspec(dllimport) unsigned int __stdcall SetErrorMode(unsigned int uMode);
}

static int sort_main(int argc, wchar_t *argv[])
{
	int arg_off = 1;
	bool success = true;
	param_t params;
	
	if (!parse_all_options(argc, argv, arg_off, params))
	{
		return EXIT_FAILURE;
	}

	_setmode(_fileno(stdin),  params.utf16 ? _O_WTEXT : _O_U8TEXT);
	_setmode(_fileno(stdout), params.utf16 ? _O_WTEXT : _O_U8TEXT);

	std::unique_ptr<IStore> store(create_store(params));

	if (params.shuffle)
	{
		mt_init();
	}

	if (arg_off < argc)
	{
		while (arg_off < argc)
		{
			const wchar_t *const file_name = argv[arg_off++];
			if (!store->read(file_name, params.utf16, params.trim, params.skip_blank))
			{
				success = false;
				if (!params.keep_going)
				{
					break; /*stop!*/
				}
			}
		}
	}
	else
	{
		if (!store->read(NULL, params.utf16, params.trim, params.skip_blank))
		{
			success = false;
		}
	}

	if (params.shuffle)
	{
		store->shuffle();
	}

	if (!store->write(params.flush))
	{
		success = false;
	}

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

int wmain(int argc, wchar_t *argv[])
{

	SetErrorMode(SetErrorMode(0x0003) | 0x0003);
	setlocale(LC_ALL, "C");
	_setmode(_fileno(stderr), _O_U8TEXT);

#ifdef NDEBUG
	int result = -1;
	try
	{
		result = sort_main(argc, argv);
	}
	catch (const std::exception& ex)
	{
		fwprintf(stderr, L"\nEXCEPTION: %S\n\n", ex.what());
		fflush(stderr);
		_exit(666);
	}
	catch (...)
	{
		fputws(L"\nEXCEPTION: Unhandeled exception error !!!\n\n", stderr);
		fflush(stderr);
		_exit(666);
	}
#else
	const int result = sort_main(argc, argv);
#endif

	return result;
}
