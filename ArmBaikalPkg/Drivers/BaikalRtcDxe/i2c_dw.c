/*
origin:  arm-tf/plat/baikal/drivers
*/

#include <stdint.h>
#include <Uefi.h>
#include <Uefi/UefiBaseType.h>

#define READ_I2C_REG(r)		(*((volatile unsigned int *) (r)))
#define WRITE_I2C_REG(r, v)	(*((volatile unsigned int *) (r)) = (unsigned int) v)
#define BIT(n)			(1 << (n))

/* base */
#define BAIKAL_I2C_BASE		0x20250000
#define BAIKAL_I2C_OFFSET	0x10000
#define BAIKAL_I2C_REG(p)	((void*)((intptr_t)(BAIKAL_I2C_BASE + (p)*BAIKAL_I2C_OFFSET)))


/* i2c_init_hw */
#define CONFIG_SYS_I2C_SLAVE	0x02

/* Speed Selection */
#define IC_SPEED_MODE_STANDARD	1
#define IC_SPEED_MODE_FAST	2
#define IC_SPEED_MODE_MAX	3
#define I2C_TIMEOUT		1000000

/* i2c enable register definitions */
#define IC_ENABLE_0B		0x0001
#define IC_STOP_DET		0x0200	/* (1<<9) R_STOP_DET [ic_raw_intr_stat] */
#define IC_CON_MM		0x0001	/* (1<<0) master enable [ic_con] */
#define IC_CON_SD		0x0040	/* (1<<6) slave disable */
#define IC_CON_SPD_FS		0x0004	/* (1<<2) speed */
#define IC_CLK			166
#define IC_TL0			0x00
#define IC_RX_TL		IC_TL0
#define IC_TX_TL		IC_TL0
#define CONFIG_SYS_HZ		1200
#define NANO_TO_MICRO		1000
#define MIN_FS_SCL_HIGHTIME	600
#define MIN_FS_SCL_LOWTIME	1300

#define IRQ_MASK		0

#define IC_CMD			0x0100	/* (1<<8) read=1,write=0 */
#define IC_STOP			0x0200	/* (1<<9) ? */

// ic_status
#define IC_STATUS_SA		BIT(6)
#define IC_STATUS_MA		BIT(5)
#define IC_STATUS_RFF		BIT(4)
#define IC_STATUS_RFNE		BIT(3)
#define IC_STATUS_TFE		BIT(2)
#define IC_STATUS_TFNF		BIT(1)
#define IC_STATUS_ACT		BIT(0)


// WEEKDAY
#define WEEKDAY_SUNDAY		1
#define WEEKDAY_MONDAY		2
#define WEEKDAY_TUESDAY		3
#define WEEKDAY_WEDNESDAY	4
#define WEEKDAY_THURSDAY	5
#define WEEKDAY_FRIDAY		6
#define WEEKDAY_SATURDAY	7


// BASIC_TIME
#define BASIC_YEAR_UNIX		1970
#define BASIC_YEAR_ABRACON	2000
#define BASIC_MONTH		1
#define BASIC_DAY		1
#define BASIC_WEEKDAY_UNIX	WEEKDAY_THURSDAY
#define BASIC_HOUR		0
#define BASIC_MINUTE		0
#define BASIC_SECOND		0

// time position
#define YEAR			6
#define MONTH			5
#define WEEKDAY			4
#define DAY			3
#define HOUR			2
#define MINUTE			1
#define SECOND			0

// time mask
#define MASK_YEAR		0x7f	/* 0-6 */
#define MASK_MONTH		0x1f	/* 0-4 */
#define MASK_WEEKDAY		0x7	/* 0-2 */
#define MASK_DAY		0x3f	/* 0-5 */
#define MASK_HOUR_24		0x3f	/* 0-5 */
#define MASK_HOUR_12		0x1f	/* 0-4 */
#define MASK_MINUTE		0x7f	/* 0-6 */
#define MASK_SECOND		0x7f	/* 0-6 */

