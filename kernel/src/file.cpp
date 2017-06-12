/* Copyright (c) 2016, 2017 Dennis Wölfing
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

/* kernel/src/file.cpp
 * File Vnode.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dennix/stat.h>
#include <dennix/kernel/file.h>

FileVnode::FileVnode(const void* data, size_t size, mode_t mode)
        : Vnode(S_IFREG | mode) {
    this->data = (char*) malloc(size);
    memcpy(this->data, data, size);
    fileSize = size;
    mutex = KTHREAD_MUTEX_INITIALIZER;
}

FileVnode::~FileVnode() {
    free(data);
}

bool FileVnode::isSeekable() {
    return true;
}

ssize_t FileVnode::pread(void* buffer, size_t size, off_t offset) {
    AutoLock lock(&mutex);
    char* buf = (char*) buffer;

    for (size_t i = 0; i < size; i++) {
        if (offset + i >= fileSize) return i;
        buf[i] = data[offset + i];
    }

    return size;
}

ssize_t FileVnode::pwrite(const void* buffer, size_t size, off_t offset) {
    if (offset < 0) {
        errno = EINVAL;
        return -1;
    }

    AutoLock lock(&mutex);
    size_t newSize;
    if (__builtin_add_overflow(offset, size, &newSize)) {
        errno = ENOSPC;
        return -1;
    }

    if (newSize > fileSize) {
        void* newData = realloc(data, newSize);
        if (!newData) {
            errno = ENOSPC;
            return -1;
        }
        data = (char*) newData;
        fileSize = newSize;
    }

    memcpy(data + offset, buffer, size);
    return size;
}
