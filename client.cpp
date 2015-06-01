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
#include "types.h"
#include <errno.h>
#include <fcntl.h>
#include <cstdlib>
 

const int BufferSize = 100;
const int BlockSize = 256 * 1024;
const int NumberCommands = 4;
//Commands: 1 - download file;
//			2 - upload file;
//			3 - create torrent;
//			0 - exit

std::string ClientIP;
std::string ClientPort;

std::string Command;
std::string File;
list UserCommands, Commands;
bool ClientIsFree = true;
int IdCommand = NumberCommands;
int NumberPthreads = 10;
struct sockaddr_in ServAddr;
unsigned char ResultingHash[MD5_DIGEST_LENGTH];
int FileDescriptor;
std::string WorkingDirectory;

std::vector<Hash_t> Hashes;
std::vector<Port_t> Ports;
std::vector<IP_t> IPs;
pthread_t* Pthreads;
std::vector<bool> UsedPthread;
bool STOP_LISTENING = false; 
char error_message[100];

ClientDataBase_t DataBase;

void printMD5(unsigned char *Buffer);

void PrintMD5ToFile(unsigned char *Buffer, std::ofstream &out);

std::string HashToString(unsigned char *Buffer);

unsigned long GetSizeByFD(int ServerDescriptor);

void* DownloadBlock(void *p);

void* DownloadRequest(void *p);

void* UploadRequest(void *p);

std::string GetTorrentName(std::string &File);

int CalculateLastBlockSize(int FileSize);

int CalculateNumberBlocks(int FileSize);

void MakeHashOfFileBlock(int ServerDescriptor, int BlockID, Hash_t &Hash, int SizeOfCurBlock);

void* CreateRequest(void *p);

void LoadData(std::string Path);

void SaveData(std::string Path);

void *Listener(void *p);


int main() {

	std::cout <<  "Enter port of client: ";
	std::cin >> ClientPort;

	WorkingDirectory = "Client:" + ClientPort + "/";
	int DirError = mkdir(WorkingDirectory.c_str(), 0770);
	DirError = mkdir((WorkingDirectory + "Torrents").c_str(), 0770);
	DirError = mkdir((WorkingDirectory + "Files").c_str(), 0770);
	DirError = mkdir((WorkingDirectory + "ClientDataBase").c_str(), 0770);

	WorkingDirectory = "Client:" + ClientPort + "/";
	LoadData(WorkingDirectory + "ClientDataBase/DataBase.txt");
	

	ClientIP = "127.0.0.1";

	memset(&ServAddr, 0, sizeof(ServAddr)); //set all bits to 0;
	ServAddr.sin_family = AF_INET; //IPV4
	ServAddr.sin_port = htons(1337); //example of port
	
	struct in_addr ServIP;

	inet_aton("127.0.0.1", &ServIP);
	ServAddr.sin_addr.s_addr = ServIP.s_addr;


	pthread_t ListenPthread;
	pthread_create(&ListenPthread, NULL, Listener, NULL);
				

	Command.reserve(BufferSize);
	UserCommands.resize(NumberCommands);
	UserCommands[0] = "exit";
	UserCommands[1] = "download";
	UserCommands[2] = "upload";
	UserCommands[3] = "create";
	getline(std::cin, Command);

	while(1) {
		getline(std::cin, Command);
		//std::cout << "$" << Command << "$\n";
		del_extra_white_spaces(Command);
		Commands.resize(0);

		parse_by_white_spaces(Command, Commands);
		//print_list(Commands, std::cout);
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
					STOP_LISTENING = true;
					//pthread_join(ListenPthread, NULL);
					SaveData(WorkingDirectory + "ClientDataBase/DataBase.txt");
					exit(0);
				}
				else 
					std::cout << "Client has been working yet. Wait, please.\n";
				break;
			}

			case 1: {
						//////////DOWNLOAD File
				ClientIsFree = false;
				pthread_t PthreadDownload;
				pthread_create(&PthreadDownload, NULL, DownloadRequest, NULL);
				pthread_join(PthreadDownload, NULL);
				ClientIsFree = true;
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

	}
	SaveData(WorkingDirectory + "ClientDataBase/DataBase.txt");

	return 0;
}

