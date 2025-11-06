#ifndef CHAT_CLIENT_LIB_H
#define CHAT_CLIENT_LIB_H

namespace CHAT_SYSTEM {

#define REGISTER "REGISTER"
#define REGISTERD "REGISTERD"
#define SEND_MSG "SEND_MSG"
#define RESULT  "RESULT"
#define DISCONNECT "DISCONNECT"
#define MESSAGE "MESSAGE"
#define RESULT_ACK "RESULT_ACK"
#define CLIENT_LIST "CLIENT_LIST"
#define GETLISTID "GETLISTID"

#define SERVER_DEFAULT 8080

#ifdef IP_DETAIL
#define IP_SERVER "1.1.1.1" //don't used, because user INADDR_ANY 
#endif

}


#endif