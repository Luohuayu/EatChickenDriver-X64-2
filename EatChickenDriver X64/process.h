#ifndef __PROCESS_H_
#define __PROCESS_H_

#include <ntddk.h>

#define _START_DEBUG

/*
	��ȡĳ�����̵�pid	
*/
ULONG Proc_GetPid(IN PEPROCESS process);
ULONG Proc_GetPidByName(IN PSTR procName);
ULONG Proc_GetCurrentPid();

/*
	��ȡĳ�����̵ľ����ļ�������������
*/
PSTR GetImageFileName(IN PEPROCESS process);

/*
	��ȡ��ǰ���̵ľ����ļ�������������
*/
PSTR GetCurrentImageFileName();

/*
	��������
*/
VOID TraverseProcess();

/*
	���̶���
*/
VOID CutProcessLink(IN PCSTR procName, IN BOOLEAN IsAll);

/*
	�жϵ�ǰ�����ǲ���ָ���Ľ���
*/
BOOLEAN IsSpecifiedProcess(IN PSTR procName);
BOOLEAN IsCE();
BOOLEAN IsOD();
BOOLEAN IsSelfDebug();
BOOLEAN IsDNFFamily();
BOOLEAN IsDNFFamilyByPid();
BOOLEAN IsDNFClient();
BOOLEAN IsDNF();
BOOLEAN IsTASLogin();
BOOLEAN IsNtQueryInformationProcess();

/*
	���ݽ�������ȡeprocess
*/
ULONG64 GetProcessByName(IN PSTR procName);

/*
	����pid��ȡeprocess
*/
ULONG GetProcessByPid(IN ULONG pid);

/*
	��ȡĳ�����̵�_HANDLE_TABLE

	win7 x32��eprocess + 0x0f4
*/
ULONG GetProcessObjectTableByName(IN PSTR procName);
ULONG GetProcessObjectTableByPid(IN ULONG pid);

/*
	��ȡ��ǰ���̵�_HANDLE_TABLE

	win7 x32��eprocess + 0x0f4
*/
ULONG GetCurrentProcessObjectTable();

NTSTATUS SetDebugPort(IN ULONG process, IN ULONG val);

/*
	��ȡ���̵�ҳĿ¼���ַ
*/
ULONG GetCurrentProcessDirectoryTableBase();
VOID SetCurrentProcessDirectoryTableBase(IN ULONG val);
ULONG GetProcessDirectoryTableBase(IN PEPROCESS process);
ULONG GetDirectoryTableBaseByPid(IN ULONG pid);
ULONG GetDirectoryTableBaseByProcNameA(IN PSTR procName);
ULONG GetDirectoryTableBaseByProcNameW(IN PWSTR procName);

/*
	��ȡ���̵�cr3
*/
ULONG GetCurrentProcessCR3();
VOID SetCurrentProcessCR3(IN ULONG cr3Val);

NTSTATUS AttachProcess(IN PEPROCESS process);
NTSTATUS UnAttachProcess(IN PEPROCESS process);

#endif
