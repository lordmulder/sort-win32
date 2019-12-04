Sort for Win32, created by LoRd_MuldeR <mulder2@gmx.de>
This work is licensed under the CC0 1.0 Universal License.

Reads lines from the stdin, sorts these lines, and prints them to the stdout.
Optionally, lines can be read from one or multiple files instead of stdin.

Usage:
   sort.exe [OPTIONS] [<FILE_1> [<FILE_2> ... ]]

Sorting options:
   --reverse       Sort the lines descending, default is ascending.
   --ignore-case   Ignore the character casing while sorting the lines.
   --unique        Discard any duplicate lines from the result set.
   --numerical     Digits in the lines are considered as numerical content.

Input options:
   --trim          Remove leading/trailing whitespace characters.
   --skip-blank    Discard any lines consisting solely of whitespaces.
   --utf16         Process input lines as UTF-16, default is UTF-8.

Other options:
   --shuffle       Shuffle the lines randomly, instead of sorting.
   --flush         Force flush of the stdout after each line was printed.
   --keep-going    Do not abort, if processing an input file failed.
