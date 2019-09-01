#if 0
#include <windows.h>
#include <stdio.h>
#include "include\detours.h"

#pragma comment( lib, "detours.lib" )

long long nQueryPerformanceFrequency_Count = 0;
extern "C" typedef BOOL(WINAPI *pfnQueryPerformanceFrequency_t)(LARGE_INTEGER *lpFrequency);
pfnQueryPerformanceFrequency_t Orig_QueryPerformanceFrequency;
extern "C" BOOL WINAPI RL_QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency) {
	++nQueryPerformanceFrequency_Count;
	BOOL ret = Orig_QueryPerformanceFrequency(lpFrequency);
	printf("QueryPerformanceFrequency()\n");
	return ret;
}

long long nQueryPerformanceCounter_Count = 0;
extern "C" typedef BOOL(WINAPI *pfnQueryPerformanceCounter_t)(LARGE_INTEGER *lpPerformanceCount);
pfnQueryPerformanceCounter_t Orig_QueryPerformanceCounter;
extern "C" BOOL WINAPI RL_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount) {
	++nQueryPerformanceCounter_Count;
	BOOL ret = Orig_QueryPerformanceCounter(lpPerformanceCount);
	//lpFrequency->QuadPart = lpFrequency->QuadPart / 4;
	printf("QueryPerformanceCounter()\n");
	return ret;
}

long long nGetTickCount_Count = 0;
extern "C" typedef DWORD (WINAPI * pfnGetTickCount_t)();
pfnGetTickCount_t Orig_GetTickCount;
extern "C" DWORD WINAPI RL_GetTickCount() {
	++nGetTickCount_Count;
	__debugbreak();
	DWORD ret = Orig_GetTickCount();
	printf("GetTickCount()\n");
	return ret;
}

long long nGetTickCount64_Count = 0;
extern "C" typedef ULONGLONG (WINAPI * pfnGetTickCount64_t)();
pfnGetTickCount64_t Orig_GetTickCount64;
extern "C" ULONGLONG WINAPI RL_GetTickCount64() {
	++nGetTickCount64_Count;
	ULONGLONG ret = Orig_GetTickCount64();
	printf("GetTickCount64()\n");
	return ret;
}

long long ntimeBeginPeriod_Count = 0;
extern "C" typedef MMRESULT(WINAPI *pfntimeBeginPeriod_t)(UINT uPeriod);
pfntimeBeginPeriod_t Orig_timeBeginPeriod;
extern "C" MMRESULT WINAPI RL_timeBeginPeriod(UINT uPeriod) {
	++ntimeBeginPeriod_Count;
	MMRESULT ret = Orig_timeBeginPeriod(uPeriod);
	printf("timeBeginPeriod()\n");
	return ret;
}

long long ntimeGetTime_Count = 0;
extern "C" typedef DWORD(WINAPI *pfntimeGetTime_t)();
pfntimeGetTime_t Orig_timeGetTime;
extern "C" DWORD WINAPI RL_timeGetTime() {
	++ntimeGetTime_Count;
	DWORD ret = Orig_timeGetTime();
	printf("timeGetTime()\n");
	return ret;
}

