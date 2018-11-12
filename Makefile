main : main.o  lookuproute.o checksum.o arpfind.o sendetherip.o recvroute.o
		g++ -std=c++11 -o main -static main.o lookuproute.o checksum.o arpfind.o sendetherip.o recvroute.o -lpthread -g
main.o      : main.cpp      lookuproute.h checksum.h arpfind.h sendetherip.h recvroute.h
		g++ -std=c++11 -c main.cpp
lookuproute.o : lookuproute.cpp lookuproute.h
		g++ -std=c++11 -c lookuproute.cpp
checksum.o  : checksum.cpp  checksum.h
		g++ -std=c++11 -c checksum.cpp
arpfind.o   : arpfind.cpp   arpfind.h
		g++ -std=c++11 -c arpfind.cpp
sendetherip.o : sendetherip.cpp sendetherip.h
		g++ -std=c++11 -c sendetherip.cpp
recvroute.o : recvroute.cpp recvroute.h
		g++ -std=c++11 -c recvroute.cpp
clean :
		rm main  main.o lookuproute.o checksum.o arpfind.o sendetherip.o recvroute.o
SUBDIR = $(shell ls ./ -R | grep /)
SUBDIRS  = $(subst :,/,$(SUBDIR))
SOURCE = $(foreach dir, $(SUBDIRS),$(wildcard $(dir)*.o))
#clean:
#	    rm  -rf $(SOURCE)