#define HOUR_12			BIT(6)	/* 0-24, 1-12 */
#define HOUR_PM			BIT(5)	/* 0-AM, 1-PM */


// rtc i2c
#define I2C_RTC_BUS		0
#define I2C_RTC_ADR		0x56
#define RTC_PAGE_OFFSET		3
#define RTC_WORD_OFFSET		0



// REG
struct i2c_regs {
	unsigned int ic_con;	/* adr type [7,10 bit] */
	unsigned int ic_tar;	/* target address */
	unsigned int ic_sar;	/* self  address */
	unsigned int ic_hs_maddr;
	unsigned int ic_cmd_data;	/* cmd[8] : data[7-0]  //cmd= 1-read,0-write */
	unsigned int ic_ss_scl_hcnt;
	unsigned int ic_ss_scl_lcnt;
	unsigned int ic_fs_scl_hcnt;
	unsigned int ic_fs_scl_lcnt;
	unsigned int ic_hs_scl_hcnt;
	unsigned int ic_hs_scl_lcnt;
	unsigned int ic_intr_stat;
	unsigned int ic_intr_mask;
	unsigned int ic_raw_intr_stat;
	unsigned int ic_rx_tl;
	unsigned int ic_tx_tl;
	unsigned int ic_clr_intr;
	unsigned int ic_clr_rx_under;
	unsigned int ic_clr_rx_over;
	unsigned int ic_clr_tx_over;
	unsigned int ic_clr_rd_req;
	unsigned int ic_clr_tx_abrt;
	unsigned int ic_clr_rx_done;
	unsigned int ic_clr_activity;
	unsigned int ic_clr_stop_det;
	unsigned int ic_clr_start_det;
	unsigned int ic_clr_gen_call;
	unsigned int ic_enable;
	unsigned int ic_status;
	unsigned int ic_txflr;
	unsigned int ic_rxflr;
	unsigned int reserved_1;	/* ic_sda_hold ??? */
	unsigned int ic_tx_abrt_source;
};
static struct i2c_regs *i2c_regs_p;

/*
------------------------
 low-level
------------------------
*/
static unsigned char bcd2bin (unsigned char val)
{
	return (val & 0x0f) + (val >> 4) * 10;
}
static unsigned char bin2bcd (unsigned char val)
{
	return ((val / 10) << 4) + val % 10;
}

static void i2c_disable (void)
{
	unsigned int enbl;
	enbl = READ_I2C_REG(&i2c_regs_p->ic_enable);
	enbl &= ~IC_ENABLE_0B;
	WRITE_I2C_REG(&i2c_regs_p->ic_enable,enbl);
}

static void i2c_enable (void)
{
	unsigned int enbl;
	enbl = READ_I2C_REG(&i2c_regs_p->ic_enable);
	enbl |= IC_ENABLE_0B;
	WRITE_I2C_REG(&i2c_regs_p->ic_enable,enbl);
}

static void i2c_flush_rxfifo (void)
{
	while (READ_I2C_REG(&i2c_regs_p->ic_status) & IC_STATUS_RFNE){
		READ_I2C_REG(&i2c_regs_p->ic_cmd_data);
	}
}

static int i2c_wait_for_bb (void)
{
	int timeout = I2C_TIMEOUT;
	while ((READ_I2C_REG(&i2c_regs_p->ic_status) & IC_STATUS_MA) ||
		!(READ_I2C_REG(&i2c_regs_p->ic_status) & IC_STATUS_TFE))
	{
		if (!timeout--){
			return -1;
		}
	}
	return 0;
}

static int i2c_xfer_finish (void)
{
	int timeout = I2C_TIMEOUT;
	while (1) {
		if ((READ_I2C_REG(&i2c_regs_p->ic_raw_intr_stat) & IC_STOP_DET)) {
			READ_I2C_REG(&i2c_regs_p->ic_clr_stop_det);
			break;
		}

		if (!timeout--){
			return -1;
		}
	}

	if (i2c_wait_for_bb()){
		return -1;
	}

	i2c_flush_rxfifo();
	return 0;
}

