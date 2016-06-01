#define DEVICE "/dev/sg1"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <scsi/sg.h>

#define SCSI_OFF sizeof(struct sg_header)

#define INQUIRY_CMD_OPCODE		0x12
#define INQUIRY_CMD_LEN			6
#define INQUIRY_CMD_REPLY_LEN	96
#define INQUIRY_RPLY_VENDER		8

static unsigned char cmd[SCSI_OFF + 18];

int fd;											//SCSI device File descriptor

int handle_SCSI_cmd(unsigned char cmd_len,		//Command length
					unsigned char in_size,		//Input data size
					unsigned char *i_buff,		//input buffer
					unsigned char out_size,		//Output data size
					unsigned char *o_buff		//Output buffer
					)
{
	int status = 0;
	struct sg_header *sg_hd;
	if(!cmd_len || !i_buff || !o_buff)
#ifdef SG_BIG_BUFF
    if (SCSI_OFF + cmd_len + in_size > SG_BIG_BUFF) return -1;
    if (SCSI_OFF + out_size > SG_BIG_BUFF) return -1;
#else
    if (SCSI_OFF + cmd_len + in_size > 4096) return -1;
    if (SCSI_OFF + out_size > 4096) return -1;
#endif
	sg_hd = (struct sg_header *) i_buff;
	sg_hd->reply_len = SCSI_OFF + out_size;
	sg_hd->twelve_byte = cmd_len == 12;
	sg_hd->result = 0;

	/*Write command on drive*/
	status = write(fd, i_buff, SCSI_OFF + cmd_len + in_size);
	if (status < 0 || status != SCSI_OFF + cmd_len + in_size || sg_hd->result ) {
		fprintf(stderr, "Write failed status=0x%x result=0x%x cmd=0x%x\n",status, sg_hd->result, i_buff[SCSI_OFF]);
		perror("");
		return status;
	}

	/*Read response from drive*/
	status = read(fd, o_buff, SCSI_OFF + out_size);
	if (status < 0 || status != SCSI_OFF + out_size || sg_hd->result ) {
		fprintf(stderr, "Read failed status=0x%x result=0x%x cmd=0x%x",status, sg_hd->result, i_buff[SCSI_OFF]);
		perror("");
		return status;
	}

	o_buff = o_buff+SCSI_OFF;
	if (status == SCSI_OFF + out_size){
		printf("\nResponse is : ");
		for (int i=0;i</*SCSI_OFF + */out_size/*+INQUIRY_RPLY_VENDER*/;i++)
			printf("%c",*(o_buff+i));
	}

	printf("\n");
	return 0;
}

int main( void )
{
  fd = open(DEVICE, O_RDWR);
  if (fd < 0) {
	fprintf(stderr, "Need read/write permission on "DEVICE".\n");
	exit(1);
  }

  unsigned char Inqbuffer[ SCSI_OFF + INQUIRY_CMD_REPLY_LEN ];

  /*
  *  +=====-========-========-========-========-========-========-========-========+
  *  |  Bit|   7    |   6    |   5    |   4    |   3    |   2    |   1    |   0    |
  *  |Byte |        |        |        |        |        |        |        |        |
  *  |=====+=======================================================================|
  *  | 0   |                           Operation Code (12h)                        |
  *  |-----+-----------------------------------------------------------------------|
  *  | 1   | Logical Unit Number      |                  Reserved         |  EVPD  |
  *  |-----+-----------------------------------------------------------------------|
  *  | 2   |                           Page Code                                   |
  *  |-----+-----------------------------------------------------------------------|
  *  | 3   |                           Reserved                                    |
  *  |-----+-----------------------------------------------------------------------|
  *  | 4   |                           Allocation Length                           |
  *  |-----+-----------------------------------------------------------------------|
  *  | 5   |                           Control                                     |
  *  +=============================================================================+
  */

  unsigned char cmdblk[ INQUIRY_CMD_LEN ] = { INQUIRY_CMD_OPCODE,		//Opcode of Inquiry command
  	  	  	  	  	  	  	  	  	  	  	  0,						//LUN/reserved
											  0,						//Page Code
											  0,						//Reserved
											  INQUIRY_CMD_REPLY_LEN,	//Allocation Length
											  0							//Control
											  };
  /*
   * +------------------+
   * | Struct sg_header | <- cmd
   * +------------------+
   * | copy of cmdblk   | <- cmd + SCSI_OFF
   * +------------------+
   */

  memcpy(cmd + SCSI_OFF, cmdblk, sizeof(cmdblk));

  handle_SCSI_cmd(sizeof(cmdblk), 0, cmd, sizeof(Inqbuffer)-SCSI_OFF, Inqbuffer);
  return 0;
}
