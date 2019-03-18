g++ -g -Wall -std=c++11  -I./dep/include -I./dep/include/asio -L./dep/lib -lpthread -lsha3 -luECC -DASIO_STANDALONE Log.cpp Clock.cpp Server.cpp asio_server.cpp Tcp.cpp Service.cpp -o testServer -Wl,-rpath=dep/lib

