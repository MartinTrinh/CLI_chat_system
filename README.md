Messaging Application Simulation Software (CLI Chat System)
Features:
- Server manages multiple users within the network
- Continuously updates users’ ACTIVE/INACTIVE status
- When user A sends a message to user B, a response is returned to A indicating whether the message was successfully delivered
  
Technical Details:
- Language: C++
- Library: Shared library (implemented as a client library)
- Multi-threading + Synchronization
- IPC: TCP/IP socket
- Design patter: Observer, Factory method
