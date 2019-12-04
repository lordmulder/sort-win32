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
#include <mutex>

extern "C"
{
	__declspec(dllimport) int __stdcall StrCmpLogicalW(const wchar_t *psz1, const wchar_t *psz2);
	__declspec(dllimport) unsigned int __stdcall SetErrorMode(unsigned int uMode);
}

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

class RandomMT19937
{
public:
	static size_t next(const size_t max)
	{
		const std::lock_guard<std::mutex> lock(mutex);
		if (!mt19937)
		{
			std::random_device rnd;
			mt19937.reset(new std::mt19937_64(rnd()));
		}
		return (*mt19937)() % max;
	}

private:
	static std::mutex mutex;
	static std::unique_ptr<std::mt19937_64> mt19937;
};

std::mutex RandomMT19937::mutex;
std::unique_ptr<std::mt19937_64> RandomMT19937::mt19937;

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

struct CompareStrL
{
	bool operator() (const std::wstring& lhs, const std::wstring& rhs) const
	{
		return (StrCmpLogicalW(lhs.c_str(), rhs.c_str()) < 0);
	}
};

struct CompareStrLR
{
	bool operator() (const std::wstring& lhs, const std::wstring& rhs) const
	{
		return (StrCmpLogicalW(lhs.c_str(), rhs.c_str()) > 0);
	}
};

// ==========================================================================
// Iterator class
// ==========================================================================

class IIterator
{
public:
	virtual bool hasNext(void) = 0;
	virtual const std::wstring &next(void) = 0;
};

template <typename IterType>
class Iterator : public IIterator
{
public:
	Iterator(const IterType &begin, const IterType &end)
		: m_current(begin), m_end(end)
	{
	}

	virtual bool hasNext(void)
	{
		return m_current != m_end;
	}

	virtual const std::wstring &next(void)
	{
		return *m_current++;
	}

private:
	Iterator(const Iterator&) = delete;
	IterType m_current;
	const IterType m_end;
};

// ==========================================================================
// Store class
// ==========================================================================

class IStore
{
public:
	IStore(const bool utf16, const bool trim, const bool skip_blank, const bool flush)
	:
		m_utf16(utf16), m_trim(trim), m_skip_blank(skip_blank), m_flush(flush)
	{
	}

	virtual bool read(const wchar_t *const file_name)
	{
		FILE *const file = file_name ? _wfopen(file_name, m_utf16 ? L"rt,ccs=UNICODE" : L"rt,ccs=UTF-8") : stdin;
		if (!file)
		{
			fwprintf(stderr, L"Failed to open input file: %s\n", file_name);
			return false;
		}

		bool truncated_this = false, truncated_last = false;
		wchar_t *line;
		while (line = read_line(m_buffer, BUFFER_SIZE, file, m_trim, truncated_this))
		{
			if (!(truncated_last || (m_skip_blank && is_blank(line))))
			{
				add(line);
			}
			truncated_last = truncated_this;
		}

		if (file_name)
		{
			fclose(file);
		}

		return true;
	}

	virtual bool write(void)
	{
		const std::unique_ptr<IIterator> iter(iterator());
		while (iter->hasNext())
		{
			if (fwprintf(stdout, L"%s\n", iter->next().c_str()) < 0)
			{
				return false;
			}
			if (m_flush)
			{
				fflush(stdout); /*force flush*/
			}
		}
		return true;
	}

protected:
	virtual void add(const wchar_t *const line) = 0;
	virtual IIterator *iterator(void) = 0;

private:
	IStore(const IStore&) = delete;
	wchar_t m_buffer[BUFFER_SIZE];
	const bool m_utf16, m_trim, m_skip_blank, m_flush;
};

template <template<class, class, class> typename Set, typename Comparator>
class Sorter : public IStore
{
public:
	Sorter(const bool utf16, const bool trim, const bool skip_blank, const bool flush)
		: IStore(utf16, trim, skip_blank, flush)
	{
	}

	static IStore* createInstance(const bool utf16, const bool trim, const bool skip_blank, const bool flush)
	{
		return new Sorter(utf16, trim, skip_blank, flush);
	}

protected:
	typedef Set<std::wstring, Comparator, std::allocator<std::wstring>> ConatinerType;

	virtual void add(const wchar_t *const line)
	{
		m_store.insert(line);
	}

