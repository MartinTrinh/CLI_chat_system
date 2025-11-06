CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -fPIC
LDFLAGS = -pthread

# Targets
all: server

# Server
server: chat_server.cpp common.h
	$(CXX) $(CXXFLAGS) -o server chat_server.cpp $(LDFLAGS)

clean:
	rm -f server
	rm -f *.o

.PHONY: all clean