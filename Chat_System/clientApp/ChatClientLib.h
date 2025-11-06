#ifndef CHAT_CLIENT_LIB_H
#define CHAT_CLIENT_LIB_H

#include <string>
#include <vector>

namespace CHAT_SYSTEM{

// Observer Interface - The client application will implement this interface
class IChatClientObserver {
public:
    virtual ~IChatClientObserver() {}
    
    // Callback when a message is received
    virtual void onMessageReceived(const std::string& fromClientId, const std::string& message) = 0;
    
    // Callback when a result/acknowledgment is received from another client
    virtual void onResultReceived(const std::string& fromClientId, const std::string& result) = 0;
    
    // Callback when successfully connected
    virtual void onConnected() = 0;
    
    // Callback when disconnected
    virtual void onDisconnected() = 0;
    
    // Callback when the client list is received
    struct ClientInfo {
        std::string clientId;
        bool isActive;
    };
    virtual void onClientListUpdated(const std::vector<ClientInfo>& clients) = 0;
    
    // Callback when an error occurs
    virtual void onError(const std::string& errorMessage) = 0;
};

// Client Library Interface
class IChatClient {
public:
    virtual ~IChatClient() {}
    
    // Register an observer
    virtual void registerObserver(IChatClientObserver* observer) = 0;
    
    // Unregister an observer
    virtual void unregisterObserver(IChatClientObserver* observer) = 0;
    
    // Connect to the server
    virtual bool connect(const std::string& clientId, const std::string& serverIP, int serverPort) = 0;
    
    // Disconnect from the server
    virtual void disconnect() = 0;
    
    // Send a message to another client
    virtual bool sendMessage(const std::string& toClientId, const std::string& message) = 0;
    
    // Send a result/acknowledgment
    virtual bool sendResult(const std::string& toClientId, const std::string& result) = 0;
    
    // Check the connection status
    virtual bool isConnected() const = 0;
    
    // Get the client ID
    virtual std::string getClientId() const = 0;
};

// Factory method to create an instance of ChatClient
IChatClient* createChatClient();

}

#endif // CHAT_CLIENT_LIB_H
