#if 0
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
using namespace std;

#include "cInjector.h"

void EnableDebugPriv() {
	cout << "Enabling Debug Privledges?\n";
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkp;

	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);

	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luid;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hToken, false, &tkp, sizeof(tkp), NULL, NULL);

	CloseHandle(hToken);
}

int main(int, char *[]) {
	cout << "RLInjector\n";
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	cout << "Searching for Speedhack-Test.exe\n";
	if (Process32First(snapshot, &entry) == TRUE) {
		bool bFound = false;
		while (Process32Next(snapshot, &entry) == TRUE) {
			if (_stricmp(entry.szExeFile, "Speedhack-Test.exe") == 0) {
				bFound = true;
				cout << "Found process\n";
				EnableDebugPriv();
				
				char dirPath[MAX_PATH];
				char fullPath[MAX_PATH];

				GetCurrentDirectory(MAX_PATH, dirPath);

				sprintf_s(fullPath, MAX_PATH, /*"%s\\RLSpeedhack.dll" */"D:/Code/Speedhack/RLSpeedhack/Debug/RLSpeedhack.dll");
				cout << "Injecting: " << fullPath << "\n";

				HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, entry.th32ProcessID);
				if (!hProcess) {
					cout << "OpenProcess Failed\n";
					break;
				}

				/*LPVOID libAddr = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
				if (!libAddr) {
					cout << "GetProcAddress Failed\n";
					CloseHandle(hProcess);
					break;
				}

				LPVOID llParam = (LPVOID)VirtualAllocEx(hProcess, NULL, strlen(fullPath), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
				if (!llParam) {
					cout << "VirtualAllocEx Failed\n";
					CloseHandle(hProcess);
					break;
				}

				BOOL ret = WriteProcessMemory(hProcess, llParam, fullPath, strlen(fullPath), NULL);
				if (!ret) {
					cout << "WriteProcessMemory Failed\n";
					CloseHandle(hProcess);
					break;
				}

				HANDLE hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)libAddr, llParam, NULL, NULL);
				if (!hThread) {
					cout << "CreateRemoteThread Failed\n";
					DWORD GLE = GetLastError();
					HRESULT res = HRESULT_FROM_WIN32(GLE);
					CloseHandle(hProcess);
					break;
				}*/

				cInjector::EnableDebugPriv();
				if (!InjectDLL(hProcess, "D:/Code/Speedhack//RLSpeedhack/Debug/RLSpeedhack.dll")) {
					printf("Could not inject dll\n");
				}

				CloseHandle(hProcess);
				break;
			}
		}

		if (!bFound) {
			cout << "Could not find process. Ensure that it is running.\n";
		}
	}

	CloseHandle(snapshot);

	return 0;
}
#else
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <Windows.h>
#include <tlhelp32.h>

#include "cInjector.h"

