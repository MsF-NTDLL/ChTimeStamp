#include <Windows.h>
#include <stdio.h>
#include "structs.h"

typedef NTSYSCALLAPI NTSTATUS(WINAPI* _NtQueryInformationFile)(
	HANDLE,
	PIO_STATUS_BLOCK,
	PVOID,
	ULONG,
	FILE_INFORMATION_CLASS
	);

typedef NTSYSCALLAPI NTSTATUS(WINAPI* _NtSetInformationFile)(
	HANDLE,
	PIO_STATUS_BLOCK,
	PVOID,
	ULONG,
	FILE_INFORMATION_CLASS
	);

unsigned char ntdll[] = { 'n','t','d','l','l','.','d','l','l',0 };


BOOL ChangeTimeStamps(char* srcFile, char* destFile) {
	HANDLE hsrcFile = CreateFileA(srcFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hsrcFile == INVALID_HANDLE_VALUE) {
		printf("Failed in CreateFileA srcFile (%u)", GetLastError());
		return FALSE;
	}

	HANDLE hdestFile = CreateFileA(destFile, GENERIC_READ | GENERIC_WRITE | FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hdestFile == INVALID_HANDLE_VALUE) {
		printf("Failed in CreateFileA destFile (%u)", GetLastError());
		return FALSE;
	}

	unsigned char ntqueryInfoFile[] = { 'N','t','Q','u','e','r','y','I','n','f','o','r','m','a','t','i','o','n','F','i','l','e',0 };
	_NtQueryInformationFile pNtQueryInformationFile = (_NtQueryInformationFile)GetProcAddress(GetModuleHandleA((LPCSTR)ntdll), (LPCSTR)ntqueryInfoFile);
	if (!pNtQueryInformationFile) {
		printf("Could not Resolve NtQuerInformationFile (%u)\n", GetLastError());
		return FALSE;
	}

	IO_STATUS_BLOCK iosb;
	FILE_BASIC_INFORMATION fbi_src, fbi_dest;

	if (pNtQueryInformationFile(hsrcFile, &iosb, &fbi_src, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation)) {
		printf("Could not get %s file information\n", srcFile);
		CloseHandle(hsrcFile);
		CloseHandle(hdestFile);
		return FALSE;
	}

	if (pNtQueryInformationFile(hdestFile, &iosb, &fbi_dest, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation)) {
		printf("Could not get %s file information\n", destFile);
		CloseHandle(hsrcFile);
		CloseHandle(hdestFile);
		return FALSE;
	}

	fbi_dest.ChangeTime = fbi_src.ChangeTime;
	fbi_dest.CreationTime = fbi_src.CreationTime;
	fbi_dest.FileAttributes = fbi_src.FileAttributes;
	fbi_dest.LastAccessTime = fbi_src.LastAccessTime;
	fbi_dest.LastWriteTime = fbi_src.LastWriteTime;

	unsigned char ntSetInfoFile[] = { 'N','t','S','e','t','I','n','f','o','r','m','a','t','i','o','n','F','i','l','e',0 };
	_NtSetInformationFile pNtSetInformationFile = (_NtSetInformationFile)GetProcAddress(GetModuleHandleA((LPCSTR)ntdll), (LPCSTR)ntSetInfoFile);
	if (!pNtSetInformationFile) {
		printf("Couldn't resolve NtSetInformationFile (%u)\n", GetLastError());
		return FALSE;
	}

	if (pNtSetInformationFile(hdestFile, &iosb, &fbi_dest, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation)) {
		printf("Could not set %s file information\n", destFile);
		CloseHandle(hsrcFile);
		CloseHandle(hdestFile);
		return FALSE;
	}

	CloseHandle(hsrcFile);
	CloseHandle(hdestFile);

	return TRUE;
}

int main(int argc, char** argv) {
	char* destFilePath = argv[1];
	char* srcFilePath = argv[2];

	if (argc != 3) {
		printf("[+] USAGE :\n%s <desFile> <srcFile>", argv[0]);
		return -1;
	}

	if (!ChangeTimeStamps(srcFilePath, destFilePath)) {
		printf("Failed in ChangeTimeStamps() (%u)\n", GetLastError());
		return -1;
	}

	printf("%s's time stamp changed to %s's time stamp\n", destFilePath, srcFilePath);

	return 0;
}