bool CacheOriginalFunctions() {
	HMODULE hKernel32DLL = GetModuleHandleA("kernel32.dll");
	if (!hKernel32DLL) {
		printf("Failed to load Load Library \"kernel32.dll\"\n");
		return false;
	}

	HMODULE hWinmmDLL = GetModuleHandleA("winmm.dll");
	if (!hWinmmDLL) {
		printf("Failed to load Load Library \"winmm.dll\"\n");
		return false;
	}

	Orig_QueryPerformanceFrequency = (pfnQueryPerformanceFrequency_t)GetProcAddress(hKernel32DLL, "QueryPerformanceFrequency");
	Orig_QueryPerformanceCounter = (pfnQueryPerformanceCounter_t)GetProcAddress(hKernel32DLL, "QueryPerformanceCounter");
	Orig_GetTickCount = (pfnGetTickCount_t)GetProcAddress(hKernel32DLL, "GetTickCount");
	Orig_GetTickCount64 = (pfnGetTickCount64_t)GetProcAddress(hKernel32DLL, "GetTickCount64");
	Orig_timeBeginPeriod = (pfntimeBeginPeriod_t)GetProcAddress(hWinmmDLL, "timeBeginPeriod");
	Orig_timeGetTime = (pfntimeGetTime_t)GetProcAddress(hWinmmDLL, "timeGetTime");
	if (!Orig_QueryPerformanceFrequency || !Orig_QueryPerformanceCounter || !Orig_GetTickCount || !Orig_GetTickCount64 ||
		!Orig_timeBeginPeriod || !Orig_timeGetTime) {
		printf("GetProcAddress failed caching original functions.\n");
		return false;
	}

	return true;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD dwReason, LPVOID) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
	{
		if (!CacheOriginalFunctions()) {
			return FALSE;
		}

		LONG error = DetourTransactionBegin();
		if (error != NO_ERROR) {
			printf("DetourTransactionBegin failed with error: %d.\n", error);
			return FALSE;
		}
		
		error = DetourUpdateThread(GetCurrentThread());
		if (error != NO_ERROR) {
			printf("DetourUpdateThread failed with error: %d.\n", error);
			return FALSE;
		}
		
		error = DetourAttach(&(PVOID &)Orig_QueryPerformanceFrequency, RL_QueryPerformanceFrequency);
		if (error != NO_ERROR) {
			printf("DetourAttach QueryPerformanceFrequency failed with error: %d.\n", error);
			return FALSE;
		}

		error = DetourAttach(&(PVOID &)Orig_QueryPerformanceCounter, RL_QueryPerformanceCounter);
		if (error != NO_ERROR) {
			printf("DetourAttach QueryPerformanceCounter failed with error: %d.\n", error);
			return FALSE;
		}

		/*error = DetourAttach(&(PVOID &)Orig_GetTickCount, RL_GetTickCount);
		if (error != NO_ERROR) {
			printf("DetourAttach GetTickCount failed with error: %d.\n", error);
			return FALSE;
		}*/

		/*
		error = DetourAttach(&(PVOID &)Orig_GetTickCount64, RL_GetTickCount64);
		if (error != NO_ERROR) {
			printf("DetourAttach GetTickCount64 failed with error: %d.\n", error);
			return FALSE;
		}*/

		error = DetourAttach(&(PVOID &)Orig_timeBeginPeriod, RL_timeBeginPeriod);
		if (error != NO_ERROR) {
			printf("DetourAttach timeBeginPeriod failed with error: %d.\n", error);
			return FALSE;
		}

		error = DetourAttach(&(PVOID &)Orig_timeGetTime, RL_timeGetTime);
		if (error != NO_ERROR) {
			printf("DetourAttach timeGetTime failed with error: %d.\n", error);
			return FALSE;
		}

		error = DetourTransactionCommit();
		if (error != NO_ERROR) {
			printf("DetourTransactionCommit failed with error: %d.\n", error);
			return FALSE;
		}

		break;
	}
	case DLL_PROCESS_DETACH:
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID &)Orig_QueryPerformanceFrequency, RL_QueryPerformanceFrequency);
		DetourDetach(&(PVOID &)Orig_QueryPerformanceCounter, RL_QueryPerformanceCounter);
		DetourDetach(&(PVOID &)Orig_GetTickCount, RL_GetTickCount);
		DetourDetach(&(PVOID &)Orig_GetTickCount64, RL_GetTickCount64);
		DetourDetach(&(PVOID &)Orig_timeBeginPeriod, RL_timeBeginPeriod);
		DetourDetach(&(PVOID &)Orig_timeGetTime, RL_timeGetTime);
		DetourTransactionCommit();

		printf("Call Counters:\n");
		printf("\tQueryPerformanceFrequency: %I64d\n", nQueryPerformanceFrequency_Count);
		printf("\tQueryPerformanceCounter: %I64d\n", nQueryPerformanceCounter_Count);
		printf("\tGetTickCount: %I64d\n", nGetTickCount_Count);
		printf("\tGetTickCount64: %I64d\n", nGetTickCount64_Count);
		printf("\ttimeBeginPeriod: %I64d\n", ntimeBeginPeriod_Count);
		printf("\ttimeGetTime: %I64d\n", ntimeGetTime_Count);
		break;
	}
	}

	return TRUE;
}

#endif

#include <windows.h>
#include <stdio.h>
#include "include\detours.h"

#pragma comment( lib, "detours.lib" )

BOOL(WINAPI *Orig_QueryPerformanceCounter)(LARGE_INTEGER *lpPerformanceCount) = QueryPerformanceCounter;
BOOL WINAPI New_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount) {
	printf("QueryPerformanceCounter()\n");
	return Orig_QueryPerformanceCounter(lpPerformanceCount);
}

DWORD(WINAPI * Orig_GetTickCount)() = GetTickCount;
DWORD WINAPI New_GetTickCount() {
	printf("GetTickCount()\n");
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD dwReason, LPVOID) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
	{
		LONG error = DetourTransactionBegin();
		if (error != NO_ERROR) {
			printf("DetourTransactionBegin failed with error: %d.\n", error);
			return FALSE;
		}

		error = DetourUpdateThread(::GetCurrentThread());
		if (error != NO_ERROR) {
			printf("DetourUpdateThread failed with error: %d.\n", error);
			return FALSE;
		}


		error = DetourAttach(&(PVOID &)Orig_QueryPerformanceCounter, New_QueryPerformanceCounter);
		if (error != NO_ERROR) {
			printf("DetourAttach QueryPerformanceCounter failed with error: %d.\n", error);
			return FALSE;
		}

		DetourSetIgnoreTooSmall(TRUE); // Doesn't help
		error = DetourAttach(&(PVOID &)Orig_GetTickCount, New_GetTickCount);
		if (error != NO_ERROR) {
		printf("DetourAttach GetTickCount failed with error: %d.\n", error);
		return FALSE;
		}

		error = DetourTransactionCommit();
		if (error != NO_ERROR) {
			printf("DetourTransactionCommit failed with error: %d.\n", error);
			return FALSE;
		}

		break;
	}
	case DLL_PROCESS_DETACH:
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID &)Orig_QueryPerformanceCounter, New_QueryPerformanceCounter);
		DetourDetach(&(PVOID &)Orig_GetTickCount, New_GetTickCount);
		DetourTransactionCommit();
		break;
	}
	}

	return TRUE;
}
