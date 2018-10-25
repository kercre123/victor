/**
 *@file  phio.c  cmd line tools to stmvl531 ioctl interface
 *
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <linux/input.h>

/* local bare driver top api file */
#include "vl53l1_def.h"
/* our driver kernel interface */
#include "stmvl53l1_if.h"
#include "stmvl53l1_internal_if.h"

#include <time.h>
#include <getopt.h>

#define UNUSED __attribute__((unused))

#ifndef MAX
#define MAX(a,b ) ((a)> (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b ) ((a)< (b) ? (a) : (b))
#endif

#define EVENT_NB	14
const int index_to_code[EVENT_NB] = {ABS_DISTANCE, ABS_HAT0X, ABS_HAT0Y, ABS_HAT1X,
	ABS_HAT1Y, ABS_HAT2X, ABS_HAT2Y, ABS_HAT3X, ABS_HAT3Y, ABS_WHEEL, ABS_PRESSURE,
	ABS_BRAKE, ABS_TILT_X, ABS_TOOL_WIDTH};
static int event_values[EVENT_NB];
static int input_fd;

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define SYSFS_MAX_LEN			4096

#define verbose(fmt, ...)\
	do{ \
		printf("[V]" fmt"\n", ##__VA_ARGS__);\
		fflush(stdout);\
	}while(0)

#define info(fmt, ...)\
	do{ \
		printf("[I]" fmt"\n", ##__VA_ARGS__);\
		fflush(stdout);\
	}while(0)

#define error(fmt, ...)  fprintf(stderr, "[E] " fmt"\n", ##__VA_ARGS__)
#define warn(fmt, ...)  fprintf(stderr, "[W] " fmt"\n", ##__VA_ARGS__)

#define debug(fmt,...)\
	do{ \
		printf("d " fmt"\n", ##__VA_ARGS__); \
		fflush(stdout);\
	}while(0)


#define ioctl_error(fmt,...) do { \
	int is_eio_error = errno == EIO; \
	fprintf(stderr, "[Eio] %s " fmt"" , __func__, ##__VA_ARGS__); \
	if (is_eio_error) \
		display_device_error(dev_fd); \
	else \
		fprintf(stderr, "\n"); \
} while(0)
#define ioctl_warn(fmt,...)  fprintf(stderr, "[Wio] %s " fmt"\n" , __func__, ##__VA_ARGS__)

#define timer_start(t) clock_gettime(CLOCK_MONOTONIC, t)

static int is_sysfs;

extern int dev_fd;

#define IMPLEMENT_PARAMETER_INTEGER(sysfs_name, parameter_name) \
int stmvl53l1_get_##sysfs_name(int fd, uint32_t *param) \
{ \
	return is_sysfs ? stmvl53l1_get_integer_sysfs(fd, param, #sysfs_name) \
		: stmvl53l1_get_integer_ioctl(fd, param, parameter_name); \
} \
 \
int stmvl53l1_set_##sysfs_name(int fd, uint32_t param) \
{ \
	return is_sysfs ? stmvl53l1_set_integer_sysfs(fd, param, #sysfs_name) \
		: stmvl53l1_set_integer_ioctl(fd, param, parameter_name); \
}

static void display_device_error(int fd)
{
	struct stmvl53l1_parameter params;

	params.is_read = 1;
	params.name = VL53L1_LASTERROR_PAR;

	ioctl(fd, VL53L1_IOCTL_PARAMETER, &params);
	fprintf(stderr, " [device i/o error is %d]\n", params.value);
}

long  timer_elapsed_us(struct timespec *pt){
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	long delta = (now.tv_sec-pt->tv_sec) + (now.tv_nsec-pt->tv_nsec)/1000;
	if( now.tv_nsec-pt->tv_nsec < 0){
		delta+=1000000;
	}
	return delta;
}

static float auto_16x16_to_float(FixPoint1616_t fix)
{
	return fix / 65536.0;
}

static FixPoint1616_t auto_float_to_16x16(float f)
{
	return (uint32_t)(f * 65536);
}

static char *strjoin_with_separator(char *a, char *b)
{
	char *res = malloc(strlen(a) + 1 + strlen(b) + 1);

	if (res) {
		strcpy(res, a);
		res[strlen(a)] = '/';
		res[strlen(a)+1] = '\0';
		strcat(res, b);
	}

	return res;
}

static char *test_sysfs_root(char *dirname)
{
	char *full_dir = strjoin_with_separator("/sys/class/input", dirname);
	char *full_name_path = NULL;
	int fd = -1;
	char b[SYSFS_MAX_LEN] = "\n";
	int rc;

	if (!full_dir)
		goto error;

	full_name_path = strjoin_with_separator(full_dir, "name");
	if (!full_name_path)
		goto error;

	fd = open(full_name_path, O_RDONLY);
	if (fd < 0)
		goto error;

	rc = read(fd, b, SYSFS_MAX_LEN);
	if (rc < 0)
		goto error;

	/* replace new line with end of string */
	b[rc-1] = '\0';
	if (strcmp("STM VL53L1 proximity sensor", b))
		goto error;

	free(full_name_path);
	close(fd);

	return full_dir;

error:
	if (full_dir)
		free(full_dir);
	if (full_name_path)
		free(full_name_path);
	if (fd >= 0)
		close(fd);

	return NULL;
}


static char *stmvl53l1_sysfs_find_device()
{
	static char *sysfs_root = NULL;

	if (!sysfs_root) {
		DIR *d;
		struct dirent *dir;

		d = opendir("/sys/class/input");
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				sysfs_root = test_sysfs_root(dir->d_name);
				if (sysfs_root)
					break;
			}
			closedir(d);
		}
		if (sysfs_root)
			verbose("sysfs dir %s", sysfs_root);
	}

	return sysfs_root;
}

static int stmvl53l1_sysfs_open(char *name, int is_read)
{
	int fd = -1;
	char *path = stmvl53l1_sysfs_find_device();
	char *full_path;

	if (!path)
		return -1;

	full_path = strjoin_with_separator(path, name);
	if (!full_path)
		return -1;

	fd = open(full_path, is_read ? O_RDONLY: O_WRONLY);

	free(full_path);

	return fd;
}

static int stmvl53l1_sysfs_open_read(char *name)
{
	return stmvl53l1_sysfs_open(name , 1);
}

static int stmvl53l1_sysfs_open_write(char *name)
{
	return stmvl53l1_sysfs_open(name , 0);
}

static int stmvl53l1_sysfs_read_integer(char *name, int *res)
{
	int fd;
	int rc;
	char b[SYSFS_MAX_LEN];

	fd = stmvl53l1_sysfs_open_read(name);
	if (fd < 0)
		return fd;

	*res = 0;
	rc = read(fd, b, SYSFS_MAX_LEN);
	if (rc < 0)
		return rc;
	*res = atoi(b);

	close(fd);

	return 0;
}

static int stmvl53l1_sysfs_read_float(char *name, float *res)
{
	int fd;
	int rc;
	char b[SYSFS_MAX_LEN];

	fd = stmvl53l1_sysfs_open_read(name);
	if (fd < 0)
		return fd;

	*res = 0;
	rc = read(fd, b, SYSFS_MAX_LEN);
	if (rc < 0)
		return rc;
	*res = atof(b);

	close(fd);

	return 0;
}

UNUSED static int stmvl53l1_sysfs_read_two_integer(char *name, int *res, int *res2)
{
	int fd;
	int rc = 0;
	char b[SYSFS_MAX_LEN];

	fd = stmvl53l1_sysfs_open_read(name);
	if (fd < 0)
		return fd;

	*res = 0;
	rc = read(fd, b, SYSFS_MAX_LEN);
	if (rc < 0)
		return rc;
	rc = sscanf(b, "%d %d", res, res2);
	if (rc != 2)
		rc = -1;

	close(fd);

	return rc;
}

static int stmvl53l1_sysfs_read_two_float(char *name, float *res, float *res2)
{
	int fd;
	int rc = 0;
	char b[SYSFS_MAX_LEN];

	fd = stmvl53l1_sysfs_open_read(name);
	if (fd < 0)
		return fd;

	*res = 0;
	rc = read(fd, b, SYSFS_MAX_LEN);
	if (rc < 0)
		return rc;
	rc = sscanf(b, "%f %f", res, res2);
	if (rc != 2)
		rc = -1;

	close(fd);

	return rc;
}

static int stmvl53l1_sysfs_write_string(char *name, char *str)
{
	int fd;
	int rc;

	fd = stmvl53l1_sysfs_open_write(name);
	if (fd < 0)
		return fd;

	rc = write(fd, str, strlen(str) + 1);
	if (rc < 0)
		return rc;

	close(fd);

	return 0;
}

static int stmvl53l1_sysfs_read_string(char *name, char *str)
{
	int fd;
	int rc;

	fd = stmvl53l1_sysfs_open_read(name);
	if (fd < 0)
		return fd;

	rc = read(fd, str, SYSFS_MAX_LEN);
	if (rc < 0)
		return rc;
	str[rc] = '\0';

	close(fd);

	return 0;
}

static int stmvl53l1_sysfs_write_integer(char *name, int res)
{
	int fd;
	int rc;
	char b[SYSFS_MAX_LEN];

	fd = stmvl53l1_sysfs_open_write(name);
	if (fd < 0)
		return fd;

	rc = sprintf(b, "%d", res);
	rc = write(fd, b, rc + 1);
	if (rc < 0)
		return rc;

	close(fd);

	return 0;
}

static int stmvl53l1_sysfs_write_two_integer(char *name, int res, int res2)
{
	int fd;
	int rc;
	char b[SYSFS_MAX_LEN];

	fd = stmvl53l1_sysfs_open_write(name);
	if (fd < 0)
		return fd;

	rc = sprintf(b, "%d %d", res, res2);
	rc = write(fd, b, rc + 1);
	if (rc < 0)
		return rc;

	close(fd);

	return 0;
}

static int stmvl53l1_sysfs_write_float(char *name, float res)
{
	int fd;
	int rc;
	char b[SYSFS_MAX_LEN];

	fd = stmvl53l1_sysfs_open_write(name);
	if (fd < 0)
		return fd;

	rc = sprintf(b, "%f", res);
	rc = write(fd, b, rc + 1);
	if (rc < 0)
		return rc;

	close(fd);

	return 0;
}

static int stmvl53l1_sysfs_read_binary_data(char *name, void *buf, int size)
{
	int fd;
	int rc = 0;

	fd = stmvl53l1_sysfs_open_read(name);
	if (fd < 0)
		return fd;

	while (size) {
		rc = read(fd, buf, size);
		if (rc <= 0)
			break;
		size -= rc;
		buf += rc;
	}

	return rc <= 0 ? -1 : 0;
}

static int stmvl53l1_sysfs_write_binary_data(char *name, void *buf, int size)
{
	int fd;
	int rc = 0;

	fd = stmvl53l1_sysfs_open_write(name);
	if (fd < 0)
		return fd;

	while (size) {
		rc = write(fd, buf, size);
		if (rc <= 0)
			break;
		size -= rc;
		buf += rc;
	}

	return rc <= 0 ? -1 : 0;
}

/* generic function to set/get parameter which is of integer type */
static int stmvl53l1_set_integer_sysfs(int fd, uint32_t param, char *sysfs_name)
{
	return stmvl53l1_sysfs_write_integer(sysfs_name, param);
}

static int stmvl53l1_set_integer_ioctl(int fd, uint32_t param, stmv53l1_parameter_name_e parameter_name)
{
	int rc;

	struct stmvl53l1_parameter params;

	params.is_read = 0;
	params.name = parameter_name;
	params.value = param;

	rc= ioctl(fd, VL53L1_IOCTL_PARAMETER, &params);
	if( rc ){
		if( errno == EBUSY ){
			ioctl_warn("ebusy can't set now");
			return errno;
		}
		ioctl_error("%d %s", rc,strerror(errno));
	}
	return rc;
}

static int stmvl53l1_get_integer_sysfs(int fd, uint32_t *param, char *sysfs_name)
{
	return stmvl53l1_sysfs_read_integer(sysfs_name, (int *) param);
}

static int stmvl53l1_get_integer_ioctl(int fd, uint32_t *param, stmv53l1_parameter_name_e parameter_name)
{
	int rc;

	struct stmvl53l1_parameter params;

	params.is_read = 1;
	params.name = parameter_name;

	rc= ioctl(fd, VL53L1_IOCTL_PARAMETER, &params);
	if( rc ){
		ioctl_error("%d %s", rc,strerror(errno));
	} else{
		rc = params.status;
		if( rc == 0 )
			*param = params.value;
	}

	return rc;
}

static int code_to_index(int code)
{
	int i;

	for(i = 0; i < EVENT_NB; i++) {
		if (index_to_code[i] == code)
			break;
	}

	return i == EVENT_NB ? -1 : i;
}

static char *test_event_path(char *event_path)
{
	int fd = -1;
	int rc;
	char name[256] = "\n";
	char *full_event_path = NULL;

	full_event_path = strjoin_with_separator("/dev/input", event_path);
	if (!full_event_path)
		goto error;

	fd = open(full_event_path, O_RDONLY);
	if (fd < 0)
		goto error;

	rc = ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	if (rc < 0)
		goto error;

	if (strcmp("STM VL53L1 proximity sensor", name))
		goto error;

	close(fd);

	return full_event_path;

error:
	if (full_event_path)
		free(full_event_path);
	if (fd >= 0)
		close(fd);

	return NULL;
}

static char *stmvl53l1_find_event_path()
{
	static char *event_path = NULL;

	if (!event_path) {
		DIR *d;
		struct dirent *dir;

		d = opendir("/dev/input");
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				event_path = test_event_path(dir->d_name);
				if (event_path)
					break;
			}
			closedir(d);
		}

		if (event_path)
			verbose("event path %s", event_path);
	}

	return event_path;
}

static int stmvl53l1_input_open()
{
	char *event = stmvl53l1_find_event_path();

	if (!event)
		return -1;

	return open(event, O_RDONLY);
}

static int open_input_subsystem()
{
	input_fd = stmvl53l1_input_open();
	int i;

	memset(&event_values, 0, sizeof(event_values));
	if (input_fd >= 0) {
		for(i = 0; i < EVENT_NB; i++) {
			struct input_absinfo info;
			int rc = ioctl(input_fd, EVIOCGABS(index_to_code[i]), &info);
			if (rc < 0) {
				close(input_fd);
				return -1;
			}
			event_values[i] = info.value;
		}
	}

	return input_fd;
}

static void close_input_subsystem()
{
	close(input_fd);
}

int write_file(char *name, char *buf, int len)
{
	int fd;
	int sz;

	fd = open(name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd < -1) {
		error("Unable to create %s", name);
		
		return fd;
	}

	while(len) {
		sz = write(fd, buf, len);
		if (sz <= 0) {
			error("Unable to write all data in %s", name);
			return -1;
		}
		len -= sz;
		buf += sz;
	}

	close(fd);

	return 0;
}

int read_file(char *name, char *buf, int len)
{
	int fd;
	int sz;

	fd = open(name, O_RDONLY);
	if (fd < -1) {
		error("Unable to open %s for read", name);
		
		return fd;
	}

	while(len) {
		sz = read(fd, buf, len);
		if (sz <= 0) {
			error("Unable to read all data from %s", name);
			return -1;
		}
		len -= sz;
		buf += sz;
	}

	close(fd);

	return 0;
}

static int stmvl53l1_start_sysfs(int fd)
{
	return stmvl53l1_sysfs_write_integer("enable_ps_sensor", 1);
}

static int stmvl53l1_start_ioctl(int fd){
	int rc;
	rc= ioctl(fd, VL53L1_IOCTL_START,NULL);
	if( rc ){
		if( errno == EBUSY){
			//the devise is already started mke err code >0
			ioctl_warn("already started or calibrating");
			return EBUSY;
		}
	}
	if( rc ){
		ioctl_error("%d %s", rc,strerror(errno));
	}
	return rc;
}

int stmvl53l1_start(int fd)
{
	return is_sysfs ? stmvl53l1_start_sysfs(fd) : stmvl53l1_start_ioctl(fd);
}

static int stmvl53l1_stop_sysfs(int fd)
{
	return stmvl53l1_sysfs_write_integer("enable_ps_sensor", 0);
}

static int stmvl53l1_stop_ioctl(int fd){
	int rc;
	rc= ioctl(fd, VL53L1_IOCTL_STOP,NULL);
	if( rc ){
		if( errno == EBUSY ){
			ioctl_warn("already stopped");
			return errno;
		}
		ioctl_error("%d %s", rc,strerror(errno));
	}
	return rc;
}

int stmvl53l1_stop(int fd)
{
	return is_sysfs ? stmvl53l1_stop_sysfs(fd) : stmvl53l1_stop_ioctl(fd);
}

IMPLEMENT_PARAMETER_INTEGER(timing_budget, VL53L1_TIMINGBUDGET_PAR)
IMPLEMENT_PARAMETER_INTEGER(set_delay_ms, VL53L1_POLLDELAY_PAR)
IMPLEMENT_PARAMETER_INTEGER(mode, VL53L1_DEVICEMODE_PAR)
IMPLEMENT_PARAMETER_INTEGER(distance_mode, VL53L1_DISTANCEMODE_PAR)
IMPLEMENT_PARAMETER_INTEGER(crosstalk_enable, VL53L1_XTALKENABLE_PAR)
IMPLEMENT_PARAMETER_INTEGER(output_mode, VL53L1_OUTPUTMODE_PAR)
IMPLEMENT_PARAMETER_INTEGER(offset_correction_mode, VL53L1_OFFSETCORRECTIONMODE_PAR)
IMPLEMENT_PARAMETER_INTEGER(force_device_on_enable, VL53L1_FORCEDEVICEONEN_PAR)
IMPLEMENT_PARAMETER_INTEGER(dmax_mode, VL53L1_DMAXMODE_PAR)
IMPLEMENT_PARAMETER_INTEGER(smudge_correction_mode, VL53L1_SMUDGECORRECTIONMODE_PAR)

int stmvl53l1_get_data(int fd, stmvl531_range_data_t *data){
	int rc;
	rc= ioctl(fd, VL53L1_IOCTL_GETDATAS, data);
	if( rc != 0){
		ioctl_error("%d %s", rc,strerror(errno));
	}
	return rc;
}


int stmvl53l1_get_mz_data(int fd, VL53L1_MultiRangingData_t *data){
	int rc;
	rc= ioctl(fd, VL53L1_IOCTL_MZ_DATA, data);
	if( rc != 0){
		ioctl_error("%d %s", rc,strerror(errno));
	}
	return rc;
}

int stmvl53l1_get_mz_data_additional(int fd, struct stmvl53l1_data_with_additional *data){
	int rc;
	rc= ioctl(fd, VL53L1_IOCTL_MZ_DATA_ADDITIONAL, data);
	if( rc != 0){
		ioctl_error("%d %s", rc,strerror(errno));
	}
	return rc;
}

static void process_ewok_event(struct input_event *event)
{
	int index = code_to_index(event->code);

	if (index < 0)
		debug("unknown code %d", event->code);
	else
		event_values[index] = event->value;
}

void event_values_to_data(stmvl531_range_data_t *data)
{
	data->TimeStamp = event_values[code_to_index(ABS_HAT0X)] * 1000 +
		event_values[code_to_index(ABS_HAT0Y)] / 1000;
	data->RangeMilliMeter = event_values[code_to_index(ABS_HAT1X)];
	data->RangeStatus = event_values[code_to_index(ABS_HAT1Y)];
	data->SignalRateRtnMegaCps = event_values[code_to_index(ABS_HAT2X)];
	data->AmbientRateRtnMegaCps = event_values[code_to_index(ABS_HAT2Y)];
	data->SigmaMilliMeter =event_values[code_to_index(ABS_HAT3X)];
//	data->DmaxMilliMeter = event_values[code_to_index(ABS_HAT3Y)];
	data->EffectiveSpadRtnCount = event_values[code_to_index(ABS_PRESSURE)];
//	data->RangeMaxMilliMeter = (event_values[code_to_index(ABS_TILT_X)] >> 16) & 0xffff;
//	data->RangeMinMilliMeter = (event_values[code_to_index(ABS_TILT_X)] >> 0) & 0xffff;
	data->RangeQualityLevel = event_values[code_to_index(ABS_TOOL_WIDTH)];
}

void event_values_to_targetdata(VL53L1_MultiRangingData_t *mdata, int obj_current)
{
	VL53L1_TargetRangeData_t *data = &mdata->RangeData[obj_current];
	mdata->TimeStamp = event_values[code_to_index(ABS_HAT0X)] * 1000 +
		event_values[code_to_index(ABS_HAT0Y)] / 1000;
	data->RangeMilliMeter = event_values[code_to_index(ABS_HAT1X)];
	data->RangeStatus = event_values[code_to_index(ABS_HAT1Y)];
	data->SignalRateRtnMegaCps = event_values[code_to_index(ABS_HAT2X)];
	data->AmbientRateRtnMegaCps = event_values[code_to_index(ABS_HAT2Y)];
	data->SigmaMilliMeter =event_values[code_to_index(ABS_HAT3X)];
	mdata->DmaxMilliMeter = event_values[code_to_index(ABS_HAT3Y)];
	mdata->EffectiveSpadRtnCount = event_values[code_to_index(ABS_PRESSURE)];
	data->RangeMaxMilliMeter = (event_values[code_to_index(ABS_TILT_X)] >> 16) & 0xffff;
	data->RangeMinMilliMeter = (event_values[code_to_index(ABS_TILT_X)] >> 0) & 0xffff;
	data->RangeQualityLevel = event_values[code_to_index(ABS_TOOL_WIDTH)];
}

static int collect_event_until_next_syn_report()
{
	struct input_event event;
	int is_syn_report_rcv = 0;
	int rc;

	while(!is_syn_report_rcv) {
		rc = read(input_fd, &event, sizeof(event));
		if (rc < 0 || rc != sizeof(event))
			return -1;
		switch(event.type) {
			case EV_ABS:
				process_ewok_event(&event);
				break;
			case EV_SYN:
				if (event.code == SYN_REPORT)
					is_syn_report_rcv = 1;
				else if (event.code == SYN_DROPPED) {
					debug("loose sync");
					return -1;
				} else {
					debug("unknown code %d", event.code);
					return -1;
				}
				break;
			default:
				debug("unsupported event type = %d", event.type);
		}
	}

	return 0;
}

static int stmvl53l1_get_data_blocking_sysfs(int fd, stmvl531_range_data_t *data)
{
	int rc;

	rc = collect_event_until_next_syn_report();
	if (rc)
		return rc;
	event_values_to_data(data);

	return 0;
}

static int stmvl53l1_get_data_blocking_ioctl(int fd, stmvl531_range_data_t *data)
{
	int rc;

	rc= ioctl(fd, VL53L1_IOCTL_GETDATAS_BLOCKING, data);
	if(rc != 0) {
		ioctl_error("%d %s", rc,strerror(errno));
	}

	return rc;
}

int stmvl53l1_get_data_blocking(int fd, stmvl531_range_data_t *data)
{
	return is_sysfs ? stmvl53l1_get_data_blocking_sysfs(fd, data)
		: stmvl53l1_get_data_blocking_ioctl(fd, data);
}

static int event_values_to_multidata(VL53L1_MultiRangingData_t *data)
{
	int roiStatus = (event_values[code_to_index(ABS_BRAKE)] >> 24) & 0xff;
	int roi = (event_values[code_to_index(ABS_BRAKE)] >> 16) & 0xff;
	int obj_number = (event_values[code_to_index(ABS_BRAKE)] >> 8) & 0xff;
	int obj_current = (event_values[code_to_index(ABS_BRAKE)] >> 0) & 0xff;

	data->RoiNumber = roi;
	data->NumberOfObjectsFound = obj_number;
	data->RoiStatus = roiStatus;
	event_values_to_targetdata(data, obj_current);

	return obj_number == (obj_current + 1);
}

static int stmvl53l1_get_mz_data_blocking_sysfs(int fd, VL53L1_MultiRangingData_t *data)
{
	int rc;
	int is_last = 0;

	do {
		rc = collect_event_until_next_syn_report();
		if (rc)
			return rc;
		is_last = event_values_to_multidata(data);
	} while(!is_last);

	return 0;
}

static int stmvl53l1_get_mz_data_blocking_ioctl(int fd, VL53L1_MultiRangingData_t *data)
{
	int rc;
	rc= ioctl(fd, VL53L1_IOCTL_MZ_DATA_BLOCKING, data);
	if( rc != 0){
		ioctl_error("%d %s", rc,strerror(errno));
	}
	return rc;
}

int stmvl53l1_get_mz_data_blocking(int fd, VL53L1_MultiRangingData_t *data)
{
	return is_sysfs ? stmvl53l1_get_mz_data_blocking_sysfs(fd, data)
		: stmvl53l1_get_mz_data_blocking_ioctl(fd, data);
}

int stmvl53l1_get_mz_data_blocking_additional(int fd, struct stmvl53l1_data_with_additional *data)
{
	int rc;
	rc= ioctl(fd, VL53L1_IOCTL_MZ_DATA_ADDITIONAL_BLOCKING, data);
	if( rc != 0){
		ioctl_error("%d %s", rc,strerror(errno));
	}
	return rc;
}

extern int parse_roi_arg(const char *buff, VL53L1_UserRoi_t *rois, uint32_t *p_n_roi);
static int stmvl53l1_get_roi_sysfs( int fd, VL53L1_UserRoi_t *roi, uint32_t n_roi, uint32_t *used_roi)
{
	int rc;
	char b[SYSFS_MAX_LEN];
	struct stmvl53l1_roi_full_t ioctl_roi;

	rc = stmvl53l1_sysfs_read_string("roi", b);
	if (rc < 0)
		return rc;

	if (strncmp("device default", b, strlen("device default")) == 0) {
		ioctl_roi.roi_cfg.NumberOfRoi = 0;
	} else {
		uint32_t nb_roi;

		rc = parse_roi_arg(b, ioctl_roi.roi_cfg.UserRois, &nb_roi);
		if (rc >= 0) {
			ioctl_roi.roi_cfg.NumberOfRoi = nb_roi;
			rc = 0;
		}
	}

	if (!rc) {
		//avoid core dump and onlyuy update number of roi in case we have to do bug check
		int to_cpy = n_roi> VL53L1_MAX_USER_ZONES ? VL53L1_MAX_USER_ZONES : n_roi;
		memcpy( roi, ioctl_roi.roi_cfg.UserRois, to_cpy*sizeof(*roi));
		*used_roi = ioctl_roi.roi_cfg.NumberOfRoi;
	}

	return rc;
}

static int stmvl53l1_get_roi_ioctl( int fd, VL53L1_UserRoi_t *roi, uint32_t n_roi, uint32_t *used_roi)
{
	struct stmvl53l1_roi_full_t ioctl_roi;
	int rc;
	ioctl_roi.is_read = 1;
	ioctl_roi.roi_cfg.NumberOfRoi = n_roi;
	rc= ioctl(fd, VL53L1_IOCTL_ROI, &ioctl_roi);
	if( rc != 0){
		ioctl_error("%d %s", rc,strerror(errno));
	}
	else{
		//avoid core dump and onlyuy update number of roi in case we have to do bug check
		int to_cpy = n_roi> VL53L1_MAX_USER_ZONES ? VL53L1_MAX_USER_ZONES : n_roi;
		memcpy( roi, ioctl_roi.roi_cfg.UserRois, to_cpy*sizeof(*roi));
		*used_roi = ioctl_roi.roi_cfg.NumberOfRoi;
	}
	return rc;
}

int stmvl53l1_get_roi( int fd, VL53L1_UserRoi_t *roi, uint32_t n_roi, uint32_t *used_roi)
{
	return is_sysfs ? stmvl53l1_get_roi_sysfs(fd, roi, n_roi, used_roi)
		: stmvl53l1_get_roi_ioctl(fd, roi, n_roi, used_roi);
}


static int stmvl53l1_set_roi_sysfs( int fd, VL53L1_UserRoi_t *roi, uint32_t n_roi)
{
	int rc;

	if (n_roi > VL53L1_MAX_USER_ZONES) {
		error("can't set %d roi > %d", (int)n_roi, VL53L1_MAX_USER_ZONES);
		return -1;
	}

	if (n_roi) {
		char b[SYSFS_MAX_LEN];
		char *c = b;
		int i;

		for(i = 0; i < n_roi; i++) {
			if (i)
				c += sprintf(c, ", ");
			c += sprintf(c, "%d %d %d %d",
				roi[i].TopLeftX,
				roi[i].TopLeftY,
				roi[i].BotRightX,
				roi[i].BotRightY);
		}
		sprintf(c, "\n");
		rc = stmvl53l1_sysfs_write_string("roi", b);
	} else {
		rc = stmvl53l1_sysfs_write_string("roi", "");
	}

	return rc;
}

static int stmvl53l1_set_roi_ioctl( int fd, VL53L1_UserRoi_t *roi, uint32_t n_roi)
{
	struct stmvl53l1_roi_full_t ioctl_roi;
	int rc;
	//int arg_len;
	if( n_roi > VL53L1_MAX_USER_ZONES){
		error("can't set %d roi > %d", (int)n_roi, VL53L1_MAX_USER_ZONES);
		return -1;
	}
	ioctl_roi.is_read = 0;
	ioctl_roi.roi_cfg.NumberOfRoi = n_roi;
	if( n_roi ){
		memcpy( ioctl_roi.roi_cfg.UserRois, roi, sizeof(VL53L1_UserRoi_t)*n_roi);
	}
	rc= ioctl(fd, VL53L1_IOCTL_ROI, &ioctl_roi);
	if( rc != 0){
		ioctl_error("%d %s", rc,strerror(errno));
	}
	return rc;
}

int stmvl53l1_set_roi( int fd, VL53L1_UserRoi_t *roi, uint32_t n_roi)
{
	return is_sysfs ? stmvl53l1_set_roi_sysfs(fd, roi, n_roi)
		: stmvl53l1_set_roi_ioctl(fd, roi, n_roi);
}

int stmvl53l1_perform_calibration(int fd, uint32_t calibration_type, uint32_t param1, uint32_t param2, uint32_t param3)
{
	int rc;
	struct stmvl53l1_ioctl_perform_calibration_t ioctl_cal = {calibration_type, param1, param2, param3};

	rc = ioctl(fd, VL53L1_IOCTL_PERFORM_CALIBRATION, &ioctl_cal);
	if( rc != 0)
		ioctl_error("%d %s", rc, strerror(errno));

	return rc;
}

int stmvl53l1_calibration_data(int fd, struct stmvl53l1_ioctl_calibration_data_t *cal)
{
	int rc;

	rc = ioctl(fd, VL53L1_IOCTL_CALIBRATION_DATA, cal);
	if( rc != 0)
		ioctl_error("%d %s", rc, strerror(errno));

	return rc;
}

int stmvl53l1_zone_calibration_data(int fd, struct stmvl53l1_ioctl_zone_calibration_data_t *cal)
{
	int rc;

	rc = ioctl(fd, VL53L1_IOCTL_ZONE_CALIBRATION_DATA, cal);
	if( rc != 0)
		ioctl_error("%d %s", rc, strerror(errno));

	return rc;
}

static int stmvl53l1_set_dmax_reflectance_sysfs(int fd, float reflectance)
{
	return stmvl53l1_sysfs_write_float("dmax_reflectance", reflectance);
}

static int stmvl53l1_set_dmax_reflectance_ioctl(int fd, float reflectance)
{
	int rc;

	struct stmvl53l1_parameter params;

	params.is_read = 0;
	params.name = VL53L1_DMAXREFLECTANCE_PAR;
	params.value = auto_float_to_16x16(reflectance);

	rc= ioctl(fd, VL53L1_IOCTL_PARAMETER, &params);
	if( rc ){
		if( errno == EBUSY ){
			ioctl_warn("ebusy can't set now");
			return errno;
		}
		ioctl_error("%d %s", rc,strerror(errno));
	}
	return rc;
}

int stmvl53l1_set_dmax_reflectance(int fd, float reflectance)
{
	return is_sysfs ? stmvl53l1_set_dmax_reflectance_sysfs(fd, reflectance)
		: stmvl53l1_set_dmax_reflectance_ioctl(fd, reflectance);
}


static int stmvl53l1_get_dmax_reflectance_sysfs(int fd, float *reflectance)
{
	return stmvl53l1_sysfs_read_float("dmax_reflectance", reflectance);
}

static int stmvl53l1_get_dmax_reflectance_ioctl(int fd, float *reflectance)
{
	int rc;

	struct stmvl53l1_parameter params;

	params.is_read = 1;
	params.name = VL53L1_DMAXREFLECTANCE_PAR;

	rc= ioctl(fd, VL53L1_IOCTL_PARAMETER, &params);
	if( rc ){
		ioctl_error("%d %s", rc,strerror(errno));
	}
	else{
		// sucessfull ioctl check status
		rc = params.status ;
		if( rc == 0 ) {
			*reflectance  = auto_16x16_to_float(params.value);
		}
	}
	return rc;
}

int stmvl53l1_get_dmax_reflectance(int fd, float *reflectance)
{
	return is_sysfs ? stmvl53l1_get_dmax_reflectance_sysfs(fd, reflectance)
		: stmvl53l1_get_dmax_reflectance_ioctl(fd, reflectance);
}

static int stmvl53l1_get_optical_center_sysfs(int fd, float *x, float *y)
{
	return stmvl53l1_sysfs_read_two_float("optical_center", x, y);
}

static int stmvl53l1_get_optical_center_ioctl(int fd, float *x, float *y)
{
	int rc;

	struct stmvl53l1_parameter params;

	params.is_read = 1;
	params.name = VL53L1_OPTICALCENTER_PAR;

	rc= ioctl(fd, VL53L1_IOCTL_PARAMETER, &params);
	if( rc ){
		ioctl_error("%d %s", rc,strerror(errno));
	}
	else{
		// sucessfull ioctl check status
		rc = params.status ;
		if( rc == 0 ){
			*x  = auto_16x16_to_float(params.value);
			*y  = auto_16x16_to_float(params.value2);
		}
	}
	return rc;
}

int stmvl53l1_get_optical_center(int fd, float *x, float *y)
{
	return is_sysfs ? stmvl53l1_get_optical_center_sysfs(fd, x, y)
		: stmvl53l1_get_optical_center_ioctl(fd, x, y);
}

static char *auto_detectionmode_2_str(VL53L1_DetectionMode DetectionMode)
{
	switch(DetectionMode) {
		case VL53L1_DETECTION_NORMAL_RUN:
			return "normal";
		case VL53L1_DETECTION_DISTANCE_ONLY:
			return "distance only";
		case VL53L1_DETECTION_RATE_ONLY:
			return "rate only";
		case VL53L1_DETECTION_DISTANCE_AND_RATE:
			return "distance and rate";
		case VL53L1_DETECTION_DISTANCE_OR_RATE:
			return "distance or rate";
	}

	return "unknown";
}

static char *auto_crossmode_2_str(VL53L1_ThresholdMode CrossMode)
{
	switch(CrossMode) {
		case VL53L1_THRESHOLD_CROSSED_LOW:
			return "low";
		case VL53L1_THRESHOLD_CROSSED_HIGH:
			return "high";
		case VL53L1_THRESHOLD_OUT_OF_WINDOW:
			return "out of window";
		case VL53L1_THRESHOLD_IN_WINDOW:
			return "in window";
	}

	return "unknown";
}

static void display_auto_config_raw(struct stmvl53l1_autonomous_config_t *full)
{
	printf("%d %d %d ", full->pollingTimeInMs, full->config.DetectionMode, full->config.IntrNoTarget);
	printf("%d %d %d ", full->config.Distance.CrossMode, full->config.Distance.High, full->config.Distance.Low);
	printf("%d %f %f ", full->config.Rate.CrossMode, auto_16x16_to_float(full->config.Rate.High), auto_16x16_to_float(full->config.Rate.Low));
	printf("\n");
}

static void display_auto_config_cook(struct stmvl53l1_autonomous_config_t *full)
{
	printf("- pollingTimeInMs = %d ms\n", full->pollingTimeInMs);
	printf("- config\n");
	printf(" - DetectionMode = %s\n", auto_detectionmode_2_str(full->config.DetectionMode));
	printf(" - IntrNoTarget = %d\n", full->config.IntrNoTarget);
	printf(" - Distance\n");
	printf("  - CrossMode = %s\n", auto_crossmode_2_str(full->config.Distance.CrossMode));
	printf("  - High = %5d mm\n", full->config.Distance.High);
	printf("  - Low  = %5d mm\n", full->config.Distance.Low);
	printf(" - Rate\n");
	printf("  - CrossMode = %s\n", auto_crossmode_2_str(full->config.Rate.CrossMode));
	printf("  - High = %f\n", auto_16x16_to_float(full->config.Rate.High));
	printf("  - Low  = %f\n", auto_16x16_to_float(full->config.Rate.Low));
}

static int parse_auto_config(char *buffer, struct stmvl53l1_autonomous_config_t *full)
{
	int n;
	float high, low;
	int DetectionMode;
	int IntrNoTarget;
	int DistanceCrossMode;
	int DistanceHigh;
	int DistanceLow;
	int RateCrossMode;

	n = sscanf(buffer, "%d %d %d %d %d %d %d %f %f",
		&full->pollingTimeInMs,
		&DetectionMode,
		&IntrNoTarget,
		&DistanceCrossMode,
		&DistanceHigh,
		&DistanceLow,
		&RateCrossMode,
		&high,
		&low);
	full->config.Rate.High = auto_float_to_16x16(high);
	full->config.Rate.Low  = auto_float_to_16x16(low);
	full->config.DetectionMode = DetectionMode;
	full->config.IntrNoTarget = IntrNoTarget;
	full->config.Distance.CrossMode = DistanceCrossMode;
	full->config.Distance.High = DistanceHigh;
	full->config.Distance.Low = DistanceLow;
	full->config.Rate.CrossMode = RateCrossMode;

	if (n != 9)
		return -1;

	return 0;
}

static int stmvl53l1_set_auto_config_sysfs(int fd, char *buffer)
{
	return stmvl53l1_sysfs_write_string("autonomous_config", buffer);
}

static int stmvl53l1_set_auto_config_ioctl(int fd, char *buffer)
{
	int rc;
	struct stmvl53l1_autonomous_config_t full;

	rc = parse_auto_config(buffer, &full);
	if (rc) {
		error("unable to parse\n");
		return rc;
	}

	full.is_read = 0;
	rc = ioctl(fd, VL53L1_IOCTL_AUTONOMOUS_CONFIG, &full);
	if( rc ){
		ioctl_error("%d %s", rc,strerror(errno));
	}

	return rc;
}

int stmvl53l1_set_auto_config(int fd, char *buffer)
{
	return is_sysfs ? stmvl53l1_set_auto_config_sysfs(fd, buffer)
		: stmvl53l1_set_auto_config_ioctl(fd, buffer);
}

static int stmvl53l1_get_auto_config_sysfs(int fd)
{
	int rc;
	char b[SYSFS_MAX_LEN];
	struct stmvl53l1_autonomous_config_t full;
	float high, low;
	int DetectionMode;
	int IntrNoTarget;
	int DistanceCrossMode;
	int DistanceHigh;
	int DistanceLow;
	int RateCrossMode;

	rc = stmvl53l1_sysfs_read_string("autonomous_config", b);
	if (rc < 0)
		return rc;

	rc = sscanf(b, "%d %d %d %d %d %d %d %f %f",
		&full.pollingTimeInMs,
		&DetectionMode,
		&IntrNoTarget,
		&DistanceCrossMode,
		&DistanceHigh,
		&DistanceLow,
		&RateCrossMode,
		&high,
		&low);
	if (rc != 9)
		return -1;

	full.config.Rate.High = auto_float_to_16x16(high);
	full.config.Rate.Low = auto_float_to_16x16(low);
	full.config.DetectionMode = DetectionMode;
	full.config.IntrNoTarget = IntrNoTarget;
	full.config.Distance.CrossMode = DistanceCrossMode;
	full.config.Distance.High = DistanceHigh;
	full.config.Distance.Low = DistanceLow;
	full.config.Rate.CrossMode = RateCrossMode;
	display_auto_config_raw(&full);
	display_auto_config_cook(&full);

	return 0;
}

static int stmvl53l1_get_auto_config_ioctl(int fd)
{
	int rc;
	struct stmvl53l1_autonomous_config_t full;

	full.is_read = 1;
	rc = ioctl(fd, VL53L1_IOCTL_AUTONOMOUS_CONFIG, &full);
	if( rc ){
		ioctl_error("%d %s", rc,strerror(errno));
	}
	else{
		/* display config */
		display_auto_config_raw(&full);
		display_auto_config_cook(&full);
	}

	return rc;
}

int stmvl53l1_get_auto_config(int fd)
{
	return is_sysfs ? stmvl53l1_get_auto_config_sysfs(fd)
		: stmvl53l1_get_auto_config_ioctl(fd);
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



static void help(int do_exit)
{
	fprintf(stderr,	"Usage: phio <option1> <option2> ....\n"
			"\t[-d --dev dev_no]\t dev_no shall be set at first arg\n"
			"\t[-m --mode[=set_mode ]] set/get ranging mode\n"
				"\t\t mode value  1 = standard, 2 =  multi zone , 3 = autonomous, 4 = lite ranging\n"
			"\t[-M --Meas[=no_of_meas]] get [no_of_meas] sample measurement\n"
			"\t[-o --roi_get no_of_roi  get up to no_of_roi\n"
			"\t[-O --roi_set[\"x0 y0 x1 y1[,[x y x y]up to 15x]]\" set rois \n\tno arg will unset roi\n"
				"\t\tbe aware of the xy system\n"
			"\t[-p[=time ms] get device poll delay [-p=xxxx] set poll delay to xxx ms\n"
				"\t\t use -P for internal ioctl polling delay\n"
			"\t[-P --Pause time_ms]\t set test loop/poll pause time_ms in milli sec\n"
			"\t[-r[=no_of_call] --range [no_of_call]] get range data (raw) repeat no_of_call time (override TBD)\n"
			"\t-s --start start ranging\n"
			"\t-S --stop  stop ranging\n"
			"\t-t [=time_us] get timing budget [=set it]\n"
			"\t-z [=no_of_call] get multi zone data\n"
			"\t-Z no_of_range  poll multi zone ranging \n"
			"\t-R --Ref_spad perform reference spad calibration \n"
			"\t-X --Xtalk <method> perform crosstalk calibration using method \n"
			"\t-F --oFfset <\"mode distance reflectance\"> perform offset calibration of type mode with target at given distance in mm and with given reflectance in percent\n"
			"\t-c --cal <file> get calibration data and write them in given file name\n"
			"\t-C --Cal <file> set calibrating data by reading given file name\n"
			"\t-D --Distance[=distance_mode] set/get distance mode\n"
				"\t\t distance value 1 = short, 2 = medium, 3 = long\n"
			"\t-E --xtalk_Enable[=<0/1>] set/get crosstalk compensation enable\n"
			"\t-U --oUtput[=output_mode] set/get output mode\n"
				"\t\t output mode value 1 = nearest, 2 = strongest\n"
			"\t-f --force[=<0/1>] set/get force device on enable\n"
			"\t-a --auto=[\"poll_ms mode no_target_irq distance_mode distance_low distance_high rate_mode rate_low rate_high\"] set/get autonomous configuration\n"
			"\t-y --sysfs_on turn on sysfs usage instead of ioctl\n"
			"\t-Y --sysfs_off turn off sysfs usage instead of ioctl\n"
			"\t-T  --offseT_correction_mode[=offset_correction_mode] set/get offset correction mode\n"
				"\t\t offset correction mode value 1 = standard, 2 = per zone\n"
			"\t-I  --optical_center get optical center offset in roi coordinate\n"
			"\t-A  --dmax_reflectance[=reflectance] set/get reflectance used for dmax calculation in percent\n"
			"\t-B  --dmax_mode[=dmax_mode] set/get dmax mode\n"
			"\t-w --wcal <file> get zone calibration data and write them in given file name\n"
			"\t-W --Wcal <file> set zone calibrating data by reading given file name\n"
			"\t-N --tuNing=\"<key> <value>\" set low level layer parameter with key to given value\n"
			"\t-g --tunings <file> get tunings values and write them in given file name\n"
			"\t-G --tuninGs <file> set tunings values by reading given file name\n"
			"\t-h --smudge[=smudge_mode] set/get smudge correction mode\n"
			"\t\t smudge_mode value  0 = disable, 1 =  continuous , 2 = single, 3 = debug\n"
			"\t-H --runtest <testName> run a specific test\n"
			"\t-l --transform calibration data from bin to text\n"
			"\t-L --transform calibration data from text to bin\n"
			/* These two options are not documented since it should not be used
			"\t-j [=no_of_call] get multi zone data with additional data\n"
			"\t-J no_of_range  poll multi zone ranging  with additional data\n"
			*/
			);

	fprintf(stderr, "\nExamples:\n"
					" - get current timing budget value\n"
					"\t ./phio -t\n"
					" - take 10 measures in ranging mode\n"
					"\t ./phio -O -m=1 -s -Z=10 -S\n"
					" - take 10 measures in multi zone mode with 4 zones\n"
					"\t ./phio -O=\"0 15 7 8, 8 15 15 8, 0 7 7 0, 8 7 15 0\" -m=2 -s -Z=10 -S\n"
					" - take 10 measures in lite ranging at 10 Hz and restore 60hz\n"
					"\t ./phio -t=100000 -O -m=4 -s -M=10 -S -t=16000\n");


	if( do_exit){
		exit(do_exit);
	}
}


int dev_no=0;
char dev_fi_name[256];
int dev_fd=-1;
int run_loops=1; // run cmd only this numebr of time defautl to run once
int run_poll_ms=0;

stmvl531_range_data_t dev_1xrange_data; /* last read */
VL53L1_MultiRangingData_t dev_mz_range_data;
struct stmvl53l1_data_with_additional dev_mz_range_data_additional;

//struct stmvl53l1_roi_full_t dev_rois;
VL53L1_UserRoi_t dev_rois[VL53L1_MAX_USER_ZONES];
/**
 * cmd line option
 * @warning keep in synch with @a string_options
 */
static struct option long_options[] =
{
	{"calbin2txt",       required_argument, 0,  'l'},
	{"caltxt2bin",       required_argument, 0,  'L'},
	{"dev",       required_argument, 0,  'd'},
	{"help",      no_argument,	 0 , '?'},
	{"mode",      optional_argument, 0 , 'm'},
	{"Meas",      optional_argument, 0 , 'M'},
	{"roi_get",   required_argument, 0 , 'o'},
	{"rOi_set",   optional_argument, 0 , 'O'},
	{"Poll",      required_argument, 0 , 'P'}, // program poll
	{"range",     optional_argument, 0 , 'r'},
	{"start",     no_argument,	 0 , 's'},
	{"stop",      no_argument,	 0 , 'S'},
	{"zone",      optional_argument, 0 , 'z'},
	{"Zone",      required_argument, 0 , 'Z'},
	{"Ref_spad",  no_argument,       0 , 'R'},
	{"Xtalk",     required_argument, 0 , 'X'},
	{"oFfset",    required_argument, 0 , 'F'},
	{"cal",       required_argument, 0 , 'c'},
	{"Cal",       required_argument, 0 , 'C'},
	{"Distance",  optional_argument, 0 , 'D'},
	{"xtalk_Enable", optional_argument, 0, 'E'},
	{"oUtput",    optional_argument, 0 , 'U'},
	{"force",     optional_argument, 0, 'f'},
	{"auto",      optional_argument, 0, 'a'},
	{"sysfs_on",  no_argument,	 0 , 'y'},
	{"sysfs_off",  no_argument,	 0 , 'Y'},
	{"offseT_correction_mode",  optional_argument,	 0 , 'T'},
	{"optIcal_center",	no_argument,	0, 'I'},
	{"dmax_reflectance",	optional_argument, 	0, 'A'},
	{"dmax_mode",	optional_argument, 	0, 'B'},
	{"wcal",      required_argument, 0 , 'w'},
	{"Wcal",      required_argument, 0 , 'W'},
	{"tuNing",    required_argument, 0 , 'N'},
	{"tunings",   required_argument, 0 , 'g'},
	{"tuninGs",   required_argument, 0 , 'G'},
	{"smudge",    required_argument, 0 , 'h'},
	{"zone_additional", optional_argument, 0 , 'j'},
	{"Zone_additional", required_argument, 0 , 'J'},
	{"run_test_config", required_argument, 0 , 'H'},
	/* must be last since then long options are no more detected */
	{NULL,        optional_argument, 0 , 't'}, //timing budget
	{NULL,        optional_argument, 0 , 'p'}, // device poll delay
	//last
	{0, 0, 0, 0}
};
/**
 * cmd line option string keep in sync with @a long_options
 * sythax is
 * 	'x' => no arg option for   case 0:
 *	"x:" => required arg for shorft opt char  x
 * *	"x::" => optional arg for short opt char  x
 */
char string_options[]="L:l:d:?m::M::o:O::p::P:r::sSt::z::Z:RX:F:c:C:D::E::U::f::a::yYT::IA::B::w:W:N:g:G:h::j::J:H:";

void set_dev_name(void){
	if( dev_no == 0 ){
		strcpy(dev_fi_name, "/dev/" VL53L1_MISC_DEV_NAME);
	}else{
		snprintf(dev_fi_name, sizeof(dev_fi_name), "%s%d","/dev/" VL53L1_MISC_DEV_NAME, dev_no);
	}
}

void set_new_device(int no) {
	if( dev_fd>0 ){
		verbose("closing %s", dev_fi_name);
		close(dev_fd);
		dev_fd=-1;
	}
	//will be reopen when needed
	dev_no=no;
	set_dev_name();
	verbose("to use dev %d %s", dev_no, dev_fi_name);
}



/**
 * open the currently set device dev no / dev_name if needed
 *  it exit if that fail
 *
 * does nothing if dev already open (will nto check if dev no did chaneg etc .)
 * @note to changed use @a set_new_device that will take care of closing existing one and set dev name
 */
void open_dev_or_die(){
	// open if not yet open
	if( dev_fd<0 ){
		set_dev_name();//ensure device name  match current dev no
		dev_fd = open(dev_fi_name ,O_RDWR );
		verbose("open fd %d for %s", dev_fd, dev_fi_name);
		if (dev_fd < 0) {
			error("open of %s", dev_fi_name);
			exit(1);
		}
	}else{
		verbose("keep using fd %d", dev_fd);
	}

}



void print_1xdata(FILE * fi, stmvl531_range_data_t *range_data){
	fprintf(fi, "cnt %4d st %d\t"
		"d=%4dmm\t"
		"min/max=%4d/%4d\t"
		"dmax=%d\t"
		"qual=%3d\t"
		"spdcnt=%d\t"
		"sigma=%3d.%02d\t"
		"rate=%5d.%02d/%5d.%02d"
		"\n",
		(int)range_data->StreamCount, (int)range_data->RangeStatus,
		(int)range_data->RangeMilliMeter, 0, 0, 0,
//		(int)range_data->RangeMinMilliMeter,
//		(int)range_data->RangeMaxMilliMeter,
//		(int)range_data->DmaxMilliMeter,
		(int)range_data->RangeQualityLevel,
		(int)range_data->EffectiveSpadRtnCount / 256,
		(int)range_data->SigmaMilliMeter >> 16,
		(int)((range_data->SigmaMilliMeter & 0xffff) * 100) >> 16,
		(int)range_data->SignalRateRtnMegaCps >> 16,
		(int)((range_data->SignalRateRtnMegaCps & 0xffff) * 100) >> 16,
		(int)range_data->AmbientRateRtnMegaCps >> 16,
		(int)((range_data->AmbientRateRtnMegaCps & 0xffff) * 100) >> 16
		);
}

void print_target1xdata(FILE * fi, VL53L1_TargetRangeData_t *range_data){
	fprintf(fi, "cnt %4d st %d\t"
		"d=%4dmm\t"
		"min/max=%4d/%4d\t"
		"dmax=%d\t"
		"qual=%3d\t"
		"spdcnt=%d\t"
		"sigma=%3d.%02d\t"
		"rate=%5d.%02d/%5d.%02d"
		"\n", 0,
//		(int)range_data->StreamCount,
		(int)range_data->RangeStatus,
		(int)range_data->RangeMilliMeter,
		(int)range_data->RangeMinMilliMeter,
		(int)range_data->RangeMaxMilliMeter, 0,
//		(int)range_data->DmaxMilliMeter,
		(int)range_data->RangeQualityLevel, 0,
//		(int)range_data->EffectiveSpadRtnCount / 256,
		(int)range_data->SigmaMilliMeter >> 16,
		(int)((range_data->SigmaMilliMeter & 0xffff) * 100) >> 16,
		(int)range_data->SignalRateRtnMegaCps >> 16,
		(int)((range_data->SignalRateRtnMegaCps & 0xffff) * 100) >> 16,
		(int)range_data->AmbientRateRtnMegaCps >> 16,
		(int)((range_data->AmbientRateRtnMegaCps & 0xffff) * 100) >> 16
		);
}

static char *roiStatus2string(VL53L1_RoiStatus st)
{
	char *msgs[] = {"is invalid roi", "is not last roi", "is last roi"};

	return msgs[st];
}

void print_mz_data(FILE *fi, VL53L1_MultiRangingData_t *data ){
//	VL53L1_TargetRangeData_t *prange;
	int max_obj, obj;
//	prange = &data->RangeData[0];
	fprintf(fi, "range ts %8u\t"
			"\troi %2d %s (%d)%s"
			"\n",
			(int)data->TimeStamp,
			data->RoiNumber, roiStatus2string(data->RoiStatus), data->RoiStatus,
			data->HasXtalkValueChanged ? " XTALK UPDATE" : "");

	max_obj = MIN(VL53L1_MAX_RANGE_RESULTS, data->NumberOfObjectsFound);
	for(obj=0; obj < max_obj; obj++) {
		fprintf(fi, " - [%d]\t", obj);
		print_target1xdata(fi, &data->RangeData[obj]);
	}
}

void print_mz_data_additional(FILE *fi, VL53L1_AdditionalData_t *data)
{
	int i;

	fprintf(fi, " - DBG: ");
	for (i = 0; i < VL53L1_MAX_BIN_SEQUENCE_LENGTH; ++i)
		fprintf(fi, "[%3d %5d]%c", data->VL53L1_PRM_00009.bin_seq[i], data->VL53L1_PRM_00009.bin_data[i], i + 1 == VL53L1_MAX_BIN_SEQUENCE_LENGTH ? '\n' : ' ');
}

void dev_print_mz_data(){
	print_mz_data(stdout, &dev_mz_range_data);
}

void dev_print_mz_data_additional(){
	print_mz_data(stdout, &dev_mz_range_data_additional.data);
	print_mz_data_additional(stdout, &dev_mz_range_data_additional.additional_data);
}

void dev_print_1xdata(){

	print_1xdata(stdout, &dev_1xrange_data);
}

int run_meas_loop(int n_loops)
{
	int rc = 0;
	int i;
	uint32_t prev_ts = -1;

	if (is_sysfs) {
		rc = open_input_subsystem();
		if (rc < 0)
			return rc;
	}
	for(i=0; i< n_loops; i++) {
		rc = stmvl53l1_get_data_blocking(dev_fd, &dev_1xrange_data);
		if (rc != 0) {
			error("loop #%d fail to get data code=%d",i, rc);
			break;
		}
		printf("#%c%5d dts=%8d\t", is_sysfs ? 'S' : 'I' ,i, i==0 ? 0 : dev_1xrange_data.TimeStamp - prev_ts);
		dev_print_1xdata();
		prev_ts = dev_1xrange_data.TimeStamp;
	}
	if (is_sysfs)
		close_input_subsystem();

	return rc;
}

#define roi_fprintf fprintf

void dump_roi(FILE * fi, VL53L1_UserRoi_t *rois, uint32_t n ){
	uint32_t i;
	for( i = 0; i< n ; i++){
		roi_fprintf(fi, "ROI#%02d %2d %2d %2d %2d\n", (int)i,
				(int)rois[i].TopLeftX, (int)rois[i].TopLeftY,
				(int)rois[i].BotRightX, (int)rois[i].BotRightY);
	}
}

/**
 * parse rois string
 *
 *  roi string in format "tlx tly br bry [,tlx tly br bry]*up_to_15x"
 *
 *  @warning parser is very simple rely on exact syntax it will not track extra token after valeu and ,
 *
 * @param buff string to parse empy string is valdi no roi
 * @param rois data (VL53L1_MAX_USER_ZONES length)  it may be partially set on error
 * @param p_n_roi [out] not set in error
 * @return >=0  valid parsing and nof of \n
 * <0 parse error p_n_roi indicates last valid parsed adn stored roi
 */
int parse_roi_arg(const char *buff, VL53L1_UserRoi_t *rois, uint32_t *p_n_roi){
	int n_roi=0;
	int n;
	const char *pc=buff;
	int tlx, tly, brx, bry;
	while(	n_roi < VL53L1_MAX_USER_ZONES && pc!=NULL && *pc != 0)
	{
		n= sscanf(pc, "%d %d %d %d", &tlx, &tly, &brx, &bry);
		if( n == 4 ){
			rois[n_roi].TopLeftX = tlx;
			rois[n_roi].TopLeftY = tly;
			rois[n_roi].BotRightX = brx;
			rois[n_roi].BotRightY = bry;
			n_roi++;
		}
		else{
			error("wrong roi syntax around %s of %s", pc, buff);
			*p_n_roi= n_roi;
			return -1;
		}
		// find next roi
		pc = strchr(pc, ',');
		if( pc ){
			pc++;
		}
	}
	*p_n_roi= n_roi;
	return n_roi;
}

int run_zone_loop(int n_loops)
{
	int i;
	int rc = 0;
	uint32_t cur_ts;
	uint32_t prev_ts = -1;

	if (is_sysfs) {
		rc = open_input_subsystem();
		if (rc < 0)
			return rc;
	}
	for(i=0; i< n_loops; i++) {
		rc = stmvl53l1_get_mz_data_blocking(dev_fd, &dev_mz_range_data);
		if(rc != 0) {
			error("loop #%d fail to get data code=%d", i, rc);
			break;
		}
		cur_ts = dev_mz_range_data.TimeStamp;
		printf("#%c%5d dts=%8d\t", is_sysfs ? 'S' : 'I',i, i==0 ? 0 : cur_ts - prev_ts);
		dev_print_mz_data();
		prev_ts = cur_ts;
	}
	if (is_sysfs)
		close_input_subsystem();

	return rc;
}

int run_zone_loop_additional(int n_loops)
{
	int i;
	int rc = 0;
	uint32_t cur_ts;
	uint32_t prev_ts = -1;

	for(i=0; i< n_loops; i++) {
		rc = stmvl53l1_get_mz_data_blocking_additional(dev_fd, &dev_mz_range_data_additional);
		if(rc != 0) {
			error("loop #%d fail to get data code=%d", i, rc);
			break;
		}
		cur_ts = 0;
		printf("#I%5d dts=%8d\t",i, i==0 ? 0 : cur_ts - prev_ts);
		dev_print_mz_data_additional();
		prev_ts = cur_ts;
	}

	return rc;
}

int perform_ref_spad_calibration()
{
	return stmvl53l1_perform_calibration(dev_fd, VL53L1_CALIBRATION_REF_SPAD, 0, 0, 0);
}

int perform_crosstalk_calibration(int method, int mode)
{
	return stmvl53l1_perform_calibration(dev_fd, VL53L1_CALIBRATION_CROSSTALK, method, mode, 0);
}

int perform_offset_calibration(int mode, int distance, float reflectance)
{
	uint32_t calibration_type;
	uint32_t param1;
	uint32_t param2;
	uint32_t param3 = auto_float_to_16x16(reflectance);

	/* then mode is the first parameter:
	 * mode = 1 ==> VL53L1_CALIBRATION_OFFSET
	 * 			internal mode=VL53L1_OFFSETCALIBRATIONMODE_STANDARD
	 * mode = 2 ==> VL53L1_CALIBRATION_OFFSET
	 * 			internal mode=VL53L1_OFFSETCALIBRATIONMODE_PRERANGE_ONLY
	 * mode = 3 ==> VL53L1_CALIBRATION_OFFSET_PER_ZONE
	 * 			internal mode=VL53L1_OFFSETCALIBRATIONMODE_MULTI_ZONE
	 * mode = 4 ==> VL53L1_CALIBRATION_OFFSET_SIMPLE
	 * 			internal mode=VL53L1_CALIBRATION_OFFSET=2 it doesn't matter
	 * */

	switch (mode) {
	case 1:
		calibration_type = VL53L1_CALIBRATION_OFFSET;
		param1 = VL53L1_OFFSETCALIBRATIONMODE_STANDARD;
		param2 = distance;
	break;
	case 2:
		calibration_type = VL53L1_CALIBRATION_OFFSET;
		param1 = VL53L1_OFFSETCALIBRATIONMODE_PRERANGE_ONLY;
		param2 = distance;
	break;
	case 3:
		calibration_type = VL53L1_CALIBRATION_OFFSET_PER_ZONE;
		param1 = VL53L1_OFFSETCALIBRATIONMODE_MULTI_ZONE;
		param2 = distance;
	break;
	default: /* 4 */
		calibration_type = VL53L1_CALIBRATION_OFFSET_SIMPLE;
		param1 = distance;
		param2 = 0;
	}

	return stmvl53l1_perform_calibration(dev_fd, calibration_type,
			param1, param2, param3);
}

static int get_calibration_data_sysfs(char *name)
{
	VL53L1_CalibrationData_t data;
	int rc;

	rc = stmvl53l1_sysfs_read_binary_data("calibration_data", &data, sizeof(data));
	if (rc == 0)
		rc = write_file(name, (void *) &data, sizeof(data));

	return rc;
}

static int get_calibration_data_ioctl(char *name)
{
	struct stmvl53l1_ioctl_calibration_data_t cal;
	int rc;

	cal.is_read = 1;
	rc = stmvl53l1_calibration_data(dev_fd, &cal);
	if (rc == 0)
		rc = write_file(name, (void *) &cal.data, sizeof(cal.data));

	return rc;
}

int get_calibration_data(char *name)
{
	return is_sysfs ? get_calibration_data_sysfs(name) :
		get_calibration_data_ioctl(name);
}

static int set_calibration_data_sysfs(char *name)
{
	VL53L1_CalibrationData_t data;
	int rc;

	rc = read_file(name, (void *) &data, sizeof(data));
	if (rc == 0)
		rc = stmvl53l1_sysfs_write_binary_data("calibration_data", &data, sizeof(data));

	return rc;
}

static int set_calibration_data_ioctl(char *name)
{
	struct stmvl53l1_ioctl_calibration_data_t cal;
	int rc;

	cal.is_read = 0;
	rc = read_file(name, (void *) &cal.data, sizeof(cal.data));
	if (rc == 0)
		rc = stmvl53l1_calibration_data(dev_fd, &cal);

	return rc;
}

int set_calibration_data(char *name)
{
	return is_sysfs ? set_calibration_data_sysfs(name) :
		set_calibration_data_ioctl(name);
}

static int get_zone_calibration_data_sysfs(char *name)
{
	stmvl531_zone_calibration_data_t data;
	int rc;

	rc = stmvl53l1_sysfs_read_binary_data("zone_calibration_data", &data, sizeof(data));
	if (rc == 0)
		rc = write_file(name, (void *) &data, sizeof(data));

	return rc;
}

static int get_zone_calibration_data_ioctl(char *name)
{
	struct stmvl53l1_ioctl_zone_calibration_data_t cal;
	int rc;

	cal.is_read = 1;
	rc = stmvl53l1_zone_calibration_data(dev_fd, &cal);
	if (rc == 0)
		rc = write_file(name, (void *) &cal.data, sizeof(cal.data));

	return rc;
}

int get_zone_calibration_data(char *name)
{
	return is_sysfs ? get_zone_calibration_data_sysfs(name) :
		get_zone_calibration_data_ioctl(name);
}

static int set_zone_calibration_data_sysfs(char *name)
{
	stmvl531_zone_calibration_data_t data;
	int rc;

	rc = read_file(name, (void *) &data, sizeof(data));
	if (rc == 0)
		rc = stmvl53l1_sysfs_write_binary_data("zone_calibration_data", &data, sizeof(data));

	return rc;
}

static int set_zone_calibration_data_ioctl(char *name)
{
	struct stmvl53l1_ioctl_zone_calibration_data_t cal;
	int rc;

	cal.is_read = 0;
	rc = read_file(name, (void *) &cal.data, sizeof(cal.data));
	if (rc == 0)
		rc = stmvl53l1_zone_calibration_data(dev_fd, &cal);

	return rc;
}

int set_zone_calibration_data(char *name)
{
	return is_sysfs ? set_zone_calibration_data_sysfs(name) :
		set_zone_calibration_data_ioctl(name);
}

static int perform_tuning_sysfs(int fd, int key, int value)
{
	return stmvl53l1_sysfs_write_two_integer("tuning", key, value);
}

static int perform_tuning_ioctl(int fd, int key, int value)
{
	int rc;

	struct stmvl53l1_parameter params;

	params.is_read = 0;
	params.name = VL53L1_TUNING_PAR;
	params.value = key;
	params.value2 = value;

	rc= ioctl(fd, VL53L1_IOCTL_PARAMETER, &params);
	if( rc ){
		if( errno == EBUSY ){
			ioctl_warn("ebusy can't set now");
			return errno;
		}
		ioctl_error("%d %s", rc,strerror(errno));
	}
	return rc;
}

int perform_tuning(int fd, int key, int value)
{
	return is_sysfs ? perform_tuning_sysfs(fd, key, value) :
		perform_tuning_ioctl(fd, key, value);
}

int get_tuning(char *name)
{
	char buf[SYSFS_MAX_LEN];
	int rc;

	rc = stmvl53l1_sysfs_read_string("tuning_status", buf);
	if (rc == 0)
		rc = write_file(name, buf, strlen(buf));

	return rc;
}

int set_tuning(char *name)
{
	FILE *f;
	char line[256];
	int key;
	int value;
	int n;
	int rc = 0;

	f = fopen(name, "r");
	if (!f)
		return errno;

	while(fgets(line, sizeof(line), f)) {
		n = sscanf(line, "%d %d", &key, &value);
		if (n != 2) {
			rc = -1;
			break;
		}
		rc = perform_tuning(dev_fd, key, value);
		if (rc)
			break;
	}
	fclose(f);

	return 0;
}

/* handler parameter boilerplate */
#define HANDLE_PARAMETER_INTEGER(sysfs_name, info_name) \
do { \
open_dev_or_die(); \
if (optarg!=NULL) { \
	if (*optarg== '=') \
		optarg++; \
	n = sscanf(optarg,"%d", &u32); \
	if (n!=1) { \
		error("invalid " info_name " %s", optarg); \
		help(-1); \
	} \
	rc = stmvl53l1_set_##sysfs_name(dev_fd, u32); \
	if (rc == 0) \
		info("set " info_name " %d", u32); \
} else { \
	rc = stmvl53l1_get_##sysfs_name(dev_fd, &u32); \
	if (rc == 0) \
		info("get " info_name " %d", u32); \
} \
} while(0)


static int read_config_file(char *name)
{
	FILE *f;
	char line[256];
	 int key;
	int value;
	int n;
	int rc = 0;

	f = fopen(name, "r");
	if (!f)
		return errno;

	while(fgets(line, sizeof(line), f)) {
		debug("%s",line);

		n = sscanf(line, "%d %d", &key, &value);
		if (n != 2) {
			rc = -1;
			break;
		}
		debug("key=%d, value=%d", key, value);

	}
	fclose(f);

	return rc;
}


//VL53L1_Error Test_Api_Version(VL53L1_DEV dev, TestResult_t *TestResult){
//	VL53L1_Error Status = VL53L1_ERROR_NONE;
//	VL53L1_Version_t Version;
//
//	SUPPRESS_UNUSED_WARNING(dev);
//
//	/*
//	*  Get the version of the VL53L1 API running in the firmware
//	*/
//	*TestResult = RESULT_TEST_PASS;
//
//	write_test_details(__FUNCTION__, "unitary test for VL53L1_GetPalSpecVersion,"
//			" VL53L1_GetVersion");
//
//	Status = VL53L1_GetPalSpecVersion(&Version);
//	if(Status == VL53L1_ERROR_NONE)
//	{
//		printf("Customer API interface (specification) version %d.%d.%d (revision %d)\n",
//				Version.major, Version.minor, Version.build, Version.revision);
//		write_info_report("Customer API interface (specification) version %d.%d.%d (revision %d)",
//			  Version.major, Version.minor, Version.build, Version.revision);
//	}
//	else
//	{
//		printf("Error ! Unexpected error code returned\n");
//	*TestResult = RESULT_TEST_FAIL;
//		print_pal_error(Status);
//		REPORT_FAIL("");
//	}
//
//   	Status = VL53L1_GetVersion(&Version);
//	if(Status == VL53L1_ERROR_NONE)
//	{
//		printf("Customer API implementation version %d.%d.%d (revision %d)\n",
//				Version.major, Version.minor, Version.build, Version.revision);
//		write_info_report("Customer API implementation version %d.%d.%d (revision %d)",
//			  Version.major, Version.minor, Version.build, Version.revision);
//	}
//	else
//	{
//		printf("Error ! Unexpected error code returned\n");
//		*TestResult = RESULT_TEST_FAIL;
//		print_pal_error(Status);
//		REPORT_FAIL("");
//	}
//
//	write_info_report("");
//
//	if (*TestResult == RESULT_TEST_PASS)
//		REPORT_PASS();
//	else
//		printf ("TEST FAIL!\n");
//
//	return Status;
//}
//

void SD_DumpCalData(VL53L1_CalibrationData_t *pCalData, FILE *f) {
    fprintf(f,"VL53L1_calibration_data_t {\n");
    fprintf(f,"struct_version = 0x%x\n", pCalData->struct_version);
    fprintf(f,"customer.global_config__spad_enables_ref_0 = %d\n", pCalData->customer.global_config__spad_enables_ref_0);
    fprintf(f,"customer.global_config__spad_enables_ref_1 = %d\n", pCalData->customer.global_config__spad_enables_ref_1);
    fprintf(f,"customer.global_config__spad_enables_ref_2 = %d\n", pCalData->customer.global_config__spad_enables_ref_2);
    fprintf(f,"customer.global_config__spad_enables_ref_3 = %d\n", pCalData->customer.global_config__spad_enables_ref_3);
    fprintf(f,"customer.global_config__spad_enables_ref_4 = %d\n", pCalData->customer.global_config__spad_enables_ref_4);
    fprintf(f,"customer.global_config__spad_enables_ref_5 = %d\n", pCalData->customer.global_config__spad_enables_ref_5);
    fprintf(f,"customer.global_config__ref_en_start_select = %d\n", pCalData->customer.global_config__ref_en_start_select);
    fprintf(f,"customer.ref_spad_man__num_requested_ref_spads = %d\n", pCalData->customer.ref_spad_man__num_requested_ref_spads);
    fprintf(f,"customer.ref_spad_man__ref_location = %d\n", pCalData->customer.ref_spad_man__ref_location );
    fprintf(f,"customer.algo__crosstalk_compensation_plane_offset_kcps = %d\n", pCalData->customer.algo__crosstalk_compensation_plane_offset_kcps );
    fprintf(f,"customer.algo__crosstalk_compensation_x_plane_gradient_kcps = %d\n", pCalData->customer.algo__crosstalk_compensation_x_plane_gradient_kcps );
    fprintf(f,"customer.algo__crosstalk_compensation_y_plane_gradient_kcps = %d\n", pCalData->customer.algo__crosstalk_compensation_y_plane_gradient_kcps );
    fprintf(f,"customer.ref_spad_char__total_rate_target_mcps  = %d\n", pCalData->customer.ref_spad_char__total_rate_target_mcps  );
    fprintf(f,"customer.algo__part_to_part_range_offset_mm = %d\n", pCalData->customer.algo__part_to_part_range_offset_mm );
    fprintf(f,"customer.mm_config__inner_offset_mm = %d\n", pCalData->customer.mm_config__inner_offset_mm );
    fprintf(f,"customer.mm_config__outer_offset_mm = %d\n", pCalData->customer.mm_config__outer_offset_mm );
    fprintf(f,"fmt_dmax_cal.ref__actual_effective_spads = %d\n", pCalData->fmt_dmax_cal.ref__actual_effective_spads );
    fprintf(f,"fmt_dmax_cal.ref__peak_signal_count_rate_mcps = %d\n", pCalData->fmt_dmax_cal.ref__peak_signal_count_rate_mcps );
    fprintf(f,"fmt_dmax_cal.ref__distance_mm = %d\n", pCalData->fmt_dmax_cal.ref__distance_mm );
    fprintf(f,"fmt_dmax_cal.ref_reflectance_pc = %d\n", pCalData->fmt_dmax_cal.ref_reflectance_pc );
    fprintf(f,"fmt_dmax_cal.coverglass_transmission = %d\n", pCalData->fmt_dmax_cal.coverglass_transmission );
    fprintf(f,"cust_dmax_cal.ref__actual_effective_spads = %d\n", pCalData->cust_dmax_cal.ref__actual_effective_spads );
    fprintf(f,"cust_dmax_cal.ref__peak_signal_count_rate_mcps = %d\n", pCalData->cust_dmax_cal.ref__peak_signal_count_rate_mcps );
    fprintf(f,"cust_dmax_cal.ref__distance_mm = %d\n", pCalData->cust_dmax_cal.ref__distance_mm );
    fprintf(f,"cust_dmax_cal.ref_reflectance_pc = %d\n", pCalData->cust_dmax_cal.ref_reflectance_pc );
    fprintf(f,"cust_dmax_cal.coverglass_transmission = %d\n", pCalData->cust_dmax_cal.coverglass_transmission );
    fprintf(f,"add_off_cal_data.result__mm_inner_actual_effective_spads = %d\n", pCalData->add_off_cal_data.result__mm_inner_actual_effective_spads );
    fprintf(f,"add_off_cal_data.result__mm_outer_actual_effective_spads = %d\n", pCalData->add_off_cal_data.result__mm_outer_actual_effective_spads );
    fprintf(f,"add_off_cal_data.result__mm_inner_peak_signal_count_rtn_mcps = %d\n", pCalData->add_off_cal_data.result__mm_inner_peak_signal_count_rtn_mcps );
    fprintf(f,"add_off_cal_data.result__mm_outer_peak_signal_count_rtn_mcps = %d\n", pCalData->add_off_cal_data.result__mm_outer_peak_signal_count_rtn_mcps );
    fprintf(f,"optical_centre.x_centre = %d\n", pCalData->optical_centre.x_centre );
    fprintf(f,"optical_centre.y_centre = %d\n", pCalData->optical_centre.y_centre );
    fprintf(f,"xtalkhisto.xtalk_shape.zone_id = %d\n", pCalData->xtalkhisto.xtalk_shape.zone_id );
    fprintf(f,"xtalkhisto.xtalk_shape.time_stamp = %d\n", pCalData->xtalkhisto.xtalk_shape.time_stamp );
    fprintf(f,"xtalkhisto.xtalk_shape.first_bin = %d\n", pCalData->xtalkhisto.xtalk_shape.VL53L1_PRM_00019 );
    fprintf(f,"xtalkhisto.xtalk_shape.buffer_size = %d\n", pCalData->xtalkhisto.xtalk_shape.VL53L1_PRM_00020 );
    fprintf(f,"xtalkhisto.xtalk_shape.number_of_bins = %d\n", pCalData->xtalkhisto.xtalk_shape.VL53L1_PRM_00021 );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[0] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[0] );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[1] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[1] );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[2] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[2] );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[3] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[3] );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[4] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[4] );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[5] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[5] );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[6] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[6] );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[7] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[7] );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[8] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[8] );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[9] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[9] );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[10] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[10] );
    fprintf(f,"xtalkhisto.xtalk_shape.bin_data[11] = %d\n", pCalData->xtalkhisto.xtalk_shape.bin_data[11] );
    fprintf(f,"xtalkhisto.xtalk_shape.phasecal_result__reference_phase = %d\n", pCalData->xtalkhisto.xtalk_shape.phasecal_result__reference_phase );
    fprintf(f,"xtalkhisto.xtalk_shape.phasecal_result__vcsel_start = %d\n", pCalData->xtalkhisto.xtalk_shape.phasecal_result__vcsel_start );
    fprintf(f,"xtalkhisto.xtalk_shape.cal_config__vcsel_start = %d\n", pCalData->xtalkhisto.xtalk_shape.cal_config__vcsel_start );
    fprintf(f,"xtalkhisto.xtalk_shape.vcsel_width = %d\n", pCalData->xtalkhisto.xtalk_shape.vcsel_width );
    fprintf(f,"xtalkhisto.xtalk_shape.fast_osc__frequency = %d\n", pCalData->xtalkhisto.xtalk_shape.VL53L1_PRM_00022 );
    fprintf(f,"xtalkhisto.xtalk_shape.zero_distance_phase = %d\n", pCalData->xtalkhisto.xtalk_shape.zero_distance_phase );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.cfg_device_state = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.cfg_device_state );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.rd_device_state = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.rd_device_state );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.zone_id = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.zone_id );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.time_stamp = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.time_stamp );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.first_bin = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00019 );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.buffer_size = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00020 );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.number_of_bins = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00021 );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.number_of_ambient_bins = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.number_of_ambient_bins );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_seq[0] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[0] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_seq[1] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[1] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_seq[2] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[2] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_seq[3] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[3] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_seq[4] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[4] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_seq[5] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[5] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_rep[0] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[0] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_rep[1] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[1] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_rep[2] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[2] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_rep[3] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[3] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_rep[4] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[4] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_rep[5] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[5] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[0] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[0] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[1] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[1] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[2] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[2] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[3] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[3] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[4] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[4] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[5] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[5] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[6] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[6] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[7] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[7] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[8] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[8] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[9] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[9] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[10] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[10] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[11] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[11] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[12] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[12] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[13] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[13] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[14] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[14] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[15] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[15] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[16] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[16] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[17] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[17] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[18] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[18] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[19] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[19] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[20] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[20] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[21] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[21] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[22] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[22] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.bin_data[23] = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.bin_data[23] );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.result__interrupt_status = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.result__interrupt_status );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.result__range_status = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.result__range_status );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.result__report_status = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.result__report_status );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.result__stream_count = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.result__stream_count );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.result__dss_actual_effective_spads = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.result__dss_actual_effective_spads );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.phasecal_result__reference_phase = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.phasecal_result__reference_phase );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.phasecal_result__vcsel_start = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.phasecal_result__vcsel_start );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.cal_config__vcsel_start = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.cal_config__vcsel_start );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.vcsel_width = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.vcsel_width );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.vcsel_period = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00008 );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.fast_osc__frequency = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00022 );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.total_periods_elapsed = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.total_periods_elapsed );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.peak_duration_us = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.peak_duration_us );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.woi_duration_us = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.woi_duration_us );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.min_bin_value = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.min_bin_value );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.max_bin_value = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.max_bin_value );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.zero_distance_phase = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.zero_distance_phase );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.number_of_ambient_samples = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.number_of_ambient_samples );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.ambient_events_sum = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.ambient_events_sum );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.ambient_events_per_bin = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00028 );

    fprintf(f,"xtalkhisto.xtalk_hist_removed.roi_config__user_roi_centre_spad = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.roi_config__user_roi_centre_spad );
    fprintf(f,"xtalkhisto.xtalk_hist_removed.roi_config__user_roi_requested_global_xy_size = %d\n", pCalData->xtalkhisto.xtalk_hist_removed.roi_config__user_roi_requested_global_xy_size );
    fprintf(f,"gain_cal.standard_ranging_gain_factor = %d\n", pCalData->gain_cal.standard_ranging_gain_factor );
    fprintf(f,"gain_cal.histogram_ranging_gain_factor = %d\n", pCalData->gain_cal.histogram_ranging_gain_factor );
    fprintf(f,"cal_peak_rate_map.cal_distance_mm = %d\n", pCalData->cal_peak_rate_map.cal_distance_mm );
    fprintf(f,"cal_peak_rate_map.cal_reflectance_pc = %d\n", pCalData->cal_peak_rate_map.cal_reflectance_pc );
    fprintf(f,"cal_peak_rate_map.max_samples = %d\n", pCalData->cal_peak_rate_map.max_samples );
    fprintf(f,"cal_peak_rate_map.width = %d\n", pCalData->cal_peak_rate_map.width );
    fprintf(f,"cal_peak_rate_map.height = %d\n", pCalData->cal_peak_rate_map.height );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[0] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[0] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[1] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[1] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[2] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[2] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[3] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[3] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[4] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[4] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[5] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[5] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[6] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[6] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[7] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[7] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[8] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[8] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[9] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[9] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[10] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[10] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[11] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[11] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[12] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[12] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[13] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[13] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[14] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[14] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[15] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[15] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[16] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[16] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[17] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[17] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[18] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[18] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[19] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[19] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[20] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[20] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[21] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[21] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[22] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[22] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[23] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[23] );
    fprintf(f,"cal_peak_rate_map.peak_rate_mcps[24] = %d\n", pCalData->cal_peak_rate_map.peak_rate_mcps[24] );
    fprintf(f,"}\n");
};

