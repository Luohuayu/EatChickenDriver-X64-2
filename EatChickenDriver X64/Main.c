#include <ntifs.h>

#include "process.h"
#include "Phytools.h"
#include "FMemory.h"

//#define __CREATE_DEVIDE_

#define DEVICE_NAME L"\\Device\\SecondProcess"
#define SYMBOLICLINK_NAME L"\\??\\SecondProcess"

// 0-2047�Ǳ�����  2048��4095
#define READ_PROCESS	CTL_CODE(FILE_DEVICE_UNKNOWN,0x700,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define WRITE_PROCESS	CTL_CODE(FILE_DEVICE_UNKNOWN,0x710,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define OPER1			CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define OPER2			CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)

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
#ifdef __CREATE_DEVIDE_
	UNICODE_STRING SymbolicLinkName = { 0 };
	RtlInitUnicodeString(&SymbolicLinkName, SYMBOLICLINK_NAME);

	IoDeleteSymbolicLink(&SymbolicLinkName);
	IoDeleteDevice(pDriverObj->DeviceObject);
#endif

	DbgPrint("������ж�� \n");
}

NTSTATUS IrpCreateProc(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	DbgPrint("IrpCreateProc run... \n");

	return STATUS_SUCCESS;
}

NTSTATUS IrpCloseProc(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	DbgPrint("IrpCloseProc run... \n");

	return STATUS_SUCCESS;
}

NTSTATUS IrpDeviceContrlProc(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	PIO_STACK_LOCATION pIrpStack;
	ULONG uIoControlCode;
	PVOID pIoBuffer;
	ULONG uInLength;
	ULONG uOutLength;
	ULONG uRead;
	ULONG uWrite;

	//������ʱ������ֵ
	uRead = 0;
	uWrite = 0x12345678;

	//��ȡIRP����
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	//��ȡ������
	uIoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	//��ȡ��������ַ(���������Ļ���������һ����
	pIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
	//Ring3 �������ݵĳ���
	uInLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	//Ring0 �������ݵĳ���
	uOutLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	ReadData readData = { 0 };
	WriteData writeData = { 0 };

	/*
	1: kd> !vtop 12a5f000  34f8f4
		Amd64VtoP: Virt 00000000`0034f8f4, pagedir 12a5f000
		Amd64VtoP: PML4E 12a5f000
		Amd64VtoP: PDPE afd38000
		Amd64VtoP: PDE 4f4fc008
		Amd64VtoP: PTE 4dccba78
	*/

	switch (uIoControlCode)
	{
	case WRITE_PROCESS:
		memcpy(&writeData, pIoBuffer, uInLength);

		DbgPrint("��R3������ֽ�����0x%llx \n", uInLength);
		DbgPrint("---����ID��%d  \n", writeData.procId);
		DbgPrint("---�Ӹõ�ַ��ʼ����%llx  \n", writeData.lpBaseAddress);
		DbgPrint("---д������ݵ�ַ��0x%llx \n", writeData.lpBuffer);
		DbgPrint("---д������ݴ�С��0x%llx \n", writeData.nSize);

		if (!NT_SUCCESS(KDR_WriteProcessMemory(writeData)))
		{
			DbgPrint("д�ڴ�ʧ�� \n");
		}

		pIrp->IoStatus.Information = 0;
		status = STATUS_SUCCESS;

		break;
	case READ_PROCESS:
		memcpy(&readData, pIoBuffer, uInLength);

		DbgPrint("��R3������ֽ�����0x%llx \n", uInLength);
		DbgPrint("---����ID��%d  \n", readData.procId);
		DbgPrint("---�Ӹõ�ַ��ʼ����%llx  \n", readData.lpBaseAddress);

		DbgPrint("����R3���������ֽ�����0x%llx \n", uOutLength);

		if (!NT_SUCCESS(KDR_ReadProcessMemory(readData, pIoBuffer, uOutLength)))
		{
			DbgPrint("���ڴ�ʧ�� \n");
		}

		pIrp->IoStatus.Information = uOutLength;
		status = STATUS_SUCCESS;

		break;
	case OPER1:
		DbgPrint("IrpDeviceContrlProc -> OPER1 ...\n");

		pIrp->IoStatus.Information = 0;
		status = STATUS_SUCCESS;
		break;
	case OPER2:
		DbgPrint("IrpDeviceContrlProc -> OPER2 �����ֽ�����%d  \n", uInLength);
		DbgPrint("IrpDeviceContrlProc -> OPER2 ����ֽ�����%d  \n", uOutLength);
		//Read From Buffer
		memcpy(&uRead, pIoBuffer, uInLength);
		DbgPrint("IrpDeviceContrlProc -> OPER2 ...%s \n", uRead);

		RtlZeroMemory(pIoBuffer, uInLength);

		//Write To Buffer
		PSTRING64 str = "�Ҿ����й��ܺã��㲹���أ�";

		memcpy(pIoBuffer, str, uOutLength);
		//Set Status

		pIrp->IoStatus.Information = 2;
		status = STATUS_SUCCESS;
		break;
	}

	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}

