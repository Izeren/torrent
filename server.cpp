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
#include <fstream>
#include <utility>
#include <iterator>
#include "types.h"

int NOT_END_OF_WORK = 1;
DataBase_t DataBase;
struct sockaddr_in servAddr;

void InitServerAddr(uint Port, sockaddr_in &servAddr);

void InitClientAddr(uint Port, unsigned IP, sockaddr_in &ClientAddr);

void LoadData(std::string Path);

void SaveData(std::string Path);

void *ClientsProcessing(void *argv);

int GetRandLR(int Left, int Right);

int GetRandN(int Number);


int main() {
	LoadData(std::string("ServerDataBase.txt"));

	pthread_t ClientsProcessingThread;
	pthread_create(&ClientsProcessingThread, NULL, ClientsProcessing, NULL);
	std::string Command;
	while(std::getline(std::cin, Command)) {
		if (Command == "exit") {
			NOT_END_OF_WORK = 0;
			//pthread_join(ClientsProcessingThread, NULL);
			std::cout << "Closing server\n";
			break;
		}
		else {
			std::cout << Command << "\n";
		}
	}
	SaveData(std::string("ServerDataBase.txt"));
	

	return 0;
}

int GetRandN(int Number) {
	return rand() % Number;
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
	memset(&ClientAddr, 0, sizeof(ClientAddr));
	ClientAddr.sin_family = AF_INET;
	ClientAddr.sin_port = htons(Port);
	ClientAddr.sin_addr.s_addr = htonl(IP);
}



void LoadData(std::string Path) {
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
			DataBase[Hash].push_back(Addr);
		}
	}
	fin.close();
}

void SaveData(std::string Path) {
	std::ofstream fout(Path.c_str());
	int BlockNumbers = DataBase.size();
	fout << BlockNumbers << "\n";
	DataBase_t::iterator it;
	for (it = DataBase.begin(); it != DataBase.end(); ++it) {
		fout << (*it).first << " ";
		int SetSize = (*it).second.size();
		fout << SetSize;
		std::vector<Addr_t> &CurVector = (*it).second; 
		std::vector<Addr_t>::iterator it1;
		for (it1 = CurVector.begin(); it1 != CurVector.end(); ++it1) {
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
	int counter = 1;
	while(NOT_END_OF_WORK) {
		int confd = accept(fd, 0, 0);

		if (confd > 0) {
			std::cout << "Connection " << counter << " opened\n";
		}
		else if (confd < 0) {
			continue;
		}
		
		ReceiveStr(confd, Command);
		if (Command == "download") {
			std::cout << "Command recognized: download\n";
			int NumberOfBlocks = ReceiveInt(confd);
			//std::cout << NumberOfBlocks << std::endl;
			for (int i = 0; i < NumberOfBlocks; ++i) {
				Hash_t Hash;
				ReceiveStr(confd, Hash);
				//std::cout << Hash << std::endl;
				std::vector<Addr_t> &AddrVector = DataBase[Hash];
				int VectorSize = AddrVector.size();
				//std::cout << "vectorsize: " << VectorSize << std::endl;
				if (VectorSize == 0) {
					std::cout << "Wrong Hash\n";
					break;
				}
				int UserNumber = GetRandN(VectorSize);
				//std::cout << "UserNumber: " << UserNumber << std::endl;
				SendStr(confd, AddrVector[UserNumber].first);
				SendStr(confd, AddrVector[UserNumber].second);
				//std::cout << "AfterSend\n";
			}

		} else if (Command == "upload") {
			std::cout << "Command recognized: upload\n";
			IP_t IP;
			Port_t Port;
			ReceiveStr(confd, IP);
			ReceiveStr(confd, Port);
			int NumberOfBlocks = ReceiveInt(confd);

			//std::cout << "IP: " << IP << " Port: " << Port << "\n";
			//std::cout << "NumberOfBlocks: " << NumberOfBlocks << "\n";
			
			for (int i = 0; i < NumberOfBlocks; ++i) {
				Hash_t Hash;
				ReceiveStr(confd, Hash);
				std::cout << "Hash received: " << Hash << "\n";
				std::vector<Addr_t> &Addrs = DataBase[Hash];
				std::pair<Port_t, IP_t> Addr = std::make_pair(Port, IP);
				if (std::find(Addrs.begin(), Addrs.end(), Addr) == Addrs.end()) {
					Addrs.push_back(Addr);
				}
			}
		}	

		close(confd);
		std::cout << "Connection " << counter << " closed\n";
		++counter;

	}
}