#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "stringf.h"
#include <vector>
#include <pthread.h>
#include "msg.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include <map>
#include <fstream>
#include <utility>
#include <fstream>
#include <iomanip>
#include <sstream>

const int BufferSize = 100;
const int BlockSize = 4 * 1024;
const int NumberCommands = 4;
//Commands: 1 - download file;
//			2 - upload file;
//			3 - create torrent;
//			0 - exit

std::string ClientIP;
std::string ClientPort = "1338";

std::string Command;
std::string File;
int FileSize;
list UserCommands, Commands;
bool ClientIsFree = true, Error = 0;
bool STOP_LISTENING = false;
int IdCommand = NumberCommands;
int FD;//FileDescriptor
struct sockaddr_in ServAddr;
struct sockaddr_in ClientAddr;
unsigned char ResultingHash[MD5_DIGEST_LENGTH];

typedef std::string Hash_t;////////////////////////////потом убрать
typedef std::map<Hash_t, std::pair<int, std::string> > ClientDataBase_t;


ClientDataBase_t DataBase;

void printMD5(unsigned char *Buffer) {
	for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
		std::cout << std::hex << (int) Buffer[i];
	}
}

void PrintMD5ToFile(unsigned char *Buffer, std::ofstream &out) {
	for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
		out << std::hex << (int) Buffer[i];
	}	
}

std::string HashToString(unsigned char *Buffer) {
	std::stringstream temp;
	for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
		temp << std::hex << (int) Buffer[i];
	}	
	Hash_t Hash;
	temp >> Hash;
	return Hash;
}

//Get size of file descriptor
unsigned long GetSizeByFD(int FD) {
	struct stat StatBuf;
	if (fstat(FD, &StatBuf) < 0)
		exit(-1);
	else 
		return StatBuf.st_size;
}

void *Listener(void *p) {
	ClientIP = "127.0.0.1";

	memset(&ClientAddr, 0, sizeof(ClientAddr)); //set all bits to 0;
	ClientAddr.sin_family = AF_INET; //IPV4
	ClientAddr.sin_port = htons(atoi(ClientPort.c_str())); //port of client
	ClientAddr.sin_addr.s_addr = INADDR_ANY;
	// new socket to listen other clients
	int AnswerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //open socket
	bind (AnswerSochet, (const sockaddr *) &ClientAddr, sizeof(ClientAddr));
	listen(AnswerSocket, 10);

	while (!STOP_LISTENING) {
		sockaddr_in Asker;
		int AnswerDescriptor = accept(AnswerSocket, 0, 0);
		Hash_t Hash;
		ReceiveStr(Hash);
	}


}

void* DownloadRequest(void *p) {
	connect(FD, (const sockaddr *) &ServAddr, sizeof(ServAddr));

	SendStr(FD, Commands[0] + " Valera a][uenen!\n");

	close(FD);
}

void* UploadRequest(void *p) {

	connect(FD, (const sockaddr *) &ServAddr, sizeof(ServAddr));

	SendStr(FD, Commands[0]);
	SendStr(FD, ClientIP);
	SendStr(FD, ClientPort);

	File = Commands[1];
	std::ifstream fin(File.c_str());
	int BlockNumbers;
	fin >> BlockNumbers;

	SendInt(FD, BlockNumbers);

	for (int i = 0; i < BlockNumbers; ++i) {
		Hash_t Hash;
		fin >> Hash;
		SendStr(FD, Hash);
	}
	fin.close();

	SendStr(FD, Commands[0]);

	close(FD);
}

std::string GetTorrentName(std::string &File) {

	std::string TorrentFile = File;
	TorrentFile.erase(TorrentFile.begin() + TorrentFile.find_last_of("."), TorrentFile.end());
	TorrentFile += ".torrent";

	return TorrentFile;
}

int CalculateLastBlockSize() {
	int SizeLastBlock = FileSize % BlockSize;
	if (SizeLastBlock == 0)
		SizeLastBlock = BlockSize;
	return SizeLastBlock;
}

int CalculateNumberBlocks() {
	return FileSize / BlockSize + (int) (FileSize % BlockSize > 0);
}

void MakeHashOfFileBlock(int FD, int BlockID, Hash_t &Hash) {
	char *FileBuffer;
	FileBuffer = (char*)mmap(0, BlockSize, PROT_READ, MAP_SHARED, FD, BlockID * BlockSize);
	MD5((unsigned char*)FileBuffer, BlockSize, ResultingHash);
	munmap((void*)FileBuffer, BlockSize);
	Hash = HashToString(ResultingHash);
}


