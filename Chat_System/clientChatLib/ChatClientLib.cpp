#include "ChatClientLib.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace CHAT_SYSTEM {

class ChatClient : public IChatClient {
private:
    int clientSocket;
    std::string clientId;
    std::string serverIP;
    int serverPort;
    bool connected;
    
    std::vector<IChatClientObserver*> observers;
    std::mutex observersMutex;
    std::mutex socketMutex;

    std::thread* receiveThread;
    bool shouldRun;
public:
    ChatClient() 
        : clientSocket(-1), serverPort(0), connected(false), 
          receiveThread(nullptr), shouldRun(false) {
    }
    
    ~ChatClient() {
        disconnect();
    }
    
    // Observer management
    void registerObserver(IChatClientObserver* observer) override {
        std::lock_guard<std::mutex> lock(observersMutex);
        observers.push_back(observer);
    }
    
    void unregisterObserver(IChatClientObserver* observer) override {
        std::lock_guard<std::mutex> lock(observersMutex);
        observers.erase(remove(observers.begin(), observers.end(), observer),observers.end());
    }
    
    // Connection management
    bool connect(const std::string& id, const std::string& ip, int port) override {
        if (connected) {
            notifyError("Already connected");
            return false;
        }
        
        clientId = id;
        serverIP = ip;
        serverPort = port;
        
        // Create socket
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            notifyError("Failed to create socket");
            return false;
        }
        
        // Setup server address
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        
        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
            notifyError("Invalid server address");
            close(clientSocket);
            clientSocket = -1;
            return false;
        }
        
        // Connect to server
        if (::connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            notifyError("Connection to server failed");
            close(clientSocket);
            clientSocket = -1;
            return false;
        }
        
        connected = true;
        
        // Send registration message
        std::string registerMsg = "REGISTER|" + clientId;
        if (!sendToServer(registerMsg)) {
            disconnect();
            return false;
        }
        
        // Start receive thread
        shouldRun = true;
        receiveThread = new std::thread(&ChatClient::receiveLoop, this);
        
        return true;
    }
    
    void disconnect() override {
        if (!connected) {
            return;
        }
        
        // Send disconnect message
        std::string disconnectMsg = "DISCONNECT|" + clientId;
        sendToServer(disconnectMsg);
        
        // Stop receive thread
        shouldRun = false;
        connected = false;
        
        if (receiveThread) {
            if (receiveThread->joinable()) {
                receiveThread->join();
            }
            delete receiveThread;
            receiveThread = nullptr;
        }
        
        // Close socket
        if (clientSocket != -1) {
            close(clientSocket);
            clientSocket = -1;
        }
        
        notifyDisconnected();
    }
    
    // Message sending
    bool sendMessage(const std::string& toClientId, const std::string& message) override {
        if (!connected) {
            notifyError("Not connected to server");
            return false;
        }
        
        std::string msg = "SEND_MSG|" + clientId + "|" + toClientId + "|" + message;
        return sendToServer(msg);
    }
    
    bool sendResult(const std::string& toClientId, const std::string& result) override {
        if (!connected) {
            notifyError("Not connected to server");
            return false;
        }
        
        std::string msg = "RESULT|" + clientId + "|" + toClientId + "|" + result;
        return sendToServer(msg);
    }
    
    // Status
    bool isConnected() const override {
        return connected;
    }
    
    std::string getClientId() const override {
        return clientId;
    }

private:
    bool sendToServer(const std::string& message) {
        std::lock_guard<std::mutex> lock(socketMutex);
        
        if (clientSocket < 0 || !connected) {
            return false;
        }
        
        ssize_t sent = send(clientSocket, message.c_str(), message.length(), 0);
        return sent > 0;
    }
    
    void receiveLoop() {
        char buffer[4096];
        
        while (shouldRun && connected) {
            memset(buffer, 0, sizeof(buffer));
            
            ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesRead <= 0) {
                if (shouldRun) {
                    connected = false;
                    notifyDisconnected();
                }
                break;
            }
            
            std::string message(buffer);
            processServerMessage(message);
        }
    }
    
    void processServerMessage(const std::string& msg) {
        size_t pos = msg.find('|');
        if (pos == std::string::npos) return;
        
        std::string command = msg.substr(0, pos);
        std::string data = msg.substr(pos + 1);
        
        
        if (command == "REGISTERD") { 
            notifyConnected();
        }
        else if (command == "MESSAGE") {
            // MESSAGE|fromId|messageText
            size_t pos2 = data.find('|');
            if (pos2 != std::string::npos) {
                std::string fromId = data.substr(0, pos2);
                std::string messageText = data.substr(pos2 + 1);
                
                notifyMessageReceived(fromId, messageText);
                
                // Auto send OK result
                sendResult(fromId, "OK");
            }
        }
        else if (command == "RESULT_ACK") { 
            // RESULT_ACK|fromId|status
            size_t pos2 = data.find('|');
            if (pos2 != std::string::npos) {
                std::string fromId = data.substr(0, pos2);
                std::string status = data.substr(pos2 + 1);
                
                notifyResultReceived(fromId, status);
            }
        }
        else if (command == "CLIENT_LIST") {
            parseAndNotifyClientList(data);
        }
        else if (command == "ERROR") {
            notifyError(data);
        }
    }
    
    void parseAndNotifyClientList(const std::string& data) {
        std::vector<IChatClientObserver::ClientInfo> clients;
        
        size_t start = 0;
        size_t pos;
        
        while ((pos = data.find('|', start)) != std::string::npos) {
            std::string clientInfo = data.substr(start, pos - start);
            parseClientInfo(clientInfo, clients);
            start = pos + 1;
        }
        
        // Last item
        if (start < data.length()) {
            std::string clientInfo = data.substr(start);
            parseClientInfo(clientInfo, clients);
        }
    
        notifyClientListUpdated(clients);
    }
    
    void parseClientInfo(const std::string& info, std::vector<IChatClientObserver::ClientInfo>& clients) {
        size_t colonPos = info.find(':');
        if (colonPos != std::string::npos) {
            IChatClientObserver::ClientInfo client;
            client.clientId = info.substr(0, colonPos);
            client.isActive = (info.substr(colonPos + 1) == "ACTIVE");
            clients.push_back(client);
        }
    }
    
    // Observer notifications
    void notifyMessageReceived(const std::string& fromClientId, const std::string& message) {
        std::lock_guard<std::mutex> lock(observersMutex);
        for (auto observer : observers) {
            observer->onMessageReceived(fromClientId, message);
        }
    }
    
    void notifyResultReceived(const std::string& fromClientId, const std::string& result) {
        std::lock_guard<std::mutex> lock(observersMutex);
        for (auto observer : observers) {
            observer->onResultReceived(fromClientId, result);
        }
    }
    
    void notifyConnected() {
        std::lock_guard<std::mutex> lock(observersMutex);
        for (auto observer : observers) {
            observer->onConnected();
        }
    }
    
    void notifyDisconnected() {
        std::lock_guard<std::mutex> lock(observersMutex);
        for (auto observer : observers) {
            observer->onDisconnected();
        }
    }
    
    void notifyClientListUpdated(const std::vector<IChatClientObserver::ClientInfo>& clients) {
        std::lock_guard<std::mutex> lock(observersMutex);
        for (auto observer : observers) {
            observer->onClientListUpdated(clients);
        }
    }
    
    void notifyError(const std::string& errorMessage) {
        std::lock_guard<std::mutex> lock(observersMutex);
        for (auto observer : observers) {
            observer->onError(errorMessage);
        }
    }
};

// Factory method implementation
IChatClient* createChatClient() {
    return new ChatClient();
}

}