	virtual IIterator *iterator(void)
	{
		return new Iterator<typename ConatinerType::const_iterator>(m_store.cbegin(), m_store.cend());
	}

private:
	ConatinerType m_store;
};

class Shuffler : public IStore
{
public:
	Shuffler(const bool utf16, const bool trim, const bool skip_blank, const bool flush)
		: IStore(utf16, trim, skip_blank, flush)
	{
	}

	static IStore* createInstance(const bool utf16, const bool trim, const bool skip_blank, const bool flush)
	{
		return new Shuffler(utf16, trim, skip_blank, flush);
	}

protected:
	virtual void add(const wchar_t *const line)
	{
		m_store.push_back(line);
	}

	virtual IIterator *iterator(void)
	{
		std::random_shuffle(m_store.begin(), m_store.end(), RandomMT19937::next);
		return new Iterator<std::vector<std::wstring>::const_iterator>(m_store.cbegin(), m_store.cend());
	}

private:
	std::vector<std::wstring> m_store;
};

// ==========================================================================
// Helper functions
// ==========================================================================

#define CREATE_SORTER(X,Y) Sorter<std::X,Y>::createInstance(params.utf16, params.trim, params.skip_blank, params.flush)

typedef struct _param_t
{
	bool reverse;
	bool ignore_case;
	bool unique;
	bool numerical;
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
	fwprintf(stderr, L"Sort/Shuf for Win32 [%S], created by LoRd_MuldeR <mulder2@gmx.de>\n", __DATE__);
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
	fputws(L"   --numerical     Digits in the lines are considered as numerical content.\n\n", stderr);
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
	else if (!_wcsicmp(arg_name, L"numerical"))
	{
		params.numerical = true;
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
	if (params.shuffle && (params.ignore_case || params.reverse || params.unique || params.numerical))
	{
		fputws(L"Error: Option \"--shuffle\" can not be combined with any of the sorting options!\n", stderr);
		fputws(L"Please type \"sort.exe --help\" for details.\n", stderr);
		return false;
	}
	if (params.ignore_case && params.numerical)
	{
		fputws(L"Error: Options \"--ignore-case\" and \"--numerical\" are mutually exclusive!\n", stderr);
		fputws(L"Please type \"sort.exe --help\" for details.\n", stderr);
		return false;
	}
	return true;
}

static IStore *create_store(const param_t &params)
{
	if (params.shuffle)
	{
		return Shuffler::createInstance(params.utf16, params.trim, params.skip_blank, params.flush);
	}
	else
	{
		if (!params.numerical)
		{
			if (params.unique)
			{
				if (params.ignore_case)
				{
					return (params.reverse) ? CREATE_SORTER(set, CompareStrIR) : CREATE_SORTER(set, CompareStrI);
				}
				else
				{
					return (params.reverse) ? CREATE_SORTER(set, CompareStrR) : CREATE_SORTER(set, CompareStr);
				}
			}
			else
			{
				if (params.ignore_case)
				{
					return (params.reverse) ? CREATE_SORTER(multiset, CompareStrIR) : CREATE_SORTER(multiset, CompareStrI);
				}
				else
				{
					return (params.reverse) ? CREATE_SORTER(multiset, CompareStrR) : CREATE_SORTER(multiset, CompareStr);
				}
			}
		}
		else
		{
			if (params.unique)
			{
				return (params.reverse) ? CREATE_SORTER(set, CompareStrLR) : CREATE_SORTER(set, CompareStrL);
			}
			else
			{
				return (params.reverse) ? CREATE_SORTER(multiset, CompareStrLR) : CREATE_SORTER(multiset, CompareStrL);
			}
		}
	}
}

// ==========================================================================
// MAIN
// ==========================================================================

extern "C"
{
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

	if (arg_off < argc)
	{
		while (arg_off < argc)
		{
			const wchar_t *const file_name = argv[arg_off++];
			if (!store->read(file_name))
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
		success = store->read(NULL); /*stdin*/
	}

	if (success || params.keep_going)
	{
		if (!store->write())
		{
			success = false; /*failed*/
		}
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
	catch (const std::bad_alloc&)
	{
		fputws(L"\nEXCEPTION: Out of memory error !!!\n\n", stderr);
		fflush(stderr);
		_exit(666);
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
