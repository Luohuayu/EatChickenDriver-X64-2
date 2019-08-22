#include "Phytools.h"

HardwarePte * NTAPI GetPxeAddress(IN PVOID VirtualAddress)
{
	return (HardwarePte *)(PXE_BASE + (MiGetPxeOffset(VirtualAddress) * 8));
}

HardwarePte * NTAPI GetPpeAddress(IN PVOID VirtualAddress)
{
	return (HardwarePte *)(PPE_BASE + (((ULONG64)VirtualAddress >> 27) & 0x1FFFF8));
}

HardwarePte * NTAPI GetPdeAddress(IN PVOID VirtualAddress)
{
	return (HardwarePte *)((((ULONG64)VirtualAddress >> 18) & 0x3FFFFFF8) - 0x904C0000000);
}

HardwarePte * NTAPI GetPteAddress(IN PVOID VirtualAddress)
{
	return (HardwarePte *)((((ULONG64)VirtualAddress >> 9) & 0x7FFFFFFFF8) - 0x98000000000);
}

void SetAddrToLineAddr(PVOID addr, ULONG64 lineAddr,ULONG sizeOfImage)
{
	//������ҳ
	HardwarePte * pxe = GetPxeAddress(addr);
	HardwarePte * ppe = GetPpeAddress(addr);
	HardwarePte * pde = GetPdeAddress(addr);
	HardwarePte * pte = GetPteAddress(addr);

	//2M��ҳ
	if (pde->large_page)
	{
		pxe->valid = 1;
		ppe->valid = 1;
		pde->valid = 1;

		pxe->owner = 1;
		ppe->owner = 1;
		pde->owner = 1;


		ppe->write = 1;
		pde->write = 1;
	}
	else
	{
		pxe->valid = 1;
		ppe->valid = 1;
		pde->valid = 1;

		pxe->owner = 1;
		ppe->owner = 1;
		pde->owner = 1;
		pte->valid = 1;
		pte->owner = 1;
		pde->write = 1;
		pte->write = 1;

		pte->no_execute = 0;
	}

	PMMPTE PxeAddress = GetPxeAddress((PVOID)lineAddr);
	PMMPTE PpeAddress = GetPpeAddress((PVOID)lineAddr);
	//PMMPTE PdeAddress = GetPdeAddress((PVOID)lineAddr);
	//PMMPTE PteAddress = GetPteAddress((PVOID)lineAddr);
	
	//������ҳ
	if (PxeAddress && PxeAddress->page_frame)
	{
		PxeAddress->user = 1;
	}
	else
	{
		memcpy(PxeAddress, pxe, sizeof(MMPTE));
		PxeAddress->user = 1;
	}

	if (PpeAddress  && PpeAddress->page_frame)
	{
		PpeAddress->user = 1;
	}
	else
	{
		memcpy(PpeAddress, ppe, sizeof(MMPTE));
		PpeAddress->user = 1;
	}

	ULONG count = (sizeOfImage % 0x200000) == 0 ? 0 : 1;
	count += sizeOfImage / 0x200000;

	for (ULONG i = 0; i < count; i++)
	{

		HardwarePte * pde1 = GetPdeAddress((PVOID)((ULONG64)addr + 0x200000 * i));
		pde1->owner = 1;
		pde1->write = 1;
		HardwarePte * pde2 = GetPdeAddress((PVOID)(lineAddr + 0x200000 * i));
		memcpy(pde2, pde1, sizeof(HardwarePte));
		pde2->no_execute = 0;
		
		//�ж� pde1�Ƿ��Ǵ�ҳ 
		if (!pde1->large_page)
		{
			//�޸�
			
			//ӳ�������ַ
			PHYSICAL_ADDRESS phyAddr = {0};
			phyAddr.QuadPart = (*(PULONG64)pde1) & 0xfffffffffffff000l;
			ULONG64 * lineaddr = MmMapIoSpace(phyAddr, PAGE_SIZE, MmNonCached);
			 
			for (int j = 0; j < 0x1FF; j++)
			{
				//HardwarePte * pte = GetPteAddress((PVOID)(((ULONG64)addr + 0x200000 * i) + (j << 3)));
				HardwarePte * pte = (HardwarePte *)(lineaddr + j);
				
				if (pte && MmIsAddressValid(pte))
				{
					pte->write = 1;
					pte->owner = 1;
					pte->no_execute = 0;
					__invlpg((PUCHAR)lineaddr);
				}
				

			}

			MmUnmapIoSpace(lineaddr, PAGE_SIZE);
		}
	}
}

