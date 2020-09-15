#include "tests_common.h"
#include "ahci.h"

#define SATA_SIG_ATA    0x00000101  // SATA drive
#define SATA_SIG_ATAPI  0xEB140101  // SATAPI drive
#define SATA_SIG_SEMB   0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM 0x96690101  // Port multiplier
#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SATAPI 4
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define HBA_PORT_DET_PRESENT 3
#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PxIS_TFES   (1 << 30)
#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX    0x35
#define HBA_PxCMD_CR    (1 << 15)
#define HBA_PxCMD_FR    (1 << 14)
#define HBA_PxCMD_FRE   (1 << 4)
#define HBA_PxCMD_SUD   (1 << 1)
#define HBA_PxCMD_ST    (1 << 0)
#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

HBA_MEM* glob_abar;
// Check device type
static int get_type(HBA_PORT *port)
{
    int32_t ssts = mmio_read_32((uint64_t)&port->ssts);
//    int32_t sctl = mmio_read_32((uint64_t)&port->sctl);
    int32_t w = 100;
    int8_t ipm = (ssts >> 8) & 0x0F;
    int8_t det = ssts & 0x0F;
    mmio_write_32((uint64_t)&port->sctl, 0x10);
//    sctl = mmio_read_32((uint64_t)&port->sctl);
//    mmio_write_32((uint64_t)&port->sctl, sctl | 0x11);
    while((ssts == 0) && ( (--w) > 0)) {
	ssts = mmio_read_32((uint64_t)&port->ssts);    
	TEST_PRINT("sts: %x\n", ssts);
    }
    TEST_PRINT("sts: %x\n", mmio_read_32((uint64_t)&port->ssts));

    ipm = (ssts >> 8) & 0x0F;
    det = ssts & 0x0F;

    if (det != HBA_PORT_DET_PRESENT)    // Check drive status
        return AHCI_DEV_NULL;
    if (ipm != HBA_PORT_IPM_ACTIVE)
        return AHCI_DEV_NULL;

    switch (mmio_read_32((uint64_t)&port->sig))
    {
    case SATA_SIG_ATAPI:
        return AHCI_DEV_SATAPI;
    case SATA_SIG_SEMB:
        return AHCI_DEV_SEMB;
    case SATA_SIG_PM:
        return AHCI_DEV_PM;
    default:
        return AHCI_DEV_SATA;
    }
}


// Start command engine
void start_cmd(HBA_PORT *port)
{
    // Set FRE (bit4) and ST (bit0)
    // Wait until CR (bit15) is cleared
    while (mmio_read_32((uint64_t)&port->cmd) & HBA_PxCMD_CR)
        TEST_PRINT("cmd: %d %d\n", port->cmd, port->cmd & HBA_PxCMD_CR);
        ;
    mmio_write_32((uint64_t)&port->cmd,mmio_read_32((uint64_t)&port->cmd) | HBA_PxCMD_FRE);
    mmio_write_32((uint64_t)&port->cmd,mmio_read_32((uint64_t)&port->cmd) | HBA_PxCMD_ST);
}

// Stop command engine
void stop_cmd(HBA_PORT *port)
{
    // Clear ST (bit0)
    mmio_write_32((uint64_t)&port->cmd, mmio_read_32((uint64_t)&port->cmd) & ~HBA_PxCMD_ST);
    mmio_write_32((uint64_t)&port->cmd, mmio_read_32((uint64_t)&port->cmd) & ~HBA_PxCMD_FRE);
    TEST_PRINT("cmd before: %d\n", port->cmd);

    // Wait until FR (bit14), CR (bit15) are cleared
    while(1)
    {
        TEST_PRINT("cmd: %d\n", port->cmd);
        if (mmio_read_32((uint64_t)&port->cmd) & HBA_PxCMD_FR)
            continue;
        if (mmio_read_32((uint64_t)&port->cmd) & HBA_PxCMD_CR)
            continue;
        break;
    }
    TEST_PRINT("here.\n");
}

// in dram
#if 1 

