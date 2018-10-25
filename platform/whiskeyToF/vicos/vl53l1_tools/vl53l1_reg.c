

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>

#include "stmvl53l1_if.h"
#include "stmvl53l1_internal_if.h"

static int stmvl53l1_reg_wr(int fd, int index, int len , void *data){
	struct stmvl53l1_register reg;
	int rc;
	reg.is_read = 0;
	reg.index = index;
	reg.cnt=  len;
	if( len > 0)
		memcpy(reg.data.bytes, data, len);
	rc= ioctl(fd, VL53L1_IOCTL_REGISTER,&reg);
	if( rc == 0 ){
		return reg.status;
	}
	return -1;
}


static int stmvl53l1_reg_rd(int fd, int index, int len , void *data){
	struct stmvl53l1_register reg;
	int rc;
	reg.is_read = 1;
	reg.index = index;
	reg.cnt = len;
	rc= ioctl(fd, VL53L1_IOCTL_REGISTER,&reg);
	if( rc == 0 ){
		if( len > 0 && len < 256 )
			memcpy(data, reg.data.bytes, len);
		return reg.status;
	}
	return -1;
}

int test_check=0;
int test_n_err=0;
#define tfail( cond, msg, ...)\
	do{\
		test_check++;\
		if( cond ) {\
			printf("ERR %s line %d\t" msg "\n", __func__, __LINE__, ##__VA_ARGS__);\
			test_n_err++;\
		}\
	}while(0)

#define t_status(msg, ...)\
	do{\
		printf("STAT %s\t %d err out of %d " msg "\n", __func__, test_n_err, test_check, ##__VA_ARGS__ );\
	}while(0)

/** ewokplus  cut 1.0 */
const int random_wr_index = 0x16 ;
const int random_sz=  0x8;
void reg_test_random(int fd){
	int sz;
	uint8_t wr_data[random_sz];
	uint8_t rd_data[random_sz];
	int rc;
	int data_ok;

	rc = stmvl53l1_reg_wr(fd, random_wr_index, 0, rd_data);
	tfail( rc == 0 ," wr 0 length must fail");

	rc = stmvl53l1_reg_rd(fd, random_wr_index, 0, rd_data);
	tfail( rc == 0 ," rd 0 length must fail");

	memset(wr_data, 0xFF, sizeof(wr_data));
	for( sz = 1 ; sz < random_sz; sz++ ){
		rc = stmvl53l1_reg_wr(fd, random_wr_index, sz, wr_data);
		tfail(rc !=0 , "wr sz %d", sz );
		if( rc )
			continue;

		rc = stmvl53l1_reg_rd(fd, random_wr_index, sz, rd_data);
		tfail( rc != 0 , "rd sz %d", sz );
		if( rc )
			continue;

		data_ok =memcmp(wr_data, rd_data, sz);
		tfail(data_ok!=0, "FF sz %d data mismacth", sz );
	}
	for( sz=0; sz<  (int)sizeof(wr_data); sz++)
		wr_data[sz] =sz;

	for( sz = 1 ; sz < random_sz; sz++ ){
		rc = stmvl53l1_reg_wr(fd, random_wr_index, sz, wr_data);
		tfail(rc !=0 , "wr sz %d", sz );
		if( rc )
			continue;

		rc = stmvl53l1_reg_rd(fd, random_wr_index, sz, rd_data);
		tfail(rc !=0  , "wr sz %d", sz );
		if( rc )
			continue;

		data_ok =memcmp(wr_data, rd_data, sz);
		tfail(data_ok!=0,  "addr test  sz %d data mismacth", sz );
	}

	// offset test
	memset(wr_data, 0, sizeof(wr_data));
	rc = stmvl53l1_reg_wr(fd, random_wr_index, sz, wr_data);
	tfail(rc!=0,  "addr test  sz %d data mismacth", sz );
	goto no_off;
	int offset,i;
	for( sz=1; sz <= 4; sz*=2){
		for( offset = 0 ; offset <= random_sz/2; offset++ ){
			for( i=0; i<sz; sz++)
				wr_data[offset+i]=offset+i+sz;
			rc = stmvl53l1_reg_wr(fd, random_wr_index+offset, sz, &wr_data[offset]);
			tfail(rc !=0 , "off wr%d sz %d", offset , sz );
			if( rc )
				break;
			// read back full chunk
			rc = stmvl53l1_reg_rd(fd, random_wr_index, sizeof(wr_data), rd_data);
			tfail(rc !=0 , "off wr %d sz %d", offset, sz );
			if( rc )
				break;

			data_ok = memcmp(wr_data, rd_data, sizeof(wr_data));
			tfail(data_ok!=0,  "off  %d z %d data mismacth", offset, sz);
			if( data_ok )
				break;
		}
	}
no_off:
	t_status("");
}

static void help(void)
{
	fprintf(stderr,
		"Usage: vl53l1_reg REG_ADDR REG_BYTES [REG_DATA]\n"
		" REG_ADDR is VL53L1 register address using 0xxx format: \n"
		" REG_BYTES is r number bytes(1, 2 or 4) \n"
		" REG_DATA is optional for writting data to requested REG_ADDR for 1 2 4 it can be single hax form or a list of byte hexa array\n"
		);
	exit(1);
}

int main(int argc, char *argv[])
{
	int fd;
	struct stmvl53l1_register reg;
	char *end;
	int rc;
	int i;

	if (argc < 3) {
		help();
		exit(1);
	}

	fd = open("/dev/" VL53L1_MISC_DEV_NAME ,O_RDWR );
	if (fd <= 0) {
		fprintf(stderr,"Error open %s device: %s\n", VL53L1_MISC_DEV_NAME, strerror(errno));
		return -1;
	}

	reg.index = strtoul(argv[1], &end, 16);
	if (*end) {
		help();
		exit(1);
	}

	switch( reg.index ){
	case 0x10000 :
		// ioctl test
		reg_test_random(fd);
		break;
	case 0x10001 :
		break;
	}
 	reg.cnt = strtoul(argv[2],&end,10);
	if (*end) {
	   	help();
		exit(1);
	}
	if (argc == 3) {
		reg.is_read = 1;
		printf("To read VL53L1 register index:0x%x, bytes:%d\n", reg.index, reg.cnt);

		rc= ioctl(fd, VL53L1_IOCTL_REGISTER,&reg);
		if( rc ){
			perror("fail");
		}
		else{
			if( reg.status !=0 ){
				//fail
				printf("device rd fail status %d\n", (int)reg.status);
				goto done;
			}
			switch( reg.cnt ){
			case 1:
				printf(" 0x%02x %d\n", (int)reg.data.b, (int)reg.data.b);
			break;

			case 2:
				printf(" 0x%04x %d\n", (int)reg.data.w, (int)reg.data.w);
			break;
			case 4:
				printf(" 0x%04x %d\n", (int)reg.data.dw, (int)reg.data.dw);
			break;
			default:
				for(i=0; i< (int)reg.cnt; i++  ){
					printf("0x%02x ", (int)reg.data.bytes[i]);
					if( i%16 == 15 && i != (int)reg.cnt-1){
						printf("\n");
					}
				}
				printf("\n");

			}
		}
	} else {
		uint32_t dw;
		reg.data.dw=0;// reset to 0 so that all 1 2 3 4 size can use direct assuming same endianness
		if( argc ==  4 ){
			// we have single data for 1 2 3 4 byte reg
			reg.is_read = 0;

			dw = strtoul(argv[3],&end,16);
			if (*end) {
				help();
				exit(1);
			}
			switch( reg.cnt ){
				case 1: reg.data.b = dw;break;
				case 2: reg.data.w = dw;break;
				case 3:
				case 4: reg.data.dw = dw;break;
				default:
					if( reg.cnt >4 )
						fprintf(stderr, "not enough data for given %d size\n",reg.cnt );
			}
		}
		else{
			uint32_t dw;
			if( argc != (int)reg.cnt + 3 ){
				fprintf(stderr, "not enough data byte for size\n");
				help();
				exit(1);
			}
			for ( i=0; i<(int)reg.cnt; i++ ){
				rc= sscanf(argv[3+i], "%x", & dw);
				if( rc != 1 ){
					fprintf(stderr, "bad syntax data[%d] %s\n", i, argv[3+i]);
					help();
					exit(1);
				}
				reg.data.bytes[i]=dw;
			}
		}
		// if we have more args scan all of them
		printf("To write VL53L0 register index:0x%x, bytes:%d as value:0x%x ",
						reg.index, reg.cnt, reg.data.dw);
		rc= ioctl(fd, VL53L1_IOCTL_REGISTER,&reg);
		printf("status %d\n", reg.status);
	}

done:
	close(fd);
	return 0;
}


