#!/bin/sh

g++ -std=c++11 -g -Wall -o lps src/*.cxx  -lpqxx -ltbb -ltbbmalloc_proxy -ltelnet -lhttpserver -lmicrohttpd -I third_party/include/ -L third_party/lib/ -Wl,-rpath=third_party/lib
