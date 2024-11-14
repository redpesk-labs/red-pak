#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
int main()
{
    const size_t mb = 1024 * 1024;
    size_t sz = 1;
    void *addr = NULL;
    
    printf("allocating %luM\n", (unsigned long)sz);
    fflush(stdout);
    addr = mmap(NULL, sz * mb, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    while (addr != MAP_FAILED) {
        memset(addr,0xff,sz * mb);
        sz *= 2;
        printf("allocating %luM\n", (unsigned long)sz);
        fflush(stdout);
        addr = mremap(addr, (sz / 2) * mb, sz * mb, MREMAP_MAYMOVE);
    }
    printf("failed to allocate %luM\n", (unsigned long)sz);
    return 0;
}
