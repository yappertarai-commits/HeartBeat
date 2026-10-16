#pragma once
#include <stdint.h>
#include "../syscall/syscall.h"
#include "../proc/process.h"
#include "../drivers/ps2_keyboard.h"
#include "../drivers/ps2_mouse.h"
#include "../graphics/framebuffer.h"
#include "../graphics/compositor.h"

namespace syscalls {

inline uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t len, uint64_t, uint64_t, uint64_t) {
    if (fd != 1 && fd != 2) return 0;
    const char* str = (const char*)buf;
    for (uint64_t i = 0; i < len; i++) {
        port::outb(0xE9, str[i]);
    }
    return len;
}

inline uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t len, uint64_t, uint64_t, uint64_t) {
    if (fd != 0) return 0;
    char* dst = (char*)buf;
    uint64_t count = 0;
    while (count < len) {
        int sc = keyboard::get_scancode();
        if (sc == -1) break;
        if (!(sc & 0x80)) {
            char c = keyboard::scancode_to_ascii(sc);
            if (c && c != '\b') {
                dst[count++] = c;
            }
        }
    }
    return count;
}

inline uint64_t sys_getpid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    Process* p = proc::get_current();
    return p ? p->pid : 0;
}

inline uint64_t sys_exit(uint64_t code, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    proc::exit_process((int)code);
    return 0;
}

inline uint64_t sys_fork(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    Process* parent = proc::get_current();
    if (!parent) return (uint64_t)-1;

    int child_pid = proc::create_process(parent->name, parent->rip, true);
    if (child_pid < 0) return (uint64_t)-1;

    Process* child = &pcb.processes[child_pid - 1];
    __builtin_memcpy(child->user_stack, parent->user_stack, 4096);

    return child_pid;
}

inline uint64_t sys_gettime(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    return pit::get_ticks();
}

inline uint64_t sys_sleep(uint64_t ms, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    Process* p = proc::get_current();
    if (p) {
        p->state = PROC_WAITING;
        p->wait_target = ms;
        proc::schedule();
    }
    return 0;
}

struct fb_meta_t {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
};

inline uint64_t sys_fb_meta(uint64_t meta_ptr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    if (!meta_ptr) return (uint64_t)-1;
    fb_meta_t* m = (fb_meta_t*)meta_ptr;
    m->width = framebuffer::get_width();
    m->height = framebuffer::get_height();
    m->pitch = framebuffer::get_pitch();
    m->bpp = framebuffer::bpp;
    return 0;
}

inline uint64_t sys_fb_map(uint64_t user_addr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    Process* p = proc::get_current();
    if (!p) return (uint64_t)-1;
    uint64_t phys = (uint64_t)framebuffer::back;
    uint64_t npages = ((uint32_t)framebuffer::get_height() * (uint32_t)framebuffer::get_pitch() + 4095) / 4096;
    for (uint64_t pg = 0; pg < npages; pg++) {
        uint64_t v = user_addr + pg * 4096;
        uint64_t pa = phys + pg * 4096;
        proc::map_user_page(p->page_table, v, pa, vmm::PAGE_PRESENT | vmm::PAGE_WRITE | vmm::PAGE_USER);
    }
    return 0;
}

inline uint64_t sys_fb_flush(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t, uint64_t) {
    if (w == 0 || h == 0) {
        compositor::begin();
        compositor::mark_all();
        compositor::present();
        return 0;
    }
    compositor::begin();
    compositor::mark((int)x, (int)y, (int)w, (int)h);
    compositor::present();
    return 0;
}

struct mouse_state_t {
    int32_t x;
    int32_t y;
    uint32_t buttons;
};

inline uint64_t sys_mouse_get(uint64_t ptr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    if (!ptr) return (uint64_t)-1;
    mouse_state_t* m = (mouse_state_t*)ptr;
    m->x = mouse::cursor_x;
    m->y = mouse::cursor_y;
    uint32_t b = 0;
    if (mouse::left_pressed) b |= 1;
    if (mouse::right_pressed) b |= 2;
    if (mouse::middle_pressed) b |= 4;
    m->buttons = b;
    return 0;
}

inline void init() {
    syscall::register_handler(SYSCALL_WRITE, sys_write);
    syscall::register_handler(SYSCALL_READ, sys_read);
    syscall::register_handler(SYSCALL_GETPID, sys_getpid);
    syscall::register_handler(SYSCALL_EXIT, sys_exit);
    syscall::register_handler(SYSCALL_FORK, sys_fork);
    syscall::register_handler(SYSCALL_GETTIME, sys_gettime);
    syscall::register_handler(SYSCALL_SLEEP, sys_sleep);
    syscall::register_handler(SYSCALL_FB_META, sys_fb_meta);
    syscall::register_handler(SYSCALL_FB_MAP, sys_fb_map);
    syscall::register_handler(SYSCALL_FB_FLUSH, sys_fb_flush);
    syscall::register_handler(SYSCALL_MOUSE_GET, sys_mouse_get);
}

}
