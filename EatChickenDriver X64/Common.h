#ifndef __COMMON_H_
#define __COMMON_H_

#include <ntifs.h>

#include "Cr3Register.h"

typedef struct
{
	ULONG64		procId;				// ����ID
	PULONG64	lpBaseAddress;		// �������ַ��ʼ��
}ReadData, *PReadData;

typedef struct
{
	ULONG64		procId;				// ����ID
	PULONG64	lpBaseAddress;		// �������ַ��ʼ��
	PULONG64	lpBuffer;			// д������
	ULONG64		nSize;				// д�����ֽ�
}WriteData, *PWriteData;

// ������ֻص�����ԭ��
typedef VOID(*Type_TestCallFun)(IN ULONG64 process);

#endif