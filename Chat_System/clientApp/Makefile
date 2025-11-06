CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -fPIC
LDFLAGS = -pthread

# Targets
all: client_app


client_app: ClientApp.cpp libchatclient.so ChatClientLib.h
	$(CXX) $(CXXFLAGS) -o client_app ClientApp.cpp -L. -lchatclient $(LDFLAGS)


clean:
	rm -f client_app
	rm -f *.o

.PHONY: all clean