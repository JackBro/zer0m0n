////////////////////////////////////////////////////////////////////////////
//
//	zer0m0n DRIVER
//
//  Copyright 2013 Conix Security, Nicolas Correia, Adrien Chevalier
//
//  This file is part of zer0m0n.
//
//  Zer0m0n is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Zer0m0n is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Zer0m0n.  If not, see <http://www.gnu.org/licenses/>.
//
//
//	File :		utils.c
//	Abstract :	Miscenaleous functions.
//	Revision : 	v1.0
//	Author :	Adrien Chevalier & Nicolas Correia
//	Email :		adrien.chevalier@conix.fr nicolas.correia@conix.fr
//	Date :		2013-12-26	  
//	Notes : 	
//
/////////////////////////////////////////////////////////////////////////////
#include "utils.h"
#include "hook.h"
#include "main.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Description :
//		Retrieves and returns the thread identifier from its handle.
//	Parameters :
//		_in_ HANDLE hThread : Thread handle. If NULL, does NOT retrieves current thread identifier.
//	Return value :
//		ULONG : Thread Identifier.
//	TODO : test hThread == 0 ?
//	TODO : get Zw*
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG getTIDByHandle(HANDLE hThread)
{
	THREAD_BASIC_INFORMATION teb;
	UNICODE_STRING func;
	
	RtlInitUnicodeString(&func, L"ZwQueryInformationThread");
	
	ZwQueryInformationThread = MmGetSystemRoutineAddress(&func);
	ZwQueryInformationThread(hThread, 0, &teb, sizeof(teb), NULL);
	
	return (ULONG)teb.ClientId.UniqueThread;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Description :
//		Retrieves and returns the process identifier from its handle.
//	Parameters :
//		_in_opt_ HANDLE hProc :	Process handle. If NULL, retrieves current process identifier.
//	Return value :
//		ULONG : -1 if an error was encountered, otherwise, process identifier.
//	TODO :
//		Place function retrieval at startup / dynamic import.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG getPIDByHandle(HANDLE hProc)
{
	PROCESS_BASIC_INFORMATION peb;
	UNICODE_STRING func;
	
	RtlInitUnicodeString(&func, L"ZwQueryInformationProcess");
	
	ZwQueryInformationProcess = MmGetSystemRoutineAddress(&func);
	if(ZwQueryInformationProcess)
	{
		if(hProc)
		{
			if(NT_SUCCESS(ZwQueryInformationProcess(hProc, 0, &peb, sizeof(PROCESS_BASIC_INFORMATION), NULL)))
				return peb.UniqueProcessId;
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Description :
//		Retrieves and returns the process name from its handle.
//	Parameters :
//		_in_opt_ HANDLE hProc : Process ID
//		_out_ PUNICODE_STRING : Caller allocated UNICODE_STRING, process name.
//	Return value :
//		NTSTATUS : STATUS_SUCCESS if no error was encountered, otherwise, relevant NTSTATUS code.
//	TODO : si pas assez de place dans le buffer donné, retourner une erreur spécifique
//	TODO : si handle invalide, retourner une erreur spécifique
//	TODO : check PAGED_CODE ?
//	TODO : ifdef debug
//	TODO : Zw*
//	TODO : check return des fonctions
//	TODO : check return du code
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS getProcNameByPID(ULONG pid, PUNICODE_STRING procName)
{
	NTSTATUS status;
	HANDLE hProcess;
	PEPROCESS eProcess = NULL;
	ULONG returnedLength;
	UNICODE_STRING func;
	PVOID buffer = NULL;
	PUNICODE_STRING imageName = NULL;

	PAGED_CODE();
	status = PsLookupProcessByProcessId((HANDLE)pid, &eProcess);
	
	if(NT_SUCCESS(status))
	{
		status = ObOpenObjectByPointer(eProcess,0, NULL, 0,0,KernelMode,&hProcess);
		if(!NT_SUCCESS(status))
			DbgPrint("ObOpenObjectByPointer Failed: %08x\n", status);
		ObDereferenceObject(eProcess);
	}
	else 
		DbgPrint("PsLookupProcessByProcessId Failed: %08x\n", status);
	
	RtlInitUnicodeString(&func, L"ZwQueryInformationProcess");
	ZwQueryInformationProcess = MmGetSystemRoutineAddress(&func);
	
	if(hProcess)
		ZwQueryInformationProcess(hProcess, ProcessImageFileName, NULL, 0, &returnedLength);
	else
		return STATUS_DATA_ERROR;
	
	buffer = ExAllocatePoolWithTag(PagedPool, returnedLength, BUF_POOL_TAG);
	if(buffer != NULL)
	{
		status = ZwQueryInformationProcess(hProcess, ProcessImageFileName, buffer, returnedLength, &returnedLength);
			
		if(NT_SUCCESS(status))
		{
			imageName = (PUNICODE_STRING)buffer;
			RtlCopyUnicodeString(procName, imageName);
		}
		ExFreePool(buffer);
		return status;
	}	
	else
		return STATUS_DATA_ERROR;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Description :
//		wcsstr case-insensitive version (scans "haystack" for "needle").
//	Parameters :
//		_in_ PWCHAR *haystack :	PWCHAR string to be scanned.
//		_in_ PWCHAR *needle :	PWCHAR string to find.
//	Return value :
//		PWCHAR : NULL if not found, otherwise "needle" first occurence pointer in "haystack".
//	Notes : http://www.codeproject.com/Articles/383185/SSE-accelerated-case-insensitive-substring-search
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
PWCHAR wcsistr(PWCHAR wcs1, PWCHAR wcs2)
{
    const wchar_t *s1, *s2;
    const wchar_t l = towlower(*wcs2);
    const wchar_t u = towupper(*wcs2);
    
    if (!*wcs2)
        return wcs1;
    
    for (; *wcs1; ++wcs1)
    {
        if (*wcs1 == l || *wcs1 == u)
        {
            s1 = wcs1 + 1;
            s2 = wcs2 + 1;
            
            while (*s1 && *s2 && towlower(*s1) == towlower(*s2))
                ++s1, ++s2;
            
            if (!*s2)
                return wcs1;
        }
    }
 
    return NULL;
} 