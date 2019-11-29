Sort for Win32 [Nov 29 2019], created by LoRd_MuldeR <mulder2@gmx.de>
This work is licensed under the CC0 1.0 Universal License.

Reads lines from the stdin, sortes these lines, and prints them to the stdout.
Optionally, lines can be read from one or multiple files instead of stdin.

Usage:
   sort.exe [OPTIONS] [<FILE_1> [<FILE_2> ... ]]

Options:
   --reverse       Sort the lines descending, instead of ascending.
   --ignore-case   Ignore the character casing when sorting the lines.
   --trim          Remove leading/trailing whitespace characters.
   --force-flush   Force flush of stdout after each line was printed.
   --unique        Discard any duplicate lines from the result set.
   --skip-blank    Discard any lines consisting solely of whitespaces.
   --natural       Sort the lines using 'natural' string order.
   --keep-going    Do not abort, if processing an input file failed.
