#define _WIN32_WINNT 0x0601 //这个定义表示win7及以上版本，否则编译不通过,而且必须放在最前面
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

#define PORT 23456
#define IP "127.0.0.1"

void receiveLoop(SOCKET sock)
{
    char buf[512];
    while (true)
    {
        int bytesRecv = recv(sock, buf, sizeof(buf), 0);
        if (bytesRecv <= 0) break;
        std::cout << "[Server] " << std::string(buf, bytesRecv) << std::endl;
    }
    std::cout << "[Info] Disconnected from server." << std::endl;
}

int main()
{
    system("chcp 65001");//统一程序的字符集为UTF-8
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "[Error] Cannot connect to server." << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    std::cout << "[Client] Connected to server." << std::endl;

    std::thread t(receiveLoop, sock);
    t.detach();

    // 发送消息
    std::string input;
    while (true)
    {
        std::getline(std::cin, input);
        if (input == "/quit") break;
        send(sock, input.c_str(), input.size(), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}