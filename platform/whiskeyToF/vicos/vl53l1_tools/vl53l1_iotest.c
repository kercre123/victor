/**
 * IOCTL test
 */

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

#include <time.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))



#define timer_start(t) clock_gettime(CLOCK_MONOTONIC, t)

long  timer_elapsed_us(struct timespec *pt){
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	long delta = (now.tv_sec-pt->tv_sec) + (now.tv_nsec-pt->tv_nsec)/1000;
	if( now.tv_nsec-pt->tv_nsec < 0){
		delta+=1000000;
	}
	return delta;
}

int stmvl53l1_reg_wr(int fd, int index, int len , void *data){
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


int stmvl53l1_reg_rd(int fd, int index, int len , void *data){
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



void reg_test_perf(int fd){
	int sz;
//	uint8_t wr_data[random_sz];
	uint8_t rd_data[256];
	int rc;
//	int data_ok;
	int n,i;
	struct timespec tstart;
	long v_min=0xFFFFFFFF, v_max=0;
	double sum;

	#define N_LOOP_MAX	4096
	long Time[N_LOOP_MAX];
	int n_loops=N_LOOP_MAX;
	for( sz=1; sz<256; sz*=2){
		for( n=0; n<n_loops; n++ ){
			timer_start(&tstart);
			rc = stmvl53l1_reg_rd(fd, 0x0, sz, rd_data);
			if( rc ){
				break;
			}
			Time[n]=timer_elapsed_us(&tstart);
		}
		if( rc ==0 ){
			printf("size %d timing result\n", sz);
			sum=0;
			v_min=0x7FFFFFFF;
			v_max=0;

			for( i=0; i<n_loops; i++){
				sum+=Time[i];
				v_max = Time[i] > v_max ? Time[i]:v_max;
				v_min = Time[i] < v_min ? Time[i]:v_min;
				printf("%ld\n", Time[i]);
			}
			printf("sz %d min %d max %d avg %.0lf all timing in us\n", sz,
					(int)v_min , (int)v_max, (double)sum/n_loops);
		}

	}
//no_off:
	t_status("");
}

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


struct test_t {
	int mask_bit_no;
	void(*f_test)(int);
	const char *name;
};
struct test_t  tests[]={
	{
		.f_test = reg_test_random,
		.name = "reg_test_random"
	},

	{
		.f_test = reg_test_perf,
		.name = "reg_test_perf"
	},
};

static void help(void)
{
	int i;
	fprintf(stderr,	"Usage: vl53l1_iotest test_mask\n");
	fprintf(stderr, "testlist :\n");
	for( i=0; i<(int)ARRAY_SIZE(tests);i++){
		fprintf(stderr, "test #%d 0x%08X %s\n", i, 1<<i,
				tests[i].name);
	}
	exit(1);
}

int main(int argc, char *argv[])
{
	int fd;
//	struct stmvl53l1_register reg;
	char *end;
//	int rc;
	int i;
	int test_mask;
	int n_run;

	if (argc < 2) {
		help();
		exit(1);
	}

	fd = open("/dev/" VL53L1_MISC_DEV_NAME ,O_RDWR );
	if (fd <= 0) {
		fprintf(stderr,"Error open %s device: %s\n", VL53L1_MISC_DEV_NAME, strerror(errno));
		return -1;
	}

	test_mask = strtoul(argv[1], &end, 16);
	if (*end) {
		help();
		exit(1);
	}

	for( i= 0, n_run=0 ; i < (int)ARRAY_SIZE(tests); i++){
		if( test_mask & (1<<i) ){
			n_run++;
			printf("test #%d %s ...", n_run, tests[i].name);
			tests[i].f_test(fd);
		}
	}

//done:
	close(fd);
	return 0;
}