ULONG64 NTAPI GetPtePhysicsAddress(IN ULONG64 address, IN PULONG64 pdeVirtualAddress)
{
	ULONG64 pdeIndex = (address >> 21) & 0x1ff;
	ULONG64 pteIndex = (address >> 12) & 0x1ff;

	ULONG64 ptePhyAddr = *(pdeVirtualAddress + pdeIndex);

	ptePhyAddr &= 0x000ffffffffff000;
	ptePhyAddr += pteIndex * 8;

	return ptePhyAddr;
}

ULONG64 NTAPI GetPteVirtualAddress(IN ULONG64 address, IN PULONG64 pdeVirtualAddress)
{
	ULONG64 ptePhyAddr = GetPtePhysicsAddress(address, pdeVirtualAddress);

	PHYSICAL_ADDRESS pa = { 0 };
	pa.LowPart = ptePhyAddr;

	ULONG64 pteVirtualAddress = MmMapIoSpace(pa, 8, MmNonCached);
	if (NULL == pteVirtualAddress)
	{
#ifdef _START_DEBUG
		DbgPrint("MmGetVirtualForPhysical ӳ��PTE���Ե�ַʧ�� \n");
#endif

		return 0;
	}

	return pteVirtualAddress;
}

ULONG64 NTAPI GetDataPhysicsAddress(IN ULONG64 address, IN PULONG64 pteVirtualAddress)
{
	ULONG64 addrOffset = address & 0xfff;

	ULONG64 phyAddr = *pteVirtualAddress;

	// NOTICE�������ַ52λ
	phyAddr &= 0x000ffffffffff000;
	phyAddr += addrOffset;

	return phyAddr;
}

ULONG64 NTAPI GetDateVirtualAddress(IN ULONG64 address, IN PULONG64 pteVirtualAddress)
{
	ULONG64 dataPhyAddr = GetDataPhysicsAddress(address, pteVirtualAddress);

	PHYSICAL_ADDRESS pa = { 0 };
	pa.LowPart = dataPhyAddr;

	ULONG64 virtualAddress = MmMapIoSpace(pa, 8, MmNonCached);
	if (NULL == virtualAddress)
	{
#ifdef _START_DEBUG
		DbgPrint("MmGetVirtualForPhysical ӳ��DATA���Ե�ַʧ�� \n");
#endif

		return 0;
	}

	return virtualAddress;
}

