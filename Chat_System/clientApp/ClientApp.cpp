#include "ChatClientLib.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace std;
using namespace CHAT_SYSTEM;
// Client Application - Implement IChatClientObserver interface
class MyClientApp : public IChatClientObserver {
private:
    IChatClient* chatClient;
    string myClientId;
    vector<ClientInfo> m_clients;
    string m_currentUser = "";
public:
    MyClientApp(string& currentUser) : chatClient(nullptr) {
        m_currentUser =currentUser;
        // Tạo instance của client library
        chatClient = createChatClient();
        
        // Đăng ký observer
        if (chatClient) {
            chatClient->registerObserver(this);
        }
    }
    
    ~MyClientApp() {
        if (chatClient) {
            chatClient->unregisterObserver(this);
            delete chatClient;
        }
    }
    
    // Override các callback từ IChatClientObserver
    void onMessageReceived(const string& fromClientId, const string& message) override {
        cout << "\n[MESSAGE] From: " << fromClientId << endl;
        cout << "Content: " << message << endl;
        cout << "(Auto-sent OK result)" << endl;
        cout << "\nEnter command: " << flush;
    }
    
    void onResultReceived(const string& fromClientId, const string& result) override {
        cout << "\n[RESULT] From: " << fromClientId << " - Status: " << result << endl;
        cout << "\nEnter command: " << flush;
    }
    
    void onConnected() override {
        cout << "\nSuccessfully connected to server!" << endl;
        cout << "Client ID: " << chatClient->getClientId() << endl;
        cout << "\nEnter command: " << flush;
    }
    
    void onDisconnected() override {
        cout << "\n⚠ Disconnected from server" << endl;
    }
    
    void onClientListUpdated(const vector<ClientInfo>& clients) override {
        cout << "\n[CLIENT LIST UPDATE]" << endl;
        m_clients = clients;
        showListUserStatus(clients);
        cout << "\nEnter command: " << flush;
    }
    
    void onError(const string& errorMessage) override {
        cout << "\n[ERROR] " << errorMessage << endl;
        cout << "\nEnter command: " << flush;
    }
    
    // Application methods
    bool connect(const string& clientId, const string& serverIP, int serverPort) {
        myClientId = clientId;
        
        cout << "Connecting to server..." << endl;
        cout << "Client ID: " << clientId << endl;
        cout << "Server: " << serverIP << ":" << serverPort << endl;
        
        return chatClient->connect(clientId, serverIP, serverPort);
    }
    
    void sendMessage(const string& toClientId, const string& message) {
        if (chatClient->sendMessage(toClientId, message)) {
            cout << "Message sent to " << toClientId << endl;
        } else {
            cout << "Failed to send message" << endl;
        }
    }

    void showListUserStatus(const vector<ClientInfo>& clients){
        for (const auto& client : clients) {
            if (client.clientId == m_currentUser)
                continue;

            string status = client.isActive ? "ACTIVE" : "INACTIVE";
            cout << "   " << status << " - " << client.clientId << endl;
        }
    }
    
    void disconnect() {
        chatClient->disconnect();
    }
    
    bool isConnected() const {
        return chatClient->isConnected();
    }
    
    void run() {
        //printHelp();
        string command;
        while (isConnected()) {
            printHelp();
            showListUserStatus(m_clients);
            cout << "\nEnter command: ";
            getline(cin, command);
            
            if (command.empty()) {
                continue;
            }
            
            processCommand(command);
        }
    }
    
private:
    void printHelp() {
        cout << "\n╔════════════════════════════════════════╗" << endl;
        cout << "║         Chat Client Commands           ║" << endl;
        cout << "╠════════════════════════════════════════╣" << endl;
        cout << "║ send <id> <msg>  - Send message        ║" << endl;
        cout << "║ help             - Show this help      ║" << endl;
        cout << "║ quit             - Disconnect & exit   ║" << endl;
        cout << "╚════════════════════════════════════════╝" << endl;
    }
    
    void processCommand(const string& command) {
        if (command == "quit") {
            cout << "Disconnecting..." << endl;
            disconnect();
        }
        else if (command == "help") {
            printHelp();
        }
        else if (command.substr(0, 4) == "send") {
            handleSendCommand(command);
        }
        else {
            cout << "Unknown command. Type 'help' for available commands." << endl;
        }
    }
    
    void handleSendCommand(const string& command) {
        // Parse: send <clientId> <message>
        size_t firstSpace = command.find(' ', 5);
        
        if (firstSpace == string::npos) {
            cout << "Usage: send <clientId> <message>" << endl;
            return;
        }
        
        string toClientId = command.substr(5, firstSpace - 5);
        string message = command.substr(firstSpace + 1);
        
        if (toClientId.empty() || message.empty()) {
            cout << "Usage: send <clientId> <message>" << endl;
            return;
        }
        
        sendMessage(toClientId, message);
    }


};

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Usage: " << argv[0] << " <clientId> <serverIP> <serverPort>" << endl;
        cout << "Example: " << argv[0] << " ClientA 127.0.0.1 8080" << endl;
        return 1;
    }
    
    string clientId = argv[1];
    string serverIP = argv[2];
    int serverPort = atoi(argv[3]);
    
    
    MyClientApp app(clientId);
    
    if (!app.connect(clientId, serverIP, serverPort)) {
        cerr << "Failed to connect to server" << endl;
        return 1;
    }

    this_thread::sleep_for(chrono::milliseconds(500));
    app.run();
    
    cout << "Application terminated" << endl;
    return 0;
}
