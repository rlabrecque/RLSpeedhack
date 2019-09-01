#include <Windows.h>
#include <iostream>
using namespace std;

double qpc_freq = 0.0;

void qpc_init() {
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li)) {
		cout << "QueryPerformanceFrequency failed!\n";
	}

	qpc_freq = double(li.QuadPart) / 1000.0;
}

long long qpc_start() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

double qpc_stop(long long start) {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - start) / qpc_freq;
}

void QueryPerformanceCounter_test() {
	qpc_init();

	auto startTime = qpc_start();
	Sleep(1000);
	auto elapsedTime = qpc_stop(startTime);
	cout << "QueryPerformanceCounter Timer 1 took: " << elapsedTime << "ms" << endl;

	startTime = qpc_start();
	Sleep(1234);
	elapsedTime = qpc_stop(startTime);
	cout << "QueryPerformanceCounter Timer 2 took: " << elapsedTime << "ms" << endl;
}

void GetTickCount_test() {
	auto startTime = GetTickCount();
	Sleep(1000);
	auto elapsedTime = GetTickCount() - startTime;
	cout << "GetTickCount Timer 1 took: " << elapsedTime << "ms" << endl;

	startTime = GetTickCount();
	Sleep(1234);
	elapsedTime = GetTickCount() - startTime;
	cout << "GetTickCount Timer 2 took: " << elapsedTime << "ms" << endl;
}

void GetTickCount64_test() {
	auto startTime = GetTickCount64();
	Sleep(1000);
	auto elapsedTime = GetTickCount64() - startTime;
	cout << "GetTickCount64 Timer 1 took: " << elapsedTime << "ms" << endl;

	startTime = GetTickCount64();
	Sleep(1234);
	elapsedTime = GetTickCount64() - startTime;
	cout << "GetTickCount64 Timer 2 took: " << elapsedTime << "ms" << endl;
}

void timeGetTime_test() {
	// Init
	auto ret = timeBeginPeriod(1);
	if (ret != TIMERR_NOERROR) {
		cout << "timeBeginPeriod failed!\n";
	}

	auto startTime = timeGetTime();
	Sleep(1000);
	auto elapsedTime = (timeGetTime() - startTime);
	cout << "timeGetTime Timer 1 took: " << elapsedTime << "ms" << endl;

	startTime = timeGetTime();
	Sleep(1234);
	elapsedTime = (timeGetTime() - startTime);
	cout << "timeGetTime Timer 2 took: " << elapsedTime << "ms" << endl;
}

void timeGetSystemTime_init() {
	auto res = timeBeginPeriod(1);
	if (res != TIMERR_NOERROR) {
		cout << "timeBeginPeriod failed!\n";
	}
}

DWORD timeGetSystemTime_start() {
	MMTIME mmtime;
	auto ret = timeGetSystemTime(&mmtime, sizeof(mmtime));
	if (ret != TIMERR_NOERROR) {
		cout << "timeGetSystemTime failed!\n";
	}

	return mmtime.u.ms;
}

DWORD timeGetSystemTime_stop(DWORD start) {
	MMTIME mmtime;
	auto ret = timeGetSystemTime(&mmtime, sizeof(mmtime));
	if (ret != TIMERR_NOERROR) {
		cout << "timeGetSystemTime failed!\n";
	}

	return mmtime.u.ms - start;
}

void timeGetSystemTime_test() {
	timeGetSystemTime_init();

	auto startTime = timeGetSystemTime_start();
	Sleep(1000);
	auto elapsedTime = timeGetSystemTime_stop(startTime);
	cout << "timeGetSystemTime Timer 1 took: " << elapsedTime << "ms" << endl;

	startTime = timeGetSystemTime_start();
	Sleep(1234);
	elapsedTime = timeGetSystemTime_stop(startTime);
	cout << "timeGetSystemTime Timer 2 took: " << elapsedTime << "ms" << endl;
}

void rdtsc_test() {
	auto startTime = __rdtsc();
	Sleep(1000);
	auto elapsedTime = (__rdtsc() - startTime) / 1;
	cout << "rdtsc Timer 1 took: " << elapsedTime << "ms" << endl;

	startTime = __rdtsc();
	Sleep(1234);
	elapsedTime = (__rdtsc() - startTime) / 1;
	cout << "rdtsc Timer 2 took: " << elapsedTime << "ms" << endl;
}

int main() {
	cout << "Starting in 5 seconds..." << endl;
	Sleep(5000);

	QueryPerformanceCounter_test();

	GetTickCount_test();

	GetTickCount64_test();

	timeGetTime_test();

	timeGetSystemTime_test();

	rdtsc_test();

	Sleep(2000);

	return 0;
}