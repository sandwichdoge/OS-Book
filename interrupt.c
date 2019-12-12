#include "interrupt.h"
#include "pic.h"

struct cpu_state {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
    unsigned int ebp;
    unsigned int esi;
    unsigned int edi;
};

struct stack_state {
    unsigned int error_code;
    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
};

struct idt_entry idt_entries[3] = {0};

void interrupt_encode_idt_entry(unsigned int interrupt_num, unsigned int f_ptr_handler) {
    idt_entries[interrupt_num].offset_high = f_ptr_handler & 0xffff;
    idt_entries[interrupt_num].offset_low = (f_ptr_handler >> 16) & 0xffff;

    idt_entries[interrupt_num].segment_selector = 0x8; // code segment in gdt
    idt_entries[interrupt_num].reserved = 0x0;

    idt_entries[interrupt_num].type_and_attr = (1 << 7) | // P
                                                0b1110;   // 32-bit interrupt gate
}

void interrupt_init_idt() {
    // TODO add other interrupts
    interrupt_encode_idt_entry(33, (unsigned int)int_handler_33);
    
    struct idt IDT;
    IDT.address = (unsigned int)idt_entries;
    IDT.size = sizeof(idt_entries);

    lidt(&IDT); // ASM wrapper
}

void interrupt_handler(struct cpu_state cpu_state, unsigned int interrupt_num, struct stack_state stack_state) {
    // TODO handle keypress
    switch (interrupt_num) {
        case 33:    // Keyboard press
            
            pic_ack(interrupt_num);
            break;
        default:
            break;
    }
}