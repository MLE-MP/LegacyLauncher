#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Libs/Window/WindowHelper.h"
#include <string>
#include "Libs/TCP/Client/TCPClient.h"
#include "Libs/TCP/PacketHandler.h"
//#include "Libs/TCP/Packet/Types/ServerboundLoginPacket.h"
#include "Libs/Config/Config.h"
//#include "Libs/TCP/Packet/Types/ClientboundObfuscationPacket.h"

#include "Libs/TCP/Packet/Types/ServerboundRegisterAccountPacket.h"

#include "Libs/TCP/Packet/Types/ClientboundAccountErrorPacket.h"
//#include "Libs/TCP/Client/ServerCommunication.h"


static WindowHelper* mainWindow = NULL;
static TCPClient* serverConnection = NULL;
static Config* configSystem = NULL;

void drawWindowContent();
void handlePacket(const BasePacket* packet);
//std::string GetMachineGuid(); //could be better but i dont give a shit just needs to be basic

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* pCmdLine, int nCmdShow) {
    configSystem = new Config("Injector");
    serverConnection = new TCPClient();

    mainWindow = new WindowHelper("Injector", "INJECTOR");
    mainWindow->onContentDraw = drawWindowContent;

    PacketHandler::onPacket(handlePacket);

    serverConnection->setOnMessage(PacketHandler::feed);
    serverConnection->connectToServer("38.49.215.81", 2053);

    //this will capture main thread and prevent code below from calling till program exit
    mainWindow->StartWindow(hInstance); 

    //do any cleanup here

    delete serverConnection;
    delete configSystem;
    delete mainWindow;
}

void handlePacket(const BasePacket* packet) {
    if (packet->getPacketType() == PacketTypes::RegisterAccount) {
        ClientboundAccountErrorPacket* castedPacket = (ClientboundAccountErrorPacket*)packet;
        errorHolder = castedPacket->getError();
    }
    /*if (packet->getPacketType() == PacketTypes::StartObfuscation) {
        ClientboundObfuscationPacket* castedPacket = (ClientboundObfuscationPacket*)(packet);
        BasePacket::readyObfuscation(castedPacket->getKey());
        
        
    }*/
}

std::string errorHolder = "";


void drawWindowContent() {
    if (serverConnection->isSocketValid() && serverConnection->isRunning()) {
        ImGui::Text("Socket Valid");

        if (!errorHolder.empty()) {
            ImGui::Text("%s", errorHolder.c_str());
        }

        ImGui::Text(PacketHandler::debug.c_str());

        static char username_inputBuffer[16];
        static char password_inputBuffer[64];

        ImGui::InputText("Username", username_inputBuffer, 16);
        ImGui::InputText("Password", password_inputBuffer, 64);

        if (ImGui::Button("Send Request")) {
            ServerboundRegisterAccountPacket* registerPacket = new ServerboundRegisterAccountPacket(username_inputBuffer, password_inputBuffer);

            serverConnection->sendMessage(registerPacket->getString());

            delete registerPacket;
        }

        /*if (BasePacket::isObfuscationReady()) {

        } else {
            static bool attemptedConfigLoad = false;

            if (!attemptedConfigLoad) {
                std::map<std::string, std::string> configFile = configSystem->ReadConfigFile();
                auto it = configFile.find("key");

                if (it != configFile.end()) {
                    if (!it->second.empty()) {
                        ServerboundLoginPacket* loginPacket = new ServerboundLoginPacket(it->second);

                        serverConnection->sendMessage(loginPacket->getString());

                        delete loginPacket;
                    }
                }

                attemptedConfigLoad = true;
            }

            static char inputBuffer[64];

            ImGui::InputText("Key", inputBuffer, 64);

            if (ImGui::Button("Send")) {
                std::string wrappedString(inputBuffer);

                ServerboundLoginPacket* loginPacket = new ServerboundLoginPacket(wrappedString);

                serverConnection->sendMessage(loginPacket->getString());

                delete loginPacket;
            }
        }*/
    } else {
        ImGui::Text("Attempting Connection To Legacy Online Services");

        if (!serverConnection->isConnecting()) {
            serverConnection->connectToServer("38.49.215.81", 2053);
        }
    }
}


/*std::string GetMachineGuid() {
    HKEY hKey;
    const char* keyPath = "SOFTWARE\\Microsoft\\Cryptography";
    const char* valueName = "MachineGuid";
    char buffer[256];
    DWORD bufferSize = sizeof(buffer);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
        return "";

    if (RegQueryValueExA(hKey, valueName, nullptr, nullptr, (LPBYTE)buffer, &bufferSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return "";
    }

    RegCloseKey(hKey);
    return std::string(buffer, bufferSize - 1); // strip null
}*/