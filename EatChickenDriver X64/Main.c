#include <ntddk.h>

#include "process.h"

VOID DriverUnload(PDRIVER_OBJECT pDriverObj)
{
	UNREFERENCED_PARAMETER(pDriverObj);

	DbgPrint("������ж�� \n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{
	DbgPrint("��������");

	UNREFERENCED_PARAMETER(pDriverObj);
	UNREFERENCED_PARAMETER(pRegistryString);

	ULONG_PTR p = 0x07FFFFFDE354;

	

	pDriverObj->DriverUnload = DriverUnload;

	return STATUS_SUCCESS;
}