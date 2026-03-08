#include "ServerCommunication.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

std::string ServerCommunication::passkey;
std::string ServerCommunication::error;
bool ServerCommunication::hasAuthenticated = false;
bool ServerCommunication::isSteam = false;


void ServerCommunication::SetPasskey(const std::string& key) {
    ServerCommunication::passkey = key;
}

const std::string& ServerCommunication::GetPasskey() {
    return ServerCommunication::passkey;
}

const std::string& ServerCommunication::GetLatestError() {
    return ServerCommunication::error;
}

void ServerCommunication::ResetError() {
    ServerCommunication::error = "";
}

bool ServerCommunication::IsAuthenticated() {
    return ServerCommunication::hasAuthenticated;
}

bool ServerCommunication::IsSteam() {
    return ServerCommunication::isSteam;
}

RegistrationResponse ServerCommunication::AttemptRegistration(const std::string& username, const std::string& password) {

    RegistrationResponse response = RegistrationResponse(ServerCommunication::MakeAPIRequest(APIType::RegisterAccount, (username + "|" + password)));

    return response;
    /*if (passkey.size() < 4) { //codes have STAR prefix so this prevents someone from just spamming enter on random codes
        ServerCommunication::error = "INVALID CODE";
    }
    else {
        const IL2CPPInternal::MethodInfo* methodInfo = IL2CPP::Wrapped::FindMethod("UnityEngine.CoreModule.dll", "UnityEngine", "SystemInfo", "get_deviceUniqueIdentifier", -1);

        auto* DeviceUID = (IL2CPPInternal::String*)IL2CPP::il2cpp_runtime_invoke(const_cast<IL2CPPInternal::MethodInfo*>(methodInfo), nullptr, nullptr, nullptr);

        LoginResponse response = LoginResponse(ServerCommunication::MakeAPIRequest(APIType::Login, (passkey + '|' + DeviceUID->ToString()))); //temp, change this with some better info once found

        if (response.isError) {
            ServerCommunication::error = response.debugInfo;
        }
        else if (!response.isValid) {
            ServerCommunication::error = "INVALID RESPONSE";
        }

        hasAuthenticated = (!response.isError || !response.isValid);

        if (hasAuthenticated) {
            isSteam = response.UserID.length() == 17;
        }
        return response;
    }*/

}

APIResponse ServerCommunication::MakeAPIRequest(APIType type, const std::string& serializedData) {

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return APIResponse(400, "Socket creation failed");
    }

    DWORD timeout = 30000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(2053);

    if (inet_pton(AF_INET, "38.49.215.81", &serverAddr.sin_addr) <= 0) {
        closesocket(sock);
        WSACleanup();
        return APIResponse(400, "Invalid IP address");
    }

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return APIResponse(400, "Connection failed");
    }

    size_t dataLength = serializedData.size();

    std::string finalData(5, 0);
    finalData += serializedData;

    finalData[0] = dataLength & 0xFF;
    finalData[1] = (dataLength >> 8) & 0xFF;
    finalData[2] = (dataLength >> 16) & 0xFF;
    finalData[3] = (dataLength >> 24) & 0xFF;
    finalData[4] = (uint8_t)type;

    size_t totalSent = 0;

    while (totalSent < finalData.size()) {

        int sent = send(sock,
            finalData.data() + totalSent,
            (int)(finalData.size() - totalSent),
            0);

        if (sent == SOCKET_ERROR) {
            closesocket(sock);
            WSACleanup();
            return APIResponse(400, "Send failed");
        }

        totalSent += sent;
    }

    uint8_t lengthBytes[4];
    size_t totalReceived = 0;

    while (totalReceived < 4) {

        int rec = recv(sock,
            (char*)lengthBytes + totalReceived,
            4 - totalReceived,
            0);

        if (rec <= 0) {
            closesocket(sock);
            WSACleanup();
            return APIResponse(400, "Receive failed during length read");
        }

        totalReceived += rec;
    }

    uint32_t packetLength =
        lengthBytes[0] |
        (lengthBytes[1] << 8) |
        (lengthBytes[2] << 16) |
        (lengthBytes[3] << 24);

    std::vector<char> buffer(packetLength + 1);

    totalReceived = 0;

    while (totalReceived < packetLength) {

        int rec = recv(sock,
            buffer.data() + totalReceived,
            packetLength - totalReceived,
            0);

        if (rec <= 0) {
            closesocket(sock);
            WSACleanup();
            return APIResponse(400, "Receive failed during data read");
        }

        totalReceived += rec;
    }

    buffer[packetLength] = '\0';

    closesocket(sock);
    WSACleanup();

    return APIResponse(200, buffer.data());
}