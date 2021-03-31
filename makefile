all:
	g++ owmread.C lib/libjsoncpp.a -lcurl -lboost_program_options -o owmread -O6 --std=c++11
