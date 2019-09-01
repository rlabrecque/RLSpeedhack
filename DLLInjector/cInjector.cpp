/*
	Gamefilter for Diablo II 1.12a
	Copyright (C) 2009 Sheppard

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include <windows.h>
#include <tlhelp32.h>

#include "cInjector.h"

BOOL cInjector::EnableDebugPriv(VOID) // Abin's function.
{
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;

	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ))
		if(LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue )) 
		{
			tkp.PrivilegeCount=1;
			tkp.Privileges[0].Luid = sedebugnameValue;
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			if(AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof tkp, NULL, NULL )) 
			{
				CloseHandle(hToken);
				return TRUE;
			}
		}

	CloseHandle(hToken);

	return FALSE; 
}

BOOL cInjector::InjectDLL(DWORD dwPid, wstring wDllName) {
	HANDLE hThread;
	HANDLE hProc;
	HMODULE hKernel32;
	LPVOID lpLoadLibraryW;
	LPVOID lpRemoteString;

	hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);

	if(hProc)
	{
		hKernel32 = LoadLibraryW(L"Kernel32.DLL");

		if(hKernel32)
		{
			lpLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");

			if(lpLoadLibraryW)
			{
				lpRemoteString = (LPVOID)VirtualAllocEx(hProc, NULL, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

				if(lpRemoteString)
				{
					WriteProcessMemory(hProc, lpRemoteString, wDllName.c_str(), wDllName.size() * 2, NULL);

					hThread = CreateRemoteThread(hProc, NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(lpLoadLibraryW), lpRemoteString, NULL, NULL);
					
					WaitForSingleObject(hThread, INFINITE);

					VirtualFreeEx(hProc, lpRemoteString, 0, MEM_RELEASE);
					
					CloseHandle(hThread);
				}
			}

			FreeLibrary(hKernel32);
		}

		CloseHandle(hProc);
	}

	return FALSE;
}

/*BOOL cInjector::UnloadDLL(DWORD dwPid, HMODULE hDll)
{
	HANDLE hThread;
	HANDLE hProc;
	HMODULE hKernel32;
	LPVOID lpFreeLibrary;

	hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);

	if(hProc)
	{
		hKernel32 = LoadLibraryW(L"Kernel32.DLL");

		if(hKernel32)
		{
			lpFreeLibrary = GetProcAddress(hKernel32, "FreeLibrary");

			if(lpFreeLibrary)
			{
				hThread = CreateRemoteThread(hProc, NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(lpFreeLibrary), hDll, NULL, NULL);
				
				WaitForSingleObject(hThread, INFINITE);

				CloseHandle(hThread);
			}

			FreeLibrary(hKernel32);
		}

		CloseHandle(hProc);
	}

	return FALSE;
}*/

HMODULE cInjector::GetRemoteDll(DWORD dwPid, wstring wDllName) {
	MODULEENTRY32W ModEntry;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, dwPid);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		if (error == ERROR_BAD_LENGTH || error == ERROR_PARTIAL_COPY) {
			__debugbreak();
		}
		__debugbreak();
	}


	ModEntry.dwSize = sizeof(MODULEENTRY32W);

	if (Module32FirstW(hSnapshot, &ModEntry)) {
		do {
			if (!wDllName.compare(ModEntry.szModule))
			{
				CloseHandle(hSnapshot);
				return ModEntry.hModule;
			}

			ModEntry.dwSize = sizeof(MODULEENTRY32W);
		} while (Module32NextW(hSnapshot, &ModEntry));
	}

	CloseHandle(hSnapshot);

	return NULL;
}