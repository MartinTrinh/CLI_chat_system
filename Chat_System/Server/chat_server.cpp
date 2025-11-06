#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "common.h"



using namespace std;
using namespace CHAT_SYSTEM;

// Structure to store client information
struct ClientInfo {
    string clientId;
    string ipAddress;
    int port;
    int socket;
    bool isActive;
};

class ChatServer {
private:
    int serverSocket;
    int port;
    map<string, ClientInfo> clients; // Key: clientId
    mutex clientsMutex;
    
public:
    ChatServer(int p) : port(p), serverSocket(-1) {}
    
    ~ChatServer() {
        if (serverSocket != -1) {
            close(serverSocket);
        }
    }
    
    bool start() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            cerr << "Failed to create socket" << endl;
            return false;
        }
        
        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);
        
        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "Bind failed" << endl;
            return false;
        }
        
        if (listen(serverSocket, 10) < 0) {
            cerr << "Listen failed" << endl;
            return false;
        }
        
        cout << "Server started on port " << port << endl;
        return true;
    }
    
    void acceptConnections() {
        while (true) {
            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
            
            if (clientSocket < 0) {
                cerr << "Accept failed" << endl;
                continue;
            }
            
            thread(&ChatServer::handleClient, this, clientSocket, clientAddr).detach();
        }
    }
    
    void handleClient(int clientSocket, sockaddr_in clientAddr) {
        char buffer[4096];
        string clientId;
        
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesRead <= 0) {
                // Client disconnected
                if (!clientId.empty()) {
                    setClientInactive(clientId);
                }
                close(clientSocket);
                break;
            }
            
            string message(buffer);
            processMessage(clientSocket, clientAddr, message, clientId);
        }
    }
    
    void processMessage(int socket, sockaddr_in addr, const string& msg, string& clientId) {
        // Message format: COMMAND|DATA
        size_t pos = msg.find('|');
        if (pos == string::npos) return;
        
        string command = msg.substr(0, pos);
        string data = msg.substr(pos + 1);
        
        if (command == REGISTER) {
            // REGISTER|clientId
            clientId = data;
            registerClient(clientId, addr, socket);
        }
        else if (command == SEND_MSG) {
            // SEND_MSG|fromId|toId|message
            handleSendMessage(data);
        }
        else if (command == RESULT) {
            // RESULT|fromId|toId|OK/NOT_OK
            handleResult(data);
        }
        else if (command == DISCONNECT) {
            // DISCONNECT|clientId
            setClientInactive(data);
        }
    }
    
    void registerClient(const string& clientId, sockaddr_in addr, int socket) {
        lock_guard<mutex> lock(clientsMutex);
        
        ClientInfo info;
        info.clientId = clientId;
        info.ipAddress = inet_ntoa(addr.sin_addr);
        info.port = ntohs(addr.sin_port);
        info.socket = socket;
        info.isActive = true;
        
        clients[clientId] = info;
        
        cout << "Client registered: " << clientId << " (" << info.ipAddress << ":" << info.port << ")" << endl;
        
        // Send a response to the client that just registered
        string response = "REGISTERED|" + clientId;
        send(socket, response.c_str(), response.length(), 0);
        
        // wait client establish completed
        this_thread::sleep_for(chrono::milliseconds(500));
        // Notify all other clients about the new client
        broadcastClientList();
    }
    
    void handleSendMessage(const string& data) {
        // Parse: fromId|toId|message
        size_t pos1 = data.find('|');
        size_t pos2 = data.find('|', pos1 + 1);
        
        if (pos1 == string::npos || pos2 == string::npos) return;
        
        string fromId = data.substr(0, pos1);
        string toId = data.substr(pos1 + 1, pos2 - pos1 - 1);
        string message = data.substr(pos2 + 1);
        
        lock_guard<mutex> lock(clientsMutex);
        
        if (clients.find(toId) != clients.end() && clients[toId].isActive) {
            // Forward message to target client
            string forwardMsg = "MESSAGE|" + fromId + "|" + message;
            send(clients[toId].socket, forwardMsg.c_str(), forwardMsg.length(), 0);
            
            cout << "Message forwarded from " << fromId << " to " << toId << endl;
        } else {
            // Notify sender that recipient is not available
            if (clients.find(fromId) != clients.end()) {
                string errorMsg = "ERROR|Client " + toId + " is not active";
                send(clients[fromId].socket, errorMsg.c_str(), errorMsg.length(), 0);
            }
        }
    }
    
    void handleResult(const string& data) {
        // Parse: fromId|toId|status
        size_t pos1 = data.find('|');
        size_t pos2 = data.find('|', pos1 + 1);
        
        if (pos1 == string::npos || pos2 == string::npos) return;
        
        string fromId = data.substr(0, pos1);
        string toId = data.substr(pos1 + 1, pos2 - pos1 - 1);
        string status = data.substr(pos2 + 1);
        
        lock_guard<mutex> lock(clientsMutex);
        
        if (clients.find(toId) != clients.end() && clients[toId].isActive) {
            string resultMsg = "RESULT_ACK|" + fromId + "|" + status;
            send(clients[toId].socket, resultMsg.c_str(), resultMsg.length(), 0);
            
            cout << "Result sent from " << fromId << " to " << toId << ": " << status << endl;
        }
    }
    
    void setClientInactive(const string& clientId) {
        lock_guard<mutex> lock(clientsMutex);
        
        if (clients.find(clientId) != clients.end()) {
            clients[clientId].isActive = false;
            cout << "Client " << clientId << " set to inactive" << endl;
            
            // Notify all other clients
            broadcastClientList();
        }
    }
    
    void broadcastClientList() {
        string clientList = CLIENT_LIST;
        
        for (const auto& pair : clients) {
            clientList += "|" + pair.second.clientId + ":" + 
                         (pair.second.isActive ? "ACTIVE" : "INACTIVE");
        }
        
        cout << "Broadcasting client list to all active clients" << endl;
        
        for (const auto& pair : clients) {
            if (pair.second.isActive) {
                send(pair.second.socket, clientList.c_str(), clientList.length(), 0);
            }
        }
    }
};

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    ChatServer server(port);
    
    if (!server.start()) {
        return 1;
    }
    
    server.acceptConnections();
    
    return 0;
}