void *Listener(void *p) {

	ClientIP = "127.0.0.1";
	sockaddr_in ClientAddr;

	memset(&ClientAddr, 0, sizeof(ClientAddr)); //set all bits to 0;
	ClientAddr.sin_family = AF_INET; //IPV4
	ClientAddr.sin_port = htons(atoi(ClientPort.c_str())); //port of client
	ClientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	// new socket to listen other clients
	int AnswerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //open socket
	bind (AnswerSocket, (const sockaddr *) &ClientAddr, sizeof(ClientAddr));
	listen(AnswerSocket, 10);

	while (!STOP_LISTENING) {
		int AnswerDescriptor = accept(AnswerSocket, 0, 0);
		Hash_t Hash;

		ReceiveStr(AnswerDescriptor, Hash);
		int SizeOfBlock = ReceiveInt(AnswerDescriptor);


		std::pair<int, std::string> Location = DataBase[Hash];
		//std::cout << "Location: " << Location.first << " " << Location.second <<std::endl;
		std::string Path = WorkingDirectory + "Files/" + Location.second;
		int LocalFile = open(Path.c_str(), O_RDONLY);

		Hash_t OurHash;
		//std::cout << "internal check: \n";
		//std::cout << Path << std::endl;
		//std::cout << "Local File: " << LocalFile << std::endl;
		MakeHashOfFileBlock(LocalFile, Location.first, OurHash, SizeOfBlock);
		//std::cout << "after hashing\n";

		if (OurHash != Hash) {
			std::cout << "Fatal error, mismatch of hashes in listener\n";
			close(AnswerDescriptor);
			continue;
		}
		//std::cout << "Point1\n";
		//std::cout << Location.first << " " << Location.second << std::endl;
		char *FileBuffer = (char*)mmap(0, SizeOfBlock, PROT_READ, MAP_SHARED, LocalFile, Location.first * BlockSize);
		//std::cout << "Point2\n";
		
		SendMsg(AnswerDescriptor, FileBuffer, SizeOfBlock);

		munmap((void*) FileBuffer, SizeOfBlock);
		close(AnswerDescriptor);
	}
	close(AnswerSocket);

}

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

unsigned long GetSizeByFD(int ServerDescriptor) {

	struct stat StatBuf;
	if (fstat(ServerDescriptor, &StatBuf) < 0)
		exit(-1);
	else 
		return StatBuf.st_size;
}

void* DownloadBlock(void *p) {

	int *a = (int*)p;

	int IndexPthread = *a;
	int IndexBlock = *(a + 1);
	int SizeOfCurBlock = *(a + 2);

	std::string FileName = File;
	FileName.erase(FileName.begin() + FileName.find_last_of("."), FileName.end());

	sockaddr_in AnotherClient;
	memset(&AnotherClient, 0, sizeof(AnotherClient));

	AnotherClient.sin_family = AF_INET;
	int AnotherClientPort = atoi(Ports[IndexBlock].c_str());
	AnotherClient.sin_port = htons(AnotherClientPort);

	struct in_addr AnotherClientAddr;
	std::string AnotherClientIP = IPs[IndexBlock].c_str();
	inet_aton(AnotherClientIP.c_str(), &AnotherClientAddr);
	AnotherClient.sin_addr.s_addr = AnotherClientAddr.s_addr;

	int ConnectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	int result = connect(ConnectionSocket, (const sockaddr *) &AnotherClient, sizeof(AnotherClient));
	if (result == -1) {
		perror(0);
		return 0;
	}

	// int result = -1;
	// while(result) {
	// 	result = connect(ConnectionSocket, (const sockaddr *) &AnotherClient, sizeof(AnotherClient));
	// 	perror(0);
	// }

	SendStr(ConnectionSocket, Hashes[IndexBlock]);
	SendInt(ConnectionSocket, SizeOfCurBlock);

	char *FileBuffer;
	FileBuffer = (char*) mmap(0, SizeOfCurBlock, PROT_WRITE, MAP_SHARED, FileDescriptor, IndexBlock * BlockSize);
	int rec = ReceiveMsg(ConnectionSocket, FileBuffer, SizeOfCurBlock);

	munmap((void*)FileBuffer, SizeOfCurBlock);
	
	close(ConnectionSocket);

	UsedPthread[IndexPthread] = false;

	int ServerDescriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //open socket

	result = connect(ServerDescriptor, (const sockaddr *) &ServAddr, sizeof(ServAddr));
	//std::cout << "result: " << result << std::endl;
	if (result == -1) {
		return 0;
	}
	SendStr(ServerDescriptor, "upload");
	SendStr(ServerDescriptor, ClientIP);
	SendStr(ServerDescriptor, ClientPort);
	SendInt(ServerDescriptor, 1);
	SendStr(ServerDescriptor, Hashes[IndexBlock]);

	close(ServerDescriptor);

	DataBase[Hashes[IndexBlock]] = std::make_pair(IndexBlock, FileName);
}

