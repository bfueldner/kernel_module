// morse.c: Verwendung von Dig-Output zum Morsen
//
// Mechanismen:
//  * Dateischnittstelle: open(), close(), write()
//  * GPIO's
//  * dynamisch oder statisch vergebene Major-Number
//  * Generierung eines /sys-FS-Eintrages inkl. UEVENT für udevd bzw. mdev
//


//#include <sys/types>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define MODULE_NAME "morse"

#define MORSE_INPUT 20
#define MORSE_OUTPUT
#define MORSE_MEMORY_PAGES 8
#define MORSE_MEMORY_ORDER 3

int main(int argc, char* argv[])
{
    int fd, i;
    unsigned int *kadr;
    int len = MORSE_MEMORY_PAGES * getpagesize();

    if ((fd = open("/dev/morse0", O_RDWR | O_SYNC)) < 0)
    {
        printf("Open failed\n");
        return -4;
    }

    kadr = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, 0);
    if (kadr == MAP_FAILED)
    {
        printf("Mapping failed\n");
        return -5;
    }

    for (i = 0; i < 16; i++)
    {
        printf("0x%X\n", kadr[i]);
    }


    close(fd);
    return 0;
}
