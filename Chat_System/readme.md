CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -fPIC
LDFLAGS = -pthread

# Targets
all: server libchatclient.so client_app

# Server
server: server.cpp
	$(CXX) $(CXXFLAGS) -o server server.cpp $(LDFLAGS)

# Client Library (Shared Library)
libchatclient.so: ChatClientLib.cpp ChatClientLib.h
	$(CXX) $(CXXFLAGS) -shared -o libchatclient.so ChatClientLib.cpp $(LDFLAGS)

# Client Application (sử dụng library)
client_app: ClientApp.cpp libchatclient.so
	$(CXX) $(CXXFLAGS) -o client_app ClientApp.cpp -L. -lchatclient $(LDFLAGS)

# Static library (optional)
libchatclient.a: ChatClientLib.cpp ChatClientLib.h
	$(CXX) $(CXXFLAGS) -c ChatClientLib.cpp -o ChatClientLib.o
	ar rcs libchatclient.a ChatClientLib.o

# Client with static library
client_app_static: ClientApp.cpp libchatclient.a
	$(CXX) $(CXXFLAGS) -o client_app_static ClientApp.cpp libchatclient.a $(LDFLAGS)

clean:
	rm -f server client_app client_app_static
	rm -f libchatclient.so libchatclient.a
	rm -f *.o

.PHONY: all clean