static void i2c_setaddress (unsigned int tar)
{
	i2c_disable();
	WRITE_I2C_REG(&i2c_regs_p->ic_tar, tar);
	i2c_enable();
}

static void i2c_hw_init (int self_adr)
{
	i2c_disable();

	WRITE_I2C_REG(&i2c_regs_p->ic_con,   IC_CON_SD | IC_CON_SPD_FS | IC_CON_MM);
	WRITE_I2C_REG(&i2c_regs_p->ic_rx_tl, IC_RX_TL);
	WRITE_I2C_REG(&i2c_regs_p->ic_tx_tl, IC_TX_TL);
	WRITE_I2C_REG(&i2c_regs_p->ic_sar,   self_adr);
	WRITE_I2C_REG(&i2c_regs_p->ic_intr_mask, IRQ_MASK);

	WRITE_I2C_REG(&i2c_regs_p->ic_fs_scl_hcnt,
		(IC_CLK * MIN_FS_SCL_HIGHTIME) / NANO_TO_MICRO);
	WRITE_I2C_REG(&i2c_regs_p->ic_fs_scl_lcnt,
		(IC_CLK * MIN_FS_SCL_LOWTIME) / NANO_TO_MICRO);

	i2c_enable();
}

static int i2c_txrx (unsigned char tar, unsigned char *tx, int txlen, unsigned char *rx, int rxlen)
{
	int err = 0;
	int timeout;

	/* WAIT */
	timeout = I2C_TIMEOUT;
	while ((READ_I2C_REG(&i2c_regs_p->ic_status) & IC_STATUS_MA) ||
		  !(READ_I2C_REG(&i2c_regs_p->ic_status) & IC_STATUS_TFE))
	{
		if(!timeout--){
			return -1;
		}
	}
	i2c_setaddress(tar);

	/* TX */
	timeout = I2C_TIMEOUT;
	while (txlen) {
		if (READ_I2C_REG(&i2c_regs_p->ic_status) & IC_STATUS_TFNF) {
			WRITE_I2C_REG(&i2c_regs_p->ic_cmd_data, *tx++);
			txlen--;
		}
		if (!timeout--){
			err = -1;
			goto exit;
		}
	}

	/* RX */
	timeout = I2C_TIMEOUT;
	while (rxlen) {
		if (rxlen == 1) WRITE_I2C_REG(&i2c_regs_p->ic_cmd_data, IC_CMD | IC_STOP);
		else            WRITE_I2C_REG(&i2c_regs_p->ic_cmd_data, IC_CMD);

		if (READ_I2C_REG(&i2c_regs_p->ic_status) & IC_STATUS_RFNE) {
			*rx++ = READ_I2C_REG(&i2c_regs_p->ic_cmd_data);
			rxlen--;
		}

		if (!timeout--){
			err = -1;
			goto exit;
		}
	}

exit:
	err |= i2c_xfer_finish();
	return err;
}

static int i2c_txtx (unsigned char tar,
	unsigned char *tx, int txlen,
	unsigned char *tx2, int txlen2)
{
	int err = 0;
	int timeout;

	/* WAIT */
	timeout = I2C_TIMEOUT;
	while ((READ_I2C_REG(&i2c_regs_p->ic_status) & IC_STATUS_MA) ||
		  !(READ_I2C_REG(&i2c_regs_p->ic_status) & IC_STATUS_TFE))
	{
		if(!timeout--){
			return -1;
		}
	}
	i2c_setaddress(tar);

	/* TX */
	timeout = I2C_TIMEOUT;
	while (txlen) {
		if (READ_I2C_REG(&i2c_regs_p->ic_status) & IC_STATUS_TFNF) {
			WRITE_I2C_REG(&i2c_regs_p->ic_cmd_data, *tx++);
			txlen--;
		}
		if (!timeout--){
			err = -1;
			goto exit;
		}
	}

	/* TX2 */
	timeout = I2C_TIMEOUT;
	while (txlen2) {
		if (READ_I2C_REG(&i2c_regs_p->ic_status) & IC_STATUS_TFNF) {
			WRITE_I2C_REG(&i2c_regs_p->ic_cmd_data, *tx2++);
			txlen2--;
		}
		if (!timeout--){
			err = -1;
			goto exit;
		}
	}

exit:
	err |= i2c_xfer_finish();
	return err;
}

