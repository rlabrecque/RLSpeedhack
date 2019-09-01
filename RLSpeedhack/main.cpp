#include <windows.h>
#include <stdio.h>
#include "mhook-lib\mhook.h"

static FILE* g_pLogFile;
void print(const char* format, ...) {
	char buff[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(buff, format, args);
	va_end(args);

	fprintf(stdout, "%s", buff);
	fprintf(g_pLogFile, "%s", buff);
}

static int scaleFactor = 4;


long long nQueryPerformanceFrequency_Count = 0;
extern "C" typedef BOOL(WINAPI *pfnQueryPerformanceFrequency_t)(LARGE_INTEGER *lpFrequency);
pfnQueryPerformanceFrequency_t Orig_QueryPerformanceFrequency;
extern "C" BOOL WINAPI RL_QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency) {
	++nQueryPerformanceFrequency_Count;
	BOOL ret = Orig_QueryPerformanceFrequency(lpFrequency);
	//print("QueryPerformanceFrequency()\n");
	return ret;
}

long long nQueryPerformanceCounter_Count = 0;
extern "C" typedef BOOL(WINAPI *pfnQueryPerformanceCounter_t)(LARGE_INTEGER *lpPerformanceCount);
pfnQueryPerformanceCounter_t Orig_QueryPerformanceCounter;
extern "C" BOOL WINAPI RL_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount) {
	++nQueryPerformanceCounter_Count;
	BOOL ret = Orig_QueryPerformanceCounter(lpPerformanceCount);
	lpPerformanceCount->QuadPart /= scaleFactor;
	//print("QueryPerformanceCounter()\n");
	return ret;
}

long long nGetTickCount_Count = 0;
extern "C" typedef DWORD(WINAPI * pfnGetTickCount_t)();
pfnGetTickCount_t Orig_GetTickCount;
extern "C" DWORD WINAPI RL_GetTickCount() {
	++nGetTickCount_Count;
	DWORD ret = Orig_GetTickCount();
	ret /= scaleFactor;
	//print("GetTickCount()\n");
	return ret;
}

long long nGetTickCount64_Count = 0;
extern "C" typedef ULONGLONG(WINAPI * pfnGetTickCount64_t)();
pfnGetTickCount64_t Orig_GetTickCount64;
extern "C" ULONGLONG WINAPI RL_GetTickCount64() {
	++nGetTickCount64_Count;
	ULONGLONG ret = Orig_GetTickCount64();
	ret /= scaleFactor;
	//print("GetTickCount64()\n");
	return ret;
}

long long ntimeBeginPeriod_Count = 0;
extern "C" typedef MMRESULT(WINAPI *pfntimeBeginPeriod_t)(UINT uPeriod);
pfntimeBeginPeriod_t Orig_timeBeginPeriod;
extern "C" MMRESULT WINAPI RL_timeBeginPeriod(UINT uPeriod) {
	++ntimeBeginPeriod_Count;
	MMRESULT ret = Orig_timeBeginPeriod(uPeriod);
	//print("timeBeginPeriod()\n");
	return ret;
}

long long ntimeGetTime_Count = 0;
extern "C" typedef DWORD(WINAPI *pfntimeGetTime_t)();
pfntimeGetTime_t Orig_timeGetTime;
extern "C" DWORD WINAPI RL_timeGetTime() {
	++ntimeGetTime_Count;
	DWORD ret = Orig_timeGetTime();
	ret /= scaleFactor;
	//print("timeGetTime()\n");
	return ret;
}

bool CacheOriginalFunctions() {
	HMODULE hKernel32DLL = GetModuleHandleA("kernel32.dll");
	if (!hKernel32DLL) {
		print("Failed to load Load Library \"kernel32.dll\"\n");
		return false;
	}

	HMODULE hWinmmDLL = GetModuleHandleA("winmm.dll");
	if (!hWinmmDLL) {
		print("Failed to load Load Library \"winmm.dll\"\n");
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
		print("GetProcAddress failed caching original functions.\n");
		return false;
	}

	return true;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD dwReason, LPVOID) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
	{
		fopen_s(&g_pLogFile, "C:/RLSpeedhack.txt", "w");

		if (!CacheOriginalFunctions()) {
			return FALSE;
		}

		if (!Mhook_SetHook((PVOID*)&Orig_QueryPerformanceFrequency, RL_QueryPerformanceFrequency)) {
			print("Mhook_SetHook QueryPerformanceFrequency failed.\n");
			return FALSE;
		}

		if (!Mhook_SetHook((PVOID*)&Orig_QueryPerformanceCounter, RL_QueryPerformanceCounter)) {
			print("Mhook_SetHook QueryPerformanceCounter failed.\n");
			return FALSE;
		}

		if (!Mhook_SetHook((PVOID*)&Orig_GetTickCount, RL_GetTickCount)) {
			print("Mhook_SetHook GetTickCount failed.\n");
			return FALSE;
		}

		if (!Mhook_SetHook((PVOID*)&Orig_GetTickCount64, RL_GetTickCount64)) {
			print("Mhook_SetHook GetTickCount64 failed.\n");
			return FALSE;
		}

		if (!Mhook_SetHook((PVOID*)&Orig_timeBeginPeriod, RL_timeBeginPeriod)) {
			print("Mhook_SetHook timeBeginPeriod failed.\n");
			return FALSE;
		}

		if (!Mhook_SetHook((PVOID*)&Orig_timeGetTime, RL_timeGetTime)) {
			print("Mhook_SetHook timeGetTime failed.\n");
			return FALSE;
		}

		break;
	}
	case DLL_PROCESS_DETACH:
	{
		Mhook_Unhook((PVOID*)&Orig_QueryPerformanceCounter);
		Mhook_Unhook((PVOID*)&Orig_GetTickCount);

		print("Call Counters:\n");
		print("\tQueryPerformanceFrequency: %I64d\n", nQueryPerformanceFrequency_Count);
		print("\tQueryPerformanceCounter: %I64d\n", nQueryPerformanceCounter_Count);
		print("\tGetTickCount: %I64d\n", nGetTickCount_Count);
		print("\tGetTickCount64: %I64d\n", nGetTickCount64_Count);
		print("\ttimeBeginPeriod: %I64d\n", ntimeBeginPeriod_Count);
		print("\ttimeGetTime: %I64d\n", ntimeGetTime_Count);

		fclose(g_pLogFile);
		break;
	}
	}

	return TRUE;
}
