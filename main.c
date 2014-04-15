#include <tchar.h>
#include <Windows.h>
#include <strsafe.h>

void MakeErrorMessage(
	__in LPCTSTR lpzsFormat,
	__in LPCTSTR lpszFunction,
	__in DWORD dwErrorCode,
	__out LPTSTR *lpDisplayBuf,
	...)
{
	LPVOID lpMsgBuf;
	va_list Args;

	va_start(Args, lpDisplayBuf);

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErrorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0,
		&Args);

	*lpDisplayBuf = (LPTSTR)LocalAlloc(
		LMEM_ZEROINIT,
		(lstrlen((LPCTSTR) lpMsgBuf) + lstrlen((LPCTSTR) lpszFunction) + 40) * sizeof(TCHAR));

	StringCchPrintf(*lpDisplayBuf,
		LocalSize(*lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dwErrorCode, lpMsgBuf);

	LocalFree(lpMsgBuf);

	va_end(Args);
}

void ErrorHandler(LPCTSTR lpszFunction)
{
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	MakeErrorMessage(
		TEXT("%s failed with error %d: %s"),
		lpszFunction,
		dw,
		(LPTSTR *) &lpDisplayBuf);

	MessageBox(
		NULL,
		(LPCTSTR) lpDisplayBuf,
		TEXT("Error"),
		MB_OK);

	// Free error-handling buffer allocations.
	LocalFree(lpDisplayBuf);
}

void TrueOrExit(BOOL call)
{
	_TCHAR *message = TEXT("Error: %d");

	if(!(call))
	{
		ErrorHandler(TEXT("Fucntion"));
		ExitProcess(1);
	}
}

HANDLE HandleOrExit(HANDLE call)
{
	TrueOrExit(call != INVALID_HANDLE_VALUE);
	return call;
}

int _tmain(int argc, _TCHAR *argv[])
{
	if(argc != 4)
	{
		_tprintf(TEXT("Usage \"%s\" <user_login> <password> <program_path_not_command_line>"), argv[0]);
		ExitProcess(1);
	}

	{
		PROCESS_INFORMATION ProcessInformation;
		STARTUPINFO StartupInfo;

		ZeroMemory(&StartupInfo, sizeof(StartupInfo));
		StartupInfo.cb = sizeof(StartupInfo);
		ZeroMemory(&ProcessInformation, sizeof(ProcessInformation));

		TrueOrExit(CreateProcessWithLogonW(
			argv[1],
			L".",
			argv[2],
			0,
			NULL,
			argv[3],
			0,
			NULL,
			NULL,
			&StartupInfo,
			&ProcessInformation));

		WaitForSingleObject(
			ProcessInformation.hProcess,
			INFINITE);
	}
}