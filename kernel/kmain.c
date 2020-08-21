#include "drivers/acpi/acpi.h"
#include "drivers/serial.h"
#include "drivers/svga.h"
#include "interrupt.h"
#include "kheap.h"
#include "kinfo.h"
#include "mmu.h"
#include "sem.h"
#include "shell.h"
#include "syscall.h"
#include "tasks.h"
#include "timer.h"
#include "panic.h"
#include "multiboot_info.h"
#include "utils/debug.h"
#include "smp.h"
#include "drivers/lapic.h"

struct semaphore s;
void test_multitask(void *done_cb) {
    _dbg_log("test start\n");
    sem_wait(&s);
    for (int i = 0; i < 4; ++i) {
        _dbg_log("[pid %u]test\n", task_getpid());
        delay(100);
    }
    void (*fp)() = done_cb;
    fp();
    sem_signal(&s);
}

void test_done_cb() { _dbg_log("Test done! Callback complete!\n"); }

void kmain(unsigned int magic, unsigned int addr) {
    // First thing first, gather all info about our hardware capabilities, store it in kinfo singleton
    serial_defconfig(SERIAL_COM1_BASE);
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        _dbg_log("Invalid mb magic:[0x%x]\n", magic);
        panic();
    }
    if (addr & 7) {
        _dbg_log("Unaligned mbi: 0x%x\n", addr);
        panic();
    }

    _dbg_log("[CPU%d]kmain\n", lapic_get_cpu_id());

    multiboot_info_init((struct multiboot_tag *)addr);
    kinfo_init();

    kheap_init();
    mmu_init();
    syscall_init();
    svga_init();
    acpi_init();

    // Setup interrupts
    interrupt_init();

    // Init SMP
    smp_init();
    
    // SMP initialized, start the scheduler
    tasks_init();

    // Perform tests
    sem_init(&s, 1);
    struct task_struct *t1 = task_new(test_multitask, (void *)test_done_cb, 1024 * 2, 10);
    struct task_struct *t2 = task_new(test_multitask, (void *)test_done_cb, 1024 * 2, 10);
    struct task_struct *t3 = task_new(test_multitask, (void *)test_done_cb, 1024 * 2, 10);
    task_detach(t1);
    task_detach(t2);

    task_new(shell_main, NULL, 4096 * 16, 10);

    asm("sti");  // Enable interrupts
    task_join(t3);
    task_yield();

    asm("hlt");
}