static int cal_bin2txt(char *filein, char *fileout)
{
	FILE *fin;
	FILE *fout;

	int rc = 0;

	VL53L1_CalibrationData_t CalData;
	VL53L1_CalibrationData_t *pCalData = &CalData;

	fin = fopen(filein, "r");

	rc = fread(pCalData, sizeof(VL53L1_CalibrationData_t), 1, fin);
	if(1 != rc) {
		printf("Failed to read data\n");
		return -1;
		}
	fclose(fin);

	fout = fopen(fileout, "w");
	SD_DumpCalData(pCalData, fout);
	fclose(fout);
	return 0;
}

#define GET_INTEGER_SET_VALUE_CALIB(parameter_name) \
		fgets(line, sizeof(line), f); \
		sscanf(line, "%s = %d ", &temp[0], &value); \
		parameter_name = value; \
		printf("line=%d, value=%d\n", i, value); \
		i++;



static int cal_txt2bin(char *filein, char *fileout)
{
	/* This function start from a calibration text file and generate a binary
	 * file compatible with the Linux Driver output.
	 * The script will not parse the single line but all the lines,
	 * this means that if the order on the txt file is changed
	 * (or setting missing) the script will fail*/
	FILE *f;
	char line[256];
	char temp[256];
	int value;
	int n;
	int i;
	int rc = 0;
	VL53L1_CalibrationData_t CalData;
	VL53L1_CalibrationData_t *pCalData = &CalData;

	f = fopen(filein, "r");
	if (!f)
		return errno;

	i = 0;
	while(fgets(line, sizeof(line), f)) {
		debug("%s",line);

		n = sscanf(line, "%s = 0x%x ", &temp[0], &value);
		if (n > 1) {
			// found hex the only number print in Hex is the structure version
			printf("HEX line=%d, value=%x\n", i, value);
			CalData.struct_version = value;
			i++;
			break;
		}

		i++;
	}
	// we are here after the struct version
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.global_config__spad_enables_ref_0)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.global_config__spad_enables_ref_1)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.global_config__spad_enables_ref_2)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.global_config__spad_enables_ref_3)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.global_config__spad_enables_ref_4)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.global_config__spad_enables_ref_5)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.global_config__ref_en_start_select)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.ref_spad_man__num_requested_ref_spads)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.ref_spad_man__ref_location)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.algo__crosstalk_compensation_plane_offset_kcps)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.algo__crosstalk_compensation_x_plane_gradient_kcps)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.algo__crosstalk_compensation_y_plane_gradient_kcps)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.ref_spad_char__total_rate_target_mcps)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.algo__part_to_part_range_offset_mm)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.mm_config__inner_offset_mm)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->customer.mm_config__outer_offset_mm)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->fmt_dmax_cal.ref__actual_effective_spads)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->fmt_dmax_cal.ref__peak_signal_count_rate_mcps)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->fmt_dmax_cal.ref__distance_mm)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->fmt_dmax_cal.ref_reflectance_pc)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->fmt_dmax_cal.coverglass_transmission)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cust_dmax_cal.ref__actual_effective_spads)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cust_dmax_cal.ref__peak_signal_count_rate_mcps)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cust_dmax_cal.ref__distance_mm)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cust_dmax_cal.ref_reflectance_pc)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cust_dmax_cal.coverglass_transmission)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->add_off_cal_data.result__mm_inner_actual_effective_spads)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->add_off_cal_data.result__mm_outer_actual_effective_spads)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->add_off_cal_data.result__mm_inner_peak_signal_count_rtn_mcps)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->add_off_cal_data.result__mm_outer_peak_signal_count_rtn_mcps)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->optical_centre.x_centre)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->optical_centre.y_centre)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.zone_id)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.time_stamp)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.VL53L1_PRM_00019)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.VL53L1_PRM_00020)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.VL53L1_PRM_00021)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[0])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[1])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[2])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[3])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[4])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[5])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[6])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[7])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[8])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[9])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[10])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.bin_data[11])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.phasecal_result__reference_phase)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.phasecal_result__vcsel_start)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.cal_config__vcsel_start)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.vcsel_width)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.VL53L1_PRM_00022)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_shape.zero_distance_phase)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.cfg_device_state)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.rd_device_state)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.zone_id)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.time_stamp)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00019)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00020)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00021)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.number_of_ambient_bins)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[0])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[1])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[2])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[3])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[4])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_seq[5])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[0])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[1])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[2])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[3])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[4])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_rep[5])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[0])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[1])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[2])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[3])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[4])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[5])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[6])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[7])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[8])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[9])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[10])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[11])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[12])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[13])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[14])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[15])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[16])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[17])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[18])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[19])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[20])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[21])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[22])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.bin_data[23])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.result__interrupt_status)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.result__range_status)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.result__report_status)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.result__stream_count)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.result__dss_actual_effective_spads)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.phasecal_result__reference_phase)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.phasecal_result__vcsel_start)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.cal_config__vcsel_start)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.vcsel_width)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00008)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00022)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.total_periods_elapsed)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.peak_duration_us)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.woi_duration_us)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.min_bin_value)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.max_bin_value)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.zero_distance_phase)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.number_of_ambient_samples)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.ambient_events_sum)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.VL53L1_PRM_00028)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.roi_config__user_roi_centre_spad)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->xtalkhisto.xtalk_hist_removed.roi_config__user_roi_requested_global_xy_size)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->gain_cal.standard_ranging_gain_factor)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->gain_cal.histogram_ranging_gain_factor)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.cal_distance_mm)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.cal_reflectance_pc)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.max_samples)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.width)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.height)
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[0])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[1])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[2])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[3])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[4])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[5])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[6])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[7])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[8])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[9])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[10])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[11])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[12])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[13])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[14])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[15])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[16])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[17])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[18])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[19])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[20])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[21])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[22])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[23])
	GET_INTEGER_SET_VALUE_CALIB(pCalData->cal_peak_rate_map.peak_rate_mcps[24])

	fclose(f);

	// create the .bin file:
	rc = write_file(fileout, (void *) pCalData, sizeof(CalData));


	return rc;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	int i,n;
	char file1_c[255];
	char file2_c[255];
	char *file1 = &file1_c[0];
	char *file2 = &file2_c[0];

	int n_loops=1;
	uint32_t n_roi_get=0;

	int option_index = 0;
	int c;
	uint32_t u32;
	float f32;
	uint32_t u32_2;
	float f32_2;
	if( argc == 1 ){
		error("what you wan't me ?");
		help(-1);
	}

	do{
		c = getopt_long (argc, argv, string_options , long_options, &option_index);
		if( c == -1 ){ // -1 mean no more options
			debug("cl end\n");
			break;
		}

		switch (c){
		case 'l':
			if(optarg == NULL) {
				error("filename are required");
				help(-1);
			}
			warn("aptarg= %s", optarg);
			n=sscanf(optarg,"%s %s", file1, file2);
			if (n != 2) {
				error("invalid param need two files use: in.bin out.txt %s", optarg);
			}

			printf("Use the following files: BIN=%s TXT=%s\n", file1, file2);

			rc = cal_bin2txt(file1, file2);


			break;
		case 'L':
			if(optarg == NULL) {
				error("filename are required");
				help(-1);
			}
			warn("aptarg= %s", optarg);
			n=sscanf(optarg,"%s %s", file1, file2);
			if (n != 2) {
				error("invalid param need two files use: in.txt out.bin %s", optarg);
			}

			printf("Use the following files: TXT=%s BIN=%s\n", file1, file2);

			rc = cal_txt2bin(file1, file2);


			break;
		case 'H':
			if(optarg == NULL) {
				error("filename is required");
				help(-1);
			}
			open_dev_or_die();
			rc = read_config_file(optarg);
			if( rc !=0) break;



			break;
		case 'd':
			n=sscanf(optarg, "%d", &i);
			if( n==1){
				set_new_device(i);
			}
			else{
				error("invalid/ wrong dev number in %s",  optarg);
				help(1);
			}
			break;
		case 'm' :
			HANDLE_PARAMETER_INTEGER(mode, "mode");
			break;

		case 'M' :
			if( optarg!=NULL){
				if( *optarg== '=' ){
					optarg++;
				}
				n=sscanf(optarg,"%d", &n_loops);
				if( n!=1){
					error("invalid wrong number of loop in %s", optarg);
					help(-1);
				}
			}
			else{
				n_loops=run_loops;
			}
			open_dev_or_die();
			run_meas_loop( n_loops);
			break;

		case 'o' : // roi get
			// skip = in short format type opt
			if( *optarg == '='){
				optarg++;
			}
			n = sscanf(optarg, "%d", &u32);
			if( n < 1){
				error("invalid no of roi in %s", optarg);
				help(-1);
			}
			open_dev_or_die();
			rc = stmvl53l1_get_roi(dev_fd, dev_rois, u32, &n_roi_get);
			if( rc == 0 ){
				if( n_roi_get != u32){
					warn("ROI asked %d but got %d", u32, n_roi_get);
				}
				dump_roi(stdout,dev_rois, MIN(n_roi_get,u32));
			}
			break;

		case 'O': //roi get
			if( optarg == NULL ){ // no arg kill
				debug("no arg unset roi");
				u32=0;
				rc = 0;
			}
			else{
				if( *optarg == '='){
					optarg++;
				}
				debug("set roi arg %s",optarg);
				rc= parse_roi_arg(optarg, dev_rois, &u32);
			}
			if( rc >= 0 ){
				verbose("%d roi to set", u32);
				dump_roi(stdout,dev_rois, u32);
				open_dev_or_die();
				rc = stmvl53l1_set_roi(dev_fd, dev_rois,u32);
			}
			else{
				error("set roi");
				help(-1);
			}
			break;

		case 'p' :
			HANDLE_PARAMETER_INTEGER(set_delay_ms, "poll delay time");
			break;

		case 'P' :
			if( *optarg== '=' ){
				optarg++;
			}
			n=sscanf(optarg,"%d" , &run_poll_ms);
			if( n != 1){
				error("invalid pause time in %s", optarg);
				help(-1);
			}
			verbose("pause time set to %d msec", run_poll_ms);
			break;
		case 'r' :
			if( optarg!=NULL){
				//process optional arg
				// on short form -r=10 may be given or --range=10
				if( *optarg== '=' ){
					optarg++;
				}
				n=sscanf(optarg,"%d", &n_loops);
				if( n!=1){
					error("invalid wrong number of loop in %s", optarg);
					help(-1);
				}
			}
			else{
				n_loops=run_loops;
			}
			open_dev_or_die();
			for( i=0; i< n_loops; i++ ){
				rc=stmvl53l1_get_data(dev_fd, &dev_1xrange_data);
				if( rc==0){
					dev_print_1xdata();
				}
				else{
					error("data error on loop %d", i);
					break;
				}
				if( i < n_loops-1){
					usleep(run_poll_ms*1000);
				}
			}
			break;

		case 's':
			open_dev_or_die();
			rc= stmvl53l1_start(dev_fd);
			verbose("started status %d",rc);
			break;

		case 'S':
			open_dev_or_die();
			rc=stmvl53l1_stop(dev_fd);
			verbose("stoped status %d", rc);
			break;

		case 't' :
			HANDLE_PARAMETER_INTEGER(timing_budget, "timing budget");
			break;

		case 'z':
			if( optarg!=NULL){
				//process optional arg
				// on short form -r=10 may be given or --range=10
				if( *optarg== '=' ){
					optarg++;
				}
				n=sscanf(optarg,"%d", &n_loops);
				if( n!=1){
					error("invalid wrong number of loop in %s", optarg);
					help(-1);
				}
			}
			else{
				n_loops=run_loops;
			}
			open_dev_or_die();
			for( i=0; i< n_loops; i++ ){
				rc=stmvl53l1_get_mz_data(dev_fd, &dev_mz_range_data);
				if( rc==0){
					dev_print_mz_data();
				}
				else{
					error("data error on loop %d", i);
					break;
				}
				if( i < n_loops-1){
					usleep(run_poll_ms*1000);
				}
			}
			break;

		case 'Z':
			if( *optarg== '=' ){
				optarg++;
			}
			n=sscanf(optarg,"%d", &u32);
			if( n!=1){
				error("invalid wrong number of zone loop in %s", optarg);
				help(-1);
			}
			open_dev_or_die();
			run_zone_loop((int)u32);
			break;

		case 'R':
			open_dev_or_die();
			rc = perform_ref_spad_calibration();
			break;

		case 'X':
			if( *optarg== '=' ) {
				optarg++;
			}
			n = sscanf(optarg,"%d %d", &u32, &u32_2);
			//printf("n=%d param1 = %d, param2 = %d\n", n, u32, u32_2);
			if (n != 2) {
				error("invalid crosstalk calibration params %s", optarg);
			}
			open_dev_or_die();
			rc = perform_crosstalk_calibration(u32, u32_2);
			break;

		case 'F':
			if( *optarg== '=' ) {
				optarg++;
			}
			n = sscanf(optarg,"%d %d %f", &u32, &u32_2, &f32);
			if (n != 3) {
				error("invalid offset calibration distance %s", optarg);
				help(-1);
			}
			open_dev_or_die();
			/* then mode is the first parameter:
			 * mode = 1 ==> VL53L1_CALIBRATION_OFFSET
			 * 			internal mode=VL53L1_CALIBRATION_OFFSET=2
			 * mode = 2 ==> VL53L1_CALIBRATION_OFFSET
			 * 			internal mode=VL53L1_CALIBRATION_OFFSET=2
			 * mode = 3 ==> VL53L1_CALIBRATION_OFFSET_PER_ZONE
			 * 			internal mode=VL53L1_OFFSETCALIBRATIONMODE_MULTI_ZONE
			 * mode = 4 ==> VL53L1_CALIBRATION_OFFSET_SIMPLE
			 * 			internal mode=VL53L1_CALIBRATION_OFFSET=2 it doesn't matter
			 * */
			rc = perform_offset_calibration(u32, u32_2, f32);
			break;

		case 'c':
			if(optarg == NULL) {
				error("filename is required");
				help(-1);
			}
			open_dev_or_die();
			rc = get_calibration_data(optarg);
			break;

		case 'C':
			if(optarg == NULL) {
				error("filename is required");
				help(-1);
			}
			open_dev_or_die();
			rc = set_calibration_data(optarg);
			break;

		case 'D':
			HANDLE_PARAMETER_INTEGER(distance_mode, "distance mode");
			break;

		case 'E':
			HANDLE_PARAMETER_INTEGER(crosstalk_enable, "crosstalk enable");
			break;

		case 'U':
			HANDLE_PARAMETER_INTEGER(output_mode, "output mode");
			break;

		case 'f':
			HANDLE_PARAMETER_INTEGER(force_device_on_enable, "force device on enable");
			break;

		case 'T':
			HANDLE_PARAMETER_INTEGER(offset_correction_mode, "offset correction mode");
			break;

		case 'a':
			open_dev_or_die();
			if(optarg != NULL) {
				if(*optarg == '=')
					optarg++;
				rc = stmvl53l1_set_auto_config(dev_fd, optarg);
			} else
				rc = stmvl53l1_get_auto_config(dev_fd);
			break;

		case 'y':
			is_sysfs = 1;
			break;

		case 'Y':
			is_sysfs = 0;
			break;

		case 'I':
			open_dev_or_die();
			rc = stmvl53l1_get_optical_center(dev_fd, &f32, &f32_2);
			if (rc == 0)
				info("optical center offset x = %f / y = %f", f32, f32_2);
			break;

		case 'A':
			open_dev_or_die();
			if( optarg!=NULL){
				if( *optarg== '=' ){
					optarg++;
				}
				n=sscanf(optarg,"%f", &f32);
				if( n!=1){
					error("invalid dmax reflectance %s", optarg);
					help(-1);
				}
				// set mode
				rc=stmvl53l1_set_dmax_reflectance(dev_fd, f32);
				if( rc == 0){
					info("set dmax reflectance %f", f32);
				}
			}
			else{
				rc=stmvl53l1_get_dmax_reflectance(dev_fd, &f32);
				if( rc == 0){
					info("get dmax reflectance %f", f32);
				}
			}
			break;

		case 'B':
			HANDLE_PARAMETER_INTEGER(dmax_mode, "dmax mode");
			break;

		case 'w':
			if(optarg == NULL) {
				error("filename is required");
				help(-1);
			}
			open_dev_or_die();
			rc = get_zone_calibration_data(optarg);
			break;

		case 'W':
			if(optarg == NULL) {
				error("filename is required");
				help(-1);
			}
			open_dev_or_die();
			rc = set_zone_calibration_data(optarg);
			break;

		case 'N':
			if( *optarg== '=' ) {
				optarg++;
			}
			n = sscanf(optarg,"%d %d", &u32, &u32_2);
			if (n != 2) {
				error("invalid tuning parameters %s", optarg);
				help(-1);
			}
			open_dev_or_die();
			rc = perform_tuning(dev_fd, u32, u32_2);
			break;

		case 'g':
			if(optarg == NULL) {
				error("filename is required");
				help(-1);
			}
			open_dev_or_die();
			rc = get_tuning(optarg);
			break;

		case 'G':
			if(optarg == NULL) {
				error("filename is required");
				help(-1);
			}
			open_dev_or_die();
			rc = set_tuning(optarg);
			break;

		case 'h':
			HANDLE_PARAMETER_INTEGER(smudge_correction_mode, "smudge correction mode");
			break;

		case 'j':
			if( optarg!=NULL){
				//process optional arg
				// on short form -r=10 may be given or --range=10
				if( *optarg== '=' ){
					optarg++;
				}
				n=sscanf(optarg,"%d", &n_loops);
				if( n!=1){
					error("invalid wrong number of loop in %s", optarg);
					help(-1);
				}
			}
			else{
				n_loops=run_loops;
			}
			open_dev_or_die();
			for( i=0; i< n_loops; i++ ){
				rc=stmvl53l1_get_mz_data_additional(dev_fd, &dev_mz_range_data_additional);
				if( rc==0){
					dev_print_mz_data_additional();
				}
				else{
					error("data error on loop %d", i);
					break;
				}
				if( i < n_loops-1){
					usleep(run_poll_ms*1000);
				}
			}
			break;

		case 'J':
			if( *optarg== '=' ){
				optarg++;
			}
			n=sscanf(optarg,"%d", &u32);
			if( n!=1){
				error("invalid wrong number of zone loop in %s", optarg);
				help(-1);
			}
			open_dev_or_die();
			run_zone_loop_additional((int)u32);
			break;

		case 0: // flags option keep going
			break;

		case '?':
			// TODO we don't quite do a good job with ? it is also unkwon option etc return from get opt

		default:
			help(1);
			exit(1);
		}
	}
	while(1);  // exit from inner loop
//done:
	if( dev_fd)
		close(dev_fd);
	return rc;
}


