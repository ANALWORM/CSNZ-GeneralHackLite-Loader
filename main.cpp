#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <ctime>
#include <conio.h>
#include "RAW.h"
#include "MMap.h"
#include "Strings.h"
#include "winapi.h"
#include "ColorOutput.h"

HWND hGame;
DWORD pID;


void MMAP()
{
	PVOID rData = reinterpret_cast<char*>(RAWData);
	pIDH = (PIMAGE_DOS_HEADER)rData;
	pINH = (PIMAGE_NT_HEADERS)((LPBYTE)rData + pIDH->e_lfanew);
	winapi.hGetWindowThreadProcessId(hGame, &pID);
	HANDLE hProc = winapi.hOpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);

	if (!hProc) {
		system(CLS);
		ColorText(NEED_ADMIN, RED);
		winapi.hSleep(5000);
		exit(1);
	}

	PVOID image = winapi.hVirtualAllocEx(hProc, NULL, pINH->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	winapi.hWriteProcessMemory(hProc, image, rData, pINH->OptionalHeader.SizeOfHeaders, NULL);
	PIMAGE_SECTION_HEADER pISH = (PIMAGE_SECTION_HEADER)(pINH + 1);

	for (i = 0; i < pINH->FileHeader.NumberOfSections; i++)
	{
		winapi.hWriteProcessMemory(hProc, (PVOID)((LPBYTE)image + pISH[i].VirtualAddress),
			(PVOID)((LPBYTE)rData + pISH[i].PointerToRawData), pISH[i].SizeOfRawData, NULL);
	}
	PVOID mem = winapi.hVirtualAllocEx(hProc, NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	memset(&ManualInject, 0, sizeof(MANUAL_INJECT));
	ManualInject.ImageBase = image;
	ManualInject.NtHeaders = (PIMAGE_NT_HEADERS)((LPBYTE)image + pIDH->e_lfanew);
	ManualInject.BaseRelocation = (PIMAGE_BASE_RELOCATION)((LPBYTE)image + pINH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
	ManualInject.ImportDirectory = (PIMAGE_IMPORT_DESCRIPTOR)((LPBYTE)image + pINH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	winapi.hWriteProcessMemory(hProc, mem, &ManualInject, sizeof(MANUAL_INJECT), NULL);
	winapi.hWriteProcessMemory(hProc, (PVOID)((PMANUAL_INJECT)mem + 1), LoadDll, (DWORD)LoadDllEnd - (DWORD)LoadDll, NULL);
	hThread = winapi.hCreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)((PMANUAL_INJECT)mem + 1), mem, 0, NULL);
	//hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)((PMANUAL_INJECT)mem + 1), mem, 0, NULL); // Create a remote thread to execute the loader code

	winapi.hWaitForSingleObject(hThread, INFINITE);
	GetExitCodeThread(hThread, &ExitCode);
}

void LoadLib()
{
	LPVOID RemoteString, LoadLibAdd;
	char PathToTemp[MAX_PATH] = { 0 }, buf[MAX_PATH] = { 0 };
	char randName[8];

	winapi.hGetTempPathA(MAX_PATH, PathToTemp);
	
	for (int i = 0; i < 7; i++)
	{
		char j = 30 + rand() % 92;

		if ((j >= 48 && j <= 57) || (j >= 65 && j <= 90) || (j >= 97 && j <= 122))
			randName[i] = (char)j;
		else
			if (i >= 0) i--;
	}
	randName[7] = '\0'; 

	sprintf(buf, PATH, PathToTemp, randName);

	FILE* fp = std::fopen(buf, WB);
	std::fwrite(&RAWData, 1, sizeof(RAWData), fp);
	std::fclose(fp);
	winapi.hGetWindowThreadProcessId(hGame, &pID);
	HANDLE hProc = winapi.hOpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);

	if (!hProc) {
		system(CLS);
		ColorText(NEED_ADMIN, RED);
		winapi.hSleep(5000);
		exit(1);
	}

	RemoteString = (LPVOID)winapi.hVirtualAllocEx(hProc, NULL, strlen(buf), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	winapi.hWriteProcessMemory(hProc, (LPVOID)RemoteString, buf, strlen(buf), NULL);
	winapi.hCreateRemoteThread(hProc, NULL, NULL, (LPTHREAD_START_ROUTINE)winapi.hLoadLibraryA, (LPVOID)RemoteString, NULL, 0);
	winapi.hCloseHandle(hProc);
}

void main()
{
	std::srand(time(NULL));

	HINSTANCE hKernel = NULL, hUser = NULL;
	hKernel = LoadLibraryA(KERNEL_DLL);

	winapi.hGetProcAddress = (tGetProcAddress)GetProcAddress((HMODULE)hKernel, GETPROCADDRESS);
	winapi.hCreateThread = (tCreateThread)winapi.hGetProcAddress((HMODULE)hKernel, CREATETHREAD);
	winapi.hLoadLibraryA = (tLoadLibraryA)winapi.hGetProcAddress((HMODULE)hKernel, LOADLIBRARY);

	hUser = winapi.hLoadLibraryA(USER_DLL);

	winapi.hFindWindowA = (tFindWindowA)winapi.hGetProcAddress((HMODULE)hUser, FINDWINDOW);
	winapi.hSetConsoleTitleA = (tSetConsoleTitleA)winapi.hGetProcAddress((HMODULE)hKernel, SETCONSOLETITLEA);
	winapi.hGetStdHandle = (tGetStdHandle)winapi.hGetProcAddress((HMODULE)hKernel, GETSTDHANDLE);
	winapi.hGetConsoleScreenBufferInfo = (tGetConsoleScreenBufferInfo)winapi.hGetProcAddress((HMODULE)hKernel, GETCONSOLESCREENBUFFERINFO);
	winapi.hSetConsoleTextAttribute = (tSetConsoleTextAttribute)winapi.hGetProcAddress((HMODULE)hKernel, SETCONSOLETEXTATTRIBUTE);
	winapi.hOpenProcess = (tOpenProcess)winapi.hGetProcAddress((HMODULE)hKernel, OPENPROCESS);
	winapi.hSleep = (tSleep)winapi.hGetProcAddress((HMODULE)hKernel, SLEEP);
	winapi.hVirtualAllocEx = (tVirtualAllocEx)winapi.hGetProcAddress((HMODULE)hKernel, VIRTUALALLOCEX);
	winapi.hWriteProcessMemory = (tWriteProcessMemory)winapi.hGetProcAddress((HMODULE)hKernel, WRITEPROCESSMEMORY);
	winapi.hCreateRemoteThread = (tCreateRemoteThread)winapi.hGetProcAddress((HMODULE)hKernel, CREATEREMOTETHREAD);
	winapi.hGetWindowThreadProcessId = (tGetWindowThreadProcessId)winapi.hGetProcAddress((HMODULE)hUser, GETWINDOWTHREADPROCESSID);
	winapi.hWaitForSingleObject = (tWaitForSingleObject)winapi.hGetProcAddress((HMODULE)hKernel, WAITFORSINGLEOBJECT);
	winapi.hCloseHandle = (tCloseHandle)winapi.hGetProcAddress((HMODULE)hKernel, CLOSEHANDLE);
	winapi.hGetTempPathA = (tGetTempPathA)winapi.hGetProcAddress((HMODULE)hKernel, GETTEMPPATHA);

	winapi.hSetConsoleTitleA(WND_NAME);
	
	hStdOut = winapi.hGetStdHandle(STD_OUTPUT_HANDLE);

	ColorText(GAME, WHITE);
	ColorText(NOT_FOUND, RED);
	ColorText(HACK, WHITE);
	ColorText(NOT_INJECTED, RED);

	do
	{
		hGame = winapi.hFindWindowA(0, "Game.exe");
	} while (hGame == NULL);

	ColorText("Select the inject method:\n1. MMap\n2. LoadLibrary\n", WHITE);

	char input = _getch();
	if (input == '1') MMAP();
	else if (input == '2') LoadLib();
	
	system(CLS);
	ColorText(GAME, WHITE);
	ColorText(FOUND, GREEN);
	ColorText(HACK, WHITE);
	ColorText(INJECTED, GREEN);

	winapi.hSleep(2500);
	exit(1);
}
