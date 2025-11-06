CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -fPIC
LDFLAGS = -pthread

# Targets
all: libchatclient


# Client Library (Shared Library)
libchatclient: ChatClientLib.cpp ChatClientLib.h
	$(CXX) $(CXXFLAGS) -shared -o libchatclient.so ChatClientLib.cpp $(LDFLAGS) 


clean:
	rm -f libchatclient.so 
	rm -f *.o

.PHONY: all clean