NTSTATUS NTAPI GetPdptePhysicsAddressByCr3(IN ULONG64 address, IN ULONG64 cr3Val, OUT PULONG64 p_phyAddr)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��֤����
	if (NULL == p_phyAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetPdptePhysicsAddress:���շ��ؽ���Ĳ���ΪNULL \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	// ӳ��CR3���Ե�ַ
	PHYSICAL_ADDRESS pa = { 0 };
	pa.LowPart = cr3Val;
	pa.HighPart = cr3Val >> 32;

	PULONG64 cr3VirAddr = MmMapIoSpace(pa, 8, MmNonCached);
	if (NULL == cr3VirAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("MmMapIoSpaceӳ��cr3���Ե�ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	// ����PML4��PDPTE index
	ULONG64 pxeIndex = (address >> 39) & 0x1ff;
	ULONG64 pdpteIndex = (address >> 30) & 0x1ff;

	ULONG64 pdptePhyAddr = *(cr3VirAddr + pxeIndex);

	pdptePhyAddr &= 0x000ffffffffff000;
	pdptePhyAddr += pdpteIndex * 8;

	// ���ؽ��
	*p_phyAddr = pdptePhyAddr;

	// �ͷ�ӳ��
	MmUnmapIoSpace(cr3VirAddr, 8);

	return status;
}

/*
	���øú���ӳ������Ե�ַ����ǵ��ͷ�
*/
NTSTATUS NTAPI GetPdpteVirtualAddressByCr3(IN ULONG64 address, IN ULONG64 cr3Val, OUT PULONG64 p_virAddr)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��֤����
	if (NULL == p_virAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetPdpteVirtualAddress:���շ��ؽ���Ĳ���ΪNULL \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	// ��ȡ�����ַ
	ULONG64 pdptePyhAddr = 0;
	if (!NT_SUCCESS(GetPdptePhysicsAddressByCr3(address, cr3Val, &pdptePyhAddr)))
	{
#ifdef _START_DEBUG
		DbgPrint("GetPdpteVirtualAddress:��ȡPDPTE�����ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

#ifdef _START_DEBUG
	DbgPrint("pdptePyhAddr=%llx \n", pdptePyhAddr);
#endif

	// ӳ�����Ե�ַ
	PHYSICAL_ADDRESS pa = { 0 };
	pa.LowPart = pdptePyhAddr;
	pa.HighPart = pdptePyhAddr >> 32;

	PULONG64 virAddr = MmMapIoSpace(pa, 8, MmNonCached);
	if (NULL == virAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetPdpteVirtualAddress:PDPTEӳ�����Ե�ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

#ifdef _START_DEBUG
	DbgPrint("virAddr=%llx \n", virAddr);
#endif

	// ���ؽ��
	*p_virAddr = virAddr;

	// �ͷţ������ͷţ��ͷ��ˣ�
	//MmUnmapIoSpace(virAddr, 8);

	return status;
}

NTSTATUS NTAPI GetPdePhysicsAddressByCr3(IN ULONG64 address, IN ULONG64 cr3Val, OUT PULONG64 p_phyAddr)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��֤����
	if (NULL == p_phyAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetPdePhysicsAddress:���շ��ؽ���Ĳ���ΪNULL \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	// ��ȡPDPTE���Ե�ַ
	ULONG64 pdpteVirAddr = 0;

	if (!NT_SUCCESS(GetPdpteVirtualAddressByCr3(address, cr3Val, &pdpteVirAddr)))
	{
#ifdef _START_DEBUG
		DbgPrint("GetPdePhysicsAddress:��ȡPDPTE���Ե�ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	};

	// ����PML4��PDPTE index
	ULONG64 pdeIndex = (address >> 21) & 0x1ff;

	ULONG64 pdePhyAddr = *(PULONG64)pdpteVirAddr;

	pdePhyAddr &= 0x000ffffffffff000;
	pdePhyAddr += pdeIndex * 8;

	// ���ؽ��
	*p_phyAddr = pdePhyAddr;

	// �ͷ�ӳ��
	MmUnmapIoSpace(pdpteVirAddr, 8);

	return status;
}

/*
	���øú���ӳ������Ե�ַ����ǵ��ͷ�
*/
NTSTATUS NTAPI GetPdeVirtualAddressByCr3(IN ULONG64 address, IN ULONG64 cr3Val, OUT PULONG64 p_virAddr)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��֤����
	if (NULL == p_virAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetPdeVirtualAddress:���շ��ؽ���Ĳ���ΪNULL \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	// ��ȡ�����ַ
	ULONG64 pdePyhAddr = 0;
	if (!NT_SUCCESS(GetPdePhysicsAddressByCr3(address, cr3Val, &pdePyhAddr)))
	{
#ifdef _START_DEBUG
		DbgPrint("GetPdeVirtualAddress:��ȡPDPTE�����ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

#ifdef _START_DEBUG
	DbgPrint("pdePyhAddr=%llx \n", pdePyhAddr);
#endif

	// ӳ�����Ե�ַ
	PHYSICAL_ADDRESS pa = { 0 };

	pa.LowPart = pdePyhAddr;
	pa.HighPart = pdePyhAddr >> 32;

	PULONG64 virAddr = MmMapIoSpace(pa, 8, MmNonCached);
	if (NULL == virAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetPdpteVirtualAddress:PDPTEӳ�����Ե�ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

#ifdef _START_DEBUG
	DbgPrint("virAddr=%llx \n", virAddr);
#endif

	// ���ؽ��
	*p_virAddr = virAddr;

	// �ͷţ������ͷţ��ͷ��ˣ�
	//MmUnmapIoSpace(virAddr, 8);

	return status;
}

NTSTATUS NTAPI GetPtePhysicsAddressByCr3(IN ULONG64 address, IN ULONG64 cr3Val, OUT PULONG64 p_phyAddr)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��֤����
	if (NULL == p_phyAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetPtePhysicsAddressByCr3:���շ��ؽ���Ĳ���ΪNULL \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	// ��ȡPDE���Ե�ַ
	ULONG64 pdeVirAddr = 0;

	if (!NT_SUCCESS(GetPdeVirtualAddressByCr3(address, cr3Val, &pdeVirAddr)))
	{
#ifdef _START_DEBUG
		DbgPrint("GetPtePhysicsAddressByCr3:��ȡPDPTE���Ե�ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	};

	ULONG64 pteIndex = (address >> 12) & 0x1ff;

	ULONG64 ptePhyAddr = *(PULONG64)pdeVirAddr;

	ptePhyAddr &= 0x000ffffffffff000;
	ptePhyAddr += pteIndex * 8;

	// ���ؽ��
	*p_phyAddr = ptePhyAddr;

	// �ͷ�ӳ��
	MmUnmapIoSpace(pdeVirAddr, 8);

	return status;
}

/*
	���øú���ӳ������Ե�ַ����ǵ��ͷ�
*/
NTSTATUS NTAPI GetPteVirtualAddressByCr3(IN ULONG64 address, IN ULONG64 cr3Val, OUT PULONG64 p_virAddr)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��֤����
	if (NULL == p_virAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetPteVirtualAddressByCr3:���շ��ؽ���Ĳ���ΪNULL \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	// ��ȡ�����ַ
	ULONG64 ptePyhAddr = 0;
	if (!NT_SUCCESS(GetPtePhysicsAddressByCr3(address, cr3Val, &ptePyhAddr)))
	{
#ifdef _START_DEBUG
		DbgPrint("GetPteVirtualAddressByCr3:��ȡPTE�����ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

#ifdef _START_DEBUG
	DbgPrint("ptePyhAddr=%llx \n", ptePyhAddr);
#endif

	// ӳ�����Ե�ַ
	PHYSICAL_ADDRESS pa = { 0 };

	pa.LowPart = ptePyhAddr;
	pa.HighPart = ptePyhAddr >> 32;

	PULONG64 virAddr = MmMapIoSpace(pa, 8, MmNonCached);
	if (NULL == virAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetPteVirtualAddressByCr3:PTEӳ�����Ե�ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

#ifdef _START_DEBUG
	DbgPrint("virAddr=%llx \n", virAddr);
#endif

	// ���ؽ��
	*p_virAddr = virAddr;

	return status;
}

NTSTATUS NTAPI GetDataPhysicsAddressByCr3(IN ULONG64 address, IN ULONG64 cr3Val, OUT PULONG64 p_phyAddr)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��֤����
	if (NULL == p_phyAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetDataPhysicsAddressByCr3:���շ��ؽ���Ĳ���ΪNULL \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	// ��ȡPDE���Ե�ַ
	ULONG64 pteVirAddr = 0;

	if (!NT_SUCCESS(GetPteVirtualAddressByCr3(address, cr3Val, &pteVirAddr)))
	{
#ifdef _START_DEBUG
		DbgPrint("GetDataPhysicsAddressByCr3:��ȡPDPTE���Ե�ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	};

	ULONG64 addrOffset = address & 0xfff;

	ULONG64 phyAddr = *(PULONG64)pteVirAddr;

	// NOTICE�������ַ52λ
	phyAddr &= 0x000ffffffffff000;
	phyAddr += addrOffset;

	// ���ؽ��
	*p_phyAddr = phyAddr;

	// �ͷ�ӳ��
	MmUnmapIoSpace(pteVirAddr, 8);

	return status;
}

/*
	���øú���ӳ������Ե�ַ����ǵ��ͷ�
*/
NTSTATUS NTAPI GetDataVirtualAddressByCr3(IN ULONG64 address, IN ULONG64 cr3Val, OUT PULONG64 p_virAddr)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��֤����
	if (NULL == p_virAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetDataVirtualAddressByCr3:���շ��ؽ���Ĳ���ΪNULL \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	// ��ȡ�����ַ
	ULONG64 pyhAddr = 0;
	if (!NT_SUCCESS(GetDataPhysicsAddressByCr3(address, cr3Val, &pyhAddr)))
	{
#ifdef _START_DEBUG
		DbgPrint("GetDataVirtualAddressByCr3:��ȡPTE�����ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

#ifdef _START_DEBUG
	DbgPrint("pyhAddr=%llx \n", pyhAddr);
#endif

	// ӳ�����Ե�ַ
	PHYSICAL_ADDRESS pa = { 0 };

	pa.LowPart = pyhAddr;
	pa.HighPart = pyhAddr >> 32;

	PULONG64 virAddr = MmMapIoSpace(pa, 8, MmNonCached);
	if (NULL == virAddr)
	{
#ifdef _START_DEBUG
		DbgPrint("GetDataVirtualAddressByCr3:PTEӳ�����Ե�ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

#ifdef _START_DEBUG
	DbgPrint("virAddr=%llx \n", virAddr);
#endif

	// ���ؽ��
	*p_virAddr = virAddr;

	return status;
}

// δ����
NTSTATUS NTAPI MySteryReadMemoryByCr3(
	IN ULONG64 cr3Val, 
	IN PVOID64 readAddr,
	IN ULONG64 readSize, 
	OUT PVOID64 p_buff)
{
	NTSTATUS status = STATUS_SUCCESS;
	PVOID64 buff = NULL;

	// ��֤����
	if (NULL == p_buff)
	{
#ifdef _START_DEBUG
		DbgPrint("MySteryReadMemoryByCr3:���շ��ؽ���Ĳ���ΪNULL \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	// �����ڴ沢��ʼ��
	buff = ExAllocatePool(NonPagedPool, readSize);
	if (NULL == buff)
	{
		DbgPrint("ExAllocatePool failed \n");

		return STATUS_UNSUCCESSFUL;
	}

	RtlZeroMemory(buff, readSize);

	// ��ȡ�µ����Ե�ַ
	ULONG64 dataAddr = 0;
	if (!NT_SUCCESS(GetDataVirtualAddressByCr3((ULONG64)readAddr, cr3Val, &dataAddr)))
	{
#ifdef _START_DEBUG
		DbgPrint("MySteryReadMemoryByCr3:��ȡPTE�����ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	//
	RtlCopyMemory(buff, readAddr, readSize);

	RtlCopyMemory(p_buff, buff, readSize);

	// �ͷ���Դ
	MmUnmapIoSpace((PVOID)dataAddr, readSize);
	ExFreePool(buff);

	return status;
}

NTSTATUS NTAPI GetPdpteByCr3(IN ULONG64 address, IN ULONG64 cr3Val, OUT PULONG64 p_pdpte)
{
	return 0;
}

NTSTATUS NTAPI GetPdeByCr3(IN ULONG64 address, IN ULONG64 cr3Val, OUT PULONG64 p_pde)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��֤����
	if (NULL == p_pde)
	{
#ifdef _START_DEBUG
		DbgPrint("GetPdeByCr3:���շ��ؽ���Ĳ���ΪNULL \n");
#endif

		return STATUS_UNSUCCESSFUL;
	}

	// ��ȡPDPTE���Ե�ַ
	ULONG64 pdpteVirAddr = 0;

	if (!NT_SUCCESS(GetPdpteVirtualAddressByCr3(address, cr3Val, &pdpteVirAddr)))
	{
#ifdef _START_DEBUG
		DbgPrint("GetPdeByCr3:��ȡPDPTE���Ե�ַʧ�� \n");
#endif

		return STATUS_UNSUCCESSFUL;
	};

	// ����PML4��PDPTE index
	ULONG64 pdeIndex = (address >> 21) & 0x1ff;

	ULONG64 pdePhyAddr = *(PULONG64)pdpteVirAddr;

	// ���ؽ��
	*p_pde = pdePhyAddr;

	// �ͷ�ӳ��
	MmUnmapIoSpace(pdpteVirAddr, 8);

	return status;
}

NTSTATUS NTAPI GetPteByCr3(IN ULONG64 address, IN ULONG64 cr3Val, OUT PULONG64 p_pte);