VOID IsNotepadCallBack(IN ULONG64 process)
{
	PULONG64 pDirectoryTableBase = process + 0x28;

	DbgPrint("���Իص��ɹ���CR3=%llx \n", *pDirectoryTableBase);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("��������");

	UNREFERENCED_PARAMETER(pDriverObj);
	UNREFERENCED_PARAMETER(pRegistryString);

	ULONG64 val = 0x4fa7d847;
	ULONG64 cr3 = GetProcessCr3ByPid(1);
	
	HardwarePte a = {0};

	GetPdeByCr3(val, cr3, &a);

	DbgPrint("valid=%d \n", a.valid);
	DbgPrint("write=%d \n", a.write);
	DbgPrint("owner=%d \n", a.owner);
	DbgPrint("write_through=%d \n", a.write_through);
	DbgPrint("cache_disable=%d \n", a.cache_disable);
	DbgPrint("accessed=%d \n", a.accessed);
	DbgPrint("dirty=%d \n", a.dirty);
	DbgPrint("large_page=%d \n", a.large_page);
	
#ifdef __CREATE_DEVIDE_
	PDEVICE_OBJECT pDeviceObj = NULL;
	UNICODE_STRING Devicename;
	UNICODE_STRING SymbolicLinkName;

	RtlInitUnicodeString(&Devicename, DEVICE_NAME);

	status = IoCreateDevice(pDriverObj, 0, &Devicename, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObj);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("�����豸ʧ��!  \r\n");
		return status;
	}

	pDeviceObj->Flags |= DO_BUFFERED_IO;

	RtlInitUnicodeString(&SymbolicLinkName, SYMBOLICLINK_NAME);

	status = IoCreateSymbolicLink(&SymbolicLinkName, &Devicename);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("������������ʧ��!  \r\n");
		IoDeleteDevice(pDeviceObj);
		return status;
	}

	pDriverObj->MajorFunction[IRP_MJ_CREATE] = IrpCreateProc;
	pDriverObj->MajorFunction[IRP_MJ_CLOSE] = IrpCloseProc;
	pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpDeviceContrlProc;
#endif

	pDriverObj->DriverUnload = DriverUnload;

	return STATUS_SUCCESS;
}


//IsNotepad(IsNotepadCallBack);

//DbgBreakPoint();

//DbgPrint("CR3: 0x%p \n", GetProcessCr3ByName("notepad.exe"));


//----------
/*PVOID64 buff = ExAllocatePool(NonPagedPool, 8);
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
}*/

/*
1: kd> !vtop 4d250000  0x31f7c8
Amd64VtoP: Virt 00000000`0031f7c8, pagedir 4d250000
Amd64VtoP: PML4E 4d250000
Amd64VtoP: PDPE 4c714000
Amd64VtoP: PDE 4d118008
Amd64VtoP: PTE 4c0688f8
Amd64VtoP: Mapped phys 4d2897c8
*/

//ULONG64 cr3 = 0x221c4a000;

//ULONG64 readAddr = 0x4afe94;
//ULONG64 tmp = 0;

//if (NT_SUCCESS(GetDataVirtualAddressByCr3(readAddr, cr3, &g_dataVirAddr)))
//{
//	DbgPrint("[�޸�ǰ]GetPdePhysicsAddress=0x%llx��val=%llx \n", g_dataVirAddr, *(PULONG)g_dataVirAddr);

//	*(PULONG)g_dataVirAddr = 0x11223344;

//	//DbgPrint("[�޸ĺ�]GetPdePhysicsAddress=0x%llx��val=%llx \n", g_dataVirAddr, *(PULONG)g_dataVirAddr);
//}
//else
//{
//	DbgPrint("GetPdptePhysicsAddress fail \n");
//}

//MySteryReadMemoryByCr3(cr3, readAddr, 8, &tmp);

//IsNotepad(IsNotepadCallBack);