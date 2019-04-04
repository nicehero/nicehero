#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <stdio.h>
#include <iostream>
#include "a.h"

using namespace std;
using namespace asio;
int main()
{
	asio::io_service ios;
    //创建resolver对象
    ip::tcp::resolver slv(ios);
    //创建query对象
    ip::tcp::resolver::query qry("www.baidu.com","80");
    //使用resolve迭代端点
    ip::tcp::resolver::iterator it = slv.resolve(qry);
    ip::tcp::resolver::iterator end;
    vector<string> ip;
    for (; it != end; it++)
    {
        ip.push_back((*it).endpoint().address().to_string());
    }
    
    for (int i = 0; i < ip.size(); i++)
    {
        std::cout << ip[i] << endl;
    }
    getchar();
    return 0;
}

