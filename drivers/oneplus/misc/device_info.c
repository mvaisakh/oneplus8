
#include <linux/kernel.h>

#include <linux/init.h>

#include <linux/types.h>

#include <linux/pstore.h>
#include <soc/qcom/socinfo.h>

#include <linux/pstore.h>
#include "../../../fs/pstore/internal.h"
#include <linux/oem/param_rw.h>
#include <linux/oem/boot_mode.h>

extern struct pstore_info *psinfo;

#define MAX_ITEM 5
#define MAX_LENGTH 32

enum
{
	serialno = 0,
	hw_version,
	rf_version,
	ddr_manufacture_info,
	pcba_number
};

char oem_serialno[16];
char oem_hw_version[3];
char oem_rf_version[3];
char oem_ddr_manufacture_info[16];
char oem_pcba_number[30];

const char cmdline_info[MAX_ITEM][MAX_LENGTH] =
{
	"androidboot.serialno=",
	"androidboot.hw_version=",
	"androidboot.rf_version=",
	"ddr_manufacture_info=",
	"androidboot.pcba_number=",
};

static int __init device_info_init(void)
{
	int i, j;
	char *substr, *target_str;

	for(i=0; i<MAX_ITEM; i++)
	{
		substr = strstr(boot_command_line, cmdline_info[i]);
		if(substr != NULL)
			substr += strlen(cmdline_info[i]);
		else
			continue;

		if(i == serialno)
			target_str = oem_serialno;
		else if(i == hw_version)
			target_str = oem_hw_version;
		else if(i == rf_version)
			target_str = oem_rf_version;
		else if(i == ddr_manufacture_info)
			target_str = oem_ddr_manufacture_info;
		else if(i == pcba_number)
			target_str = oem_pcba_number;

		for(j=0; substr[j] != ' '; j++)
			target_str[j] = substr[j];
		target_str[j] = '\0';
	}
	return 1;
}

static int is_na_board(void)
{
	int pj_id = 0;
	int rf_id = 0;
	int i = 0;
	int ret = 0;
	int na_board_info[][2]={
	/*  prj     rf     */
	{   11   ,  15     },
	{   12   ,  12     },
	{   12   ,  15     },
	{   13   ,  15     },
	{   13   ,  25     },
	};

	pj_id = get_prj_version();
	rf_id = get_rf_version();


	for ( i = 0 ; i < ARRAY_SIZE(na_board_info) ; i++ ) {
		if (na_board_info[i][0] == pj_id && na_board_info[i][1] == rf_id) {
			ret = 1;
			break;
		}
	}
	return ret;
}

/*device info init to black*/
static void  pstore_device_info_init(void )
{
    size_t oldsize;
    size_t size =0;

    struct ramoops_context *cxt = psinfo->data;
    struct pstore_record record;

    if (psinfo == NULL)
        return;

    size = cxt->device_info_size;

	pstore_record_init(&record, psinfo);
	record.type = PSTORE_TYPE_DEVICE_INFO;
    record.buf = psinfo->buf;
    record.size = size;

    oldsize = psinfo->bufsize;


    if (size > psinfo->bufsize)
        size = psinfo->bufsize;

    memset(record.buf, ' ', size);
    psinfo->write(&record);
  
    psinfo->bufsize = oldsize ;
}

static void pstore_write_device_info(const char *s, unsigned c)
{

	const char *e = s + c;

	if (psinfo == NULL)
		return;

	while (s < e) {
		struct pstore_record record;
		pstore_record_init(&record, psinfo);
		record.type = PSTORE_TYPE_DEVICE_INFO;

		if (c > psinfo->bufsize)
			c = psinfo->bufsize;

		record.buf = (char *)s;
		record.size = c;
		psinfo->write(&record);
		s += c;
		c = e - s;
	}

}

static void write_device_info(const char *key, const char *value)
{
	pstore_write_device_info(key, strlen(key));
	pstore_write_device_info(": ", 2);
	pstore_write_device_info(value, strlen(value));
	pstore_write_device_info("\r\n", 2);
}

static int __init init_device_info(void)
{
	char *ptr = NULL;
	ptr = oem_pcba_number;
	get_param_by_index_and_offset(0, 0x4D, ptr, 28);

	pstore_device_info_init();

	device_info_init();

	write_device_info("hardware version", oem_hw_version);
	write_device_info("rf version", oem_rf_version);
	write_device_info("modem baseline", is_na_board() ? "NA":"NON_NA" );
	write_device_info("ddr manufacturer", oem_ddr_manufacture_info);
	write_device_info("pcba number", oem_pcba_number);
	write_device_info("serial number", oem_serialno);

	scnprintf(oem_serialno, sizeof(oem_serialno), "%x", socinfo_get_serial_number());
	write_device_info("socinfo serial_number", oem_serialno);

	write_device_info("kernel version", linux_banner);
	write_device_info("boot command", saved_command_line);

	return 0;
}

late_initcall(init_device_info);

void save_dump_reason_to_device_info(char *reason) {
        write_device_info("dump reason is ", reason);
}
