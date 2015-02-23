all:	P2.cpp P2.h
	g++ -o P2 P2.cpp
debug:	P2.cpp P2.h
	g++ -g -o P2 P2.cpp
clean:
	rm -f *.o *~ P2 core
