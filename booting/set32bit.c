#include <global32bit.h>

static uint32 count_phy_memory_size(void);
static void printnumber(uint8 num);

void prepair_jumping_64bit(void)
{
    global.phy_memory_size = count_phy_memory_size();
    printnumber(global.phy_memory_size);
}

/* [count physical memory size]
 * return : physical memory size (MB)
 */
static uint32 count_phy_memory_size(void)
{
    uint32 phy_memory_size = 0;
    uint32 array_size;
    volatile int8 *note;
    const int8 testing_number[] = {0x69, 0x12, 0xff, 0x00};
    
    array_size = ARRAY_SIZE(testing_number);

    /* count size until 4gb adress */
    for (phy_memory_size = 0; phy_memory_size < 4 * 1024; phy_memory_size++) {
        note = (volatile int8 *)(phy_memory_size * 1024 * 1024);
        for (uint32 i = 0; i < array_size; i++) {
            *note = testing_number[i];
            if (*note != testing_number[i])
                goto report;
        }
    }

report:
    return phy_memory_size;
}
static void printnumber(uint8 num)
{
    volatile uint8 *video = (uint8 *)0xb8000;
    *video = (num / 100) + '0';
    *(video + 2) = ((num % 100) / 10) + '0';
    *(video + 4) = (num % 10) + '0';
    *(video + 6) = 'M';
    *(video + 8) = 'B';
    *(video + 10) = ' ';
    *(video + 12) = 'M';
    *(video + 14) = 'E';
    *(video + 16) = 'M';
}
