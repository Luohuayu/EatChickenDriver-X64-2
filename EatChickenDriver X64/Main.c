#include <ntifs.h>

#include "process.h"
#include "Phytools.h"
#include "FMemory.h"

VOID test1(VOID) {
	ULONG_PTR p = 0x07FFFFFDE354;

	DbgPrint("PxeOffset: 0x%p \n", MiGetPxeOffset(p));
	DbgPrint("PxeAddress: 0x%p \n", GetPxeAddress(p));
	DbgPrint("PpeAddress: 0x%p \n", GetPpeAddress(p));
	DbgPrint("PdeAddress: 0x%p \n", GetPdeAddress(p));
	DbgPrint("PteAddress: 0x%p \n", GetPteAddress(p));
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObj)
{
	UNREFERENCED_PARAMETER(pDriverObj);

	DbgPrint("������ж�� \n");
}

VOID IsNotepadCallBack(VOID)
{
	DbgPrint("���Իص��ɹ� \n");

	DbgBreakPoint();
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{
	DbgPrint("��������");

	UNREFERENCED_PARAMETER(pDriverObj);
	UNREFERENCED_PARAMETER(pRegistryString);

	//IsNotepad(IsNotepadCallBack);
	
	//DbgBreakPoint();

	//DbgPrint("CR3: 0x%p \n", GetProcessCr3ByName("notepad.exe"));


	//----------
	PVOID64 buff = ExAllocatePool(NonPagedPool, 8);
	if (NULL == buff)
	{
		DbgPrint("ExAllocatePool failed \n");

		return STATUS_UNSUCCESSFUL;
	}
	else 
	{
		RtlZeroMemory(buff, 8);

		FReadProcessMemoryByName("notepad.exe", 0x7FFFFFDE354, 8, buff);

		DbgPrint("buff: %ws \n", buff);
	}

	pDriverObj->DriverUnload = DriverUnload;

	return STATUS_SUCCESS;
}