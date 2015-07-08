# barred
Binary Arrays Editor

This program allows to view and edit binary files which contain
array of constant-width records, like databases used in games.

Usage:
     barred [-s <number>] [-n <number>] <file>
  where:
 [file]   - File name to edit,
 -s <number> - Index of the byte within file which will be used as first.
 -n <number> - Amount of bytes per line.
  
If run without parameters, the program will ask for file name.

Note that it is a console tool, with Midnight Commander / FAR Manager styled interface.
