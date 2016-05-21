/* Copyright (c) 2016, Dennis Wölfing
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* kernel/src/kernel.cpp
 * The kernel's main function.
 */

#include <string.h>
#include <dennix/kernel/addressspace.h>
#include <dennix/kernel/log.h>
#include <dennix/kernel/physicalmemory.h>
#include <dennix/kernel/process.h>

static void startProcesses(multiboot_info* multiboot);

extern "C" void kmain(uint32_t /*magic*/, paddr_t multibootAddress) {
    Log::printf("Hello World!\n");
    AddressSpace::initialize();
    Log::printf("Address space initialized!\n");

    multiboot_info* multiboot = (multiboot_info*) kernelSpace->map(
            multibootAddress, PAGE_PRESENT | PAGE_WRITABLE);

    PhysicalMemory::initialize(multiboot);
    Log::printf("Physical Memory initialized\n");

    Process::initialize();
    startProcesses(multiboot);
    Log::printf("Processes initialized\n");
    kernelSpace->unmap((vaddr_t) multiboot);

    Interrupts::initPic();
    Interrupts::enable();
    Log::printf("Interrupts enabled!\n");

    while (true) {
        asm volatile ("hlt");
    }
}

static void startProcesses(multiboot_info* multiboot) {
    paddr_t modulesAligned = multiboot->mods_addr & ~0xFFF;
    ptrdiff_t offset = multiboot->mods_addr - modulesAligned;

    //FIXME: This assumes that the module list is in a single page.
    vaddr_t modulesPage = kernelSpace->map(modulesAligned,
            PAGE_PRESENT | PAGE_WRITABLE);

    const multiboot_mod_list* modules = (multiboot_mod_list*)
            (modulesPage + offset);
    for (size_t i = 0; i < multiboot->mods_count; i++) {
        size_t nPages = ALIGNUP(modules[i].mod_end - modules[i].mod_start,
                0x1000) / 0x1000;
        vaddr_t elf = kernelSpace->mapRange(modules[i].mod_start,
                nPages, PAGE_PRESENT);
        Process::loadELF(elf);
        kernelSpace->unmapRange(elf, nPages);
    }
    kernelSpace->unmap((vaddr_t) modulesPage);
}