//#define CMD_LIST_BASE 0x80020000
//#define CMD_LIST_SIZE 4096
//#define FIS_BASE 0x80030000
//#define FIS_SIZE 0x100
//#define CMD_TBL_BASE 0x80040000
//#define CMD_TBL_SIZE 0x100
//#define CMD_TBL_COUNT 32
//#define DATA_BASE 0x80050000


#define CMD_LIST_BASE 0x80200000
#define CMD_LIST_SIZE 4096
#define FIS_BASE 0x80300000
#define FIS_SIZE 4096
#define CMD_TBL_BASE 0x80400000
#define CMD_TBL_SIZE (4096*2)
#define CMD_TBL_COUNT 32
#define DATA_BASE 0x80500000
#else
// in mail box
#define CMD_LIST_BASE 0xd000
#define CMD_LIST_SIZE 0x80
#define FIS_BASE 0xdA00
#define FIS_SIZE 0x100
#define CMD_TBL_BASE 0xdc00
#define CMD_TBL_SIZE 0x100
#define CMD_TBL_COUNT 1 
#define DATA_BASE 0xe000
#endif

void port_rebase(HBA_PORT *port, int portno, struct port_data *pdata)
{
    TEST_PRINT("Port: %p\n", port);
    stop_cmd(port); // Stop command engine

    // Command list offset: 1K*portno
    // Command list entry size = 32
    // Command list entry maxim count = 32
    // Command list maxim size = 32*32 = 1K per port
    void *mapped_clb = (void*)CMD_LIST_BASE;
    memset(mapped_clb, 0, CMD_LIST_SIZE);
    mmio_write_32((uint64_t)&port->clb, (uint32_t)(uint64_t)mapped_clb);
    mmio_write_32((uint64_t)&port->clbu, 0);
    pdata->clb = mapped_clb;
//    flush_dcache_range((uintptr_t)mapped_clb, CMD_LIST_SIZE);

    // FIS offset: 32K+256*portno
    // FIS entry size = 256 bytes per port
    void *mapped_fb = (void*)FIS_BASE;
    memset(mapped_fb, 0, FIS_SIZE);
    mmio_write_32((uint64_t)&port->fb,  (uint32_t)(uint64_t)mapped_fb);
    mmio_write_32((uint64_t)&port->fbu,  0);
    pdata->fb = mapped_fb;
//    flush_dcache_range((uintptr_t)mapped_fb, FIS_SIZE);

    //cur_addr = port->fb + KERN_VMBASE;

    // Command table offset: 40K + 8K*portno
    // Command table size = 256*32 = 8K per port

    //HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(KERN_VMBASE + port->clb);
    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(mapped_clb);

    for (int i=0; i<32; i++)
    {
        cmdheader[i].prdtl = 8; // 8 prdt entries per command table
                    // 256 bytes per command table, 64+16+48+16*8
        // Command table offset: 40K + 8K*portno + cmdheader_index*256
	void *ctba_buf = (void*)(uint64_t)(CMD_TBL_BASE + i * CMD_TBL_SIZE);

	//flush_dcache_range((uintptr_t)ctba_buf, CMD_TBL_SIZE);
	memset(ctba_buf, 0, CMD_TBL_SIZE);
	pdata->ctba[i] = ctba_buf;
        cmdheader[i].ctba = (uint64_t)ctba_buf;
        cmdheader[i].ctbau = 0;
    }
    pdata->port = port;

    start_cmd(port);    // Start command engine
}

struct port_data *pdtable[32];
struct port_data pd;

