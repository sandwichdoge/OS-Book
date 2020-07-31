#include "drivers/acpi/acpi.h"
#include "kinfo.h"
#include "builddef.h"
#include "pageframe_alloc.h"
#include "paging.h"
#include "utils/debug.h"
#include "utils/maths.h"

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

static struct RSDT *_rsdt = NULL;
static struct XSDT *_xsdt = NULL;

int doChecksum(struct ACPISDTHeader *tableHeader) {
    unsigned char sum = 0;
    for (int i = 0; i < tableHeader->Length; i++) {
        sum += ((char *) tableHeader)[i];
    }
    return sum == 0;
}

private void map_sdt_entries() {
    int acpi_ver = acpi_get_ver();
    int entries = 0;
    if (acpi_ver == 1) {
        entries = (_rsdt->h.Length - sizeof(_rsdt->h)) / 4;
    } else if (acpi_ver == 2) {
        entries = (_xsdt->h.Length - sizeof(_xsdt->h)) / 8;
    }
    _dbg_log("SDT entries:%d..\n", entries);
    _dbg_screen("SDT entries:%d\n", entries);
    for (int i = 0; i < entries; ++i) {
        struct ACPISDTHeader *sdt = NULL;
        if (acpi_ver == 1) {
            uint32_t* tail = &_rsdt->others;
            tail += i;
            sdt = (struct ACPISDTHeader *)*tail;
        } else if (acpi_ver == 2) {
            uint64_t* tail = &_xsdt->others;
            tail += i;
            sdt = (struct ACPISDTHeader *)*tail;
        }

        // Map page before we can do anything.
        pageframe_set_page_from_addr((void*)sdt, 1);
        paging_map_page((uint32_t)sdt, (uint32_t)sdt, get_kernel_pd());
        if (!doChecksum(sdt)) {
            _dbg_screen("Invalid checksum.\n");
        }

        // Allocate exact number of pages needed.
        uint32_t start_page = (uint32_t)sdt / PAGE_SIZE;
        uint32_t end_page = ((uint32_t)sdt + sdt->Length) / PAGE_SIZE;
        uint32_t pages_to_alloc = end_page - start_page + 1;
        _dbg_screen("sdt[0x%x], start[%u], end[%u], len[%u], pages_to_alloc:%u\n", sdt, start_page, end_page, sdt->Length, pages_to_alloc);
        _dbg_log("i[%d],sdt[0x%x], signature[%s]\n", i, sdt, sdt->Signature);
        _dbg_screen("i[%d],sdt[0x%x], signature[%s]\n", i, sdt, sdt->Signature);

        pageframe_set_page_from_addr((void*)sdt, pages_to_alloc);
        for (uint32_t j = 0; j < pages_to_alloc; ++j) {
            paging_map_page((uint32_t)sdt + j * PAGE_SIZE, (uint32_t)sdt + j * PAGE_SIZE, get_kernel_pd());
        }
    }
    _dbg_screen("Done acpi init\n");
}

public struct RSDT *acpi_get_rsdt() {
    return _rsdt;
}

public struct XSDT *acpi_get_xsdt() {
    return _xsdt;
}

public int acpi_get_ver() {
    struct kinfo* kinfo = get_kernel_info();
    return kinfo->acpi_ver;
}

public void acpi_init() {
    struct kinfo* kinfo = get_kernel_info();
    int acpi_ver = kinfo->acpi_ver;
    uint32_t to_map = 0;
    if (acpi_ver == 1) {
        struct RSDPDescriptor *rsdp = (struct RSDPDescriptor*)kinfo->rsdp;
        if (kinfo->is_paging_enabled) {
            pageframe_set_page_from_addr((void*)rsdp, 1);
            paging_map_page((uint32_t)rsdp, (uint32_t)rsdp, get_kernel_pd());
        }
        _rsdt = (struct RSDT*)rsdp->RsdtAddress;
        to_map = (uint32_t)_rsdt;
    } else if (acpi_ver == 2) {
        struct RSDPDescriptor20 *rsdp2 = (struct RSDPDescriptor20*)kinfo->rsdp;
        if (kinfo->is_paging_enabled) {
            pageframe_set_page_from_addr((void*)rsdp2, 1);
            paging_map_page((uint32_t)rsdp2, (uint32_t)rsdp2, get_kernel_pd());
        }
        _xsdt = rsdp2->XsdtAddress;
        to_map = (uint32_t)_xsdt;
        _dbg_screen("ACPI %d detected, RSDP at [0x%x], signature[%s], RSDT at[0x%x]\n", acpi_ver, rsdp2, rsdp2->firstPart.Signature, rsdp2->XsdtAddress);
    }
    if (!to_map) {
        _dbg_screen("No ACPI detected\n");
        return;
    }
    //_dbg_log("ACPI %d detected, RSDP at [0x%x], signature[%s], OEMID[%s], RSDT at[0x%x]\n", acpi_ver, rsdp, rsdp->Signature, rsdp->OEMID, rsdp->RsdtAddress);
    if (kinfo->is_paging_enabled) {
        _dbg_screen("Mapping\n");
        pageframe_set_page_from_addr((void*)to_map, 1);
        paging_map_page(to_map, to_map, get_kernel_pd());
        _dbg_screen("Map sdt entries..\n");
        map_sdt_entries();
        _dbg_screen("Done mapping\n");
    }
    // TODO validate checksum
}
