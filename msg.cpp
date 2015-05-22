#include "msg.h"
#include <iostream>

int SendStr(int ConnectionFD, const std::string &Msg) {
	int Len = Msg.size();
	char *Buffer = new char[Len + 1];
	strcpy(Buffer, Msg.c_str());

	int Result = SendPcktMsg(ConnectionFD, Buffer);

	delete []Buffer;

	return Result;

}

int ReceiveStr(int ConnectionFD, std::string &Msg) {
	char *Buffer;
	ReceivePcktMsg(ConnectionFD, &Buffer);
	int Len = strlen(Buffer);
	Msg = std::string(Buffer);
	delete []Buffer;
	return Len;
}


int SendMsg(int ConnectionFD, const void *Msg, int Len, int Flags) {

	int total = 0;
	int was_sent;

	while (total < Len) {
		was_sent = send(ConnectionFD, (const char *) Msg + total, Len - total, Flags);
		if (was_sent == -1) {
			break;
		}
		total += was_sent;

	}

	return (was_sent == -1 ? -1 : total);

}

int SendPcktMsg(int ConnectionFD, const char *Msg, int Flags) {
	uint32_t Len = strlen(Msg);
	uint32_t *length = new uint32_t;
	*length = htonl(Len);
	int was_sent = SendMsg(ConnectionFD, (const void *) length, 4);
	was_sent = SendMsg(ConnectionFD, Msg, (int) Len);
	delete length;

	return was_sent;
}

int ReceiveMsg(int ConnectionFD, void *Buffer, int Len, int Flags) {

	int total = 0;
	int was_recvd;
	
	while (total < Len) {
		was_recvd = recv(ConnectionFD, ((char *) Buffer + total), Len - total, Flags);
		if (was_recvd == -1) {
			break;
		}
		total += was_recvd;
	}
	return (was_recvd == -1 ? -1 : total);

}

int ReceivePcktMsg(int ConnectionFD, char **Msg, int Flags) {

	uint32_t *length = new uint32_t;
	int WasRecvd = ReceiveMsg(ConnectionFD, (void *) length, 4, Flags);
	uint32_t Len = ntohl((uint32_t) *length);

	(*Msg) = new char[Len + 1];
	(*Msg)[Len] = '\0';
	WasRecvd = ReceiveMsg(ConnectionFD, *Msg, Len, Flags);

	delete length;

	return WasRecvd;

}

