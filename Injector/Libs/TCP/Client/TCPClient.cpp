#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "TCPClient.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

TCPClient::TCPClient() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
}

TCPClient::~TCPClient() {
    close();
    WSACleanup();
}

bool TCPClient::isRunning() {
    return running_;
}

bool TCPClient::isConnecting() {
    return connecting_;
}

bool TCPClient::isSocketValid() {
    return socket_ != INVALID_SOCKET;
}

bool TCPClient::connectToServer(const std::string& host, int port) {
    if (isRunning() || isConnecting()) return false;

    connecting_ = true;

    std::thread([this, host, port]() {
        SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == INVALID_SOCKET) {
            connecting_ = false;
            return;
        }

        sockaddr_in server{};
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = inet_addr(host.c_str());

        if (connect(s, (sockaddr*)&server, sizeof(server)) < 0) {
            closesocket(s);
            connecting_ = false;
            return;
        }

        socket_ = s;
        running_ = true;
        listener_ = std::thread(&TCPClient::listenThread, this);
        connecting_ = false;
        }).detach();

    return true;
}

void TCPClient::listenThread() {
    char buffer[1024];
    while (running_) {
        int bytes = recv(socket_, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            if (onMessage_)
                onMessage_(std::string(buffer));
        }
        else if (bytes == 0 || WSAGetLastError() == WSAECONNRESET) {
            close();
        }
    }
}

void TCPClient::sendMessage(const std::string& message) {
    if (socket_ != INVALID_SOCKET) {
        send(socket_, message.c_str(), (int)message.size(), 0);
    }
}

void TCPClient::setOnMessage(MessageCallback cb) {
    onMessage_ = cb;
}

void TCPClient::close() {
    running_ = false;
    connecting_ = false;

    if (std::this_thread::get_id() != listener_.get_id()) {
        if (listener_.joinable()) listener_.join();
    }

    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}
