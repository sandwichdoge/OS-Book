#include "utils/rwlock.h"
#include "builddef.h"
#include "cpu.h"
#include "utils/debug.h"
#include "utils/string.h"

public
void rwlock_init(struct rwlock* lock) {
    _memset(lock, 0, sizeof(*lock));
    lock->sem_count = 1;
    lock->reader_count = 0;
}

private
void _psem_wait(struct rwlock* lock) {
    spinlock_lock(&lock->sem_mtx);
    while (atomic_load(&lock->sem_count) == 0)
        ;
    atomic_fetch_sub(&lock->sem_count, 1);
    spinlock_unlock(&lock->sem_mtx);
}

private
void _psem_signal(struct rwlock* lock) {
    atomic_fetch_add(&lock->sem_count, 1);
}

public
void rwlock_read_acquire(struct rwlock* lock) {
    size_t eflags = get_flags_reg();
    lock->is_irq_enabled_when_locked = eflags & CPU_EFLAGS_IF;
    if (lock->is_irq_enabled_when_locked) {
        asm("cli");
    }
    spinlock_lock(&lock->mtx);
    lock->reader_count++;
    if (lock->reader_count == 1) {
        _psem_wait(lock);
    }
    spinlock_unlock(&lock->mtx);
}

public
void rwlock_read_release(struct rwlock* lock) {
    spinlock_lock(&lock->mtx);
    lock->reader_count--;
    if (lock->reader_count == 0) {
        _psem_signal(lock);
    }
    spinlock_unlock(&lock->mtx);
    if (lock->is_irq_enabled_when_locked) {
        asm("sti");
    }
}

public
inline void rwlock_write_acquire(struct rwlock* lock) {
    size_t eflags = get_flags_reg();
    lock->is_irq_enabled_when_locked = eflags & CPU_EFLAGS_IF;
    if (lock->is_irq_enabled_when_locked) {
        asm("cli");
    }
    _psem_wait(lock);
}

public
inline void rwlock_write_release(struct rwlock* lock) {
    _psem_signal(lock);
    if (lock->is_irq_enabled_when_locked) {
        asm("sti");
    }
}
