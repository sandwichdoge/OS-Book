#include "drivers/acpi/acpi.h"

#include "builddef.h"
#include "kinfo.h"
#include "multiboot_info.h"
#include "pageframe_alloc.h"
#include "paging.h"
#include "utils/debug.h"
#include "utils/hashtable.h"

static struct hashtable SDTs;

struct RSDPDescriptor {
    uint8_t Signature[8];
    uint8_t Checksum;
    uint8_t OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
} __attribute__((packed));

struct RSDPDescriptor20 {
    struct RSDPDescriptor firstPart;

    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} __attribute__((packed));

static struct RSDT* _rsdt = NULL;
static struct XSDT* _xsdt = NULL;

public
int sdt_checksum_ok(struct ACPISDTHeader* header) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < header->Length; i++) {
        sum += ((uint8_t*)header)[i];
    }
    return sum == 0;
}

public
void* acpi_get_sdt_from_sig(char* table_signature) {
    void* ret = NULL;
    hashtable_get(&SDTs, table_signature, &ret, sizeof(void*));
    return ret;
}

public
void acpi_map_sdt(void* acpi_sdt) {
    // Map 2 pages before we can do anything. 2 in case the header overflows to the next page.
    pageframe_set_page_from_addr(acpi_sdt, 2);
    paging_map_page((size_t)acpi_sdt, (size_t)acpi_sdt, get_kernel_pd());
    paging_map_page((size_t)acpi_sdt + PAGE_SIZE, (size_t)acpi_sdt + PAGE_SIZE, get_kernel_pd());

    struct ACPISDTHeader* sdt = acpi_sdt;

    // Allocate exact number of pages needed.
    uint32_t start_page = (size_t)sdt / PAGE_SIZE;
    uint32_t end_page = ((size_t)sdt + sdt->Length) / PAGE_SIZE;
    uint32_t pages_to_alloc = end_page - start_page + 1;
    _dbg_screen("sdt[0x%x], start[%u], len[%u], pages_to_alloc:%u, sig[%s]\n", sdt, start_page, sdt->Length, pages_to_alloc, sdt->Signature);
    _dbg_log("sdt[0x%x], start[%u], len[%u], pages_to_alloc:%u, sig[%s]\n", sdt, start_page, sdt->Length, pages_to_alloc, sdt->Signature);

    pageframe_set_page_from_addr((void*)sdt, pages_to_alloc);
    for (uint32_t j = 0; j < pages_to_alloc; ++j) {
        paging_map_page((uint32_t)sdt + j * PAGE_SIZE, (uint32_t)sdt + j * PAGE_SIZE, get_kernel_pd());
    }
}

private
void map_sdt_entries() {
    _dbg_screen("Map sdt entries..\n");

    hashtable_init(&SDTs, NULL, 64);
    int32_t acpi_ver = acpi_get_ver();
    int32_t entries = 0;
    if (acpi_ver == 1) {
        entries = (_rsdt->h.Length - sizeof(_rsdt->h)) / 4;
    } else if (acpi_ver == 2) {
        entries = (_xsdt->h.Length - sizeof(_xsdt->h)) / 8;
    }
    _dbg_log("SDT entries:%d..\n", entries);
    _dbg_screen("SDT entries:%d\n", entries);
    for (int32_t i = 0; i < entries; ++i) {
        _dbg_screen("Entry %d\n", i);
        struct ACPISDTHeader* sdt = NULL;
        if (acpi_ver == 1) {
            uint32_t* tail = &_rsdt->others;
            tail += i;
            sdt = (struct ACPISDTHeader*)*tail;
        } else if (acpi_ver == 2) {
            uint64_t* tail = &_xsdt->others;
            tail += i;
            sdt = (struct ACPISDTHeader*)*tail;
        }

        acpi_map_sdt(sdt);

        if (!sdt_checksum_ok(sdt)) {
            _dbg_screen("Invalid checksum.\n");
        } else {
            char sig[5];
            _memset(sig, 0, sizeof(sig));
            _memcpy(sig, sdt->Signature, 4);
            hashtable_insert(&SDTs, sig, &sdt, sizeof(sdt));
        }
    }
}

public
struct RSDT* acpi_get_rsdt() {
    return _rsdt;
}

public
struct XSDT* acpi_get_xsdt() {
    return _xsdt;
}

public
int acpi_get_ver() {
    struct multiboot_info* mbinfo = get_multiboot_info();
    return mbinfo->acpi_ver;
}

public
void acpi_init() {
    struct multiboot_info* mbinfo = get_multiboot_info();
    struct kinfo* kinfo = get_kernel_info();
    int32_t acpi_ver = mbinfo->acpi_ver;
    uint32_t to_map = 0;
    if (acpi_ver == 1) {
        struct RSDPDescriptor* rsdp = (struct RSDPDescriptor*)mbinfo->rsdp;
        if (kinfo->is_paging_enabled) {
            pageframe_set_page_from_addr((void*)rsdp, 1);
            paging_map_page((uint32_t)rsdp, (uint32_t)rsdp, get_kernel_pd());
        }
        _rsdt = (struct RSDT*)rsdp->RsdtAddress;
        to_map = (uint32_t)_rsdt;
    } else if (acpi_ver == 2) {
        struct RSDPDescriptor20* rsdp2 = (struct RSDPDescriptor20*)mbinfo->rsdp;
        if (kinfo->is_paging_enabled) {
            pageframe_set_page_from_addr((void*)rsdp2, 1);
            paging_map_page((uint32_t)rsdp2, (uint32_t)rsdp2, get_kernel_pd());
        }
        _xsdt = (struct XSDT*)rsdp2->XsdtAddress;
        to_map = (uint32_t)_xsdt;
        _dbg_screen("ACPI %d detected, RSDP at [0x%x], signature[%s], RSDT at[0x%x]\n", acpi_ver, rsdp2, rsdp2->firstPart.Signature, rsdp2->XsdtAddress);
    }
    if (!to_map) {
        _dbg_screen("No ACPI detected\n");
        return;
    }
    //_dbg_log("ACPI %d detected, RSDP at [0x%x], signature[%s], OEMID[%s], RSDT at[0x%x]\n", acpi_ver, rsdp, rsdp->Signature, rsdp->OEMID, rsdp->RsdtAddress);
    if (kinfo->is_paging_enabled) {
        acpi_map_sdt((void*)to_map);
        map_sdt_entries();
    }
    _dbg_screen("ACPI init done.\n");
    // TODO validate checksum
}
