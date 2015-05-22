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
int IdCommand = NumberCommands;
int FD;//FileDescriptor
struct sockaddr_in ServAddr;

typedef std::string Hash_t;////////////////////////////потом убрать

std::map<Hash_t, std::pair<int, std::string> > DataBase;

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

//Get size of file descriptor
unsigned long GetSizeByFD(int FD) {
	struct stat StatBuf;
	if (fstat(FD, &StatBuf) < 0)
		exit(-1);
	else 
		return StatBuf.st_size;
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

	File = Command[1];
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

void* CreateRequest(void *p) {
	unsigned char ResultingHash[MD5_DIGEST_LENGTH];
	std::string TorrentFile;
	File = Commands[1];
	TorrentFile = File;
	TorrentFile.erase(TorrentFile.begin() + TorrentFile.find_last_of("."), TorrentFile.end());
	TorrentFile += ".torrent";
	std::cout << TorrentFile << std::endl;

	std::ofstream fout(TorrentFile.c_str());

	int FD = open(File.c_str(), O_RDONLY);
	if (FD < 0)
		exit(-1);
	FileSize = GetSizeByFD(FD);

	int SizeLastBlock = FileSize % BlockSize;
	if (SizeLastBlock == 0)
		SizeLastBlock = BlockSize;

	std::string FileBuffer;

	int NumberBlocks = FileSize / BlockSize; 
	if (FileSize % BlockSize > 0)
		++NumberBlocks;

	fout << NumberBlocks << "\n";
	std::cout << "before for \n";

	for (int i = 0; i < NumberBlocks - 1; ++i) {
		FileBuffer = (char*)mmap(0, BlockSize, PROT_READ, MAP_SHARED, FD, i * BlockSize);
		MD5((unsigned char*)FileBuffer.c_str(), BlockSize, ResultingHash);
		PrintMD5ToFile(ResultingHash, fout);
		fout << " ";
		munmap((void*)FileBuffer.c_str(), BlockSize);
		std::string HashString((char*)ResultingHash);
		DataBase[HashString] = make_pair(i, File);
	}
	std::cout << "after for \n";
	FileBuffer = (char*)mmap(0, SizeLastBlock, PROT_READ, MAP_SHARED, FD, BlockSize * (NumberBlocks - 1));
	std::cout << "point 1 \n";
	
	MD5((unsigned char*)FileBuffer.c_str(), SizeLastBlock, ResultingHash);
	PrintMD5ToFile(ResultingHash, fout);
	fout << " ";
	munmap((void*)FileBuffer.c_str(), SizeLastBlock);
	std::string HashString((char*)ResultingHash);
	DataBase[HashString] = make_pair(NumberBlocks - 1, File);

	fout.close();
}

int main() {
	
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
				if (ClientIsFree)
					exit(0);
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


	return 0;
}
