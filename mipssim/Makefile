CPP=g++
FLAGS=-g -std=c++11 -Wall -Werror

all:	mipssim

mipssim:	mipssim.o instruction.o utils.o
		$(CPP) $(FLAGS) -o mipssim mipssim.o instruction.o utils.o

mipssim.o:	mipssim.cpp
		$(CPP) $(FLAGS) -c mipssim.cpp

instruction.o:	instruction.cpp instruction.h
		$(CPP) $(FLAGS) -c instruction.cpp

utils.o:	utils.cpp utils.h
		$(CPP) $(FLAGS) -c utils.cpp

clean:
		rm -f *.o mipssim

