#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "stringf.h"
#include "msg.h"
#include <map>
#include <set>
#include <fstream>
#include <utility>
#include <iterator>

typedef std::string Hash_t;
typedef std::string Port_t;
typedef std::string IP_t;
typedef std::pair<Port_t, IP_t> Addr_t;
typedef std::map<Hash_t, std::set<Addr_t> > DataBase_t;

int NOT_END_OF_WORK = 1;
DataBase_t DataBase;
struct sockaddr_in servAddr;

void InitServerAddr(uint Port, sockaddr_in &servAddr);

void InitClientAddr(uint Port, unsigned IP, sockaddr_in &ClientAddr);

void LoadData(std::string &Path);

void SaveData(std::string &Path);

void *ClientsProcessing(void *argv);

int GetRandLR(int Left, int Right);

int GetRandN(int Number);


int main() {

	pthread_t ClientsProcessingThread;
	pthread_create(&ClientsProcessingThread, NULL, ClientsProcessing, NULL);
	std::string Command;
	while(std::getline(std::cin, Command)) {
		if (Command == "exit") {
			NOT_END_OF_WORK = 0;
			pthread_join(ClientsProcessingThread, NULL);
			std::cout << "Closing server\n";
			break;
		}
		else {
			std::cout << Command << "\n";
		}
	}
	

	return 0;
}

int GetRandN(int Number) {
	return rand() * rand() % Number;
}

int GetRandLR(int Left, int Right) {
	return Left + GetRandN(Right - Left);
}


void InitServerAddr(uint Port, sockaddr_in &servAddr) {
	memset(&servAddr, 0, sizeof(servAddr)); //set all bits to 0;
	servAddr.sin_family = AF_INET; //IPV4
	servAddr.sin_port = htons(Port); //example of port
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); //accept all users
}

void InitClientAddr(uint Port, unsigned IP, sockaddr_in &ClientAddr) {
	memset(&ClientAddr, 0, sizeof(ClientAddr)); //set all bits to 0;
	ClientAddr.sin_family = AF_INET; //IPV4
	ClientAddr.sin_port = htons(Port); //example of port
	ClientAddr.sin_addr.s_addr = htonl(IP); //accept all users	
}



void LoadData(std::string &Path) {
	std::ifstream fin(Path.c_str());
	int BlockNumbers;
	fin >> BlockNumbers;
	for (int i = 0; i < BlockNumbers; ++i) {
		int SetSize;
		Hash_t Hash;
		fin >> Hash;
		fin >> SetSize;
		for (int j = 0; j < SetSize; ++j) {
			Addr_t Addr;
			fin >> Addr.first >> Addr.second;
			DataBase[Hash].insert(Addr);
		}
	}
	fin.close();
}

void SaveData(std::string &Path) {
	std::ofstream fout(Path.c_str());
	int BlockNumbers = DataBase.size();
	fout << BlockNumbers << "\n";
	std::map<Hash_t, std::set<Addr_t> >::iterator it;
	for (it = DataBase.begin(); it != DataBase.end(); ++it) {
		fout << (*it).first << " ";
		int SetSize = (*it).second.size();
		fout << SetSize;
		std::set<Addr_t> &CurSet = (*it).second; 
		std::set<Addr_t>::iterator it1;
		for (it1 = CurSet.begin(); it1 != CurSet.end(); ++it1) {
			fout << " " << (*it1).first << " " << (*it1).second;
		}
		fout << "\n";
	}
	fout.close();
}

void *ClientsProcessing(void *argv) {
	InitServerAddr(1337, servAddr);

	int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //open socket
	
	bind(fd, (const sockaddr *) &servAddr, sizeof(servAddr));
	listen(fd, 10);
	std::string Command;
	while(NOT_END_OF_WORK) {
		int confd = accept(fd, 0, 0);

		if (confd > 0) {
			std::cout << "Processing client\n";
		}
		else if (confd < 0) {
			continue;
		}
		
		ReceiveStr(confd, Command);
		if (Command == "download") {
			int NumberOfBlocks = ReceiveInt(confd);
			for (int i = 0; i < NumberOfBlocks; ++i) {
				Hash_t Hash;
				ReceiveStr(confd, Hash);
				std::set<Addr_t> &AddrSet = DataBase[Hash];
				int SetSize = AddrSet.size();
				int UserNumber = GetRandN(SetSize);
				SendStr(confd, (AddrSet.begin() + UserNumber)->first);
				SendStr(confd, (AddrSet.begin() + UserNumber)->second);
			}

		} else if (Command == "upload") {

			int NumberOfBlocks = ReceiveInt(confd);
			IP_t IP;
			Port_t Port;
			ReceiveStr(confd, IP);
			ReceiveStr(confd, Port);
			for (int i = 0; i < NumberOfBlocks; ++i) {
				Hash_t Hash;
				ReceiveStr(confd, Hash);
				DataBase[Hash].insert(std::make_pair(IP, Port));
			}
		}	

		close(confd);
	}
}