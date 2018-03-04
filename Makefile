SOURCES=myShell.cpp myShell.h
OBJS=$(patsubst %.cpp, %.o, $(SOURCES))
CPPFLAGS=-ggdb3 -Wall -Werror -pedantic -std=c++03 -O3

myShell: $(OBJS)
	g++ $(CPPFLAGS) -o myShell $(OBJS)
%.o: %.cpp myShell.h
	g++ $(CPPFLAGS) -c $<

clean:
	rm myshell *~ *.o
