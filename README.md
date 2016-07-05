README for Plethora Backend Challenge
Author: Mark Shen

===============================================================================

How to install all dependencies:
- GCC must be installed. On OSX, install XCode with command line tools or 
- Install libcURL
	- I used brew to install this, command was:
	- $ brew install curl 
- Install RapidJSON 
	- Get from: https://github.com/miloyip/rapidjson/
	- Once downloaded, copy the include/rapidjson directory to a common include 
	  path as RapidJSON is a header-only library

===============================================================================

How to build your program (if applicable):
- Navigate to directory of source
- $ g++ pChal.cpp  -I/usr/local/include/ -std=c++11 -lcurl -g -o pChal.o
	- g argument was for debug
	- /usr/local/include is to include RapidJSON and libcURL
	- std=c++11 enables a few function calls made

===============================================================================

How to run your program (example commands that illustrate the different command
line options and expected output are especially helpful)

Run after compiling with ./pChal.o 
The --days argument causes the file to look for a quake history file and will 
build and append to one after retrieving the latest data. 

	MarkSMBA:PChalCpp userid$ ./pChal.o --days 35

	REGION		EARTHQUAKE COUNT	TOTAL MAGNITUDE
	R0			6643				6.29445
	R1			63					6.50085
	R2			5					5.10806
	R3			8					5.66511
	R4			35					6.62135
	R5			19					6.12771
	R6			1563				6.97168
	R7			141					7.17401

If days is not specified, an error message is displayed

	MarkSMBA:PChalCpp userid$ ./pChal.o --days
	--days option requires one argument.

If the days argument is not used, the default used is 30 days.

	MarkSMBA:PChalCpp reverla$ ./pChal.o

	REGION		EARTHQUAKE COUNT	TOTAL MAGNITUDE
	R0			6635				6.29438
	R1			63					6.50085
	R2			5					5.10806
	R3			7					5.62602
	R4			35					6.62135
	R5			19					6.12771
	R6			1560				6.97093
	R7			141					7.17401