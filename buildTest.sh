g++ -g -Wall -std=c++11  -I./dep/include -I./dep/include/asio -L./dep/lib -lpthread -lsha3 -luECC -lmongoc-1.0 -lbson-1.0 -DASIO_STANDALONE Log.cpp Clock.cpp Server.cpp client_test.cpp Tcp.cpp Service.cpp TestProtocolCmd.cpp -o testClient -Wl,-rpath=dep/lib
g++ -g -Wall -std=c++11  -I./dep/include -I./dep/include/asio -L./dep/lib -lpthread -lsha3 -luECC -lmongoc-1.0 -lbson-1.0 -DASIO_STANDALONE Log.cpp Clock.cpp Server.cpp asio_server.cpp Tcp.cpp Service.cpp TestProtocolCmd.cpp -o testServer -Wl,-rpath=dep/lib