bool InjectDLL(DWORD dwPid, const char* wDllName) {
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	printf("OpenProcess() %p\n", hProc);
	if (!hProc) {
		printf("OpenProcess failed\n");
		return false;
	}

	HMODULE hKernel32 = LoadLibraryA("Kernel32.DLL");
	printf("LoadLibrary(\"Kernel32.dll\") %p\n", hKernel32);
	if (!hKernel32) {
		printf("LoadLibrary(\"Kernel32.dll\") failed\n");
		CloseHandle(hProc);
		return false;
	}

	LPVOID lpLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");
	printf("GetProcAddress(hKernel32, \"LoadLibraryA\") %p\n", lpLoadLibraryA);
	if (!lpLoadLibraryA) {
		printf("GetProcAddress(hKernel32, \"LoadLibraryA\") failed\n");
		FreeLibrary(hKernel32);
		CloseHandle(hProc);
		return false;
	}

	LPVOID lpRemoteString = (LPVOID)VirtualAllocEx(hProc, NULL, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	printf("VirtualAllocEx() %p\n", lpRemoteString);
	if (!lpRemoteString) {
		printf("VirtualAllocEx() failed\n");
		FreeLibrary(hKernel32);
		CloseHandle(hProc);
		return false;
	}

	BOOL ret = WriteProcessMemory(hProc, lpRemoteString, wDllName, strlen(wDllName) + 1, NULL);
	printf("WriteProcessMemory() %d\n", ret);

	HANDLE hThread = CreateRemoteThread(hProc, NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(lpLoadLibraryA), lpRemoteString, NULL, NULL);
	printf("CreateRemoteThread() %p\n", hThread);
	if (!hThread) {
		printf("CreateRemoteThread() failed\n");
		DWORD GLE = GetLastError();
		HRESULT res = HRESULT_FROM_WIN32(GLE);
		VirtualFreeEx(hProc, lpRemoteString, 0, MEM_RELEASE);
		FreeLibrary(hKernel32);
		CloseHandle(hProc);
		return false;
	}

	DWORD wfso = WaitForSingleObject(hThread, INFINITE);
	printf("WaitForSingleObject(hThread, INFINITE) %d\n", wfso);

	ret = CloseHandle(hThread);
	printf("CloseHandle(hThread) %d\n", ret);
	ret = VirtualFreeEx(hProc, lpRemoteString, 0, MEM_RELEASE);
	printf("VirtualFreeEx() %d\n", ret);
	ret = FreeLibrary(hKernel32);
	printf("FreeLibrary(hKernel32) %d\n", ret);
	ret = CloseHandle(hProc);
	printf("CloseHandle(hProc) %d\n", ret);

	return true;
}

int LaunchProcess(const char* processname) {
	PROCESS_INFORMATION pi;
	STARTUPINFOA si;

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);

	char path[MAX_PATH];
	strcpy_s(path, processname);

	char *pos = strrchr(path, '\\');
	if (pos != NULL) {
		*pos = '\0';
	}
	else {
		pos = strrchr(path, '/');
		if (pos != NULL) {
			*pos = '\0';
		}
	}

	cout << "Path: " << path << "\n";
	
	if (!CreateProcessA(processname, (char*)processname, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, path, &si, &pi)) {
		printf("Could not not start process\n");
		return 1;
	}


	printf("----------------------------------------------\n");
	cInjector::EnableDebugPriv();
	if (!InjectDLL(pi.dwProcessId, "D:/Code/Speedhack/RLSpeedhack/Debug/RLSpeedhack.dll")) {
		printf("Could not inject dll\n");
	}
	printf("----------------------------------------------\n\n");
	ResumeThread(pi.hThread);

	WaitForSingleObject(pi.hProcess, 5000);

	return 0;
}

int FindProcess(const char* processname) {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	cout << "Searching for " << processname << "\n";

	bool bFound = false;
	if (Process32First(snapshot, &entry) == TRUE) {
		while (Process32Next(snapshot, &entry) == TRUE) {
			if (_stricmp(entry.szExeFile, processname) == 0) {
				bFound = true;
				cout << "Found process\n";
				printf("----------------------------------------------\n");
				cInjector::EnableDebugPriv();
				if (!InjectDLL(entry.th32ProcessID, "D:/Code/Speedhack/RLSpeedhack/Debug/RLSpeedhack.dll")) {
					printf("Could not inject dll\n");
				}
				printf("----------------------------------------------\n\n");
				return 0;
			}
		}
	}

	if (!bFound) {
		cout << "Could not find process. Ensure that it is running.\n";
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		cout << "You must supply an executable to hook! Usage: DLLInjector.exe [-l] <ApplicationToHook.exe>\n";
		return 1;
	}
	
	char* processname = argv[argc-1];

	if (_stricmp(processname + (strlen(processname) - 4), ".exe") != 0) {
		cout << "You must supply an executable to hook! Usage: DLLInjector.exe [-l] <ApplicationToHook.exe>\n";
		return 1;
	}

	for (int i = 0; i < argc; ++i) {
		if (_stricmp(argv[i], "-l") == 0) {
			cout << "Launching " << processname << "\n";
			return LaunchProcess(processname);
		}
	}

	return FindProcess(processname);
}
#endif