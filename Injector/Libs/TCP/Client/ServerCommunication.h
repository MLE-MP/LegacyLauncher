#pragma once

#include <string>
#include <vector>

class APIResponse {
public:
    int code;
    const char* rawMessage;

    explicit APIResponse() : code(400), rawMessage() {}
    explicit APIResponse(int _code) : code(_code), rawMessage() {}
    explicit APIResponse(int _code, const char* _message) : code(_code), rawMessage(_message) {}

    static std::vector<std::string> splitFast(const std::string& str, char delimiter);
};
/*
class LoginResponse {
public:
    bool isValid;
    bool isError;
    std::string debugInfo;

    std::string EntityToken;
    std::string MothershipID;
    std::string MothershipToken;
    std::string Nonce;
    std::string SessionTicket;
    std::string UserID;
    std::string PlayFabID;
    std::string EntityID;

    explicit LoginResponse(APIResponse _base) : isValid((_base.code == 200) && (strlen(_base.rawMessage) > 0)) {
        if (!isValid) {
            debugInfo = _base.rawMessage;
            return;
        }

        isError = false; //make sure value is reset, weird issues could happen if we dont verify it
        std::vector<std::string> splitMessage = APIResponse::splitFast(_base.rawMessage, '|');

        if (splitMessage.size() == 2) { //error
            isError = true;
            debugInfo = splitMessage[1];
            return;
        }
        else if (splitMessage.size() != 6) { //checks that we have the right data
            isValid = false;
            //LOG(ANDROID_LOG_ERROR, "Invalid Split Message Length");
            debugInfo = _base.rawMessage;
            return;
        }

        EntityToken = splitMessage[0];
        MothershipID = splitMessage[1];
        MothershipToken = splitMessage[2];
        Nonce = splitMessage[3];
        SessionTicket = splitMessage[4];
        UserID = splitMessage[5];//if userid is 17 in length then its steamid64 (i think)

        std::vector<std::string> splitTicket = APIResponse::splitFast(SessionTicket, '-');

        PlayFabID = splitTicket[0];
        EntityID = splitTicket[2];
    }
};
*/
/*
class NewNonceResponse {
public:
    bool isValid;
    std::string debugInfo;

    std::string Nonce;

    explicit NewNonceResponse(std::string _cachedNonce) : isValid(true), Nonce(_cachedNonce) {}

    explicit NewNonceResponse(APIResponse _base) : isValid((_base.code == 200) && (strlen(_base.rawMessage) > 0)) {
        if (!isValid) {
            debugInfo = _base.rawMessage;
            return;
        }

        Nonce = _base.rawMessage;
    }
};
*/

class RegistrationResponse {
private:
    bool isValid; //todo: abstract this and the valid checks
    std::string error; //todo: abstract this and the valid checks

    int statusCode;
    std::string token;
public:

    explicit RegistrationResponse(APIResponse _base) : isValid((_base.code == 200) && (strlen(_base.rawMessage) > 0)) {
        if (isValid && (std::string(_base.rawMessage).length() > 3)) {
            token = _base.rawMessage;
            return;
        }

        if (isValid) {
            statusCode = std::stoi(_base.rawMessage);
        } else {
            statusCode = _base.code;
            error = _base.rawMessage;
        }
    }

    bool DidError() {
        return this->statusCode > 0;
    }

    std::string GetError() {
        switch (this->statusCode) {
        case 1:
            return "This Username Has Been Blacklisted";
        default:
            return ("Unknown Registration Error (" + std::to_string(this->statusCode) + ") " + this->error);
        }
    }
};

enum APIType : uint8_t {
    RegisterAccount = 1,
};

class ServerCommunication {
public:
    //static LoginResponse GetLoginDetails();
    //static NewNonceResponse GetNewNonce();

    static RegistrationResponse AttemptRegistration(const std::string& username, const std::string& password);

    static void SetPasskey(const std::string& key);
    static const std::string& GetPasskey();

    static const std::string& GetLatestError();
    static void ResetError();

    static bool IsAuthenticated();
    static bool IsSteam();

private:
    static APIResponse MakeAPIRequest(APIType type, const std::string& serializedData);

    static std::string passkey;
    static std::string error;
    static bool hasAuthenticated;
    static bool isSteam;
};