struct port_data **probe_port(HBA_MEM *abar)
{
    glob_abar = abar;
    // Search disk in impelemented ports
    int32_t pi = abar->pi;
    int i = 0;
    while (i<32)
    {
        if (pi & 1)
        {
            int dt = get_type(&abar->ports[i]);
            if (dt == AHCI_DEV_SATA)
            {
		pdtable[i] = &pd;
                TEST_PRINT("SATA drive found at port %d\n", i);
                port_rebase(abar -> ports, i, pdtable[i]);
            }
            else if (dt == AHCI_DEV_SATAPI)
            {
                TEST_PRINT("SATAPI drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_SEMB)
            {
                TEST_PRINT("SEMB drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_PM)
            {
                TEST_PRINT("PM drive found at port %d\n", i);
            }
            else
            {
                TEST_PRINT("No drive found at port %d\n", i);
            }
        }

        pi >>= 1;
        i ++;
    }
    return pdtable;
}
// Find a free command list slot
int find_cmdslot(HBA_PORT *port)
{
    // If not set in SACT and CI, the slot is free
    uint32_t slots = (mmio_read_32((uint64_t)&port->sact) | mmio_read_32((uint64_t)&port->ci));
    int cmdslots = (mmio_read_32((uint64_t)&glob_abar -> cap) & 0x0f00) >> 8;
    for (int i=0; i<cmdslots; i++)
    {
        if ((slots&1) == 0)
            return i;
        slots >>= 1;
    }
    TEST_PRINT("Cannot find free command list entry\n");
    return -1;
}

uint8_t identify_sata(struct port_data *pdata, char *buf)
{
    mmio_write_32((uint64_t)&pdata->port->is,  (uint32_t)-1);       // Clear pending interrupt bits
    int spin = 0; // Spin lock timeout counter
    int slot = find_cmdslot(pdata->port);
    //uint64_t buf_phys = (uint64_t)buf - KERN_VMBASE;
    uint64_t buf_phys = (uint64_t)buf;
    //printf("%x\n", buf_phys);
    if (slot == -1)
        return 0;

    //HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*) (KERN_VMBASE + port->clb);
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*) pdata->clb;
    cmdheader += slot;
    cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t); // Command FIS size
    cmdheader->w = 0;       // Read device
    cmdheader->prdtl = (uint16_t)1;    // PRDT entries count

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*) pdata->ctba[slot];
    //HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)(KERN_VMBASE + cmdheader->ctba);
    //memset(cmdtbl, 0, sizeof(HBA_CMD_TBL) +
    //    (cmdheader->prdtl-1)*sizeof(HBA_PRDT_ENTRY));

    TEST_PRINT("cmdtbl : 0x%p\n", (void*)cmdtbl);
   // Last entry
    cmdtbl->prdt_entry[0].dba = (uint32_t) (buf_phys & 0xffffffff);
    cmdtbl->prdt_entry[0].dbau = (uint32_t) ( (buf_phys >> 32) & 0xffffffff);
    cmdtbl->prdt_entry[0].dbc = 512;   // 512 bytes per sector
    cmdtbl->prdt_entry[0].i = 1;
    TEST_PRINT("dba & dbau cnt : %x %x %x\n", cmdtbl ->prdt_entry[0].dba, cmdtbl -> prdt_entry[0].dbau, cmdtbl->prdt_entry[0].dbc);

    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    TEST_PRINT("FIS: %p \n", cmdfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = 0x0EC;

    cmdfis->lba0 = (uint8_t)0;
    cmdfis->lba1 = (uint8_t)0;
    cmdfis->lba2 = (uint8_t)0;
    cmdfis->device = 1<<6;  // LBA mode

    cmdfis->lba3 = (uint8_t)0;
    cmdfis->lba4 = (uint8_t)0;
    cmdfis->lba5 = (uint8_t)0;

    cmdfis->countl = 1;
    cmdfis->counth = 0;
    TEST_PRINT("Flush...\n");
    TEST_PRINT("TFD: %p \n", &pdata->port->tfd);
  
//    flush_dcache_range((uintptr_t)cmdfis, 4096);
    // The below loop waits until the port is no longer busy before issuing a new command
    TEST_PRINT("PORT INFO: %p %d %d\n", pdata->port, pdata->port->ci, pdata->port->tfd);
    while ((mmio_read_32((uint64_t)&pdata->port->tfd) & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
    {
        spin++;
    }
    if (spin == 1000000)
    {
        TEST_PRINT("Port is hung\n");
        return 0;
    }

    TEST_PRINT("identify...\n");
    TEST_PRINT("PORT INFO: %p %d %d\n", pdata->port, pdata->port->ci, pdata->port->tfd);

    mmio_write_32((uint64_t)&pdata->port->ci,  (1<<slot)); // Issue command
    TEST_PRINT("PORT INFO: %p %d %d\n", pdata->port, pdata->port->ci, pdata->port->tfd);
    // Wait for completion
    while (1)
    {
        // In some longer duration reads, it may be helpful to spin on the DPS bit
        // in the PxIS port field as well (1 << 5)
        // TEST_PRINT("value: %d\n", (mmio_read_32((uint64_t)&pdata->port -> ci) & (1<<slot) )  );
        if ((mmio_read_32((uint64_t)&pdata->port->ci) & (1<<slot)) == 0)
            break;
        if (mmio_read_32((uint64_t)&pdata->port->is) & HBA_PxIS_TFES)   // Task file error
        {
            TEST_PRINT("identify disk error\n");
            return 0;
        }
    }

    // Check again
    if (mmio_read_32((uint64_t)&pdata->port->is) & HBA_PxIS_TFES)
    {
        TEST_PRINT("identify disk error\n");
        return 0;
    }

    return 1;
}

uint8_t read_sata(struct port_data *pdata, uint32_t startl, uint32_t starth, uint32_t count,char *buf)
{
    mmio_write_32((uint64_t)&pdata->port->is,  (uint32_t)-1);       // Clear pending interrupt bits
    int spin = 0; // Spin lock timeout counter
    int slot = find_cmdslot(pdata->port);
    //uint64_t buf_phys = (uint64_t)buf - KERN_VMBASE;
    uint64_t buf_phys = (uint64_t)buf;
    //printf("%x\n", buf_phys);
    if (slot == -1)
        return 0;

    //HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*) (KERN_VMBASE + port->clb);
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*) pdata->clb;
    cmdheader += slot;
    cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t); // Command FIS size
    cmdheader->w = 0;       // Read device
    cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;    // PRDT entries count

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*) pdata->ctba[slot];
    //HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)(KERN_VMBASE + cmdheader->ctba);
    //memset(cmdtbl, 0, sizeof(HBA_CMD_TBL) +
    //    (cmdheader->prdtl-1)*sizeof(HBA_PRDT_ENTRY));

    TEST_PRINT("cmdtbl : 0x%p\n", (void*)cmdtbl);
    int i;
    // 8K bytes (16 sectors) per PRDT
    for (i=0; i<cmdheader->prdtl-1; i++)
    {
        cmdtbl->prdt_entry[i].dba = (uint32_t) (buf_phys & 0xffffffff);
        cmdtbl->prdt_entry[i].dbau = (uint32_t) ( ( (buf_phys) >> 32) & 0xffffffff);
        cmdtbl->prdt_entry[i].dbc = 8*1024; // 8K bytes
        cmdtbl->prdt_entry[i].i = 1;
        buf += 4*1024;  // 4K words
        count -= 16;    // 16 sectors
    }
    // Last entry
    cmdtbl->prdt_entry[i].dba = (uint32_t) (buf_phys & 0xffffffff);
    cmdtbl->prdt_entry[i].dbau = (uint32_t) ( (buf_phys >> 32) & 0xffffffff);
    cmdtbl->prdt_entry[i].dbc = count<<9;   // 512 bytes per sector
    cmdtbl->prdt_entry[i].i = 1;
    TEST_PRINT("dba & dbau cnt : %x %x %x\n", cmdtbl ->prdt_entry[i].dba, cmdtbl -> prdt_entry[i].dbau, cmdtbl->prdt_entry[i].dbc);

    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = ATA_CMD_READ_DMA_EX;

    cmdfis->lba0 = (uint8_t)startl;
    cmdfis->lba1 = (uint8_t)(startl>>8);
    cmdfis->lba2 = (uint8_t)(startl>>16);
    cmdfis->device = 1<<6;  // LBA mode

    cmdfis->lba3 = (uint8_t)(startl>>24);
    cmdfis->lba4 = (uint8_t)starth;
    cmdfis->lba5 = (uint8_t)(starth>>8);

    cmdfis->countl = (count & 0xff);
    cmdfis->counth = (count >> 8);
    //flush_dcache_range((uintptr_t)0x80100000, 4096);
    //flush_dcache_range((uintptr_t)0x80200000, 4096);
    //flush_dcache_range((uintptr_t)0x80300000, 4096);

    // The below loop waits until the port is no longer busy before issuing a new command
    while ((mmio_read_32((uint64_t)&pdata->port->tfd) & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
    {
        spin++;
    }
    if (spin == 1000000)
    {
        TEST_PRINT("Port is hung\n");
        return 0;
    }

    mmio_write_32((uint64_t)&pdata->port->ci,  (1<<slot)); // Issue command
    TEST_PRINT("PORT INFO: %p %d %d\n", pdata->port, pdata->port->ci, pdata->port->tfd);
    TEST_PRINT("Reading disk...\n");

    // Wait for completion
    while (1)
    {
        // In some longer duration reads, it may be helpful to spin on the DPS bit
        // in the PxIS port field as well (1 << 5)
        // TEST_PRINT("value: %d\n", (mmio_read_32((uint64_t)&pdata->port -> ci) & (1<<slot) )  );
        if ((mmio_read_32((uint64_t)&pdata->port->ci) & (1<<slot)) == 0)
            break;
        if (mmio_read_32((uint64_t)&pdata->port->is) & HBA_PxIS_TFES)   // Task file error
        {
            TEST_PRINT("Read disk error\n");
            return 0;
        }
    }

    // Check again
    if (mmio_read_32((uint64_t)&pdata->port->is) & HBA_PxIS_TFES)
    {
        TEST_PRINT("Read disk error\n");
        return 0;
    }

    return 1;
}

uint8_t write_sata(struct port_data *pdata, uint32_t startl, uint32_t starth, uint32_t count, char *buf)
{
    pdata->port->is = (uint32_t)-1;       // Clear pending interrupt bits
    int spin = 0; // Spin lock timeout counter
    int slot = find_cmdslot(pdata->port);
    //uint64_t buf_phys = (uint64_t)buf - KERN_VMBASE;
    uint64_t buf_phys = (uint64_t)buf;
    //printf("%x\n", buf_phys);

    //HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*) (KERN_VMBASE + port->clb);
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)pdata->clb;
    cmdheader += slot;

    cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t); // Command FIS size
    cmdheader->w = 1;       // Write device
    cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;    // PRDT entries count

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*) pdata->ctba[slot];
    //HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)(KERN_VMBASE + cmdheader->ctba);
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL) +
        (cmdheader->prdtl-1)*sizeof(HBA_PRDT_ENTRY));

    int i;
    // 8K bytes (16 sectors) per PRDT
    for (i=0; i<cmdheader->prdtl-1; i++)
    {
        cmdtbl->prdt_entry[i].dba = (uint32_t) (buf_phys & 0xffffffff);
        cmdtbl->prdt_entry[i].dbau = (uint32_t) ( ( (buf_phys) >> 32) & 0xffffffff);
        cmdtbl->prdt_entry[i].dbc = 8*1024-1; // 8K bytes
        //cmdtbl->prdt_entry[i].i = 1;
        buf += 8*1024;  // 4K words
        count -= 16;    // 16 sectors
    }
    // Last entry
    //printf("TTT%d %x %d\n", sizeof(HBA_PRDT_ENTRY), *(((uint32_t *)&cmdtbl->prdt_entry[i])+3), count);
    cmdtbl->prdt_entry[i].dba = (uint32_t) (buf_phys & 0xffffffff);
    cmdtbl->prdt_entry[i].dbau = (uint32_t) ( (buf_phys >> 32) & 0xffffffff);
    TEST_PRINT("dba & dbau: %x %x\n", cmdtbl ->prdt_entry[i].dba, cmdtbl -> prdt_entry[i].dbau);
    //printf("TT2%d %x %d\n", sizeof(HBA_PRDT_ENTRY), *(((uint32_t *)&cmdtbl->prdt_entry[i])+3), count);
    cmdtbl->prdt_entry[i].dbc = (count<<9)-1;   // 512 bytes per sector
    //printf("TT3%d %x %d\n", sizeof(HBA_PRDT_ENTRY), *(((uint32_t *)&cmdtbl->prdt_entry[i])+3), count);
    //cmdtbl->prdt_entry[i].i = 1;
    //printf("%d %x\n", sizeof(HBA_PRDT_ENTRY), *(((uint32_t *)&cmdtbl->prdt_entry[i])+3));

    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = ATA_CMD_WRITE_DMA_EX;

    cmdfis->lba0 = (uint8_t)startl;
    cmdfis->lba1 = (uint8_t)(startl>>8);
    cmdfis->lba2 = (uint8_t)(startl>>16);
    cmdfis->device = 1<<6;  // LBA mode

    cmdfis->lba3 = (uint8_t)(startl>>24);
    cmdfis->lba4 = (uint8_t)starth;
    cmdfis->lba5 = (uint8_t)(starth>>8);

    cmdfis->countl = (count & 0xff);
    cmdfis->counth = (count >> 8);
 
    // The below loop waits until the port is no longer busy before issuing a new command
    while ((pdata->port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
    {
        spin++;
    }
    if (spin == 1000000)
    {
        TEST_PRINT("Port is hung\n");
        return 0;
    }

    pdata->port->ci = (1<<slot); // Issue command
    TEST_PRINT("PORT INFO: %p %d %d\n", pdata->port, pdata->port->ci, pdata->port->tfd);

    // Wait for completion
    while (1)
    {
        TEST_PRINT("Writing disk...\n");
        // In some longer duration reads, it may be helpful to spin on the DPS bit
        // in the PxIS port field as well (1 << 5)
        TEST_PRINT("value: %d\n", (pdata->port -> ci & (1<<slot) )  );
        if ((pdata->port->ci & (1<<slot)) == 0)
            break;
        if (pdata->port->is & HBA_PxIS_TFES)   // Task file error
        {
            TEST_PRINT("Write disk error\n");
            return 0;
        }
    }

    // Check again
    if (pdata->port->is & HBA_PxIS_TFES)
    {
        ERROR("Write disk error\n");
        return 0;
    }

    return 1;
}

void test_sata(void) {
        TEST_PRINT("Probe\n");
	char temp[128];
	int  i, j;
	char *data = (char*)DATA_BASE;
	probe_port((volatile HBA_MEM *)0x2c600000);
	memset((char*)DATA_BASE, 0x55, 1024);

	flush_dcache_range((uintptr_t)DATA_BASE, 1024);
        TEST_PRINT("value: %x\n", *((uint32_t*)DATA_BASE)  );
	identify_sata(pdtable[0], (char*)DATA_BASE);
	j = 0;
	// model
	for(i=54; i < 94; i +=2) {
		temp[j] = data[i+1];
		temp[j+1] = data[i];
		j += 2;
	}
	temp[j] = ' ';
	j++;
	// serial
	for(i=20; i < 40; i +=2) {
		temp[j] = data[i+1];
		temp[j+1] = data[i];
		j += 2;
	}
	temp[j] = ' ';
	j++;
	// FW
	for(i=46; i < 54; i +=2) {
		temp[j] = data[i+1];
		temp[j+1] = data[i];
		j += 2;
	}
	temp[j] = ' ';
	j++;
	temp[j]=0;
	TEST_PRINT("ID: %a\n", temp);
	read_sata(pdtable[0], 64, 0, 2, (char*)DATA_BASE);
        TEST_PRINT("Read[0]: %x\n", *((uint32_t*)DATA_BASE)  );
}

void mmusb_on(void);
void mmusb_toNSW(void);

void prepare_mmusb(void) {
	mmusb_on();
	mmusb_toNSW();
}

TEST_DEC(prep_mmusb, prepare_mmusb, "2.1 Prepare MM_USB");
TEST_DEC(sata0, test_sata, "2.2 Test SATA0");

