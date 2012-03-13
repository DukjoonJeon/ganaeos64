typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned char uint8;
typedef signed char int8;

#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof(_a[0]))

struct global_area {
    uint32 phy_memory_size;
};

extern struct global_area global;
