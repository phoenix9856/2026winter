
#include <pcie_console.h>
/* ltoh: little to host */
/* htol: little to host */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#  define ltohl(x)       (x)
#  define ltohs(x)       (x)
#  define htoll(x)       (x)
#  define htols(x)       (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#  define ltohl(x)     __bswap_32(x)
#  define ltohs(x)     __bswap_16(x)
#  define htoll(x)     __bswap_32(x)
#  define htols(x)     __bswap_16(x)
#endif
#define MAP_SIZE (1*1024*1024)//1024*1024UL
#define MAP_MASK (MAP_SIZE - 1)

//static void* user_base;
//static int h_c2h0;
//static int h_h2c0;
//static int start_en;
//unsigned int  user_irq_ack;

//IQ struct
//struct EImIQGet{
//    short Idata;
//    short Qdata;
//}

pcie_console::pcie_console()
{
    // TODO Auto-generated constructor stub
}

pcie_console::~pcie_console()
{
    // TODO Auto-generated destructor stub
}




//extern void process_data(char * data, double rf, double bandWidth);

/*?????????*/
int pcie_console::open_control(char* filename)
{
    int fd;
    fd = open(filename, O_RDWR | O_SYNC);
    if (fd == -1)
    {
        printf("open control error\n");
        return -1;
    }
    return fd;
}
/*???????????????????*/
void* pcie_console::mmap_control(int fd, long mapsize)
{
    void* vir_addr;
    vir_addr = mmap(0, mapsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    return vir_addr;
}

/*??û??????*/
void pcie_console::user_write(unsigned int address, unsigned int val)
{
    unsigned int writeval = htoll(val);
    *((unsigned int*)(user_base + address)) = writeval;
}

/*???û??????*/
unsigned int pcie_console::user_read(unsigned int address)
{
    unsigned int read_result = *((unsigned int*)(user_base + address));
    read_result = ltohl(read_result);
    return read_result;
}

/*h2c write data*/
void pcie_console::write_device(int device, unsigned int address, unsigned int len, unsigned char* buffer)
{
    lseek(device, address, SEEK_SET);
    write(device, buffer, len);
}

/*c2h read data*/
void pcie_console::read_device(int device, uint64_t address, unsigned int len, unsigned char* buffer)
{
    lseek(device, address, SEEK_SET);
    read(device, buffer, len);
}

void pcie_console::print_bytes_binary(unsigned char* data, size_t length, const char* filename) {
    FILE* output = fopen(filename, "wb");
    if (output == NULL) {
        perror(" failed to open file");
        return;
    }
    fwrite(data, sizeof(unsigned char), length, output);
    fclose(output);
}

/* Subtract timespec t2 from t1
 *
 * Both t1 and t2 must already be normalized
 * i.e. 0 <= nsec < 1000000000 */
void pcie_console::timespec_sub(struct timespec* t1, const struct timespec* t2)
{
    assert(t1->tv_nsec >= 0);
    assert(t1->tv_nsec < 1000000000);
    assert(t2->tv_nsec >= 0);
    assert(t2->tv_nsec < 1000000000);
    t1->tv_sec -= t2->tv_sec;
    t1->tv_nsec -= t2->tv_nsec;
    if (t1->tv_nsec >= 1000000000)
    {
        t1->tv_sec++;
        t1->tv_nsec -= 1000000000;
    }
    else if (t1->tv_nsec < 0)
    {
        t1->tv_sec--;
        t1->tv_nsec += 1000000000;
    }
}

/*interrupt envent_0*/
void* pcie_console::event0_process()
{
    unsigned char* buffer = NULL;
    /////////////////////////time////////////////////////
    int rc;
    struct timespec ts_start, ts_end;
    /////////////////////////time////////////////////////

    ////////////////////////////buffer///////////////////
    uint32_t offset = 0;

    unsigned char* allocated = NULL;
    posix_memalign((void**)&allocated, 4096/*alignment*/, 0x04000000 + 4096);
    assert(allocated);
    buffer = allocated + offset;
    printf("host memory buffer = %p\n", buffer);
    memset(buffer, 0, 0x04000000);//??buffer??0
    ////////////////////////////buffer///////////////////
    char filename[20];
    int file_counter = 1;
    sprintf(filename, "%d.bin", file_counter);


    int val;
    int h_event0;
    h_event0 = open("/dev/xdma0_events_0", O_RDWR | O_SYNC);
    if (h_event0 < 0)
    {
        printf("open event0 error\n");
    }
    else
    {
        printf("open event0\n");
        while (1)
        {
            if (start_en) {
                read(h_event0, &val, 4);
                if (val == 1) {
                    //printf("event_0 done!\n");
                    rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);//????????????
                    read_device(h_c2h0, 0x04000000, 0x04000000, buffer);
                    rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);	//?????????????
                    timespec_sub(&ts_end, &ts_start);		//?????????
                    user_irq_ack = 0xffffffff;
                    user_write(0x00000, user_irq_ack);//start irq
                    printf("CLOCK_MONOTONIC reports %ld.%09ld seconds (total) for last transfer ", ts_end.tv_sec, ts_end.tv_nsec);//?????????????

                    print_bytes_binary(buffer, 0x04000000, filename);
                    file_counter = file_counter + 2;
                    sprintf(filename, "%d.bin", file_counter);
                }

                //else
                //	printf("event_0 timeout!\n");
            }
            //usleep(1);
        }
        close(h_event0);
    }
}

