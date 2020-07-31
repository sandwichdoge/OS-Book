#include "drivers/acpi/acpi.h"
#include "kinfo.h"
#include "builddef.h"
#include "pageframe_alloc.h"
#include "paging.h"
#include "utils/debug.h"

struct RSDPDescriptor {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
} __attribute__ ((packed));

struct RSDPDescriptor20 {
    struct RSDPDescriptor firstPart;
    
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} __attribute__ ((packed));

static struct RSDT *_rsdt;

private void map_sdt_entries() {
    int entries = (_rsdt->h.Length - sizeof(struct ACPISDTHeader)) / 4;
    _dbg_log("SDT entries:%d..\n", entries);
    for (int i = 0; i < entries; ++i) {
        uint32_t* tail = &_rsdt->others;
        tail += i;
        struct ACPISDTHeader *sdt = (struct ACPISDTHeader *)*tail;
        pageframe_set_page_from_addr((void*)sdt, 1);
        paging_map_page((uint32_t)sdt, (uint32_t)sdt, get_kernel_pd());
        _dbg_log("i[%d],sdt[0x%x], signature[%s]\n", i, sdt, sdt->Signature);
    }
}

public struct RSDT *acpi_get_rsdt() {
    return _rsdt;
}

public void acpi_init(int is_paging_enabled) {
    struct kinfo* kinfo = get_kernel_info();
    int acpi_ver = kinfo->acpi_ver;
    struct RSDPDescriptor *rsdp;
    if (acpi_ver == 1) {
        rsdp = (struct RSDPDescriptor *)kinfo->rsdp;
    } else {
        rsdp = &((struct RSDPDescriptor20 *)kinfo->rsdp)->firstPart;
    }
    if (!rsdp) {
        return;
    }
    _dbg_log("ACPI detected, RSDP at [0x%x], signature[%s], OEMID[%s], RSDT at[0x%x]\n", rsdp, rsdp->Signature, rsdp->OEMID, rsdp->RsdtAddress);
    // TODO validate checksum

    _rsdt = (struct RSDT*)rsdp->RsdtAddress;
    if (is_paging_enabled) {
        pageframe_set_page_from_addr((void*)_rsdt, 1);
        paging_map_page((uint32_t)_rsdt, (uint32_t)_rsdt, get_kernel_pd());
        map_sdt_entries(is_paging_enabled);
    }
}
