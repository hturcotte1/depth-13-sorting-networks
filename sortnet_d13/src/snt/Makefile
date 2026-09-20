CXX ?= g++
CXXFLAGS = -std=c++20 -O3 -march=native -Wall -Wno-sign-compare -pthread
snt: snt.cc gen.cc iso.cc cnf.cc sn.h iso.h cnf.h
	$(CXX) $(CXXFLAGS) snt.cc gen.cc iso.cc cnf.cc -o snt
clean:
	rm -f snt
