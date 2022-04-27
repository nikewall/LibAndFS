# LibAndFS

This is my implementation for the final project in my Operating Systems course.

Written in pure C++11, the code within the `lib` directory defines/implements the structures to handle the data from an
input file.

The code within the `fs` directory requires `libfuse` on UNIX systems, and implements the mounting of a new file structure, as specified by the `lib` parsing of the file. This file structure is mountable to any empty directory.
