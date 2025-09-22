////////////////////////////////////
#ifndef PCIE_CONSOLE_H
#define PCIE_CONSOLE_H
#endif // !PCIE_CONSOLE

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sched.h>

////////////////////////
#if __BYTE_ORDER == _LITTLE_ENDIAN
#define ltohl(x)		(x)
#define ltohs(x)		(x)
#define htoll(x)		(x)
#define htols(x)		(x)
#elif __BYTE_ORDER == _BIG_ENDIAN
#define ltohl(x)		_bswap_32(x)
#define ltohs(x)		_bswap_16(x)
#define htoll(x)		_bswap_32(x)
#define htols(x)		_bswap_16(x)
#endif
#define MAP_SIZE (1*1024*1024)
#define MAP_MASK (MAP_SIZE -1)

class pcie_console
{
public:
	pcie_console();
	virtual ~pcie_console();
	
	
    int open_control(char* filename);
    void* mmap_control(int fd, long mapsize);
    void user_write(unsigned int address, unsigned int val);
    unsigned int user_read(unsigned int address);
    void write_device(int device, unsigned int address, unsigned int len, unsigned char* buffer);
    static void read_device(int device, uint64_t address, unsigned int len, unsigned char* buffer);
    
    void print_bytes_binary(unsigned char* data,size_t length,const char* filename);
    void timespec_sub(struct timespec* t1, const struct timespec* t2);
    void* event0_process();
    void* event1_process();
    void* event2_process();
    void* event3_process();

	//void pcie_test();
	void RawDataSample(unsigned char* SrcAddr, unsigned int DataLen, unsigned char mode);
    void pcie_test();
    void pcie_get_IQ(unsigned char* SrcAddr, unsigned int Datalen);



private:

    void* user_base;

    int h_c2h0;
    int h_h2c0;
    int start_en;
    unsigned int user_irq_ack;
};
#pragma pack()