void* CreateRequest(void *p) {
	
	File = Commands[1];

	std::string TorrentFile = GetTorrentName(File);

	std::cout << TorrentFile << std::endl;

	std::ofstream fout(TorrentFile.c_str());

	int FD = open(File.c_str(), O_RDONLY);
	if (FD < 0)
		exit(-1);

	FileSize = GetSizeByFD(FD);
	int NumberBlocks = CalculateNumberBlocks();
	int SizeLastBlock = CalculateLastBlockSize();
	char *FileBuffer;

	fout << NumberBlocks << "\n";
	Hash_t HashString;

	for (int i = 0; i < NumberBlocks - 1; ++i) {
	
		MakeHashOfFileBlock(FD, i, HashString);
		DataBase[HashString] = make_pair(i, File);
		fout << HashString << " ";
	}
	MakeHashOfFileBlock(FD, NumberBlocks - 1, HashString);
	DataBase[HashString] = make_pair(NumberBlocks - 1, File);
	fout << HashString;

	fout.close();
}


void LoadData(std::string Path) {
	std::ifstream fin(Path.c_str());
	int BlockNumbers;
	fin >> BlockNumbers;
	for (int i = 0; i < BlockNumbers; ++i) {
		Hash_t Hash;
		fin >> std::hex >> Hash;
		std::cout << Hash << "\n";
		int Position;
		std::string FileName;
		fin >> Position >> FileName;
		DataBase[Hash] = make_pair(Position, FileName);
	}
	fin.close();
}

void SaveData(std::string Path) {
	std::cout << "Saving data\n";
	std::ofstream fout(Path.c_str());
	int BlockNumbers = DataBase.size();
	fout << BlockNumbers << "\n";
	ClientDataBase_t::iterator it;
	for (it = DataBase.begin(); it != DataBase.end(); ++it) {
		fout << std::hex << (*it).first << " ";
		fout << std::hex << (*it).second.first << " " << (*it).second.second << "\n";
	}
	fout.close();
}

int main() {

	LoadData(std::string("ClientDataBase.txt"));
	
	ClientIP = "127.0.0.1";

	memset(&ServAddr, 0, sizeof(ServAddr)); //set all bits to 0;
	ServAddr.sin_family = AF_INET; //IPV4
	ServAddr.sin_port = htons(1337); //example of port
	
	struct in_addr ServIP;

	inet_aton(ClientIP.c_str(), &ServIP);
	ServAddr.sin_addr.s_addr = ServIP.s_addr;


	FD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //open socket

	Command.reserve(BufferSize);
	UserCommands.resize(NumberCommands);
	UserCommands[0] = "exit";
	UserCommands[1] = "download";
	UserCommands[2] = "upload";
	UserCommands[3] = "create";

	while(1) {
		std::cout << "while_begin\n";
		getline(std::cin, Command);
		std::cout << "$" << Command << "$\n";
		del_extra_white_spaces(Command);
		Commands.resize(0);

		parse_by_white_spaces(Command, Commands);
		print_list(Commands, std::cout);
		IdCommand = NumberCommands;
		for (int i = 0; i < NumberCommands; ++i) {
			if (Commands[0] == UserCommands[i]) {
				IdCommand = i;
				break;
			}
		}

		switch (IdCommand) {
			case 0: {
				if (ClientIsFree) {
					SaveData(std::string("ClientDataBase.txt"));
					exit(0);
				}
				else 
					std::cout << "Client has been working yet. Wait, please.\n";
				break;
			}

			case 1: {
						//////////DOWNLOAD File
				pthread_t PthreadDownload;
				pthread_create(&PthreadDownload, NULL, DownloadRequest, NULL);
				pthread_join(PthreadDownload, NULL);
				break;
			}

			case 2: {
						//////////UPLOAD File
				pthread_t PthreadUpload;
				pthread_create(&PthreadUpload, NULL, UploadRequest, NULL);
				pthread_join(PthreadUpload, NULL);
				break;
			}

			case 3: {
						//////////CREATE Torrent
				pthread_t PthreadCreate;
				pthread_create(&PthreadCreate, NULL, CreateRequest, NULL);
				pthread_join(PthreadCreate, NULL);
				break;
			}

			default: {
				std::cout << "It is wrong request or this command has not been adding.\n";
			}
		}

		std::cout << "while_end\n\n";
	}
	SaveData(std::string("ClientDataBase.txt"));


	return 0;
}