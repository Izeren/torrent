#ifndef msg
#define msg

#include <string>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

int SendStr(int ConnectionFD, const std::string &Msg);

int ReceiveStr(int ConnectionFD, std::string &Msg);

int SendMsg(int ConnectionFD, const std::string &Msg, int Flags = 0);

int SendMsg(int ConnectionFD, const void *Msg, int Len, int Flags = 0);

int SendPcktMsg(int ConnectionFD, const char *Msg, int Flags = 0);

int ReceiveMsg(int ConnectionFD, void *Buffer, int Len, int Flags = 0);

int ReceivePcktMsg(int ConnectionFD, char **Msg, int Flags = 0);

int ReceiveInt(int FD);

void SendInt(int FD, int Number);

long ReceiveLong(int FD);

void SendLong(int FD, long Number);

#endif //msg