void* DownloadRequest(void *p) {
	 	
	int ServerDescriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //open socket

	connect(ServerDescriptor, (const sockaddr *) &ServAddr, sizeof(ServAddr));

	std::string TorrentFile = Commands[1];
	std::string TorrentPath = WorkingDirectory + "Torrents/" + TorrentFile;
	
	std::ifstream fin(TorrentPath.c_str());

	int NumberBlocks;
	fin >> NumberBlocks;

	Hashes.resize(NumberBlocks);
	Ports.resize(NumberBlocks);
	IPs.resize(NumberBlocks);
				
	SendStr(ServerDescriptor, Commands[0]);
	SendInt(ServerDescriptor, NumberBlocks);

	for (int  i = 0; i < NumberBlocks; ++i) {
		fin >> Hashes[i];
		//std::cout << "Hash: " << Hashes[i] << "\n";
		SendStr(ServerDescriptor, Hashes[i]);
		ReceiveStr(ServerDescriptor, Ports[i]);
		ReceiveStr(ServerDescriptor, IPs[i]);
	}

	close(ServerDescriptor);

	int FileSize;
	fin >> FileSize;

	//std::cout << "FileSize: " << FileSize << "\n";

	std::string FileName = Commands[1];
	File = FileName;
	FileName.erase(FileName.begin() + FileName.find_last_of("."), FileName.end());
	

	int SizeOfLastBlock = CalculateLastBlockSize(FileSize);

	std::string FilePath = WorkingDirectory + "Files/" + FileName;
	FileDescriptor = open(FilePath.c_str(), O_RDWR | O_CREAT, 0644);
	posix_fallocate(FileDescriptor, 0, FileSize);

	Pthreads = new pthread_t[NumberPthreads];
	UsedPthread.resize(NumberPthreads, 0);

	int Index = 0;
	int IndexCurrentBlock = 0;
	int **Args = new int*[NumberBlocks];
	for (int i = 0; i < NumberBlocks; ++i) {
		Args[i] = new int[3];
	}

	while(Index >= 0) {
		Args[Index][0] = Index % NumberPthreads;

		if (UsedPthread[Args[Index][0]])
			continue;

		else {
			UsedPthread[Args[Index][0]] = true;
			
			Args[Index][1] = IndexCurrentBlock;
			Args[Index][2] = BlockSize;
			if (IndexCurrentBlock == NumberBlocks - 1)
				Args[Index][2] = SizeOfLastBlock;

			pthread_create(Pthreads + Args[Index][0], NULL, DownloadBlock, (void*)Args[Index]);
			IndexCurrentBlock++;

		}

		Index++;

 		if (IndexCurrentBlock == NumberBlocks)
 			break;
	}
	for (int i = 0; i < NumberPthreads; ++i) {
		if (UsedPthread[i]) {
			pthread_join(Pthreads[i], NULL);
		}
	}

	for (int i = 0; i < NumberBlocks; ++i) {
		delete []Args[i];
	}
	delete []Args;
	delete []Pthreads;
	//SaveData(WorkingDirectory + "ClientDataBase/DataBase.txt");
	close(FileDescriptor);
}

