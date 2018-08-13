#include <Instrumentation.h>
#include <Memory.h>
#include <Execution.h>
#include <Types.h>
#include <stdio.h>
int main() {

    Memory mem;

    // TEST store with stride
    unsigned long data;
    char type;
    bool dev;
/*    mem.store(0x100, 0xdeadbeef, INT32, 0); 
    mem.load(0x100, &data, &type, &dev); 
    printf("Read %p\n", data);
    mem.store(0x104, 0xcafebabe, INT32, 0); 
    mem.load(0x104, &data, &type, &dev); 
    printf("Read %p\n", data);
    mem.store(0x105, 0x12345678, INT32, 0); // address corrupted by 'fault'

    mem.load(0x100, &data, &type, &dev); 
    printf("Read %p\n", data);
    mem.load(0x104, &data, &type, &dev); 
    printf("Read %p\n", data);
    
*/

    // TEST load with stride
    mem.store(0x100, 0xdeadbeef, INT32, 0); 
    mem.load(0x100, &data, &type, &dev); 
    printf("Read %p\n", data);
    
    mem.store(0x104, 0xcafebabe, INT32, 0); 
    mem.load(0x104, &data, &type, &dev); 
    printf("Read %p\n", data);

    data = 0;
    mem.load(0x105, &data, &type, &dev); // address corrupted by 'fault'
    printf("Read %p, %s, %d\n", data, TYPE_STR[type], dev);
    data = 0;
    mem.load(0x102, &data, &type, &dev); 
    printf("Read %p, %s, %d\n", data, TYPE_STR[type], dev);
    printf("uint32_t %p\n", *((unsigned int*)&data));
    data = 0;
    mem.load(0x100, &data, &type, &dev); 
    printf("Read %p, %s, %d\n", data, TYPE_STR[type], dev);

    return 0;
}