/*interrupt envent_1*/
void* pcie_console::event1_process()
{
    unsigned char* buffer = NULL;
    /////////////////////////time////////////////////////
    int rc;
    struct timespec ts_start, ts_end;
    /////////////////////////time////////////////////////
    ////////////////////////////buffer///////////////////
    uint32_t offset = 0;

    unsigned char* allocated = NULL;
    posix_memalign((void**)&allocated, 4096/*alignment*/, 0x04000000 + 4096);
    assert(allocated);
    buffer = allocated + offset;
    printf("host memory buffer = %p\n", buffer);
    memset(buffer, 0, 0x04000000);//??buffer??0
    ////////////////////////////buffer///////////////////
    char filename[20];
    int file_counter = 2;
    sprintf(filename, "%d.bin", file_counter);

    int val;
    int h_event1;
    h_event1 = open("/dev/xdma0_events_1", O_RDWR | O_SYNC);
    if (h_event1 < 0)
    {
        printf("open event1 error\n");
    }
    else
    {
        printf("open event1\n");
        while (1)
        {
            if (start_en) {
                read(h_event1, &val, 4);
                if (val == 1) {
                    //printf("event_0 done!\n");
                    rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);//????????????
                    read_device(h_c2h0, 0x04000000, 0x04000000, buffer);
                    rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);	//?????????????
                    timespec_sub(&ts_end, &ts_start);		//?????????
                    user_irq_ack = 0xffffffff;
                    user_write(0x00000, user_irq_ack);//start irq
                    printf("CLOCK_MONOTONIC reports %ld.%09ld seconds (total) for last transfer ", ts_end.tv_sec, ts_end.tv_nsec);//?????????????

                    print_bytes_binary(buffer, 0x04000000, filename);
                    file_counter = file_counter + 2;
                    sprintf(filename, "%d.bin", file_counter);
                }

                //else
                //	printf("event_0 timeout!\n");
            }
            //usleep(1);
        }
        close(h_event1);
    }
}

/*interrupt envent_2*/
void* pcie_console::event2_process()
{
    int val;
    int h_event2;
    h_event2 = open("/dev/xdma0_events_2", O_RDWR | O_SYNC);
    if (h_event2 < 0)
    {
        printf("open event2 error\n");
    }
    else
    {
        printf("open event2\n");
        while (1)
        {
            if (start_en) {
                read(h_event2, &val, 4);
                if (val == 1)
                    printf("event_2 done!\n");
                else
                    printf("event_2 timeout!\n");
            }
            usleep(1);
        }
        close(h_event2);
    }
}