void* UploadRequest(void *p) {

	int ServerDescriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //open socket

	connect(ServerDescriptor, (const sockaddr *) &ServAddr, sizeof(ServAddr));

	SendStr(ServerDescriptor, Commands[0]);
	SendStr(ServerDescriptor, ClientIP);
	SendStr(ServerDescriptor, ClientPort);

	File = Commands[1];

	std::string Path = WorkingDirectory + "Torrents/" + File;
	std::ifstream fin(Path.c_str());
	int NumberBlocks;
	fin >> NumberBlocks;

	SendInt(ServerDescriptor, NumberBlocks);

	for (int i = 0; i < NumberBlocks; ++i) {
		Hash_t Hash;
		fin >> Hash;
		SendStr(ServerDescriptor, Hash);
	}
	fin.close();

	close(ServerDescriptor);
}

std::string GetTorrentName(std::string &File) {

	std::string TorrentFile = File;
	//TorrentFile.erase(TorrentFile.begin() + TorrentFile.find_last_of("."), TorrentFile.end());
	TorrentFile = WorkingDirectory + "Torrents/" + TorrentFile + ".torrent";

	return TorrentFile;
}

int CalculateLastBlockSize(int FileSize) {

	int SizeOfLastBlock = FileSize % BlockSize;

	if (SizeOfLastBlock == 0)
		SizeOfLastBlock = BlockSize;

	return SizeOfLastBlock;
}

int CalculateNumberBlocks(int FileSize) {

	return FileSize / BlockSize + (int) (FileSize % BlockSize > 0);
}

void MakeHashOfFileBlock(int FD, int BlockID, Hash_t &Hash, int SizeOfCurBlock) {

	char *FileBuffer;
	FileBuffer = (char*)mmap(0, SizeOfCurBlock, PROT_READ, MAP_SHARED, FD, BlockID * BlockSize);

	MD5((unsigned char*)FileBuffer, SizeOfCurBlock, ResultingHash);

	munmap((void*)FileBuffer, SizeOfCurBlock);

	Hash = HashToString(ResultingHash);
}

void* CreateRequest(void *p) {
	
	File = Commands[1];

	std::string TorrentFile = GetTorrentName(File);

	std::cout << TorrentFile << std::endl;

	std::ofstream fout(TorrentFile.c_str());

	std::string Path = WorkingDirectory + "Files/" + File;
	int FD = open(Path.c_str(), O_RDONLY);
	if (FD < 0) {
		std::cout << "File not found\n";
		return 0;
	}

	int FileSize = GetSizeByFD(FD);
	int NumberBlocks = CalculateNumberBlocks(FileSize);
	int SizeOfLastBlock = CalculateLastBlockSize(FileSize);
	char *FileBuffer;

	fout << NumberBlocks << "\n";
	Hash_t HashString;

	for (int i = 0; i < NumberBlocks - 1; ++i) {
	
		MakeHashOfFileBlock(FD, i, HashString, BlockSize);

		DataBase[HashString] = make_pair(i, File);
		fout << HashString << " ";
	}

	MakeHashOfFileBlock(FD, NumberBlocks - 1, HashString, SizeOfLastBlock);

	DataBase[HashString] = make_pair(NumberBlocks - 1, File);
	fout << HashString << " ";

	fout << FileSize;

	close(FD);

	fout.close();
}


void LoadData(std::string Path) {

	std::ifstream fin(Path.c_str());

	int NumberBlocks;
	fin >> NumberBlocks;

	for (int i = 0; i < NumberBlocks; ++i) {
		Hash_t Hash;
		fin >> std::hex >> Hash;
		//std::cout << Hash << "\n";

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

	int NumberBlocks = DataBase.size();
	fout << NumberBlocks << "\n";

	ClientDataBase_t::iterator it;
	for (it = DataBase.begin(); it != DataBase.end(); ++it) {
		fout << std::hex << (*it).first << " ";
		fout << std::hex << (*it).second.first << " " << (*it).second.second << "\n";
	}

	fout.close();
}