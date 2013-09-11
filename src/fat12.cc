#include <iostream>
#include <fstream>
#include <stdint.h>


using std::ios;
using std::ifstream;
using std::ofstream;
#define INDENT(x) for(int temp = 0; temp < x; ++temp) printf("\t" );
typedef struct root_Entries
{
   char short_FileName[11];
   uint8_t fileAttributes;
   uint8_t reserved;
   uint8_t createTime_ms;
   uint16_t createTime;
   uint16_t createDate;
   uint16_t accessedDate;
   uint16_t clusterNumber_High;
   uint16_t modifiedTime;
   uint16_t modifiedDate;
   uint16_t firstClusterAddress_FAT12;
   uint32_t sizeofFile;
} root;

using std::cout;

void printRootDirEntry(root* r, int indentationLevel) {

	if(*reinterpret_cast<uint8_t*>(r->short_FileName) == 0)
		return;

	INDENT(indentationLevel)
	printf("======================================= : %02X\n", r->short_FileName[0] & 0xFF );
	if(*reinterpret_cast<uint8_t*>(r->short_FileName) == 0xE5) {
		INDENT(indentationLevel)
		printf("DELETED!\n");
	}
	INDENT(indentationLevel)
	printf("%s\n", reinterpret_cast<char*>(r->short_FileName));
	INDENT(indentationLevel)
	printf("%02x\n", reinterpret_cast<uint8_t>(r->fileAttributes));
	if(r->fileAttributes & 0b1) {
		INDENT(indentationLevel)
		printf("read only\n");
	}
	if(r->fileAttributes & 0b10) {
		INDENT(indentationLevel)
		printf("hidden\n");
	}
	if(r->fileAttributes & 0b100) {
		INDENT(indentationLevel)
		printf("system file\n");
	}
	if(r->fileAttributes & 0b1000) {
		INDENT(indentationLevel)
		printf("volume label\n");
	}
	if(r->fileAttributes & 0b10000) {
		INDENT(indentationLevel)
		printf("sub dir\n");
	}
	if(r->fileAttributes & 0b100000) {
		INDENT(indentationLevel)
		printf("archive\n");
	}
	INDENT(indentationLevel)
	printf("create time: %02u:%02u:%02u\n", 
		(r->createTime & 0b1111100000000000) >> 11, 
		(r->createTime & 0b0000011111100000) >> 5 , 
		(r->createTime & 0b0000000000011111) * 2 );
	INDENT(indentationLevel)
	printf("create date: %02u-%02u-%02u\n", 
		((r->createDate & 0b1111111000000000) >> 9) + 1980, 
		(r->createDate & 0b0000000111100000) >> 5 , 
		(r->createDate & 0b0000000000011111) );
	INDENT(indentationLevel)
	printf("access date: %02u-%02u-%02u\n", 
		((r->accessedDate & 0b1111111000000000) >> 9) + 1980, 
		(r->accessedDate & 0b0000000111100000) >> 5 , 
		(r->accessedDate & 0b0000000000011111) );
	INDENT(indentationLevel)
	printf("modify date: %02u-%02u-%02u\n", 
		((r->modifiedDate & 0b1111111000000000) >> 9) + 1980, 
		(r->modifiedDate & 0b0000000111100000) >> 5 , 
		(r->modifiedDate & 0b0000000000011111) );
	INDENT(indentationLevel)
	printf("modify time: %02u:%02u:%02u\n", 
		(r->modifiedTime & 0b1111100000000000) >> 11, 
		(r->modifiedTime & 0b0000011111100000) >> 5 , 
		(r->modifiedTime & 0b0000000000011111) * 2 );
	INDENT(indentationLevel)
	printf("first cluster address: %d\n", reinterpret_cast<uint16_t>(r->firstClusterAddress_FAT12));
	INDENT(indentationLevel)
	printf("%u\n", reinterpret_cast<uint32_t>(r->sizeofFile));
}
uint16_t getFatEntry(int num, uint8_t* startOfFat){
	uint8_t* startOfTripple = startOfFat + (num/2) * 3;
	printf("%02X%02X%02X\n",*startOfTripple,*startOfTripple+1,*startOfTripple+2 );
	uint16_t rtn = 0;
	if(num % 2 == 0){
		rtn = (*(startOfTripple+1) & 0x0F) << 8;
		rtn = (rtn | *(startOfTripple) ) & 0xFFF;
	} else {
		rtn = (*(startOfTripple+1) & 0xF0) >> 4;
		rtn = (rtn | (*(startOfTripple+2) << 4) ) & 0xFFF;
	}
	printf("%03X\n",rtn );
	return rtn;
}
void recoverFile(root* dirEntry, char* filename, uint8_t* startOfFat, uint8_t* startOfData) {
	ofstream data(filename, ios::out | ios::binary | ios::ate | ios::trunc);
	uint16_t currentCluster = dirEntry->firstClusterAddress_FAT12;
	while(currentCluster < 0xFF0 && currentCluster > 0){
		printf("currentCluster: %d\n", currentCluster);
		currentCluster = getFatEntry(currentCluster,startOfFat);
	}
	//data.write()

	data.close();
}
void printDirectorys(char* dirSector, int indentationLevel, char* dataAreaStart) {
	root* r = reinterpret_cast<root*>(dirSector);
	while((*reinterpret_cast<uint8_t*>(r->short_FileName) != 0)) {
		r->short_FileName[10] = 0;
		if(r->sizeofFile == 1978) {
			recoverFile(r, "out.zip", reinterpret_cast<uint8_t*>(dataAreaStart - 9*512), reinterpret_cast<uint8_t*>(dataAreaStart));
		}
		printRootDirEntry(r,indentationLevel);
		++r;
	}
}
void printDirectory(char* dirSector,int numDirs, int indentationLevel, char* dataAreaStart) {
	root* r = reinterpret_cast<root*>(dirSector);
	for(int i = 0; i < numDirs; ++i) {
		r->short_FileName[10] = 0;
		printRootDirEntry(r,indentationLevel);
		if(r->fileAttributes & 0b10000 && (*reinterpret_cast<uint8_t*>(r->short_FileName) != 0xE5)) {
			printDirectorys(dataAreaStart + (r->firstClusterAddress_FAT12 - 2) * 512,indentationLevel+1,dataAreaStart);
		}
		++r;
	}	
}
bool compareFatTable(uint8_t* firstFAT, uint8_t* secondFAT) {
	uint8_t* firstEnd = secondFAT;
	size_t total = secondFAT - firstFAT;
	while(*firstFAT == *secondFAT && firstEnd != firstFAT){
		++firstFAT;
		++secondFAT;
	}
	if(firstFAT != firstEnd) {	
		printf("%02X %02X at offset %zu\n at %02f%%", *firstFAT & 0xFF, *secondFAT & 0xFF, secondFAT - firstEnd, static_cast<float>(secondFAT - firstFAT)*100/total);
		return false;
	}
	return true;
}

int main() {
	
	char* buffer;
	long size;

	ifstream data("evidence.dat", ios::in | ios::binary | ios::ate);
	size = data.tellg();
	data.seekg(0, ios::beg);
	buffer = new char[size];

	data.read(buffer, size);
	data.close();


	uint16_t bytesPerSector = *reinterpret_cast<uint16_t*>(buffer + 0x0b);
	uint8_t numFAT = *reinterpret_cast<uint8_t*>(buffer + 0x10);

	uint16_t numRootDirs = *reinterpret_cast<uint16_t*>(buffer + 0x11);
	uint16_t sectorsPerFAT = *reinterpret_cast<uint16_t*>(buffer + 0x16);

	size_t rootDirOff = (1 + numFAT * sectorsPerFAT) * bytesPerSector;

	printf("%d\n",numRootDirs);

	printf("%zu\n", rootDirOff);

	printDirectory(buffer + rootDirOff,numRootDirs, 0, buffer + 16896);
	
	if(compareFatTable(reinterpret_cast<uint8_t*>(buffer+(1*512)),reinterpret_cast<uint8_t*>(buffer + (10*512)))) {
		printf("fat tables are identical\n" );
	} else {
		printf("fat missmatch\n");
	}





}