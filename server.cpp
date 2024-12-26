#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENTS 10
#define PORT 23456

static std::vector<SOCKET> clients;
static std::mutex clientsMutex;

void broadcastMessage(const std::string &msg)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto &sock : clients)
    {
        send(sock, msg.c_str(), msg.size(), 0);
    }
}

void handleClient(SOCKET clientSock)
{
    char buf[512];
    while (true)
    {
        //接收客户端数据,阻塞式
        int bytesRecv = recv(clientSock, buf, sizeof(buf), 0);
        // 连接中断或出错
        if (bytesRecv <= 0) break;
        std::string msg(buf, bytesRecv);
        broadcastMessage(msg);
    }
    //移除断开连接的客户端
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        //使用erase-remove idiom移除断开连接的客户端
        clients.erase(std::remove(clients.begin(), clients.end(), clientSock), clients.end());
    }
    //关闭套接字
    closesocket(clientSock);
}

int main()
{
    //确定版本号，否则无法使用
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    //构建网络套接字，使用ipv4，流式套接字，tcp协议
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //判断是否创建成功
    if (listenSock == INVALID_SOCKET)
    {
        std::cerr << "socket() failed." << std::endl;
        WSACleanup();
        return -1;
    }
    //配置socket地址
    sockaddr_in serverAddr{};
    //地址簇
    serverAddr.sin_family = AF_INET;
    //端口号(使用htons函数将主机字节序转换为网络字节序,同理有ntohs将网络字节序转换为主机字节序)
    serverAddr.sin_port = htons(PORT);
    //ip地址接收任意地址
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    //绑定socket地址 + 异常处理
    if(bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "bind() failed." << std::endl;
        closesocket(listenSock);
        WSACleanup();
        return -1;
    }
    //监听,最大连接数为MAX_CLIENTS(10) + 异常处理
    if(listen(listenSock, MAX_CLIENTS) == SOCKET_ERROR)
    {
        std::cerr << "listen() failed." << std::endl;
        closesocket(listenSock);
        WSACleanup();
        return -1;
    }

    std::cout << "[Server] Listening on port " << PORT << ". . ." << std::endl;
    while(true)
    {
        //阻塞式接受连接
        SOCKET clientSock = accept(listenSock, nullptr, nullptr);
        //如果接受失败则退出
        if (clientSock == INVALID_SOCKET) break;
        //将客户端socket加入clients
        {
            //加锁，防止多线程操作clients
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientSock);
        }
        //创建线程处理客户端,datach()使线程在后台运行
        std::thread(handleClient, clientSock).detach();
    }

    closesocket(listenSock);
    WSACleanup();
    return 0;
}