static int rtc_read (int bus, int page, int word, unsigned char *buf, int len)
{
	i2c_regs_p = BAIKAL_I2C_REG(bus);
	unsigned char adr = (page << RTC_PAGE_OFFSET) | (word << RTC_WORD_OFFSET);

	i2c_hw_init (CONFIG_SYS_I2C_SLAVE);
	if (i2c_txrx (I2C_RTC_ADR, &adr,sizeof(adr), 0,0)) {
		return -1;
	}

	i2c_hw_init (CONFIG_SYS_I2C_SLAVE);
	if (i2c_txrx (I2C_RTC_ADR, 0,0, buf,len)) {
		return -1;
	}
	return 0;
}

static int rtc_write (int bus, int page, int word, unsigned char *buf, int len)
{
	i2c_regs_p = BAIKAL_I2C_REG(bus);
	unsigned char adr = (page << RTC_PAGE_OFFSET) | (word << RTC_WORD_OFFSET);

	i2c_hw_init (CONFIG_SYS_I2C_SLAVE);
	if (i2c_txtx (I2C_RTC_ADR, &adr,sizeof(adr), buf,len)) {
		return -1;
	}
	return 0;
}

static int raw2hour (unsigned char raw)
{
	int data;
	if (raw & HOUR_12) {
		data = bcd2bin(raw & MASK_HOUR_12);	/* 1-12 AM\PM */
		if (raw & HOUR_PM) {
			data += 12;			/* 1-24 */
		}
		data -= 1;				/* 0-23 */
	} else {
		data = bcd2bin(raw & MASK_HOUR_24);	/* 0-23 */
	}
	return data;
}

/*
------------------------
 extern
------------------------
*/
EFI_STATUS set_time (EFI_TIME *time)
{
	int bus = I2C_RTC_BUS;
	unsigned char raw[7];

	raw[YEAR]	= bin2bcd(time->Year - BASIC_YEAR_ABRACON) & MASK_YEAR;
	raw[MONTH]	= bin2bcd(time->Month)	& MASK_MONTH;
	raw[DAY]	= bin2bcd(time->Day)	& MASK_DAY;
	raw[MINUTE]	= bin2bcd(time->Minute)	& MASK_MINUTE;
	raw[SECOND]	= bin2bcd(time->Second)	& MASK_SECOND;
	raw[HOUR]	= bin2bcd(time->Hour)	& MASK_HOUR_24;

	if (rtc_write(bus,1,0,raw,sizeof(raw))){
		return EFI_DEVICE_ERROR;
	}

	return EFI_SUCCESS;
}

EFI_STATUS get_time (EFI_TIME *time)
{
	int bus = I2C_RTC_BUS;
	unsigned char raw[7];
	if (rtc_read(bus,1,0,raw,sizeof(raw))){
		return EFI_DEVICE_ERROR;
	}

	time->Year	= bcd2bin (raw[YEAR]   & MASK_YEAR) + BASIC_YEAR_ABRACON;
	time->Month	= bcd2bin (raw[MONTH]  & MASK_MONTH);
	time->Day	= bcd2bin (raw[DAY]    & MASK_DAY);
	time->Minute	= bcd2bin (raw[MINUTE] & MASK_MINUTE);
	time->Second	= bcd2bin (raw[SECOND] & MASK_SECOND);
	time->Hour	= raw2hour(raw[HOUR]);

	/* not supported */
 	time->Pad1 = 0;
 	time->Nanosecond = 0;
 	time->TimeZone = 0;
 	time->Daylight = 0;
 	time->Pad2 = 0;

	return EFI_SUCCESS;
}


