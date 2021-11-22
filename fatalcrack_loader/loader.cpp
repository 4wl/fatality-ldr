#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include "fagality.h"
#include "steam_module.h"
#include "fatality_loader.h"

DWORD FindProcessId( const std::wstring &processName )
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof( processInfo );

	HANDLE processesSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, NULL );
	if( processesSnapshot == INVALID_HANDLE_VALUE )
		return 0;

	Process32First( processesSnapshot, &processInfo );
	if( !processName.compare( processInfo.szExeFile ) )
	{
		CloseHandle( processesSnapshot );
		return processInfo.th32ProcessID;
	}

	while( Process32Next( processesSnapshot, &processInfo ) )
	{
		if( !processName.compare( processInfo.szExeFile ) )
		{
			CloseHandle( processesSnapshot );
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle( processesSnapshot );
	return 0;
}

HANDLE get_csgo_handle( int perms = PROCESS_ALL_ACCESS ) {
	DWORD pid = FindProcessId( L"csgo.exe" );

	if( !pid )
		return INVALID_HANDLE_VALUE;

	return OpenProcess( perms, FALSE, pid );
}

HANDLE get_steam_handle( ) {
	DWORD pid = FindProcessId( L"steam.exe" );

	if( !pid )
		return INVALID_HANDLE_VALUE;

	return OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
}

bool write_memory_to_file( HANDLE hFile, LONG offset, DWORD size, LPCVOID dataBuffer )
{
	DWORD lpNumberOfBytesWritten = 0;
	DWORD retValue = 0;
	DWORD dwError = 0;

	if( ( hFile != INVALID_HANDLE_VALUE ) && dataBuffer )
	{
		retValue = SetFilePointer( hFile, offset, NULL, FILE_BEGIN );
		dwError = GetLastError( );

		if( ( retValue == INVALID_SET_FILE_POINTER ) && ( dwError != NO_ERROR ) )
		{
			return false;
		}
		else
		{
			if( WriteFile( hFile, dataBuffer, size, &lpNumberOfBytesWritten, 0 ) )
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}
}

bool write_memory_to_new_file( const CHAR *file, DWORD size, LPCVOID dataBuffer )
{
	HANDLE hFile = CreateFileA( file, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0 );

	if( hFile != INVALID_HANDLE_VALUE )
	{
		bool resultValue = write_memory_to_file( hFile, 0, size, dataBuffer );
		CloseHandle( hFile );
		return resultValue;
	}
	else
	{
		return false;
	}
}

int main( ) {
	HANDLE csgo_handle = get_csgo_handle( );
	if( csgo_handle != INVALID_HANDLE_VALUE ) {
		TerminateProcess( csgo_handle, 0 );
		MessageBoxA( 0, "Please re-open CS:GO", "", 0 );
	}

	HANDLE steam_handle = get_steam_handle( );
	if( steam_handle == INVALID_HANDLE_VALUE ) {
		MessageBoxA( 0, "Please open Steam before loading", "", 0 );
		return 1;
	}

	if( !write_memory_to_new_file( "C:/Windows/SysWOW64/ftc_dependency.dll", sizeof( fagality_dll ), fagality_dll ) ) {
		printf( "[-] failed to write to C:/Windows/SysWOW64/ftc_dependency.dll. (missing admin perms?)\n" );
		return 1;
	}

	printf( "[+] wrote to C:/Windows/SysWOW64/ftc_dependency.dll\n" );

	if( !write_memory_to_new_file( "C:/Windows/SysWOW64/ftc_steam_module.dll", sizeof( steam_module ), steam_module ) ) {
		printf( "[-] failed to write to C:/Windows/SysWOW64/ftc_steam_module.dll. (missing admin perms?)\n" );
		return 1;
	}

	printf( "[+] wrote to C:/Windows/SysWOW64/ftc_steam_module.dll\n" );

	if( !write_memory_to_new_file( "C:/Windows/SysWOW64/ftc_loader.dll", sizeof( rawData ), rawData ) ) {
		printf( "[-] failed to write to C:/Windows/SysWOW64/ftc_loader.dll. (missing admin perms?)\n" );
		return 1;
	}

	printf( "[+] wrote to C:/Windows/SysWOW64/ftc_loader.dll\n" );

	printf( "[*] waiting for steam.exe\n" );
	while( ( steam_handle = get_steam_handle( ), steam_handle == INVALID_HANDLE_VALUE ) )
		Sleep( 1000 );

	printf( "[+] found steam.exe\n" );

	char steam_mod_path[] = "C:/Windows/SysWOW64/ftc_steam_module.dll";
	void *steam_module = VirtualAllocEx( steam_handle, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	WriteProcessMemory( steam_handle, steam_module, steam_mod_path, sizeof( steam_mod_path ), nullptr );
	HANDLE steam_thread = CreateRemoteThread( steam_handle, nullptr, 0, ( LPTHREAD_START_ROUTINE )LoadLibraryA, steam_module, 0, 0 );

	printf( "[*] waiting for steam thread to finish\n" );
	WaitForSingleObject( steam_thread, INFINITE );

	printf( "[*] waiting for csgo.exe\n" );
	while( ( csgo_handle = get_csgo_handle( ), csgo_handle == INVALID_HANDLE_VALUE ) )
		Sleep( 1000 );

	printf( "[+] found csgo.exe\n" );

	Sleep( 1000 );

	char csgo1_mod_path[] = "C:/Windows/SysWOW64/ftc_loader.dll";
	void *csgo1_module = VirtualAllocEx( csgo_handle, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	WriteProcessMemory( csgo_handle, csgo1_module, csgo1_mod_path, sizeof( csgo1_mod_path ), nullptr );
	HANDLE csgo1_thread = CreateRemoteThread( csgo_handle, nullptr, 0, ( LPTHREAD_START_ROUTINE )LoadLibraryA, csgo1_module, 0, 0 );

	printf( "[*] waiting for loader thread to finish\n" );
	WaitForSingleObject( csgo1_thread, INFINITE );

	char csgo_mod_path[] = "C:/Windows/SysWOW64/ftc_dependency.dll";
	void *csgo_module = VirtualAllocEx( csgo_handle, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	WriteProcessMemory( csgo_handle, csgo_module, csgo_mod_path, sizeof( csgo_mod_path ), nullptr );
	HANDLE csgo_thread = CreateRemoteThread( csgo_handle, nullptr, 0, ( LPTHREAD_START_ROUTINE )LoadLibraryA, csgo_module, 0, 0 );

	printf( "[+] mapped dependency\n" );

	printf( "[+] done\n" );
	
	Sleep( 3000 );
	return 0;
}