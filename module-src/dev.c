#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <asm/atomic.h>
#include <linux/timer.h>


MODULE_LICENSE("GPL");

const int kPs2KeyboardIrq = 1;
const int kInterruptRegisterFlags = IRQF_SHARED;
const int kPeriodInSeconds = 5;

static atomic_t key_presses_this_minute;

void* irq_cookie;

static irqreturn_t interrupt_handler(int irq, void* dev) { 
    atomic_add(1, &key_presses_this_minute);
    return IRQ_HANDLED; 
}

static int register_interrupt_handler(void) {
    int ret;

    atomic_set(&key_presses_this_minute, 0);

    irq_cookie = kmalloc(8, GFP_KERNEL);

    ret = request_irq(kPs2KeyboardIrq, interrupt_handler, kInterruptRegisterFlags, "ps2_keyboard", irq_cookie);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

static struct timer_list update_count; 

static void timer_func(struct timer_list* t);

void set_timer(void) {
    int err;
    err = mod_timer(&update_count, jiffies + msecs_to_jiffies(kPeriodInSeconds * 1000));
    if (err < 0) {
        printk(KERN_ALERT "failed to set timer: %d", err);
    }
}

static void timer_func(struct timer_list* _update_count) {
    int cnt = atomic_read(&key_presses_this_minute);
    // this is a tiny race condition, but it is harmless
    atomic_set(&key_presses_this_minute, 0);
    printk(KERN_INFO "keyboard events last minute: %d", cnt);
    set_timer();
}

static int register_timer(void) {
    timer_setup(&update_count, timer_func, 0);
    set_timer();
    return 0;
}

static int __init kbspy_mod_init(void) {
    int err;
    printk(KERN_INFO "hello world!\n");
    err = 0;//register_interrupt_handler();
    if (err) {
        printk(KERN_ALERT "unable to register keyboard handler: %d", err);
        return err;
    }
    printk(KERN_INFO "successfully installed IRQ handler");
    err = register_timer();
    if (err) {
        printk(KERN_ALERT "unable to register timer: %d", err);
        return err;
    }
    printk(KERN_INFO "successfully installed timer");
    return 0;
}

static void __exit kbspy_mod_exit(void) {
    // TODO: unregister handler
    printk(KERN_INFO "goodbye world\n");
}

module_init(kbspy_mod_init);
module_exit(kbspy_mod_exit);