/*interrupt envent_3*/
void* pcie_console::event3_process()
{
    int val;
    int h_event3;
    h_event3 = open("/dev/xdma0_events_3", O_RDWR | O_SYNC);
    if (h_event3 < 0)
    {
        printf("open event3 error\n");
    }
    else
    {
        printf("open event3\n");
        while (1)
        {
            if (start_en) {
                read(h_event3, &val, 4);
                if (val == 1)
                    printf("event_3 done!\n");
                else
                    printf("event_3 timeout!\n");
            }
            usleep(1);
        }
        close(h_event3);
    }
}

/******************************************************
 * param1: SrcAddr, CPU memory address
 * param2: DataLen, sample data length
 * param3: mode, sample mode
******************************************************/
void pcie_console::RawDataSample(unsigned char* SrcAddr, unsigned int DataLen, unsigned char mode)
{
    //ready
    int c2h_fd;
    c2h_fd = open("/dev/xdma0_c2h_0", O_RDWR);//????xdma_h2c?????

    int h_user;
    h_user = open_control("/dev/xdma0_user");//?????????
    user_base = mmap_control(h_user, MAP_SIZE);


    unsigned int sample_start = 0;
    //0.reset FPGA logic and buffer, bit[ 0: 0]
    sample_start |= 0x00000001;//assert reset
    user_write(0x08, sample_start);
    usleep(10);
    sample_start &= ~0x00000001;//disassert reset
    user_write(0x08, sample_start);
    usleep(10);

    //1.send sample mode, bit[ 2: 1]
    sample_start |= (mode << 1);
    user_write(0x08, sample_start);//should send datalen to IQ boad??


    //2.start sample, bit[ 3: 3]
    sample_start |= 0x00000008;
    user_write(0x08, sample_start);

    //3.sample len
    user_write(0x0c, DataLen / 4);

    //4.wait sample done
    unsigned int sta = 0;
    while ((sta & 0x00000001) != 0x00000001)
    {
        sta = user_read(0x08);
    }

    //5.read IF/IQ data
    read_device(c2h_fd, 0x00000000, DataLen / 4, SrcAddr);
    read_device(c2h_fd, 0x80000000, DataLen / 4, SrcAddr + DataLen / 4);
    read_device(c2h_fd, 0x100000000, DataLen / 4, SrcAddr + DataLen / 2);
    read_device(c2h_fd, 0x180000000, DataLen / 4, SrcAddr + DataLen * 3 / 4);

    //6.end
    close(c2h_fd);
    close(h_user);
}

void pcie_console::pcie_test(void)
{
    int c2h_fd;
    c2h_fd = open("/dev/xdma0_c2h_0", O_RDWR);//????xdma_c2h?????

    int h2c_fd;
    h2c_fd = open("/dev/xdma0_h2c_0", O_RDWR);//????xdma_h2c?????

    unsigned char buf0[10086];
    unsigned int k;
    for (k = 0; k < 10086; k++) {
        buf0[k] = 0x55;
    }
    write_device(h2c_fd, 0x00000000, 10086, buf0);
    memset(buf0, 0x00, 128);
    read_device(c2h_fd, 0x00000000, 10086, buf0);
    k = 0;
    for (k = 0; k < 10086; k++) {
        if (buf0[k] != 0x55) break;
    }
    close(h2c_fd);
    close(c2h_fd);
}

void pcie_console::pcie_get_IQ(unsigned char* SrcAddr, unsigned int DataLen)
{
//    struct IQ_data_
//    {
//        short I_data;
//        short Q_data;
//    };

//    struct IQ_data_ IQ_data0[DataLen / 64][16];

//    void* ptr = NULL;
//    //ptr=IQ_data0[1][0];
//    memcpy(&IQ_data0[1][0], SrcAddr[0], 4);//sizeof(channel_A)
//    memcpy(IQ_data0[1][0].Q_data, SrcAddr[2], 1);//sizeof(channel_A)
}
