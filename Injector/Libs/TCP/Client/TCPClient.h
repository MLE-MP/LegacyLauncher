#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>

typedef unsigned long long UINT_PTR; //simple workarounds for not including winsocket in header
typedef UINT_PTR SOCKET;

#define INVALID_SOCKET (SOCKET)(~0)

class TCPClient {
public:
    using MessageCallback = std::function<void(const std::string&)>;
    using ConnectCallback = std::function<void(bool)>;

    TCPClient();
    ~TCPClient();

    bool isRunning();
    bool isConnecting();
    bool isSocketValid();

    bool connectToServer(const std::string& host, int port);
    void sendMessage(const std::string& message);
    void setOnMessage(MessageCallback cb);
    void setOnConnect(ConnectCallback cb);
    void close();

private:
    void listenThread();

    SOCKET socket_ = INVALID_SOCKET;
    std::thread listener_;
    std::atomic<bool> running_ = false;
    std::atomic<bool> connecting_ = false;
    MessageCallback onMessage_;
};
