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
		TEXT("%s failed with error %d: %s\n"),
		lpszFunction,
		dw,
		(LPTSTR *) &lpDisplayBuf);

	_ftprintf(stderr, lpDisplayBuf);

	MessageBox(
		NULL,
		(LPCTSTR) lpDisplayBuf,
		TEXT("Runasp error"),
		MB_OK);

	// Free error-handling buffer allocations.
	LocalFree(lpDisplayBuf);
}

void TrueOrExit(LPCTSTR message, BOOL call)
{
	if(!(call))
	{
		ErrorHandler(message);
		ExitProcess(0xFF000004);
	}
}

HANDLE HandleOrExit(LPCTSTR message, HANDLE call)
{
	TrueOrExit(message, call != INVALID_HANDLE_VALUE);
	return call;
}

int _tmain(int argc, _TCHAR *argv[])
{
	DWORD dwExitCode;

	SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);

	if(argc != 5)
	{
		_ftprintf(stderr, TEXT("Runasp. Usage \"%s\" <user_login> <password> <command_line> <redirect_streams>"), argv[0]);
		ExitProcess(0xFF000002);
	}

	{
		DWORD dwCwdBufLen = 500 * sizeof(TCHAR);
		LPTSTR lpCwdBuf = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, dwCwdBufLen);

		{
			GetCurrentDirectory(dwCwdBufLen, lpCwdBuf);

			_ftprintf(
				stderr,
				TEXT("Runasp arguments are:\n%s\n%s\n%s\n%s\n%s\n\nRunasp working dir is:\n%s\n\n"),
				argv[0],
				argv[1],
				argv[2],
				argv[3],
				argv[4],
				lpCwdBuf);
			fflush(stderr);
		}

		{
			JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInfo;

			ZeroMemory(
				&BasicLimitInfo,
				sizeof(BasicLimitInfo));
			QueryInformationJobObject(
				NULL,
				JobObjectBasicLimitInformation,
				&BasicLimitInfo,
				sizeof(BasicLimitInfo), NULL);

			_ftprintf(stderr, TEXT("Runasp. Runasp basic limit flags are:\n0x%08X\n\n"), BasicLimitInfo.LimitFlags);
			fflush(stderr);
		}

		{
			PROCESS_INFORMATION ProcessInformation;
			STARTUPINFO StartupInfo;

			ZeroMemory(&StartupInfo, sizeof(StartupInfo));
			StartupInfo.cb = sizeof(StartupInfo);
			StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
			if(_ttoi(argv[4]))
			{
				_ftprintf(stderr, TEXT("Runasp. Creating process. Streams are redirected\n"));
				StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
				StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
				StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
			}
			else
			{
				_ftprintf(stderr, TEXT("Runasp. Creating process. Streams are suppressed\n"));
				StartupInfo.hStdInput = NULL;
				StartupInfo.hStdOutput = NULL;
				StartupInfo.hStdError = NULL;
			}
			StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
			StartupInfo.wShowWindow = SW_HIDE;
			ZeroMemory(&ProcessInformation, sizeof(ProcessInformation));

			_ftprintf(stderr, TEXT("Runasp. Calling CreateProcessWithLogonW...\n"));
			fflush(stderr);
			TrueOrExit(TEXT("CreateProcessWithLogonW"), CreateProcessWithLogonW(
				argv[1],
				L".",
				argv[2],
				0,
				NULL,
				argv[3],
				CREATE_NO_WINDOW,
				NULL,
				lpCwdBuf,
				&StartupInfo,
				&ProcessInformation));
			_ftprintf(stderr, TEXT("Runasp. CreateProcessWithLogonW has been called. Waiting for process...\n"));
			fflush(stderr);
			_ftprintf(stderr, TEXT("Runasp. Child process has exited. WaitForSingleObject returned 0x%08X\n"), WaitForSingleObject(
				ProcessInformation.hProcess,
				INFINITE));
			fflush(stderr);

			TrueOrExit(TEXT("GetExitCodeProcess"), GetExitCodeProcess(ProcessInformation.hProcess, &dwExitCode));
			_ftprintf(stderr, TEXT("Runasp. Child process exit code is 0x%08X\n"), dwExitCode);
			fflush(stderr);
		}

		LocalFree(lpCwdBuf);
	}

	return dwExitCode;
}
