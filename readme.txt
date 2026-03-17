busmon
------

Tool for logging values off a canbus in a terminal, hacked 
together from a bunch of old code. Right now the frame 
configuration file format is a custom legacy format from
a different project, but I will also add support for DBC 
files at some point.

Usage
-----

Usage: ./bin/busmon -f <mappings.txt> [options]

Options:
  -f <path>    Packet mapping config file (required)
  -h <host>    CAN bridge host  [192.168.0.20]
  -p <port>    CAN bridge port  [10001]
  -r <hz>      Display refresh rate  [10]
  -v           Verbose CAN traffic logging

License
-------

Written by Justus Languell <justus@tamu.edu>, MIT license applies.

Copyright 2026 Texas A&M University

Permission is hereby granted, free of charge, to any person 
obtaining a copy of this software and associated documentation 
files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


