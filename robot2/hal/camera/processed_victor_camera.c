// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/mm_qcamera_main_menu.c"
// # 1 "/mnt/devel/la2.0.2/kernel//"
// # 1 "<built-in>"
// # 1 "<command-line>"
// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/mm_qcamera_main_menu.c"
// # 30 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/mm_qcamera_main_menu.c"
// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 1
// # 1 "bionic/libc/include/stdio.h" 1 3 4
// # 41 "bionic/libc/include/stdio.h" 3 4
// # 1 "bionic/libc/include/sys/cdefs.h" 1 3 4
// # 396 "bionic/libc/include/sys/cdefs.h" 3 4
// # 1 "bionic/libc/include/android/api-level.h" 1 3 4
// # 397 "bionic/libc/include/sys/cdefs.h" 2 3 4
// # 465 "bionic/libc/include/sys/cdefs.h" 3 4
extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline))
int __size_mul_overflow(unsigned int a, unsigned int b, unsigned int *result) {
    *result = a * b;
    static const unsigned int mul_no_overflow = 1UL << (sizeof(unsigned int) * 4);
    return (a >= mul_no_overflow || b >= mul_no_overflow) && a > 0 && (unsigned int)-1 / a < b;
}
// # 42 "bionic/libc/include/stdio.h" 2 3 4
// # 1 "bionic/libc/include/sys/types.h" 1 3 4
// # 31 "bionic/libc/include/sys/types.h" 3 4
// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 1 3 4
// # 147 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 3 4
typedef int ptrdiff_t;
// # 212 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 3 4
typedef unsigned int size_t;
// # 324 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 3 4
typedef unsigned int wchar_t;
// # 32 "bionic/libc/include/sys/types.h" 2 3 4
// # 1 "bionic/libc/include/stdint.h" 1 3 4
// # 32 "bionic/libc/include/stdint.h" 3 4
// # 1 "bionic/libc/include/bits/wchar_limits.h" 1 3 4
// # 33 "bionic/libc/include/stdint.h" 2 3 4
// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 1 3 4
// # 34 "bionic/libc/include/stdint.h" 2 3 4

typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef short __int16_t;
typedef unsigned short __uint16_t;
typedef int __int32_t;
typedef unsigned int __uint32_t;




typedef long long __int64_t;
typedef unsigned long long __uint64_t;






typedef int __intptr_t;
typedef unsigned int __uintptr_t;


typedef __int8_t int8_t;
typedef __uint8_t uint8_t;

typedef __int16_t int16_t;
typedef __uint16_t uint16_t;

typedef __int32_t int32_t;
typedef __uint32_t uint32_t;

typedef __int64_t int64_t;
typedef __uint64_t uint64_t;

typedef __intptr_t intptr_t;
typedef __uintptr_t uintptr_t;

typedef int8_t int_least8_t;
typedef uint8_t uint_least8_t;

typedef int16_t int_least16_t;
typedef uint16_t uint_least16_t;

typedef int32_t int_least32_t;
typedef uint32_t uint_least32_t;

typedef int64_t int_least64_t;
typedef uint64_t uint_least64_t;

typedef int8_t int_fast8_t;
typedef uint8_t uint_fast8_t;

typedef int64_t int_fast64_t;
typedef uint64_t uint_fast64_t;







typedef int32_t int_fast16_t;
typedef uint32_t uint_fast16_t;
typedef int32_t int_fast32_t;
typedef uint32_t uint_fast32_t;


typedef uint64_t uintmax_t;
typedef int64_t intmax_t;
// # 33 "bionic/libc/include/sys/types.h" 2 3 4


// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/types.h" 1 3 4



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/types.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/types.h" 1 3 4






// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/int-ll64.h" 1 3 4
// # 11 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/int-ll64.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/bitsperlong.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/bitsperlong.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/bitsperlong.h" 2 3 4
// # 12 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/int-ll64.h" 2 3 4







typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;


__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
// # 8 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/types.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/types.h" 2 3 4
// # 5 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/types.h" 2 3 4



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/posix_types.h" 1 3 4



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/stddef.h" 1 3 4
// # 5 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/posix_types.h" 2 3 4
// # 24 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/posix_types.h" 3 4
typedef struct {
 unsigned long fds_bits[1024 / (8 * sizeof(long))];
} __kernel_fd_set;


typedef void (*__kernel_sighandler_t)(int);


typedef int __kernel_key_t;
typedef int __kernel_mqd_t;

// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/posix_types.h" 1 3 4
// # 22 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/posix_types.h" 3 4
typedef unsigned short __kernel_mode_t;


typedef unsigned short __kernel_nlink_t;


typedef unsigned short __kernel_ipc_pid_t;


typedef unsigned short __kernel_uid_t;
typedef unsigned short __kernel_gid_t;


typedef unsigned short __kernel_old_dev_t;


// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/posix_types.h" 1 3 4



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/bitsperlong.h" 1 3 4
// # 5 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/posix_types.h" 2 3 4
// # 14 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/posix_types.h" 3 4
typedef long __kernel_long_t;
typedef unsigned long __kernel_ulong_t;



typedef __kernel_ulong_t __kernel_ino_t;







typedef int __kernel_pid_t;
// # 40 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/posix_types.h" 3 4
typedef __kernel_long_t __kernel_suseconds_t;



typedef int __kernel_daddr_t;



typedef unsigned int __kernel_uid32_t;
typedef unsigned int __kernel_gid32_t;



typedef __kernel_uid_t __kernel_old_uid_t;
typedef __kernel_gid_t __kernel_old_gid_t;
// # 67 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/posix_types.h" 3 4
typedef unsigned int __kernel_size_t;
typedef int __kernel_ssize_t;
typedef int __kernel_ptrdiff_t;
// # 78 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/posix_types.h" 3 4
typedef struct {
 int val[2];
} __kernel_fsid_t;





typedef __kernel_long_t __kernel_off_t;
typedef long long __kernel_loff_t;
typedef __kernel_long_t __kernel_time_t;
typedef __kernel_long_t __kernel_clock_t;
typedef int __kernel_timer_t;
typedef int __kernel_clockid_t;
typedef char * __kernel_caddr_t;
typedef unsigned short __kernel_uid16_t;
typedef unsigned short __kernel_gid16_t;
// # 39 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/posix_types.h" 2 3 4
// # 36 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/posix_types.h" 2 3 4
// # 9 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/types.h" 2 3 4
// # 27 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/types.h" 3 4
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;

typedef __u16 __sum16;
typedef __u32 __wsum;
// # 36 "bionic/libc/include/sys/types.h" 2 3 4



typedef __kernel_gid32_t __gid_t;
typedef __gid_t gid_t;
typedef __kernel_uid32_t __uid_t;
typedef __uid_t uid_t;
typedef __kernel_pid_t __pid_t;
typedef __pid_t pid_t;
typedef uint32_t __id_t;
typedef __id_t id_t;

typedef unsigned long blkcnt_t;
typedef unsigned long blksize_t;
typedef __kernel_caddr_t caddr_t;
typedef __kernel_clock_t clock_t;

typedef __kernel_clockid_t __clockid_t;
typedef __clockid_t clockid_t;

typedef __kernel_daddr_t daddr_t;
typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;

typedef __kernel_mode_t __mode_t;
typedef __mode_t mode_t;

typedef __kernel_key_t __key_t;
typedef __key_t key_t;

typedef __kernel_ino_t __ino_t;
typedef __ino_t ino_t;

typedef uint32_t __nlink_t;
typedef __nlink_t nlink_t;

typedef void* __timer_t;
typedef __timer_t timer_t;

typedef __kernel_suseconds_t __suseconds_t;
typedef __suseconds_t suseconds_t;


typedef uint32_t __useconds_t;
typedef __useconds_t useconds_t;



typedef uint32_t dev_t;





typedef __kernel_time_t __time_t;
typedef __time_t time_t;







typedef __kernel_off_t off_t;
typedef __kernel_loff_t loff_t;
typedef loff_t off64_t;
// # 120 "bionic/libc/include/sys/types.h" 3 4
typedef int32_t __socklen_t;




typedef __socklen_t socklen_t;

typedef __builtin_va_list __va_list;
// # 136 "bionic/libc/include/sys/types.h" 3 4
typedef __kernel_ssize_t ssize_t;


typedef unsigned int uint_t;
typedef unsigned int uint;


// # 1 "bionic/libc/include/sys/sysmacros.h" 1 3 4
// # 144 "bionic/libc/include/sys/types.h" 2 3 4

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef uint32_t u_int32_t;
typedef uint16_t u_int16_t;
typedef uint8_t u_int8_t;
typedef uint64_t u_int64_t;
// # 43 "bionic/libc/include/stdio.h" 2 3 4

// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stdarg.h" 1 3 4
// # 40 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
// # 98 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stdarg.h" 3 4
typedef __gnuc_va_list va_list;
// # 45 "bionic/libc/include/stdio.h" 2 3 4
// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 1 3 4
// # 46 "bionic/libc/include/stdio.h" 2 3 4


// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 1 3 4
// # 49 "bionic/libc/include/stdio.h" 2 3 4



typedef off_t fpos_t;
typedef off64_t fpos64_t;

struct __sFILE;
typedef struct __sFILE FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;
// # 105 "bionic/libc/include/stdio.h" 3 4
void clearerr(FILE *);
int fclose(FILE *);
int feof(FILE *);
int ferror(FILE *);
int fflush(FILE *);
int fgetc(FILE *);
char *fgets(char * __restrict, int, FILE * __restrict);
int fprintf(FILE * __restrict , const char * __restrict, ...)
  __attribute__((__format__(printf, 2, 3))) __attribute__((__nonnull__ (2)));
int fputc(int, FILE *);
int fputs(const char * __restrict, FILE * __restrict);
size_t fread(void * __restrict, size_t, size_t, FILE * __restrict);
int fscanf(FILE * __restrict, const char * __restrict, ...)
  __attribute__((__format__(scanf, 2, 3))) __attribute__((__nonnull__ (2)));
size_t fwrite(const void * __restrict, size_t, size_t, FILE * __restrict);
int getc(FILE *);
int getchar(void);
ssize_t getdelim(char ** __restrict, size_t * __restrict, int,
     FILE * __restrict);
ssize_t getline(char ** __restrict, size_t * __restrict, FILE * __restrict);

void perror(const char *);
int printf(const char * __restrict, ...)
  __attribute__((__format__(printf, 1, 2))) __attribute__((__nonnull__ (1)));
int putc(int, FILE *);
int putchar(int);
int puts(const char *);
int remove(const char *);
void rewind(FILE *);
int scanf(const char * __restrict, ...)
  __attribute__((__format__(scanf, 1, 2))) __attribute__((__nonnull__ (1)));
void setbuf(FILE * __restrict, char * __restrict);
int setvbuf(FILE * __restrict, char * __restrict, int, size_t);
int sscanf(const char * __restrict, const char * __restrict, ...)
  __attribute__((__format__(scanf, 2, 3))) __attribute__((__nonnull__ (2)));
int ungetc(int, FILE *);
int vfprintf(FILE * __restrict, const char * __restrict, __va_list)
  __attribute__((__format__(printf, 2, 0))) __attribute__((__nonnull__ (2)));
int vprintf(const char * __restrict, __va_list)
  __attribute__((__format__(printf, 1, 0))) __attribute__((__nonnull__ (1)));

int dprintf(int, const char * __restrict, ...) __attribute__((__format__(printf, 2, 3))) __attribute__((__nonnull__ (2)));
int vdprintf(int, const char * __restrict, __va_list) __attribute__((__format__(printf, 2, 0))) __attribute__((__nonnull__ (2)));



char* gets(char*) __attribute__((deprecated("gets is unsafe, use fgets instead")));

int sprintf(char* __restrict, const char* __restrict, ...)
    __attribute__((__format__(printf, 2, 3))) __attribute__((__nonnull__ (2))) __attribute__((__warning__("sprintf is often misused; please use snprintf")));
int vsprintf(char* __restrict, const char* __restrict, __va_list)
    __attribute__((__format__(printf, 2, 0))) __attribute__((__nonnull__ (2))) __attribute__((__warning__("vsprintf is often misused; please use vsnprintf")));
char* tmpnam(char*) __attribute__((deprecated("tmpnam is unsafe, use mkstemp or tmpfile instead")));

char* tempnam(const char*, const char*)
    __attribute__((deprecated("tempnam is unsafe, use mkstemp or tmpfile instead")));



int rename(const char*, const char*);
int renameat(int, const char*, int, const char*);

int fseek(FILE*, long, int);
long ftell(FILE*);
// # 183 "bionic/libc/include/stdio.h" 3 4
int fgetpos(FILE*, fpos_t*);
int fsetpos(FILE*, const fpos_t*);
int fseeko(FILE*, off_t, int);
off_t ftello(FILE*);
// # 195 "bionic/libc/include/stdio.h" 3 4
int fgetpos64(FILE*, fpos64_t*);
int fsetpos64(FILE*, const fpos64_t*);
int fseeko64(FILE*, off64_t, int);
off64_t ftello64(FILE*);
// # 207 "bionic/libc/include/stdio.h" 3 4
FILE* fopen(const char* __restrict, const char* __restrict);
FILE* fopen64(const char* __restrict, const char* __restrict);
FILE* freopen(const char* __restrict, const char* __restrict, FILE* __restrict);
FILE* freopen64(const char* __restrict, const char* __restrict, FILE* __restrict);
FILE* tmpfile(void);
FILE* tmpfile64(void);


int snprintf(char * __restrict, size_t, const char * __restrict, ...)
  __attribute__((__format__(printf, 3, 4))) __attribute__((__nonnull__ (3)));
int vfscanf(FILE * __restrict, const char * __restrict, __va_list)
  __attribute__((__format__(scanf, 2, 0))) __attribute__((__nonnull__ (2)));
int vscanf(const char *, __va_list)
  __attribute__((__format__(scanf, 1, 0))) __attribute__((__nonnull__ (1)));
int vsnprintf(char * __restrict, size_t, const char * __restrict, __va_list)
  __attribute__((__format__(printf, 3, 0))) __attribute__((__nonnull__ (3)));
int vsscanf(const char * __restrict, const char * __restrict, __va_list)
  __attribute__((__format__(scanf, 2, 0))) __attribute__((__nonnull__ (2)));
// # 233 "bionic/libc/include/stdio.h" 3 4
FILE *fdopen(int, const char *);
int fileno(FILE *);


int pclose(FILE *);
FILE *popen(const char *, const char *);



void flockfile(FILE *);
int ftrylockfile(FILE *);
void funlockfile(FILE *);





int getc_unlocked(FILE *);
int getchar_unlocked(void);
int putc_unlocked(int, FILE *);
int putchar_unlocked(int);



FILE* fmemopen(void*, size_t, const char*);
FILE* open_memstream(char**, size_t*);
// # 267 "bionic/libc/include/stdio.h" 3 4
int asprintf(char ** __restrict, const char * __restrict, ...)
  __attribute__((__format__(printf, 2, 3))) __attribute__((__nonnull__ (2)));
char *fgetln(FILE * __restrict, size_t * __restrict);
int fpurge(FILE *);
void setbuffer(FILE *, char *, int);
int setlinebuf(FILE *);
int vasprintf(char ** __restrict, const char * __restrict,
    __va_list)
  __attribute__((__format__(printf, 2, 0))) __attribute__((__nonnull__ (2)));

void clearerr_unlocked(FILE*);
int feof_unlocked(FILE*);
int ferror_unlocked(FILE*);
int fileno_unlocked(FILE*);





extern char* __fgets_chk(char*, int, FILE*, size_t);
extern char* __fgets_real(char*, int, FILE*) __asm__("fgets");
extern void __fgets_too_big_error(void) __attribute__((__error__("fgets called with size bigger than buffer")));
extern void __fgets_too_small_error(void) __attribute__((__error__("fgets called with size less than zero")));

extern size_t __fread_chk(void * __restrict, size_t, size_t, FILE * __restrict, size_t);
extern size_t __fread_real(void * __restrict, size_t, size_t, FILE * __restrict) __asm__("fread");
extern void __fread_too_big_error(void) __attribute__((__error__("fread called with size * count bigger than buffer")));
extern void __fread_overflow(void) __attribute__((__error__("fread called with overflowing size * count")));

extern size_t __fwrite_chk(const void * __restrict, size_t, size_t, FILE * __restrict, size_t);
extern size_t __fwrite_real(const void * __restrict, size_t, size_t, FILE * __restrict) __asm__("fwrite");
extern void __fwrite_too_big_error(void) __attribute__((__error__("fwrite called with size * count bigger than buffer")));
extern void __fwrite_overflow(void) __attribute__((__error__("fwrite called with overflowing size * count")));



extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
__attribute__((__format__(printf, 3, 0))) __attribute__((__nonnull__ (3)))
int vsnprintf(char *dest, size_t size, const char *format, __va_list ap)
{
    return __builtin___vsnprintf_chk(dest, size, 0, __builtin_object_size((dest), 1), format, ap);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
__attribute__((__format__(printf, 2, 0))) __attribute__((__nonnull__ (2)))
int vsprintf(char *dest, const char *format, __va_list ap)
{
    return __builtin___vsprintf_chk(dest, 0, __builtin_object_size((dest), 1), format, ap);
}







extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
__attribute__((__format__(printf, 3, 4))) __attribute__((__nonnull__ (3)))
int snprintf(char *dest, size_t size, const char *format, ...)
{
    return __builtin___snprintf_chk(dest, size, 0,
        __builtin_object_size((dest), 1), format, __builtin_va_arg_pack());
}
// # 338 "bionic/libc/include/stdio.h" 3 4
extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
__attribute__((__format__(printf, 2, 3))) __attribute__((__nonnull__ (2)))
int sprintf(char *dest, const char *format, ...)
{
    return __builtin___sprintf_chk(dest, 0,
        __builtin_object_size((dest), 1), format, __builtin_va_arg_pack());
}


extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
size_t fread(void * __restrict buf, size_t size, size_t count, FILE * __restrict stream) {
    size_t bos = __builtin_object_size((buf), 0);


    if (bos == ((size_t) -1)) {
        return __fread_real(buf, size, count, stream);
    }

    if (__builtin_constant_p(size) && __builtin_constant_p(count)) {
        size_t total;
        if (__size_mul_overflow(size, count, &total)) {
            __fread_overflow();
        }

        if (total > bos) {
            __fread_too_big_error();
        }

        return __fread_real(buf, size, count, stream);
    }


    return __fread_chk(buf, size, count, stream, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
size_t fwrite(const void * __restrict buf, size_t size, size_t count, FILE * __restrict stream) {
    size_t bos = __builtin_object_size((buf), 0);


    if (bos == ((size_t) -1)) {
        return __fwrite_real(buf, size, count, stream);
    }

    if (__builtin_constant_p(size) && __builtin_constant_p(count)) {
        size_t total;
        if (__size_mul_overflow(size, count, &total)) {
            __fwrite_overflow();
        }

        if (total > bos) {
            __fwrite_too_big_error();
        }

        return __fwrite_real(buf, size, count, stream);
    }


    return __fwrite_chk(buf, size, count, stream, bos);
}



extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
char *fgets(char* dest, int size, FILE* stream) {
    size_t bos = __builtin_object_size((dest), 1);



    if (__builtin_constant_p(size) && (size < 0)) {
        __fgets_too_small_error();
    }


    if (bos == ((size_t) -1)) {
        return __fgets_real(dest, size, stream);
    }



    if (__builtin_constant_p(size) && (size <= (int) bos)) {
        return __fgets_real(dest, size, stream);
    }



    if (__builtin_constant_p(size) && (size > (int) bos)) {
        __fgets_too_big_error();
    }

    return __fgets_chk(dest, size, stream, bos);
}






// # 2 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2
// # 1 "bionic/libc/include/string.h" 1 3 4
// # 33 "bionic/libc/include/string.h" 3 4
// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 1 3 4
// # 34 "bionic/libc/include/string.h" 2 3 4
// # 1 "bionic/libc/include/xlocale.h" 1 3 4
// # 33 "bionic/libc/include/xlocale.h" 3 4
struct __locale_t;
typedef struct __locale_t* locale_t;
// # 35 "bionic/libc/include/string.h" 2 3 4







extern void* memccpy(void* __restrict, const void* __restrict, int, size_t);
extern void* memchr(const void *, int, size_t) __attribute__((pure));
extern void* memrchr(const void *, int, size_t) __attribute__((pure));
extern int memcmp(const void *, const void *, size_t) __attribute__((pure));
extern void* memcpy(void* __restrict, const void* __restrict, size_t);



extern void* memmove(void *, const void *, size_t);
extern void* memset(void *, int, size_t);
extern void* memmem(const void *, size_t, const void *, size_t) __attribute__((pure));

extern char* strchr(const char *, int) __attribute__((pure));
extern char* __strchr_chk(const char *, int, size_t);
// # 65 "bionic/libc/include/string.h" 3 4
extern char* strrchr(const char *, int) __attribute__((pure));
extern char* __strrchr_chk(const char *, int, size_t);

extern size_t strlen(const char *) __attribute__((pure));
extern size_t __strlen_chk(const char *, size_t);
extern int strcmp(const char *, const char *) __attribute__((pure));
extern char* stpcpy(char* __restrict, const char* __restrict);
extern char* strcpy(char* __restrict, const char* __restrict);
extern char* strcat(char* __restrict, const char* __restrict);

int strcasecmp(const char*, const char*) __attribute__((pure));
int strcasecmp_l(const char*, const char*, locale_t) __attribute__((pure));
int strncasecmp(const char*, const char*, size_t) __attribute__((pure));
int strncasecmp_l(const char*, const char*, size_t, locale_t) __attribute__((pure));

extern char* strdup(const char *);

extern char* strstr(const char *, const char *) __attribute__((pure));
extern char* strcasestr(const char *haystack, const char *needle) __attribute__((pure));
extern char* strtok(char* __restrict, const char* __restrict);
extern char* strtok_r(char* __restrict, const char* __restrict, char** __restrict);

extern char* strerror(int);
extern char* strerror_l(int, locale_t);



extern int strerror_r(int, char*, size_t);


extern size_t strnlen(const char *, size_t) __attribute__((pure));
extern char* strncat(char* __restrict, const char* __restrict, size_t);
extern char* strndup(const char *, size_t);
extern int strncmp(const char *, const char *, size_t) __attribute__((pure));
extern char* stpncpy(char* __restrict, const char* __restrict, size_t);
extern char* strncpy(char* __restrict, const char* __restrict, size_t);

extern size_t strlcat(char* __restrict, const char* __restrict, size_t);
extern size_t strlcpy(char* __restrict, const char* __restrict, size_t);

extern size_t strcspn(const char *, const char *) __attribute__((pure));
extern char* strpbrk(const char *, const char *) __attribute__((pure));
extern char* strsep(char** __restrict, const char* __restrict);
extern size_t strspn(const char *, const char *);

extern char* strsignal(int sig);

extern int strcoll(const char *, const char *) __attribute__((pure));
extern size_t strxfrm(char* __restrict, const char* __restrict, size_t);

extern int strcoll_l(const char *, const char *, locale_t) __attribute__((pure));
extern size_t strxfrm_l(char* __restrict, const char* __restrict, size_t, locale_t);
// # 132 "bionic/libc/include/string.h" 3 4
extern void* __memchr_chk(const void*, int, size_t, size_t);
extern void __memchr_buf_size_error(void) __attribute__((__error__("memchr called with size bigger than buffer")));

extern void* __memrchr_chk(const void*, int, size_t, size_t);
extern void __memrchr_buf_size_error(void) __attribute__((__error__("memrchr called with size bigger than buffer")));
extern void* __memrchr_real(const void*, int, size_t) __asm__("memrchr");

extern char* __stpncpy_chk2(char* __restrict, const char* __restrict, size_t, size_t, size_t);
extern char* __strncpy_chk2(char* __restrict, const char* __restrict, size_t, size_t, size_t);
extern size_t __strlcpy_real(char* __restrict, const char* __restrict, size_t) __asm__("strlcpy");
extern size_t __strlcpy_chk(char *, const char *, size_t, size_t);
extern size_t __strlcat_real(char* __restrict, const char* __restrict, size_t) __asm__("strlcat");
extern size_t __strlcat_chk(char* __restrict, const char* __restrict, size_t, size_t);



extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
void* memchr(const void *s, int c, size_t n) {
    size_t bos = __builtin_object_size((s), 1);


    if (bos == ((size_t) -1)) {
        return __builtin_memchr(s, c, n);
    }

    if (__builtin_constant_p(n) && (n > bos)) {
        __memchr_buf_size_error();
    }

    if (__builtin_constant_p(n) && (n <= bos)) {
        return __builtin_memchr(s, c, n);
    }


    return __memchr_chk(s, c, n, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
void* memrchr(const void *s, int c, size_t n) {
    size_t bos = __builtin_object_size((s), 1);


    if (bos == ((size_t) -1)) {
        return __memrchr_real(s, c, n);
    }

    if (__builtin_constant_p(n) && (n > bos)) {
        __memrchr_buf_size_error();
    }

    if (__builtin_constant_p(n) && (n <= bos)) {
        return __memrchr_real(s, c, n);
    }


    return __memrchr_chk(s, c, n, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
void* memcpy(void* __restrict dest, const void* __restrict src, size_t copy_amount) {
    return __builtin___memcpy_chk(dest, src, copy_amount, __builtin_object_size((dest), 0));
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
void* memmove(void *dest, const void *src, size_t len) {
    return __builtin___memmove_chk(dest, src, len, __builtin_object_size((dest), 0));
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
char* stpcpy(char* __restrict dest, const char* __restrict src) {
    return __builtin___stpcpy_chk(dest, src, __builtin_object_size((dest), 1));
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
char* strcpy(char* __restrict dest, const char* __restrict src) {
    return __builtin___strcpy_chk(dest, src, __builtin_object_size((dest), 1));
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
char* stpncpy(char* __restrict dest, const char* __restrict src, size_t n) {
    size_t bos_dest = __builtin_object_size((dest), 1);
    size_t bos_src = __builtin_object_size((src), 1);

    if (bos_src == ((size_t) -1)) {
        return __builtin___stpncpy_chk(dest, src, n, bos_dest);
    }

    if (__builtin_constant_p(n) && (n <= bos_src)) {
        return __builtin___stpncpy_chk(dest, src, n, bos_dest);
    }

    size_t slen = __builtin_strlen(src);
    if (__builtin_constant_p(slen)) {
        return __builtin___stpncpy_chk(dest, src, n, bos_dest);
    }

    return __stpncpy_chk2(dest, src, n, bos_dest, bos_src);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
char* strncpy(char* __restrict dest, const char* __restrict src, size_t n) {
    size_t bos_dest = __builtin_object_size((dest), 1);
    size_t bos_src = __builtin_object_size((src), 1);

    if (bos_src == ((size_t) -1)) {
        return __builtin___strncpy_chk(dest, src, n, bos_dest);
    }

    if (__builtin_constant_p(n) && (n <= bos_src)) {
        return __builtin___strncpy_chk(dest, src, n, bos_dest);
    }

    size_t slen = __builtin_strlen(src);
    if (__builtin_constant_p(slen)) {
        return __builtin___strncpy_chk(dest, src, n, bos_dest);
    }

    return __strncpy_chk2(dest, src, n, bos_dest, bos_src);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
char* strcat(char* __restrict dest, const char* __restrict src) {
    return __builtin___strcat_chk(dest, src, __builtin_object_size((dest), 1));
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
char *strncat(char* __restrict dest, const char* __restrict src, size_t n) {
    return __builtin___strncat_chk(dest, src, n, __builtin_object_size((dest), 1));
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
void* memset(void *s, int c, size_t n) {
    return __builtin___memset_chk(s, c, n, __builtin_object_size((s), 0));
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
size_t strlcpy(char* __restrict dest, const char* __restrict src, size_t size) {
    size_t bos = __builtin_object_size((dest), 1);



    if (bos == ((size_t) -1)) {
        return __strlcpy_real(dest, src, size);
    }



    if (__builtin_constant_p(size) && (size <= bos)) {
        return __strlcpy_real(dest, src, size);
    }


    return __strlcpy_chk(dest, src, size, bos);
}


extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
size_t strlcat(char* __restrict dest, const char* __restrict src, size_t size) {
    size_t bos = __builtin_object_size((dest), 1);



    if (bos == ((size_t) -1)) {
        return __strlcat_real(dest, src, size);
    }



    if (__builtin_constant_p(size) && (size <= bos)) {
        return __strlcat_real(dest, src, size);
    }


    return __strlcat_chk(dest, src, size, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
size_t strlen(const char *s) {
    size_t bos = __builtin_object_size((s), 1);



    if (bos == ((size_t) -1)) {
        return __builtin_strlen(s);
    }

    size_t slen = __builtin_strlen(s);
    if (__builtin_constant_p(slen)) {
        return slen;
    }


    return __strlen_chk(s, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
char* strchr(const char *s, int c) {
    size_t bos = __builtin_object_size((s), 1);



    if (bos == ((size_t) -1)) {
        return __builtin_strchr(s, c);
    }

    size_t slen = __builtin_strlen(s);
    if (__builtin_constant_p(slen) && (slen < bos)) {
        return __builtin_strchr(s, c);
    }


    return __strchr_chk(s, c, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
char* strrchr(const char *s, int c) {
    size_t bos = __builtin_object_size((s), 1);



    if (bos == ((size_t) -1)) {
        return __builtin_strrchr(s, c);
    }

    size_t slen = __builtin_strlen(s);
    if (__builtin_constant_p(slen) && (slen < bos)) {
        return __builtin_strrchr(s, c);
    }


    return __strrchr_chk(s, c, bos);
}





// # 3 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2
// # 1 "bionic/libc/include/unistd.h" 1 3 4
// # 32 "bionic/libc/include/unistd.h" 3 4
// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 1 3 4
// # 33 "bionic/libc/include/unistd.h" 2 3 4


// # 1 "bionic/libc/include/sys/select.h" 1 3 4
// # 32 "bionic/libc/include/sys/select.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/time.h" 1 3 4
// # 9 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/time.h" 3 4
struct timespec {
 __kernel_time_t tv_sec;
 long tv_nsec;
};


struct timeval {
 __kernel_time_t tv_sec;
 __kernel_suseconds_t tv_usec;
};

struct timezone {
 int tz_minuteswest;
 int tz_dsttime;
};
// # 38 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/time.h" 3 4
struct itimerspec {
 struct timespec it_interval;
 struct timespec it_value;
};

struct itimerval {
 struct timeval it_interval;
 struct timeval it_value;
};
// # 33 "bionic/libc/include/sys/select.h" 2 3 4
// # 1 "bionic/libc/include/signal.h" 1 3 4
// # 32 "bionic/libc/include/signal.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/sigcontext.h" 1 3 4
// # 9 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/sigcontext.h" 3 4
struct sigcontext {
 unsigned long trap_no;
 unsigned long error_code;
 unsigned long oldmask;
 unsigned long arm_r0;
 unsigned long arm_r1;
 unsigned long arm_r2;
 unsigned long arm_r3;
 unsigned long arm_r4;
 unsigned long arm_r5;
 unsigned long arm_r6;
 unsigned long arm_r7;
 unsigned long arm_r8;
 unsigned long arm_r9;
 unsigned long arm_r10;
 unsigned long arm_fp;
 unsigned long arm_ip;
 unsigned long arm_sp;
 unsigned long arm_lr;
 unsigned long arm_pc;
 unsigned long arm_cpsr;
 unsigned long fault_address;
};
// # 33 "bionic/libc/include/signal.h" 2 3 4
// # 1 "bionic/libc/include/bits/pthread_types.h" 1 3 4
// # 34 "bionic/libc/include/bits/pthread_types.h" 3 4
typedef long pthread_t;

typedef struct {
  uint32_t flags;
  void* stack_base;
  size_t stack_size;
  size_t guard_size;
  int32_t sched_policy;
  int32_t sched_priority;



} pthread_attr_t;
// # 34 "bionic/libc/include/signal.h" 2 3 4
// # 1 "bionic/libc/include/bits/timespec.h" 1 3 4
// # 35 "bionic/libc/include/signal.h" 2 3 4
// # 1 "bionic/libc/include/limits.h" 1 3 4
// # 53 "bionic/libc/include/limits.h" 3 4
// # 1 "bionic/libc/include/sys/limits.h" 1 3 4
// # 30 "bionic/libc/include/sys/limits.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/limits.h" 1 3 4
// # 31 "bionic/libc/include/sys/limits.h" 2 3 4
// # 54 "bionic/libc/include/limits.h" 2 3 4


// # 1 "bionic/libc/include/sys/syslimits.h" 1 3 4
// # 57 "bionic/libc/include/limits.h" 2 3 4
// # 87 "bionic/libc/include/limits.h" 3 4
// # 1 "bionic/libc/include/bits/posix_limits.h" 1 3 4
// # 88 "bionic/libc/include/limits.h" 2 3 4
// # 36 "bionic/libc/include/signal.h" 2 3 4
// # 48 "bionic/libc/include/signal.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/signal.h" 1 3 4



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/signal.h" 1 3 4






struct siginfo;




typedef unsigned long sigset_t;
// # 91 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/signal.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/signal-defs.h" 1 3 4
// # 17 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/signal-defs.h" 3 4
typedef void __signalfn_t(int);
typedef __signalfn_t *__sighandler_t;

typedef void __restorefn_t(void);
typedef __restorefn_t *__sigrestore_t;
// # 92 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/signal.h" 2 3 4



struct sigaction {
 union {
   __sighandler_t _sa_handler;
   void (*_sa_sigaction)(int, struct siginfo *, void *);
 } _u;
 sigset_t sa_mask;
 unsigned long sa_flags;
 void (*sa_restorer)(void);
};





typedef struct sigaltstack {
 void *ss_sp;
 int ss_flags;
 size_t ss_size;
} stack_t;
// # 5 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/signal.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/siginfo.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/siginfo.h" 1 3 4






typedef union sigval {
 int sival_int;
 void *sival_ptr;
} sigval_t;
// # 48 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/siginfo.h" 3 4
typedef struct siginfo {
 int si_signo;
 int si_errno;
 int si_code;

 union {
  int _pad[((128 - (3 * sizeof(int))) / sizeof(int))];


  struct {
   __kernel_pid_t _pid;
   __kernel_uid32_t _uid;
  } _kill;


  struct {
   __kernel_timer_t _tid;
   int _overrun;
   char _pad[sizeof( __kernel_uid32_t) - sizeof(int)];
   sigval_t _sigval;
   int _sys_private;
  } _timer;


  struct {
   __kernel_pid_t _pid;
   __kernel_uid32_t _uid;
   sigval_t _sigval;
  } _rt;


  struct {
   __kernel_pid_t _pid;
   __kernel_uid32_t _uid;
   int _status;
   __kernel_clock_t _utime;
   __kernel_clock_t _stime;
  } _sigchld;


  struct {
   void *_addr;



   short _addr_lsb;
  } _sigfault;


  struct {
   long _band;
   int _fd;
  } _sigpoll;


  struct {
   void *_call_addr;
   int _syscall;
   unsigned int _arch;
  } _sigsys;
 } _sifields;
} siginfo_t;
// # 276 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/siginfo.h" 3 4
typedef struct sigevent {
 sigval_t sigev_value;
 int sigev_signo;
 int sigev_notify;
 union {
  int _pad[((64 - (sizeof(int) * 2 + sizeof(sigval_t))) / sizeof(int))];
   int _tid;

  struct {
   void (*_function)(sigval_t);
   void *_attribute;
  } _sigev_thread;
 } _sigev_un;
} sigevent_t;
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/siginfo.h" 2 3 4
// # 6 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/signal.h" 2 3 4
// # 49 "bionic/libc/include/signal.h" 2 3 4


// # 1 "bionic/libc/include/sys/ucontext.h" 1 3 4
// # 32 "bionic/libc/include/sys/ucontext.h" 3 4
// # 1 "bionic/libc/include/signal.h" 1 3 4
// # 33 "bionic/libc/include/sys/ucontext.h" 2 3 4
// # 1 "bionic/libc/include/sys/user.h" 1 3 4
// # 33 "bionic/libc/include/sys/user.h" 3 4
// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 1 3 4
// # 34 "bionic/libc/include/sys/user.h" 2 3 4


// # 188 "bionic/libc/include/sys/user.h" 3 4
struct user_fpregs {
  struct fp_reg {
    unsigned int sign1:1;
    unsigned int unused:15;
    unsigned int sign2:1;
    unsigned int exponent:14;
    unsigned int j:1;
    unsigned int mantissa1:31;
    unsigned int mantissa0:32;
  } fpregs[8];
  unsigned int fpsr:32;
  unsigned int fpcr:32;
  unsigned char ftype[8];
  unsigned int init_flag;
};
struct user_regs {
  unsigned long uregs[18];
};
struct user_vfp {
  unsigned long long fpregs[32];
  unsigned long fpscr;
};
struct user_vfp_exc {
  unsigned long fpexc;
  unsigned long fpinst;
  unsigned long fpinst2;
};
struct user {
  struct user_regs regs;
  int u_fpvalid;
  unsigned long int u_tsize;
  unsigned long int u_dsize;
  unsigned long int u_ssize;
  unsigned long start_code;
  unsigned long start_stack;
  long int signal;
  int reserved;
  struct user_regs* u_ar0;
  unsigned long magic;
  char u_comm[32];
  int u_debugreg[8];
  struct user_fpregs u_fp;
  struct user_fpregs* u_fp0;
};
// # 253 "bionic/libc/include/sys/user.h" 3 4

// # 34 "bionic/libc/include/sys/ucontext.h" 2 3 4





enum {
  REG_R0 = 0,
  REG_R1,
  REG_R2,
  REG_R3,
  REG_R4,
  REG_R5,
  REG_R6,
  REG_R7,
  REG_R8,
  REG_R9,
  REG_R10,
  REG_R11,
  REG_R12,
  REG_R13,
  REG_R14,
  REG_R15,
};



typedef int greg_t;
typedef greg_t gregset_t[18];
typedef struct user_fpregs fpregset_t;


typedef struct sigcontext mcontext_t;

typedef struct ucontext {
  unsigned long uc_flags;
  struct ucontext* uc_link;
  stack_t uc_stack;
  mcontext_t uc_mcontext;
  sigset_t uc_sigmask;

  uint32_t __padding_rt_sigset;

  char __padding[120];
  unsigned long uc_regspace[128] __attribute__((__aligned__(8)));
} ucontext_t;
// # 309 "bionic/libc/include/sys/ucontext.h" 3 4

// # 52 "bionic/libc/include/signal.h" 2 3 4




typedef int sig_atomic_t;
// # 70 "bionic/libc/include/signal.h" 3 4
extern int __libc_current_sigrtmin(void);
extern int __libc_current_sigrtmax(void);

extern const char* const sys_siglist[];
extern const char* const sys_signame[];

typedef __sighandler_t sig_t;
typedef __sighandler_t sighandler_t;
// # 106 "bionic/libc/include/signal.h" 3 4
extern int sigaction(int, const struct sigaction*, struct sigaction*);

extern sighandler_t signal(int, sighandler_t) ;

extern int siginterrupt(int, int);

extern int sigaddset(sigset_t*, int) ;
extern int sigdelset(sigset_t*, int) ;
extern int sigemptyset(sigset_t*) ;
extern int sigfillset(sigset_t*) ;
extern int sigismember(const sigset_t*, int) ;

extern int sigpending(sigset_t*) __attribute__((__nonnull__ (1)));
extern int sigprocmask(int, const sigset_t*, sigset_t*);
extern int sigsuspend(const sigset_t*) __attribute__((__nonnull__ (1)));
extern int sigwait(const sigset_t*, int*) __attribute__((__nonnull__ (1, 2)));

extern int raise(int);
extern int kill(pid_t, int);
extern int killpg(int, int);

extern int sigaltstack(const stack_t*, stack_t*);

extern void psiginfo(const siginfo_t*, const char*);
extern void psignal(int, const char*);

extern int pthread_kill(pthread_t, int);
extern int pthread_sigmask(int, const sigset_t*, sigset_t*);

extern int sigqueue(pid_t, int, const union sigval);
extern int sigtimedwait(const sigset_t*, siginfo_t*, const struct timespec*);
extern int sigwaitinfo(const sigset_t*, siginfo_t*);






// # 34 "bionic/libc/include/sys/select.h" 2 3 4









typedef struct {
  unsigned long fds_bits[(1024/(8 * sizeof(unsigned long)))];
} fd_set;
// # 60 "bionic/libc/include/sys/select.h" 3 4
extern void __FD_CLR_chk(int, fd_set*, size_t);
extern void __FD_SET_chk(int, fd_set*, size_t);
extern int __FD_ISSET_chk(int, fd_set*, size_t);
// # 74 "bionic/libc/include/sys/select.h" 3 4
extern int select(int, fd_set*, fd_set*, fd_set*, struct timeval*);
extern int pselect(int, fd_set*, fd_set*, fd_set*, const struct timespec*, const sigset_t*);


// # 36 "bionic/libc/include/unistd.h" 2 3 4
// # 1 "bionic/libc/include/sys/sysconf.h" 1 3 4
// # 33 "bionic/libc/include/sys/sysconf.h" 3 4

// # 191 "bionic/libc/include/sys/sysconf.h" 3 4
long sysconf(int);


// # 37 "bionic/libc/include/unistd.h" 2 3 4

// # 1 "bionic/libc/include/bits/lockf.h" 1 3 4
// # 39 "bionic/libc/include/bits/lockf.h" 3 4





int lockf(int, int, off_t);

int lockf64(int, int, off64_t);


// # 39 "bionic/libc/include/unistd.h" 2 3 4







// # 81 "bionic/libc/include/unistd.h" 3 4
extern char** environ;

extern __attribute__((__noreturn__)) void _exit(int __status);
extern pid_t fork(void);
extern pid_t vfork(void);
extern pid_t getpid(void);
extern pid_t gettid(void) __attribute__((__const__));
extern pid_t getpgid(pid_t __pid);
extern int setpgid(pid_t __pid, pid_t __pgid);
extern pid_t getppid(void);
extern pid_t getpgrp(void);
extern int setpgrp(void);
extern pid_t getsid(pid_t __pid) ;
extern pid_t setsid(void);

extern int execv(const char* __path, char* const* __argv);
extern int execvp(const char* __file, char* const* __argv);
extern int execvpe(const char* __file, char* const* __argv, char* const* __envp)
  ;
extern int execve(const char* __file, char* const* __argv, char* const* __envp);
extern int execl(const char* __path, const char* __arg0, ...);
extern int execlp(const char* __file, const char* __arg0, ...);
extern int execle(const char* __path, const char* __arg0, ...);

extern int nice(int __incr);

extern int setuid(uid_t __uid);
extern uid_t getuid(void);
extern int seteuid(uid_t __uid);
extern uid_t geteuid(void);
extern int setgid(gid_t __gid);
extern gid_t getgid(void);
extern int setegid(gid_t __gid);
extern gid_t getegid(void);
extern int getgroups(int __size, gid_t* __list);
extern int setgroups(size_t __size, const gid_t* __list);
extern int setreuid(uid_t __ruid, uid_t __euid);
extern int setregid(gid_t __rgid, gid_t __egid);
extern int setresuid(uid_t __ruid, uid_t __euid, uid_t __suid);
extern int setresgid(gid_t __rgid, gid_t __egid, gid_t __sgid);
extern int getresuid(uid_t* __ruid, uid_t* __euid, uid_t* __suid);
extern int getresgid(gid_t* __rgid, gid_t* __egid, gid_t* __sgid);
extern char* getlogin(void);

extern long fpathconf(int __fd, int __name);
extern long pathconf(const char* __path, int __name);

extern int access(const char* __path, int __mode);
extern int faccessat(int __dirfd, const char* __path, int __mode, int __flags)
  ;
extern int link(const char* __oldpath, const char* __newpath);
extern int linkat(int __olddirfd, const char* __oldpath, int __newdirfd,
                  const char* __newpath, int __flags) ;
extern int unlink(const char* __path);
extern int unlinkat(int __dirfd, const char* __path, int __flags);
extern int chdir(const char* __path);
extern int fchdir(int __fd);
extern int rmdir(const char* __path);
extern int pipe(int* __pipefd);



extern int chroot(const char* __path);
extern int symlink(const char* __oldpath, const char* __newpath);
extern int symlinkat(const char* __oldpath, int __newdirfd,
                     const char* __newpath) ;
extern ssize_t readlink(const char* __path, char* __buf, size_t __bufsiz);
extern ssize_t readlinkat(int __dirfd, const char* __path, char* __buf,
                          size_t __bufsiz) ;
extern int chown(const char* __path, uid_t __owner, gid_t __group);
extern int fchown(int __fd, uid_t __owner, gid_t __group);
extern int fchownat(int __dirfd, const char* __path, uid_t __owner,
                    gid_t __group, int __flags);
extern int lchown(const char* __path, uid_t __owner, gid_t __group);
extern char* getcwd(char* __buf, size_t __size);

extern int sync(void);

extern int close(int __fd);

extern ssize_t read(int __fd, void* __buf, size_t __count);
extern ssize_t write(int __fd, const void* __buf, size_t __count);

extern int dup(int __oldfd);
extern int dup2(int __oldfd, int __newfd);
extern int dup3(int __oldfd, int __newfd, int __flags) ;
extern int fcntl(int __fd, int __cmd, ...);
extern int ioctl(int __fd, int __request, ...);
extern int fsync(int __fd);
extern int fdatasync(int __fd) ;




extern off_t lseek(int __fd, off_t __offset, int __whence);


extern off64_t lseek64(int __fd, off64_t __offset, int __whence);
// # 188 "bionic/libc/include/unistd.h" 3 4
extern int truncate(const char* __path, off_t __length);
extern ssize_t pread(int __fd, void* __buf, size_t __count, off_t __offset);
extern ssize_t pwrite(int __fd, const void* __buf, size_t __count,
                      off_t __offset);
extern int ftruncate(int __fd, off_t __length);


extern int truncate64(const char* __path, off64_t __length) ;
extern ssize_t pread64(int __fd, void* __buf, size_t __count, off64_t __offset) ;
extern ssize_t pwrite64(int __fd, const void* __buf, size_t __count,
                        off64_t __offset) ;
extern int ftruncate64(int __fd, off64_t __length) ;

extern int pause(void);
extern unsigned int alarm(unsigned int __seconds);
extern unsigned int sleep(unsigned int __seconds);
extern int usleep(useconds_t __usec);

int gethostname(char* __name, size_t __len);
int sethostname(const char* __name, size_t __len);

extern void* __brk(void* __addr);
extern int brk(void* __addr);
extern void* sbrk(ptrdiff_t __increment);

extern int getopt(int __argc, char* const* __argv, const char* __argstring);
extern char* optarg;
extern int optind, opterr, optopt;

extern int isatty(int __fd);
extern char* ttyname(int __fd);
extern int ttyname_r(int __fd, char* __buf, size_t __buflen) ;

extern int acct(const char* __filepath);

long sysconf(int __name);


int getpagesize(void);






long syscall(long __number, ...);

extern int daemon(int __nochdir, int __noclose);


extern int cacheflush(long __addr, long __nbytes, long __cache);



extern pid_t tcgetpgrp(int __fd);
extern int tcsetpgrp(int __fd, pid_t __pid);
// # 254 "bionic/libc/include/unistd.h" 3 4
extern char* __getcwd_chk(char*, size_t, size_t);
extern void __getcwd_dest_size_error(void) __attribute__((__error__("getcwd called with size bigger than destination")));
extern char* __getcwd_real(char*, size_t) __asm__("getcwd");

extern ssize_t __pread_chk(int, void*, size_t, off_t, size_t);
extern void __pread_dest_size_error(void) __attribute__((__error__("pread called with size bigger than destination")));
extern void __pread_count_toobig_error(void) __attribute__((__error__("pread called with count > SSIZE_MAX")));
extern ssize_t __pread_real(int, void*, size_t, off_t) __asm__("pread");

extern ssize_t __pread64_chk(int, void*, size_t, off64_t, size_t);
extern void __pread64_dest_size_error(void) __attribute__((__error__("pread64 called with size bigger than destination")));
extern void __pread64_count_toobig_error(void) __attribute__((__error__("pread64 called with count > SSIZE_MAX")));
extern ssize_t __pread64_real(int, void*, size_t, off64_t) __asm__("pread64");

extern ssize_t __pwrite_chk(int, const void*, size_t, off_t, size_t);
extern void __pwrite_dest_size_error(void) __attribute__((__error__("pwrite called with size bigger than destination")));
extern void __pwrite_count_toobig_error(void) __attribute__((__error__("pwrite called with count > SSIZE_MAX")));
extern ssize_t __pwrite_real(int, const void*, size_t, off_t) __asm__("pwrite");

extern ssize_t __pwrite64_chk(int, const void*, size_t, off64_t, size_t);
extern void __pwrite64_dest_size_error(void) __attribute__((__error__("pwrite64 called with size bigger than destination")));
extern void __pwrite64_count_toobig_error(void) __attribute__((__error__("pwrite64 called with count > SSIZE_MAX")));
extern ssize_t __pwrite64_real(int, const void*, size_t, off64_t) __asm__("pwrite64");

extern ssize_t __read_chk(int, void*, size_t, size_t);
extern void __read_dest_size_error(void) __attribute__((__error__("read called with size bigger than destination")));
extern void __read_count_toobig_error(void) __attribute__((__error__("read called with count > SSIZE_MAX")));
extern ssize_t __read_real(int, void*, size_t) __asm__("read");

extern ssize_t __write_chk(int, const void*, size_t, size_t);
extern void __write_dest_size_error(void) __attribute__((__error__("write called with size bigger than destination")));
extern void __write_count_toobig_error(void) __attribute__((__error__("write called with count > SSIZE_MAX")));
extern ssize_t __write_real(int, const void*, size_t) __asm__("write");

extern ssize_t __readlink_chk(const char*, char*, size_t, size_t);
extern void __readlink_dest_size_error(void) __attribute__((__error__("readlink called with size bigger than destination")));
extern void __readlink_size_toobig_error(void) __attribute__((__error__("readlink called with size > SSIZE_MAX")));
extern ssize_t __readlink_real(const char*, char*, size_t) __asm__("readlink");

extern ssize_t __readlinkat_chk(int dirfd, const char*, char*, size_t, size_t);
extern void __readlinkat_dest_size_error(void) __attribute__((__error__("readlinkat called with size bigger than destination")));
extern void __readlinkat_size_toobig_error(void) __attribute__((__error__("readlinkat called with size > SSIZE_MAX")));
extern ssize_t __readlinkat_real(int dirfd, const char*, char*, size_t) __asm__("readlinkat");



extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
char* getcwd(char* buf, size_t size) {
    size_t bos = __builtin_object_size((buf), 1);
// # 315 "bionic/libc/include/unistd.h" 3 4
    if (bos == ((size_t) -1)) {
        return __getcwd_real(buf, size);
    }

    if (__builtin_constant_p(size) && (size > bos)) {
        __getcwd_dest_size_error();
    }

    if (__builtin_constant_p(size) && (size <= bos)) {
        return __getcwd_real(buf, size);
    }


    return __getcwd_chk(buf, size, bos);
}







extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
ssize_t pread(int fd, void* buf, size_t count, off_t offset) {
    size_t bos = __builtin_object_size((buf), 0);


    if (__builtin_constant_p(count) && (count > 0x7fffffffL)) {
        __pread_count_toobig_error();
    }

    if (bos == ((size_t) -1)) {
        return __pread_real(fd, buf, count, offset);
    }

    if (__builtin_constant_p(count) && (count > bos)) {
        __pread_dest_size_error();
    }

    if (__builtin_constant_p(count) && (count <= bos)) {
        return __pread_real(fd, buf, count, offset);
    }


    return __pread_chk(fd, buf, count, offset, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
ssize_t pread64(int fd, void* buf, size_t count, off64_t offset) {
    size_t bos = __builtin_object_size((buf), 0);


    if (__builtin_constant_p(count) && (count > 0x7fffffffL)) {
        __pread64_count_toobig_error();
    }

    if (bos == ((size_t) -1)) {
        return __pread64_real(fd, buf, count, offset);
    }

    if (__builtin_constant_p(count) && (count > bos)) {
        __pread64_dest_size_error();
    }

    if (__builtin_constant_p(count) && (count <= bos)) {
        return __pread64_real(fd, buf, count, offset);
    }


    return __pread64_chk(fd, buf, count, offset, bos);
}







extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset) {
    size_t bos = __builtin_object_size((buf), 0);


    if (__builtin_constant_p(count) && (count > 0x7fffffffL)) {
        __pwrite_count_toobig_error();
    }

    if (bos == ((size_t) -1)) {
        return __pwrite_real(fd, buf, count, offset);
    }

    if (__builtin_constant_p(count) && (count > bos)) {
        __pwrite_dest_size_error();
    }

    if (__builtin_constant_p(count) && (count <= bos)) {
        return __pwrite_real(fd, buf, count, offset);
    }


    return __pwrite_chk(fd, buf, count, offset, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
ssize_t pwrite64(int fd, const void* buf, size_t count, off64_t offset) {
    size_t bos = __builtin_object_size((buf), 0);


    if (__builtin_constant_p(count) && (count > 0x7fffffffL)) {
        __pwrite64_count_toobig_error();
    }

    if (bos == ((size_t) -1)) {
        return __pwrite64_real(fd, buf, count, offset);
    }

    if (__builtin_constant_p(count) && (count > bos)) {
        __pwrite64_dest_size_error();
    }

    if (__builtin_constant_p(count) && (count <= bos)) {
        return __pwrite64_real(fd, buf, count, offset);
    }


    return __pwrite64_chk(fd, buf, count, offset, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
ssize_t read(int fd, void* buf, size_t count) {
    size_t bos = __builtin_object_size((buf), 0);


    if (__builtin_constant_p(count) && (count > 0x7fffffffL)) {
        __read_count_toobig_error();
    }

    if (bos == ((size_t) -1)) {
        return __read_real(fd, buf, count);
    }

    if (__builtin_constant_p(count) && (count > bos)) {
        __read_dest_size_error();
    }

    if (__builtin_constant_p(count) && (count <= bos)) {
        return __read_real(fd, buf, count);
    }


    return __read_chk(fd, buf, count, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
ssize_t write(int fd, const void* buf, size_t count) {
    size_t bos = __builtin_object_size((buf), 0);
// # 479 "bionic/libc/include/unistd.h" 3 4
    if (bos == ((size_t) -1)) {





        return __write_real(fd, buf, count);
    }

    if (__builtin_constant_p(count) && (count > bos)) {
        __write_dest_size_error();
    }

    if (__builtin_constant_p(count) && (count <= bos)) {





        return __write_real(fd, buf, count);
    }


    return __write_chk(fd, buf, count, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
ssize_t readlink(const char* path, char* buf, size_t size) {
    size_t bos = __builtin_object_size((buf), 1);


    if (__builtin_constant_p(size) && (size > 0x7fffffffL)) {
        __readlink_size_toobig_error();
    }

    if (bos == ((size_t) -1)) {
        return __readlink_real(path, buf, size);
    }

    if (__builtin_constant_p(size) && (size > bos)) {
        __readlink_dest_size_error();
    }

    if (__builtin_constant_p(size) && (size <= bos)) {
        return __readlink_real(path, buf, size);
    }


    return __readlink_chk(path, buf, size, bos);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
ssize_t readlinkat(int dirfd, const char* path, char* buf, size_t size) {
    size_t bos = __builtin_object_size((buf), 1);


    if (__builtin_constant_p(size) && (size > 0x7fffffffL)) {
        __readlinkat_size_toobig_error();
    }

    if (bos == ((size_t) -1)) {
        return __readlinkat_real(dirfd, path, buf, size);
    }

    if (__builtin_constant_p(size) && (size > bos)) {
        __readlinkat_dest_size_error();
    }

    if (__builtin_constant_p(size) && (size <= bos)) {
        return __readlinkat_real(dirfd, path, buf, size);
    }


    return __readlinkat_chk(dirfd, path, buf, size, bos);
}




// # 4 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2
// # 1 "bionic/libc/include/stdlib.h" 1 3 4
// # 35 "bionic/libc/include/stdlib.h" 3 4
// # 1 "bionic/libc/include/alloca.h" 1 3 4
// # 36 "bionic/libc/include/stdlib.h" 2 3 4
// # 1 "bionic/libc/include/malloc.h" 1 3 4
// # 21 "bionic/libc/include/malloc.h" 3 4
// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 1 3 4
// # 22 "bionic/libc/include/malloc.h" 2 3 4




extern void* malloc(size_t byte_count) __attribute__((malloc)) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(1)));
extern void* calloc(size_t item_count, size_t item_size) __attribute__((malloc)) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(1,2)));
extern void* realloc(void* p, size_t byte_count) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(2)));
extern void free(void* p);

extern void* memalign(size_t alignment, size_t byte_count) __attribute__((malloc)) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(2)));
extern size_t malloc_usable_size(const void* p);



struct mallinfo {
  size_t arena;
  size_t ordblks;
  size_t smblks;
  size_t hblks;
  size_t hblkhd;
  size_t usmblks;
  size_t fsmblks;
  size_t uordblks;
  size_t fordblks;
  size_t keepcost;
};


extern struct mallinfo mallinfo(void);
// # 71 "bionic/libc/include/malloc.h" 3 4
extern int malloc_info(int, FILE *);


// # 37 "bionic/libc/include/stdlib.h" 2 3 4
// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 1 3 4
// # 38 "bionic/libc/include/stdlib.h" 2 3 4






extern __attribute__((__noreturn__)) void abort(void);
extern __attribute__((__noreturn__)) void exit(int);
extern __attribute__((__noreturn__)) void _Exit(int);
extern int atexit(void (*)(void));






extern char* getenv(const char*);
extern int putenv(char*);
extern int setenv(const char*, const char*, int);
extern int unsetenv(const char*);
extern int clearenv(void);

extern char* mkdtemp(char*);
extern char* mktemp(char*) __attribute__((deprecated("mktemp is unsafe, use mkstemp or tmpfile instead")));

extern int mkostemp64(char*, int);
extern int mkostemp(char*, int);
extern int mkostemps64(char*, int, int);
extern int mkostemps(char*, int, int);
extern int mkstemp64(char*);
extern int mkstemp(char*);
extern int mkstemps64(char*, int);
extern int mkstemps(char*, int);

extern long strtol(const char *, char **, int);
extern long long strtoll(const char *, char **, int);
extern unsigned long strtoul(const char *, char **, int);
extern unsigned long long strtoull(const char *, char **, int);

extern int posix_memalign(void **memptr, size_t alignment, size_t size);

extern double atof(const char*) ;

extern double strtod(const char*, char**) __attribute__((visibility ("default")));
extern float strtof(const char*, char**) __attribute__((visibility ("default"))) ;
extern long double strtold(const char*, char**) __attribute__((visibility ("default")));

extern long double strtold_l(const char *, char **, locale_t) __attribute__((visibility ("default")));
extern long long strtoll_l(const char *, char **, int, locale_t) __attribute__((visibility ("default")));
extern unsigned long long strtoull_l(const char *, char **, int, locale_t) __attribute__((visibility ("default")));

extern int atoi(const char*) __attribute__((pure));
extern long atol(const char*) __attribute__((pure));
extern long long atoll(const char*) __attribute__((pure));

extern int abs(int) __attribute__((__const__)) ;
extern long labs(long) __attribute__((__const__)) ;
extern long long llabs(long long) __attribute__((__const__)) ;

extern char * realpath(const char *path, char *resolved);
extern int system(const char *string);

extern void * bsearch(const void *key, const void *base0,
 size_t nmemb, size_t size,
 int (*compar)(const void *, const void *));

extern void qsort(void *, size_t, size_t, int (*)(const void *, const void *));

uint32_t arc4random(void);
uint32_t arc4random_uniform(uint32_t);
void arc4random_buf(void*, size_t);



int rand(void) ;
int rand_r(unsigned int*);
void srand(unsigned int) ;

double drand48(void);
double erand48(unsigned short[3]);
long jrand48(unsigned short[3]);
void lcong48(unsigned short[7]);
long lrand48(void);
long mrand48(void);
long nrand48(unsigned short[3]);
unsigned short* seed48(unsigned short[3]);
void srand48(long);

char* initstate(unsigned int, char*, size_t);
long random(void) ;
char* setstate(char*);
void srandom(unsigned int) ;

int getpt(void);
int grantpt(int) ;
int posix_openpt(int);
char* ptsname(int);
int ptsname_r(int, char*, size_t);
int unlockpt(int);

typedef struct {
    int quot;
    int rem;
} div_t;

extern div_t div(int, int) __attribute__((__const__));

typedef struct {
    long int quot;
    long int rem;
} ldiv_t;

extern ldiv_t ldiv(long, long) __attribute__((__const__));

typedef struct {
    long long int quot;
    long long int rem;
} lldiv_t;

extern lldiv_t lldiv(long long, long long) __attribute__((__const__));


extern const char* getprogname(void);
extern void setprogname(const char*);


extern int mblen(const char *, size_t);
extern size_t mbstowcs(wchar_t *, const char *, size_t);
extern int mbtowc(wchar_t *, const char *, size_t);


extern int wctomb(char *, wchar_t);
extern size_t wcstombs(char *, const wchar_t *, size_t);

extern size_t __ctype_get_mb_cur_max(void);
// # 181 "bionic/libc/include/stdlib.h" 3 4
extern char* __realpath_real(const char*, char*) __asm__("realpath");
extern void __realpath_size_error(void) __attribute__((__error__("realpath output parameter must be NULL or a >= PATH_MAX bytes buffer")));


extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
char* realpath(const char* path, char* resolved) {
    size_t bos = __builtin_object_size((resolved), 1);


    if (bos != ((size_t) -1) && bos < 4096) {
        __realpath_size_error();
    }

    return __realpath_real(path, resolved);
}





// # 5 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2
// # 1 "bionic/libc/include/ctype.h" 1 3 4
// # 57 "bionic/libc/include/ctype.h" 3 4


extern const char *_ctype_;


int isalnum(int);
int isalpha(int);
int iscntrl(int);
int isdigit(int);
int isgraph(int);
int islower(int);
int isprint(int);
int ispunct(int);
int isspace(int);
int isupper(int);
int isxdigit(int);
int tolower(int);
int toupper(int);


int isalnum_l(int, locale_t);
int isalpha_l(int, locale_t);
int isblank_l(int, locale_t);
int iscntrl_l(int, locale_t);
int isdigit_l(int, locale_t);
int isgraph_l(int, locale_t);
int islower_l(int, locale_t);
int isprint_l(int, locale_t);
int ispunct_l(int, locale_t);
int isspace_l(int, locale_t);
int isupper_l(int, locale_t);
int isxdigit_l(int, locale_t);
int tolower_l(int, locale_t);
int toupper_l(int, locale_t);




int isblank(int);



int isascii(int);
int toascii(int);
int _tolower(int);
int _toupper(int);





// # 6 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2
// # 1 "bionic/libc/include/inttypes.h" 1 3 4
// # 251 "bionic/libc/include/inttypes.h" 3 4
typedef struct {
 intmax_t quot;
 intmax_t rem;
} imaxdiv_t;


intmax_t imaxabs(intmax_t) __attribute__((__const__));
imaxdiv_t imaxdiv(intmax_t, intmax_t) __attribute__((__const__));
intmax_t strtoimax(const char *, char **, int);
uintmax_t strtoumax(const char *, char **, int);
intmax_t wcstoimax(const wchar_t * __restrict,
      wchar_t ** __restrict, int);
uintmax_t wcstoumax(const wchar_t * __restrict,
      wchar_t ** __restrict, int);

// # 7 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2

// # 1 "system/core/include/cutils/properties.h" 1 3 4
// # 21 "system/core/include/cutils/properties.h" 3 4
// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 1 3 4
// # 22 "system/core/include/cutils/properties.h" 2 3 4
// # 1 "bionic/libc/include/sys/system_properties.h" 1 3 4
// # 34 "bionic/libc/include/sys/system_properties.h" 3 4


typedef struct prop_info prop_info;
// # 47 "bionic/libc/include/sys/system_properties.h" 3 4
int __system_property_get(const char *name, char *value);



int __system_property_set(const char *key, const char *value);
// # 62 "bionic/libc/include/sys/system_properties.h" 3 4
const prop_info *__system_property_find(const char *name);
// # 73 "bionic/libc/include/sys/system_properties.h" 3 4
int __system_property_read(const prop_info *pi, char *name, char *value);
// # 84 "bionic/libc/include/sys/system_properties.h" 3 4
const prop_info *__system_property_find_nth(unsigned n);
// # 96 "bionic/libc/include/sys/system_properties.h" 3 4
int __system_property_foreach(
        void (*propfn)(const prop_info *pi, void *cookie),
        void *cookie);


// # 23 "system/core/include/cutils/properties.h" 2 3 4
// # 46 "system/core/include/cutils/properties.h" 3 4
int property_get(const char *key, char *value, const char *default_value);
// # 62 "system/core/include/cutils/properties.h" 3 4
int8_t property_get_bool(const char *key, int8_t default_value);
// # 83 "system/core/include/cutils/properties.h" 3 4
int64_t property_get_int64(const char *key, int64_t default_value);
// # 104 "system/core/include/cutils/properties.h" 3 4
int32_t property_get_int32(const char *key, int32_t default_value);



int property_set(const char *key, const char *value);

int property_list(void (*propfn)(const char *key, const char *value, void *cookie), void *cookie);



extern int __property_get_real(const char *, char *, const char *)
    __asm__( "property_get");
extern void __property_get_too_small_error(void) __attribute__((__error__("property_get() called with too small of a buffer")));

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
int property_get(const char *key, char *value, const char *default_value) {
    size_t bos = __builtin_object_size((value), 1);
    if (bos < 92) {
        __property_get_too_small_error();
    }
    return __property_get_real(key, value, default_value);
}
// # 9 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2
// # 1 "bionic/libc/include/fcntl.h" 1 3 4
// # 34 "bionic/libc/include/fcntl.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/fadvise.h" 1 3 4
// # 35 "bionic/libc/include/fcntl.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/fcntl.h" 1 3 4



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/fcntl.h" 1 3 4
// # 9 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/fcntl.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/fcntl.h" 1 3 4
// # 131 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/fcntl.h" 3 4
struct f_owner_ex {
 int type;
 __kernel_pid_t pid;
};
// # 171 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/fcntl.h" 3 4
struct flock {
 short l_type;
 short l_whence;
 __kernel_off_t l_start;
 __kernel_off_t l_len;
 __kernel_pid_t l_pid;

};
// # 188 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/fcntl.h" 3 4
struct flock64 {
 short l_type;
 short l_whence;
 __kernel_loff_t l_start;
 __kernel_loff_t l_len;
 __kernel_pid_t l_pid;

};
// # 10 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/fcntl.h" 2 3 4
// # 5 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/fcntl.h" 2 3 4
// # 36 "bionic/libc/include/fcntl.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/stat.h" 1 3 4
// # 37 "bionic/libc/include/fcntl.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/uio.h" 1 3 4
// # 16 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/uio.h" 3 4
struct iovec
{
 void *iov_base;
 __kernel_size_t iov_len;
};
// # 38 "bionic/libc/include/fcntl.h" 2 3 4






// # 64 "bionic/libc/include/fcntl.h" 3 4
extern int creat(const char*, mode_t);
extern int creat64(const char*, mode_t);
extern int fcntl(int, int, ...);
extern int openat(int, const char*, int, ...);
extern int openat64(int, const char*, int, ...);
extern int open(const char*, int, ...);
extern int open64(const char*, int, ...);
extern ssize_t splice(int, off64_t*, int, off64_t*, size_t, unsigned int);
extern ssize_t tee(int, int, size_t, unsigned int);
extern int unlinkat(int, const char*, int);
extern ssize_t vmsplice(int, const struct iovec*, size_t, unsigned int);






extern int fallocate(int, int, off_t, off_t);
extern int posix_fadvise(int, off_t, off_t, int);
extern int posix_fallocate(int, off_t, off_t);

extern int fallocate64(int, int, off64_t, off64_t);
extern int posix_fadvise64(int, off64_t, off64_t, int);
extern int posix_fallocate64(int, off64_t, off64_t);

extern int __open_2(const char*, int);
extern int __open_real(const char*, int, ...) __asm__("open");
extern int __openat_2(int, const char*, int);
extern int __openat_real(int, const char*, int, ...) __asm__("openat");
extern void __creat_missing_mode(void) __attribute__((__error__("called with O_CREAT, but missing mode")));
extern void __creat_too_many_args(void) __attribute__((__error__("too many arguments")));





extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
int open(const char* pathname, int flags, ...) {
    if (__builtin_constant_p(flags)) {
        if ((flags & 00000100) && __builtin_va_arg_pack_len() == 0) {
            __creat_missing_mode();
        }
    }

    if (__builtin_va_arg_pack_len() > 1) {
        __creat_too_many_args();
    }

    if ((__builtin_va_arg_pack_len() == 0) && !__builtin_constant_p(flags)) {
        return __open_2(pathname, flags);
    }

    return __open_real(pathname, flags, __builtin_va_arg_pack());
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
int openat(int dirfd, const char* pathname, int flags, ...) {
    if (__builtin_constant_p(flags)) {
        if ((flags & 00000100) && __builtin_va_arg_pack_len() == 0) {
            __creat_missing_mode();
        }
    }

    if (__builtin_va_arg_pack_len() > 1) {
        __creat_too_many_args();
    }

    if ((__builtin_va_arg_pack_len() == 0) && !__builtin_constant_p(flags)) {
        return __openat_2(dirfd, pathname, flags);
    }

    return __openat_real(dirfd, pathname, flags, __builtin_va_arg_pack());
}






// # 10 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2
// # 1 "bionic/libc/include/dlfcn.h" 1 3 4
// # 33 "bionic/libc/include/dlfcn.h" 3 4


typedef struct {
    const char *dli_fname;

    void *dli_fbase;

    const char *dli_sname;

    void *dli_saddr;

} Dl_info;

extern void* dlopen(const char* filename, int flag);
extern int dlclose(void* handle);
extern const char* dlerror(void);
extern void* dlsym(void* handle, const char* symbol) __attribute__((__nonnull__ (2)));
extern void* dlvsym(void* handle, const char* symbol, const char* version) __attribute__((__nonnull__ (2, 3)));
extern int dladdr(const void* addr, Dl_info *info);

enum {



  RTLD_NOW = 0,

  RTLD_LAZY = 1,

  RTLD_LOCAL = 0,



  RTLD_GLOBAL = 2,

  RTLD_NOLOAD = 4,
  RTLD_NODELETE = 0x01000,
};
// # 79 "bionic/libc/include/dlfcn.h" 3 4

// # 11 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_ion.h" 1



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/msm_ion.h" 1



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/ion.h" 1
// # 20 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/ion.h"
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/ioctl.h" 1



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/ioctl.h" 1
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/ioctl.h" 1
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/ioctl.h" 2
// # 5 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/ioctl.h" 2
// # 21 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/ion.h" 2


typedef int ion_user_handle_t;
// # 37 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/ion.h"
enum ion_heap_type {
 ION_HEAP_TYPE_SYSTEM,
 ION_HEAP_TYPE_SYSTEM_CONTIG,
 ION_HEAP_TYPE_CARVEOUT,
 ION_HEAP_TYPE_CHUNK,
 ION_HEAP_TYPE_DMA,
 ION_HEAP_TYPE_CUSTOM,

 ION_NUM_HEAPS = 16,
};
// # 86 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/ion.h"
struct ion_allocation_data {
 size_t len;
 size_t align;
 unsigned int heap_id_mask;
 unsigned int flags;
 ion_user_handle_t handle;
};
// # 104 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/ion.h"
struct ion_fd_data {
 ion_user_handle_t handle;
 int fd;
};





struct ion_handle_data {
 ion_user_handle_t handle;
};
// # 125 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/ion.h"
struct ion_custom_data {
 unsigned int cmd;
 unsigned long arg;
};
// # 5 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/msm_ion.h" 2

enum msm_ion_heap_types {
 ION_HEAP_TYPE_MSM_START = ION_HEAP_TYPE_CUSTOM + 1,
 ION_HEAP_TYPE_SECURE_DMA = ION_HEAP_TYPE_MSM_START,
 ION_HEAP_TYPE_REMOVED,




};
// # 25 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/msm_ion.h"
enum ion_heap_ids {
 INVALID_HEAP_ID = -1,
 ION_CP_MM_HEAP_ID = 8,
 ION_CP_MFC_HEAP_ID = 12,
 ION_CP_WB_HEAP_ID = 16,
 ION_CAMERA_HEAP_ID = 20,
 ION_SYSTEM_CONTIG_HEAP_ID = 21,
 ION_ADSP_HEAP_ID = 22,
 ION_PIL1_HEAP_ID = 23,
 ION_SF_HEAP_ID = 24,
 ION_SYSTEM_HEAP_ID = 25,
 ION_PIL2_HEAP_ID = 26,
 ION_QSECOM_HEAP_ID = 27,
 ION_AUDIO_HEAP_ID = 28,

 ION_MM_FIRMWARE_HEAP_ID = 29,

 ION_HEAP_ID_RESERVED = 31
};
// # 52 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/msm_ion.h"
enum ion_fixed_position {
 NOT_FIXED,
 FIXED_LOW,
 FIXED_MIDDLE,
 FIXED_HIGH,
};

enum cp_mem_usage {
 VIDEO_BITSTREAM = 0x1,
 VIDEO_PIXEL = 0x2,
 VIDEO_NONPIXEL = 0x3,
 DISPLAY_SECURE_CP_USAGE = 0x4,
 CAMERA_SECURE_CP_USAGE = 0x5,
 MAX_USAGE = 0x6,
 UNKNOWN = 0x7FFFFFFF,
};
// # 131 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/staging/android/uapi/msm_ion.h"
struct ion_flush_data {
 ion_user_handle_t handle;
 int fd;
 void *vaddr;
 unsigned int offset;
 unsigned int length;
};


struct ion_prefetch_data {
 int heap_id;
 unsigned long len;
};
// # 5 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_ion.h" 2
// # 12 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2
// # 1 "bionic/libc/include/sys/mman.h" 1 3 4
// # 33 "bionic/libc/include/sys/mman.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/mman.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/mman.h" 1 3 4



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/mman-common.h" 1 3 4
// # 5 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/mman.h" 2 3 4
// # 2 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/mman.h" 2 3 4
// # 34 "bionic/libc/include/sys/mman.h" 2 3 4


// # 55 "bionic/libc/include/sys/mman.h" 3 4
extern void* mmap(void*, size_t, int, int, int, off_t);

extern void* mmap64(void*, size_t, int, int, int, off64_t);

extern int munmap(void*, size_t);
extern int msync(const void*, size_t, int);
extern int mprotect(const void*, size_t, int);
extern void* mremap(void*, size_t, size_t, int, ...);

extern int mlockall(int);
extern int munlockall(void);
extern int mlock(const void*, size_t);
extern int munlock(const void*, size_t);
extern int madvise(void*, size_t, int);

extern int mlock(const void*, size_t);
extern int munlock(const void*, size_t);

extern int mincore(void*, size_t, unsigned char*);

extern int posix_madvise(void*, size_t, int);


// # 13 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2

// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 1
// # 33 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h"
// # 1 "bionic/libc/include/pthread.h" 1 3 4
// # 34 "bionic/libc/include/pthread.h" 3 4
// # 1 "bionic/libc/include/sched.h" 1 3 4
// # 32 "bionic/libc/include/sched.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/sched.h" 1 3 4
// # 33 "bionic/libc/include/sched.h" 2 3 4







struct sched_param {
  int sched_priority;
};

extern int sched_setscheduler(pid_t, int, const struct sched_param*);
extern int sched_getscheduler(pid_t);
extern int sched_yield(void);
extern int sched_get_priority_max(int);
extern int sched_get_priority_min(int);
extern int sched_setparam(pid_t, const struct sched_param*);
extern int sched_getparam(pid_t, struct sched_param*);
extern int sched_rr_get_interval(pid_t, struct timespec*);
// # 149 "bionic/libc/include/sched.h" 3 4

// # 35 "bionic/libc/include/pthread.h" 2 3 4


// # 1 "bionic/libc/include/time.h" 1 3 4
// # 33 "bionic/libc/include/time.h" 3 4
// # 1 "bionic/libc/include/sys/time.h" 1 3 4
// # 39 "bionic/libc/include/sys/time.h" 3 4


extern int gettimeofday(struct timeval *, struct timezone *);
extern int settimeofday(const struct timeval *, const struct timezone *);

extern int getitimer(int, struct itimerval *);
extern int setitimer(int, const struct itimerval *, struct itimerval *);

extern int utimes(const char *, const struct timeval *);
// # 89 "bionic/libc/include/sys/time.h" 3 4

// # 34 "bionic/libc/include/time.h" 2 3 4






extern char* tzname[] __attribute__((visibility ("default")));
extern int daylight __attribute__((visibility ("default")));
extern long int timezone __attribute__((visibility ("default")));

struct sigevent;

struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
  long int tm_gmtoff;
  const char* tm_zone;
};



extern time_t time(time_t*) __attribute__((visibility ("default")));
extern int nanosleep(const struct timespec*, struct timespec*) __attribute__((visibility ("default")));

extern char* asctime(const struct tm*) __attribute__((visibility ("default")));
extern char* asctime_r(const struct tm*, char*) __attribute__((visibility ("default")));

extern double difftime(time_t, time_t) __attribute__((visibility ("default")));
extern time_t mktime(struct tm*) __attribute__((visibility ("default")));

extern struct tm* localtime(const time_t*) __attribute__((visibility ("default")));
extern struct tm* localtime_r(const time_t*, struct tm*) __attribute__((visibility ("default")));

extern struct tm* gmtime(const time_t*) __attribute__((visibility ("default")));
extern struct tm* gmtime_r(const time_t*, struct tm*) __attribute__((visibility ("default")));

extern char* strptime(const char*, const char*, struct tm*) __attribute__((visibility ("default")));
extern size_t strftime(char*, size_t, const char*, const struct tm*) __attribute__((visibility ("default")));
extern size_t strftime_l(char *, size_t, const char *, const struct tm *, locale_t) __attribute__((visibility ("default")));

extern char* ctime(const time_t*) __attribute__((visibility ("default")));
extern char* ctime_r(const time_t*, char*) __attribute__((visibility ("default")));

extern void tzset(void) __attribute__((visibility ("default")));

extern clock_t clock(void) __attribute__((visibility ("default")));

extern int clock_getcpuclockid(pid_t, clockid_t*) __attribute__((visibility ("default")));

extern int clock_getres(clockid_t, struct timespec*) __attribute__((visibility ("default")));
extern int clock_gettime(clockid_t, struct timespec*) __attribute__((visibility ("default")));
extern int clock_nanosleep(clockid_t, int, const struct timespec*, struct timespec*) __attribute__((visibility ("default")));
extern int clock_settime(clockid_t, const struct timespec*) __attribute__((visibility ("default")));

extern int timer_create(int, struct sigevent*, timer_t*) __attribute__((visibility ("default")));
extern int timer_delete(timer_t) __attribute__((visibility ("default")));
extern int timer_settime(timer_t, int, const struct itimerspec*, struct itimerspec*) __attribute__((visibility ("default")));
extern int timer_gettime(timer_t, struct itimerspec*) __attribute__((visibility ("default")));
extern int timer_getoverrun(timer_t) __attribute__((visibility ("default")));


extern time_t timelocal(struct tm*) __attribute__((visibility ("default")));
extern time_t timegm(struct tm*) __attribute__((visibility ("default")));


// # 38 "bionic/libc/include/pthread.h" 2 3 4

typedef struct {



  int32_t __private[1];

} pthread_mutex_t;

typedef long pthread_mutexattr_t;

enum {
    PTHREAD_MUTEX_NORMAL = 0,
    PTHREAD_MUTEX_RECURSIVE = 1,
    PTHREAD_MUTEX_ERRORCHECK = 2,

    PTHREAD_MUTEX_ERRORCHECK_NP = PTHREAD_MUTEX_ERRORCHECK,
    PTHREAD_MUTEX_RECURSIVE_NP = PTHREAD_MUTEX_RECURSIVE,

    PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL
};





typedef struct {



  int32_t __private[1];

} pthread_cond_t;

typedef long pthread_condattr_t;



typedef struct {



  int32_t __private[10];

} pthread_rwlock_t;

typedef long pthread_rwlockattr_t;



enum {
  PTHREAD_RWLOCK_PREFER_READER_NP = 0,
  PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP = 1,
};

typedef int pthread_key_t;

typedef int pthread_once_t;



typedef struct {



  int32_t __private[8];

} pthread_barrier_t;

typedef int pthread_barrierattr_t;



typedef struct {



  int32_t __private[2];

} pthread_spinlock_t;
// # 134 "bionic/libc/include/pthread.h" 3 4


int pthread_atfork(void (*)(void), void (*)(void), void(*)(void));

int pthread_attr_destroy(pthread_attr_t*) __attribute__((__nonnull__ (1)));
int pthread_attr_getdetachstate(const pthread_attr_t*, int*) __attribute__((__nonnull__ (1, 2)));
int pthread_attr_getguardsize(const pthread_attr_t*, size_t*) __attribute__((__nonnull__ (1, 2)));
int pthread_attr_getschedparam(const pthread_attr_t*, struct sched_param*) __attribute__((__nonnull__ (1, 2)));
int pthread_attr_getschedpolicy(const pthread_attr_t*, int*) __attribute__((__nonnull__ (1, 2)));
int pthread_attr_getscope(const pthread_attr_t*, int*) __attribute__((__nonnull__ (1, 2)));
int pthread_attr_getstack(const pthread_attr_t*, void**, size_t*) __attribute__((__nonnull__ (1, 2, 3)));
int pthread_attr_getstacksize(const pthread_attr_t*, size_t*) __attribute__((__nonnull__ (1, 2)));
int pthread_attr_init(pthread_attr_t*) __attribute__((__nonnull__ (1)));
int pthread_attr_setdetachstate(pthread_attr_t*, int) __attribute__((__nonnull__ (1)));
int pthread_attr_setguardsize(pthread_attr_t*, size_t) __attribute__((__nonnull__ (1)));
int pthread_attr_setschedparam(pthread_attr_t*, const struct sched_param*) __attribute__((__nonnull__ (1, 2)));
int pthread_attr_setschedpolicy(pthread_attr_t*, int) __attribute__((__nonnull__ (1)));
int pthread_attr_setscope(pthread_attr_t*, int) __attribute__((__nonnull__ (1)));
int pthread_attr_setstack(pthread_attr_t*, void*, size_t) __attribute__((__nonnull__ (1)));
int pthread_attr_setstacksize(pthread_attr_t*, size_t) __attribute__((__nonnull__ (1)));

int pthread_condattr_destroy(pthread_condattr_t*) __attribute__((__nonnull__ (1)));
int pthread_condattr_getclock(const pthread_condattr_t*, clockid_t*) __attribute__((__nonnull__ (1, 2)));
int pthread_condattr_getpshared(const pthread_condattr_t*, int*) __attribute__((__nonnull__ (1, 2)));
int pthread_condattr_init(pthread_condattr_t*) __attribute__((__nonnull__ (1)));
int pthread_condattr_setclock(pthread_condattr_t*, clockid_t) __attribute__((__nonnull__ (1)));
int pthread_condattr_setpshared(pthread_condattr_t*, int) __attribute__((__nonnull__ (1)));

int pthread_cond_broadcast(pthread_cond_t*) __attribute__((__nonnull__ (1)));
int pthread_cond_destroy(pthread_cond_t*) __attribute__((__nonnull__ (1)));
int pthread_cond_init(pthread_cond_t*, const pthread_condattr_t*) __attribute__((__nonnull__ (1)));
int pthread_cond_signal(pthread_cond_t*) __attribute__((__nonnull__ (1)));
int pthread_cond_timedwait(pthread_cond_t*, pthread_mutex_t*, const struct timespec*) __attribute__((__nonnull__ (1, 2, 3)));
int pthread_cond_wait(pthread_cond_t*, pthread_mutex_t*) __attribute__((__nonnull__ (1, 2)));

int pthread_create(pthread_t*, pthread_attr_t const*, void *(*)(void*), void*) __attribute__((__nonnull__ (1, 3)));
int pthread_detach(pthread_t);
void pthread_exit(void*) __attribute__((__noreturn__));

int pthread_equal(pthread_t, pthread_t);

int pthread_getattr_np(pthread_t, pthread_attr_t*) __attribute__((__nonnull__ (2)));

int pthread_getcpuclockid(pthread_t, clockid_t*) __attribute__((__nonnull__ (2)));

int pthread_getschedparam(pthread_t, int*, struct sched_param*) __attribute__((__nonnull__ (2, 3)));

void* pthread_getspecific(pthread_key_t);

pid_t pthread_gettid_np(pthread_t);

int pthread_join(pthread_t, void**);

int pthread_key_create(pthread_key_t*, void (*)(void*)) __attribute__((__nonnull__ (1)));
int pthread_key_delete(pthread_key_t);

int pthread_mutexattr_destroy(pthread_mutexattr_t*) __attribute__((__nonnull__ (1)));
int pthread_mutexattr_getpshared(const pthread_mutexattr_t*, int*) __attribute__((__nonnull__ (1, 2)));
int pthread_mutexattr_gettype(const pthread_mutexattr_t*, int*) __attribute__((__nonnull__ (1, 2)));
int pthread_mutexattr_init(pthread_mutexattr_t*) __attribute__((__nonnull__ (1)));
int pthread_mutexattr_setpshared(pthread_mutexattr_t*, int) __attribute__((__nonnull__ (1)));
int pthread_mutexattr_settype(pthread_mutexattr_t*, int) __attribute__((__nonnull__ (1)));

int pthread_mutex_destroy(pthread_mutex_t*) __attribute__((__nonnull__ (1)));
int pthread_mutex_init(pthread_mutex_t*, const pthread_mutexattr_t*) __attribute__((__nonnull__ (1)));

int pthread_mutex_lock(pthread_mutex_t*) ;



int pthread_mutex_timedlock(pthread_mutex_t*, const struct timespec*) __attribute__((__nonnull__ (1, 2)));
int pthread_mutex_trylock(pthread_mutex_t*) __attribute__((__nonnull__ (1)));

int pthread_mutex_unlock(pthread_mutex_t*) ;




int pthread_once(pthread_once_t*, void (*)(void)) __attribute__((__nonnull__ (1, 2)));

int pthread_rwlockattr_init(pthread_rwlockattr_t*) __attribute__((__nonnull__ (1)));
int pthread_rwlockattr_destroy(pthread_rwlockattr_t*) __attribute__((__nonnull__ (1)));
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t*, int*) __attribute__((__nonnull__ (1, 2)));
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t*, int) __attribute__((__nonnull__ (1)));
int pthread_rwlockattr_getkind_np(const pthread_rwlockattr_t*, int*) __attribute__((__nonnull__ (1, 2)));
int pthread_rwlockattr_setkind_np(pthread_rwlockattr_t*, int) __attribute__((__nonnull__ (1)));

int pthread_rwlock_destroy(pthread_rwlock_t*) __attribute__((__nonnull__ (1)));
int pthread_rwlock_init(pthread_rwlock_t*, const pthread_rwlockattr_t*) __attribute__((__nonnull__ (1)));
int pthread_rwlock_rdlock(pthread_rwlock_t*) __attribute__((__nonnull__ (1)));
int pthread_rwlock_timedrdlock(pthread_rwlock_t*, const struct timespec*) __attribute__((__nonnull__ (1, 2)));
int pthread_rwlock_timedwrlock(pthread_rwlock_t*, const struct timespec*) __attribute__((__nonnull__ (1, 2)));
int pthread_rwlock_tryrdlock(pthread_rwlock_t*) __attribute__((__nonnull__ (1)));
int pthread_rwlock_trywrlock(pthread_rwlock_t*) __attribute__((__nonnull__ (1)));
int pthread_rwlock_unlock(pthread_rwlock_t *) __attribute__((__nonnull__ (1)));
int pthread_rwlock_wrlock(pthread_rwlock_t*) __attribute__((__nonnull__ (1)));

int pthread_barrierattr_init(pthread_barrierattr_t* attr) __attribute__((__nonnull__ (1)));
int pthread_barrierattr_destroy(pthread_barrierattr_t* attr) __attribute__((__nonnull__ (1)));
int pthread_barrierattr_getpshared(pthread_barrierattr_t* attr, int* pshared) __attribute__((__nonnull__ (1, 2)));
int pthread_barrierattr_setpshared(pthread_barrierattr_t* attr, int pshared) __attribute__((__nonnull__ (1)));

int pthread_barrier_init(pthread_barrier_t*, const pthread_barrierattr_t*, unsigned) __attribute__((__nonnull__ (1)));
int pthread_barrier_destroy(pthread_barrier_t*) __attribute__((__nonnull__ (1)));
int pthread_barrier_wait(pthread_barrier_t*) __attribute__((__nonnull__ (1)));

int pthread_spin_destroy(pthread_spinlock_t*) __attribute__((__nonnull__ (1)));
int pthread_spin_init(pthread_spinlock_t*, int) __attribute__((__nonnull__ (1)));
int pthread_spin_lock(pthread_spinlock_t*) __attribute__((__nonnull__ (1)));
int pthread_spin_trylock(pthread_spinlock_t*) __attribute__((__nonnull__ (1)));
int pthread_spin_unlock(pthread_spinlock_t*) __attribute__((__nonnull__ (1)));

pthread_t pthread_self(void) __attribute__((__const__));

int pthread_setname_np(pthread_t, const char*) __attribute__((__nonnull__ (2)));

int pthread_setschedparam(pthread_t, int, const struct sched_param*) __attribute__((__nonnull__ (3)));

int pthread_setspecific(pthread_key_t, const void*);

typedef void (*__pthread_cleanup_func_t)(void*);

typedef struct __pthread_cleanup_t {
  struct __pthread_cleanup_t* __cleanup_prev;
  __pthread_cleanup_func_t __cleanup_routine;
  void* __cleanup_arg;
} __pthread_cleanup_t;

extern void __pthread_cleanup_push(__pthread_cleanup_t* c, __pthread_cleanup_func_t, void*);
extern void __pthread_cleanup_pop(__pthread_cleanup_t*, int);
// # 286 "bionic/libc/include/pthread.h" 3 4
int pthread_cond_timedwait_monotonic_np(pthread_cond_t*, pthread_mutex_t*, const struct timespec*);
int pthread_cond_timedwait_monotonic(pthread_cond_t*, pthread_mutex_t*, const struct timespec*);

int pthread_cond_timedwait_relative_np(pthread_cond_t*, pthread_mutex_t*, const struct timespec*) ;

int pthread_cond_timeout_np(pthread_cond_t*, pthread_mutex_t*, unsigned) ;

int pthread_mutex_lock_timeout_np(pthread_mutex_t*, unsigned) __attribute__((deprecated("use pthread_mutex_timedlock instead")));




// # 34 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 2
// # 1 "bionic/libc/include/errno.h" 1 3 4
// # 32 "bionic/libc/include/errno.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/errno.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/errno.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/errno.h" 1 3 4



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/errno-base.h" 1 3 4
// # 5 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/errno.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/errno.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/errno.h" 2 3 4
// # 33 "bionic/libc/include/errno.h" 2 3 4


// # 44 "bionic/libc/include/errno.h" 3 4
extern volatile int* __errno(void) __attribute__((__const__));
// # 53 "bionic/libc/include/errno.h" 3 4

// # 35 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 2
// # 1 "bionic/libc/include/sys/ioctl.h" 1 3 4
// # 37 "bionic/libc/include/sys/ioctl.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/termios.h" 1 3 4




// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/termios.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/termios.h" 1 3 4
// # 11 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/termios.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/termbits.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/termbits.h" 1 3 4





typedef unsigned char cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;


struct termios {
 tcflag_t c_iflag;
 tcflag_t c_oflag;
 tcflag_t c_cflag;
 tcflag_t c_lflag;
 cc_t c_line;
 cc_t c_cc[19];
};

struct termios2 {
 tcflag_t c_iflag;
 tcflag_t c_oflag;
 tcflag_t c_cflag;
 tcflag_t c_lflag;
 cc_t c_line;
 cc_t c_cc[19];
 speed_t c_ispeed;
 speed_t c_ospeed;
};

struct ktermios {
 tcflag_t c_iflag;
 tcflag_t c_oflag;
 tcflag_t c_cflag;
 tcflag_t c_lflag;
 cc_t c_line;
 cc_t c_cc[19];
 speed_t c_ispeed;
 speed_t c_ospeed;
};
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/termbits.h" 2 3 4
// # 12 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/termios.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/ioctls.h" 1 3 4





// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/ioctls.h" 1 3 4
// # 7 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/ioctls.h" 2 3 4
// # 13 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/termios.h" 2 3 4

struct winsize {
 unsigned short ws_row;
 unsigned short ws_col;
 unsigned short ws_xpixel;
 unsigned short ws_ypixel;
};


struct termio {
 unsigned short c_iflag;
 unsigned short c_oflag;
 unsigned short c_cflag;
 unsigned short c_lflag;
 unsigned char c_line;
 unsigned char c_cc[8];
};
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/termios.h" 2 3 4
// # 6 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/termios.h" 2 3 4



struct termiox
{
 __u16 x_hflag;
 __u16 x_cflag;
 __u16 x_rflag[5];
 __u16 x_sflag;
};
// # 38 "bionic/libc/include/sys/ioctl.h" 2 3 4

// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/termbits.h" 1 3 4
// # 40 "bionic/libc/include/sys/ioctl.h" 2 3 4
// # 1 "bionic/libc/include/sys/ioctl_compat.h" 1 3 4
// # 46 "bionic/libc/include/sys/ioctl_compat.h" 3 4
struct tchars {
 char t_intrc;
 char t_quitc;
 char t_startc;
 char t_stopc;
 char t_eofc;
 char t_brkc;
};

struct ltchars {
 char t_suspc;
 char t_dsuspc;
 char t_rprntc;
 char t_flushc;
 char t_werasc;
 char t_lnextc;
};






struct sgttyb {
 char sg_ispeed;
 char sg_ospeed;
 char sg_erase;
 char sg_kill;
 short sg_flags;
};
// # 41 "bionic/libc/include/sys/ioctl.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/tty.h" 1 3 4
// # 42 "bionic/libc/include/sys/ioctl.h" 2 3 4



extern int ioctl(int, int, ...);


// # 36 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 2

// # 1 "bionic/libc/include/sys/stat.h" 1 3 4
// # 37 "bionic/libc/include/sys/stat.h" 3 4

// # 117 "bionic/libc/include/sys/stat.h" 3 4
struct stat { unsigned long long st_dev; unsigned char __pad0[4]; unsigned long __st_ino; unsigned int st_mode; nlink_t st_nlink; uid_t st_uid; gid_t st_gid; unsigned long long st_rdev; unsigned char __pad3[4]; long long st_size; unsigned long st_blksize; unsigned long long st_blocks; struct timespec st_atim; struct timespec st_mtim; struct timespec st_ctim; unsigned long long st_ino; };
struct stat64 { unsigned long long st_dev; unsigned char __pad0[4]; unsigned long __st_ino; unsigned int st_mode; nlink_t st_nlink; uid_t st_uid; gid_t st_gid; unsigned long long st_rdev; unsigned char __pad3[4]; long long st_size; unsigned long st_blksize; unsigned long long st_blocks; struct timespec st_atim; struct timespec st_mtim; struct timespec st_ctim; unsigned long long st_ino; };
// # 138 "bionic/libc/include/sys/stat.h" 3 4
extern int chmod(const char*, mode_t);
extern int fchmod(int, mode_t);
extern int mkdir(const char*, mode_t);

extern int fstat(int, struct stat*);
extern int fstat64(int, struct stat64*);
extern int fstatat(int, const char*, struct stat*, int);
extern int fstatat64(int, const char*, struct stat64*, int);
extern int lstat(const char*, struct stat*);
extern int lstat64(const char*, struct stat64*);
extern int stat(const char*, struct stat*);
extern int stat64(const char*, struct stat64*);

extern int mknod(const char*, mode_t, dev_t);
extern mode_t umask(mode_t);

extern mode_t __umask_chk(mode_t);
extern mode_t __umask_real(mode_t) __asm__("umask");
extern void __umask_invalid_mode(void) __attribute__((__error__("umask called with invalid mode")));



extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
mode_t umask(mode_t mode) {

  if (__builtin_constant_p(mode)) {
    if ((mode & 0777) != mode) {
      __umask_invalid_mode();
    }
    return __umask_real(mode);
  }

  return __umask_chk(mode);
}


extern int mkfifo(const char*, mode_t) ;
extern int mkfifoat(int, const char*, mode_t);

extern int fchmodat(int, const char*, mode_t, int);
extern int mkdirat(int, const char*, mode_t);
extern int mknodat(int, const char*, mode_t, dev_t);



extern int utimensat(int fd, const char *path, const struct timespec times[2], int flags);
extern int futimens(int fd, const struct timespec times[2]);






// # 38 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 2

// # 1 "bionic/libc/include/poll.h" 1 3 4
// # 33 "bionic/libc/include/poll.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/poll.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/poll.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/poll.h" 1 3 4
// # 33 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/poll.h" 3 4
struct pollfd {
 int fd;
 short events;
 short revents;
};
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/poll.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/poll.h" 2 3 4
// # 34 "bionic/libc/include/poll.h" 2 3 4





typedef unsigned int nfds_t;

int poll(struct pollfd*, nfds_t, int);
int ppoll(struct pollfd*, nfds_t, const struct timespec*, const sigset_t*);

int __poll_chk(struct pollfd*, nfds_t, int, size_t);
int __poll_real(struct pollfd*, nfds_t, int) __asm__("poll");
extern void __poll_too_small_error(void) __attribute__((__error__("poll: pollfd array smaller than fd count")));

int __ppoll_chk(struct pollfd*, nfds_t, const struct timespec*, const sigset_t*, size_t);
int __ppoll_real(struct pollfd*, nfds_t, const struct timespec*, const sigset_t*) __asm__("ppoll");
extern void __ppoll_too_small_error(void) __attribute__((__error__("ppoll: pollfd array smaller than fd count")));



extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
int poll(struct pollfd* fds, nfds_t fd_count, int timeout) {



  if (__builtin_object_size((fds), 1) != ((size_t) -1)) {
    if (!__builtin_constant_p(fd_count)) {
      return __poll_chk(fds, fd_count, timeout, __builtin_object_size((fds), 1));
    } else if (__builtin_object_size((fds), 1) / sizeof(*fds) < fd_count) {
      __poll_too_small_error();
    }
  }
  return __poll_real(fds, fd_count, timeout);

}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
int ppoll(struct pollfd* fds, nfds_t fd_count, const struct timespec* timeout, const sigset_t* mask) {



  if (__builtin_object_size((fds), 1) != ((size_t) -1)) {
    if (!__builtin_constant_p(fd_count)) {
      return __ppoll_chk(fds, fd_count, timeout, mask, __builtin_object_size((fds), 1));
    } else if (__builtin_object_size((fds), 1) / sizeof(*fds) < fd_count) {
      __ppoll_too_small_error();
    }
  }
  return __ppoll_real(fds, fd_count, timeout, mask);

}




// # 40 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 2
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/fb.h" 1




// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/i2c.h" 1
// # 68 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/i2c.h"
struct i2c_msg {
 __u16 addr;
 __u16 flags;
// # 79 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/i2c.h"
 __u16 len;
 __u8 *buf;
};
// # 128 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/i2c.h"
union i2c_smbus_data {
 __u8 byte;
 __u16 word;
 __u8 block[32 + 2];

};
// # 6 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/fb.h" 2
// # 154 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/fb.h"
struct fb_fix_screeninfo {
 char id[16];
 unsigned long smem_start;

 __u32 smem_len;
 __u32 type;
 __u32 type_aux;
 __u32 visual;
 __u16 xpanstep;
 __u16 ypanstep;
 __u16 ywrapstep;
 __u32 line_length;
 unsigned long mmio_start;

 __u32 mmio_len;
 __u32 accel;

 __u16 capabilities;
 __u16 reserved[2];
};
// # 185 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/fb.h"
struct fb_bitfield {
 __u32 offset;
 __u32 length;
 __u32 msb_right;

};
// # 238 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/fb.h"
struct fb_var_screeninfo {
 __u32 xres;
 __u32 yres;
 __u32 xres_virtual;
 __u32 yres_virtual;
 __u32 xoffset;
 __u32 yoffset;

 __u32 bits_per_pixel;
 __u32 grayscale;

 struct fb_bitfield red;
 struct fb_bitfield green;
 struct fb_bitfield blue;
 struct fb_bitfield transp;

 __u32 nonstd;

 __u32 activate;

 __u32 height;
 __u32 width;

 __u32 accel_flags;


 __u32 pixclock;
 __u32 left_margin;
 __u32 right_margin;
 __u32 upper_margin;
 __u32 lower_margin;
 __u32 hsync_len;
 __u32 vsync_len;
 __u32 sync;
 __u32 vmode;
 __u32 rotate;
 __u32 colorspace;
 __u32 reserved[4];
};

struct fb_cmap {
 __u32 start;
 __u32 len;
 __u16 *red;
 __u16 *green;
 __u16 *blue;
 __u16 *transp;
};

struct fb_con2fbmap {
 __u32 console;
 __u32 framebuffer;
};
// # 299 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/fb.h"
enum {

 FB_BLANK_UNBLANK = 0,


 FB_BLANK_NORMAL = 0 + 1,


 FB_BLANK_VSYNC_SUSPEND = 1 + 1,


 FB_BLANK_HSYNC_SUSPEND = 2 + 1,


 FB_BLANK_POWERDOWN = 3 + 1
};
// # 326 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/fb.h"
struct fb_vblank {
 __u32 flags;
 __u32 count;
 __u32 vcount;
 __u32 hcount;
 __u32 reserved[4];
};





struct fb_copyarea {
 __u32 dx;
 __u32 dy;
 __u32 width;
 __u32 height;
 __u32 sx;
 __u32 sy;
};

struct fb_fillrect {
 __u32 dx;
 __u32 dy;
 __u32 width;
 __u32 height;
 __u32 color;
 __u32 rop;
};

struct fb_image {
 __u32 dx;
 __u32 dy;
 __u32 width;
 __u32 height;
 __u32 fg_color;
 __u32 bg_color;
 __u8 depth;
 const char *data;
 struct fb_cmap cmap;
};
// # 380 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/fb.h"
struct fbcurpos {
 __u16 x, y;
};

struct fb_cursor {
 __u16 set;
 __u16 enable;
 __u16 rop;
 const char *mask;
 struct fbcurpos hot;
 struct fb_image image;
};
// # 41 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 2
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h" 1
// # 108 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
enum {
 NOTIFY_UPDATE_INIT,
 NOTIFY_UPDATE_DEINIT,
 NOTIFY_UPDATE_START,
 NOTIFY_UPDATE_STOP,
 NOTIFY_UPDATE_POWER_OFF,
};

enum {
 NOTIFY_TYPE_NO_UPDATE,
 NOTIFY_TYPE_SUSPEND,
 NOTIFY_TYPE_UPDATE,
 NOTIFY_TYPE_BL_UPDATE,
};

enum {
 MDP_RGB_565,
 MDP_XRGB_8888,
 MDP_Y_CBCR_H2V2,
 MDP_Y_CBCR_H2V2_ADRENO,
 MDP_ARGB_8888,
 MDP_RGB_888,
 MDP_Y_CRCB_H2V2,
 MDP_YCRYCB_H2V1,
 MDP_CBYCRY_H2V1,
 MDP_Y_CRCB_H2V1,
 MDP_Y_CBCR_H2V1,
 MDP_Y_CRCB_H1V2,
 MDP_Y_CBCR_H1V2,
 MDP_RGBA_8888,
 MDP_BGRA_8888,
 MDP_RGBX_8888,
 MDP_Y_CRCB_H2V2_TILE,
 MDP_Y_CBCR_H2V2_TILE,
 MDP_Y_CR_CB_H2V2,
 MDP_Y_CR_CB_GH2V2,
 MDP_Y_CB_CR_H2V2,
 MDP_Y_CRCB_H1V1,
 MDP_Y_CBCR_H1V1,
 MDP_YCRCB_H1V1,
 MDP_YCBCR_H1V1,
 MDP_BGR_565,
 MDP_BGR_888,
 MDP_Y_CBCR_H2V2_VENUS,
 MDP_BGRX_8888,
 MDP_RGBA_8888_TILE,
 MDP_ARGB_8888_TILE,
 MDP_ABGR_8888_TILE,
 MDP_BGRA_8888_TILE,
 MDP_RGBX_8888_TILE,
 MDP_XRGB_8888_TILE,
 MDP_XBGR_8888_TILE,
 MDP_BGRX_8888_TILE,
 MDP_YCBYCR_H2V1,
 MDP_RGB_565_TILE,
 MDP_BGR_565_TILE,
 MDP_ARGB_1555,
 MDP_RGBA_5551,
 MDP_ARGB_4444,
 MDP_RGBA_4444,
 MDP_IMGTYPE_LIMIT,
 MDP_RGB_BORDERFILL,
 MDP_FB_FORMAT = 0x10000,
 MDP_IMGTYPE_LIMIT2
};

enum {
 PMEM_IMG,
 FB_IMG,
};

enum {
 HSIC_HUE = 0,
 HSIC_SAT,
 HSIC_INT,
 HSIC_CON,
 NUM_HSIC_PARAM,
};

enum mdss_mdp_max_bw_mode {
 MDSS_MAX_BW_LIMIT_DEFAULT = 0x1,
 MDSS_MAX_BW_LIMIT_CAMERA = 0x2,
 MDSS_MAX_BW_LIMIT_HFLIP = 0x4,
 MDSS_MAX_BW_LIMIT_VFLIP = 0x8,
};
// # 255 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
struct mdp_rect {
 uint32_t x;
 uint32_t y;
 uint32_t w;
 uint32_t h;
};

struct mdp_img {
 uint32_t width;
 uint32_t height;
 uint32_t format;
 uint32_t offset;
 int memory_id;
 uint32_t priv;
};
// # 281 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
struct mdp_ccs {
 int direction;
 uint16_t ccs[9];
 uint16_t bv[3];
};

struct mdp_csc {
 int id;
 uint32_t csc_mv[9];
 uint32_t csc_pre_bv[3];
 uint32_t csc_post_bv[3];
 uint32_t csc_pre_lv[6];
 uint32_t csc_post_lv[6];
};
// # 303 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
struct color {
 uint32_t r;
 uint32_t g;
 uint32_t b;
 uint32_t alpha;
};

struct mdp_blit_req {
 struct mdp_img src;
 struct mdp_img dst;
 struct mdp_rect src_rect;
 struct mdp_rect dst_rect;
 struct color const_color;
 uint32_t alpha;
 uint32_t transp_mask;
 uint32_t flags;
 int sharpening_strength;
 uint8_t color_space;
 uint32_t fps;
};

struct mdp_blit_req_list {
 uint32_t count;
 struct mdp_blit_req req[];
};



struct msmfb_data {
 uint32_t offset;
 int memory_id;
 int id;
 uint32_t flags;
 uint32_t priv;
 uint32_t iova;
};



struct msmfb_overlay_data {
 uint32_t id;
 struct msmfb_data data;
 uint32_t version_key;
 struct msmfb_data plane1_data;
 struct msmfb_data plane2_data;
 struct msmfb_data dst_data;
};

struct msmfb_img {
 uint32_t width;
 uint32_t height;
 uint32_t format;
};


struct msmfb_writeback_data {
 struct msmfb_data buf_info;
 struct msmfb_img img;
};
// # 408 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
struct mdp_qseed_cfg {
 uint32_t table_num;
 uint32_t ops;
 uint32_t len;
 uint32_t *data;
};

struct mdp_sharp_cfg {
 uint32_t flags;
 uint32_t strength;
 uint32_t edge_thr;
 uint32_t smooth_thr;
 uint32_t noise_thr;
};

struct mdp_qseed_cfg_data {
 uint32_t block;
 struct mdp_qseed_cfg qseed_data;
};
// # 441 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
struct mdp_csc_cfg {

 uint32_t flags;
 uint32_t csc_mv[9];
 uint32_t csc_pre_bv[3];
 uint32_t csc_post_bv[3];
 uint32_t csc_pre_lv[6];
 uint32_t csc_post_lv[6];
};

struct mdp_csc_cfg_data {
 uint32_t block;
 struct mdp_csc_cfg csc_data;
};

struct mdp_pa_cfg {
 uint32_t flags;
 uint32_t hue_adj;
 uint32_t sat_adj;
 uint32_t val_adj;
 uint32_t cont_adj;
};

struct mdp_pa_mem_col_cfg {
 uint32_t color_adjust_p0;
 uint32_t color_adjust_p1;
 uint32_t hue_region;
 uint32_t sat_region;
 uint32_t val_region;
};



struct mdp_pa_v2_data {

 uint32_t flags;
 uint32_t global_hue_adj;
 uint32_t global_sat_adj;
 uint32_t global_val_adj;
 uint32_t global_cont_adj;
 struct mdp_pa_mem_col_cfg skin_cfg;
 struct mdp_pa_mem_col_cfg sky_cfg;
 struct mdp_pa_mem_col_cfg fol_cfg;
 uint32_t six_zone_len;
 uint32_t six_zone_thresh;
 uint32_t *six_zone_curve_p0;
 uint32_t *six_zone_curve_p1;
};

struct mdp_igc_lut_data {
 uint32_t block;
 uint32_t len, ops;
 uint32_t *c0_c1_data;
 uint32_t *c2_data;
};

struct mdp_histogram_cfg {
 uint32_t ops;
 uint32_t block;
 uint8_t frame_cnt;
 uint8_t bit_mask;
 uint16_t num_bins;
};

struct mdp_hist_lut_data {
 uint32_t block;
 uint32_t ops;
 uint32_t len;
 uint32_t *data;
};

struct mdp_overlay_pp_params {
 uint32_t config_ops;
 struct mdp_csc_cfg csc_cfg;
 struct mdp_qseed_cfg qseed_cfg[2];
 struct mdp_pa_cfg pa_cfg;
 struct mdp_pa_v2_data pa_v2_cfg;
 struct mdp_igc_lut_data igc_cfg;
 struct mdp_sharp_cfg sharp_cfg;
 struct mdp_histogram_cfg hist_cfg;
 struct mdp_hist_lut_data hist_lut_cfg;
};
// # 543 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
enum mdss_mdp_blend_op {
 BLEND_OP_NOT_DEFINED = 0,
 BLEND_OP_OPAQUE,
 BLEND_OP_PREMULTIPLIED,
 BLEND_OP_COVERAGE,
 BLEND_OP_MAX,
};



struct mdp_scale_data {
 uint8_t enable_pxl_ext;

 int init_phase_x[4];
 int phase_step_x[4];
 int init_phase_y[4];
 int phase_step_y[4];

 int num_ext_pxls_left[4];
 int num_ext_pxls_right[4];
 int num_ext_pxls_top[4];
 int num_ext_pxls_btm[4];

 int left_ftch[4];
 int left_rpt[4];
 int right_ftch[4];
 int right_rpt[4];

 int top_rpt[4];
 int btm_rpt[4];
 int top_ftch[4];
 int btm_ftch[4];

 uint32_t roi_w[4];
};
// # 589 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
enum mdp_overlay_pipe_type {
 PIPE_TYPE_AUTO = 0,
 PIPE_TYPE_VIG,
 PIPE_TYPE_RGB,
 PIPE_TYPE_DMA,
 PIPE_TYPE_CURSOR,
 PIPE_TYPE_MAX,
};
// # 648 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
struct mdp_overlay {
 struct msmfb_img src;
 struct mdp_rect src_rect;
 struct mdp_rect dst_rect;
 uint32_t z_order;
 uint32_t is_fg;
 uint32_t alpha;
 uint32_t blend_op;
 uint32_t transp_mask;
 uint32_t flags;
 uint32_t pipe_type;
 uint32_t id;
 uint8_t priority;
 uint32_t user_data[6];
 uint32_t bg_color;
 uint8_t horz_deci;
 uint8_t vert_deci;
 struct mdp_overlay_pp_params overlay_pp_cfg;
 struct mdp_scale_data scale;
 uint8_t color_space;
};

struct msmfb_overlay_3d {
 uint32_t is_3d;
 uint32_t width;
 uint32_t height;
};


struct msmfb_overlay_blt {
 uint32_t enable;
 uint32_t offset;
 uint32_t width;
 uint32_t height;
 uint32_t bpp;
};

struct mdp_histogram {
 uint32_t frame_cnt;
 uint32_t bin_cnt;
 uint32_t *r;
 uint32_t *g;
 uint32_t *b;
};


enum {
 DISPLAY_MISR_EDP,
 DISPLAY_MISR_DSI0,
 DISPLAY_MISR_DSI1,
 DISPLAY_MISR_HDMI,
 DISPLAY_MISR_LCDC,
 DISPLAY_MISR_MDP,
 DISPLAY_MISR_ATV,
 DISPLAY_MISR_DSI_CMD,
 DISPLAY_MISR_MAX
};

enum {
 MISR_OP_NONE,
 MISR_OP_SFM,
 MISR_OP_MFM,
 MISR_OP_BM,
 MISR_OP_MAX
};

struct mdp_misr {
 uint32_t block_id;
 uint32_t frame_count;
 uint32_t crc_op_mode;
 uint32_t crc_value[32];
};
// # 734 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
enum {
 MDP_BLOCK_RESERVED = 0,
 MDP_BLOCK_OVERLAY_0,
 MDP_BLOCK_OVERLAY_1,
 MDP_BLOCK_VG_1,
 MDP_BLOCK_VG_2,
 MDP_BLOCK_RGB_1,
 MDP_BLOCK_RGB_2,
 MDP_BLOCK_DMA_P,
 MDP_BLOCK_DMA_S,
 MDP_BLOCK_DMA_E,
 MDP_BLOCK_OVERLAY_2,
 MDP_LOGICAL_BLOCK_DISP_0 = 0x10,
 MDP_LOGICAL_BLOCK_DISP_1,
 MDP_LOGICAL_BLOCK_DISP_2,
 MDP_BLOCK_MAX,
};






struct mdp_histogram_start_req {
 uint32_t block;
 uint8_t frame_cnt;
 uint8_t bit_mask;
 uint16_t num_bins;
};






struct mdp_histogram_data {
 uint32_t block;
 uint32_t bin_cnt;
 uint32_t *c0;
 uint32_t *c1;
 uint32_t *c2;
 uint32_t *extra_info;
};

struct mdp_pcc_coeff {
 uint32_t c, r, g, b, rr, gg, bb, rg, gb, rb, rgb_0, rgb_1;
};

struct mdp_pcc_cfg_data {
 uint32_t block;
 uint32_t ops;
 struct mdp_pcc_coeff r, g, b;
};



enum {
 mdp_lut_igc,
 mdp_lut_pgc,
 mdp_lut_hist,
 mdp_lut_rgb,
 mdp_lut_max,
};

struct mdp_ar_gc_lut_data {
 uint32_t x_start;
 uint32_t slope;
 uint32_t offset;
};

struct mdp_pgc_lut_data {
 uint32_t block;
 uint32_t flags;
 uint8_t num_r_stages;
 uint8_t num_g_stages;
 uint8_t num_b_stages;
 struct mdp_ar_gc_lut_data *r_data;
 struct mdp_ar_gc_lut_data *g_data;
 struct mdp_ar_gc_lut_data *b_data;
};





struct mdp_rgb_lut_data {
 uint32_t flags;
 uint32_t lut_type;
 struct fb_cmap cmap;
};

enum {
 mdp_rgb_lut_gc,
 mdp_rgb_lut_hist,
};

struct mdp_lut_cfg_data {
 uint32_t lut_type;
 union {
  struct mdp_igc_lut_data igc_lut_data;
  struct mdp_pgc_lut_data pgc_lut_data;
  struct mdp_hist_lut_data hist_lut_data;
  struct mdp_rgb_lut_data rgb_lut_data;
 } data;
};

struct mdp_bl_scale_data {
 uint32_t min_lvl;
 uint32_t scale;
};

struct mdp_pa_cfg_data {
 uint32_t block;
 struct mdp_pa_cfg pa_data;
};

struct mdp_pa_v2_cfg_data {
 uint32_t block;
 struct mdp_pa_v2_data pa_v2_data;
};

struct mdp_dither_cfg_data {
 uint32_t block;
 uint32_t flags;
 uint32_t g_y_depth;
 uint32_t r_cr_depth;
 uint32_t b_cb_depth;
};

struct mdp_gamut_cfg_data {
 uint32_t block;
 uint32_t flags;
 uint32_t gamut_first;
 uint32_t tbl_size[8];
 uint16_t *r_tbl[8];
 uint16_t *g_tbl[8];
 uint16_t *b_tbl[8];
};

struct mdp_calib_config_data {
 uint32_t ops;
 uint32_t addr;
 uint32_t data;
};

struct mdp_calib_config_buffer {
 uint32_t ops;
 uint32_t size;
 uint32_t *buffer;
};

struct mdp_calib_dcm_state {
 uint32_t ops;
 uint32_t dcm_state;
};

struct mdp_pp_init_data {
 uint32_t init_request;
};

enum {
 MDP_PP_DISABLE,
 MDP_PP_ENABLE,
};

enum {
 DCM_UNINIT,
 DCM_UNBLANK,
 DCM_ENTER,
 DCM_EXIT,
 DCM_BLANK,
 DTM_ENTER,
 DTM_EXIT,
};
// # 926 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
struct mdss_ad_init {
 uint32_t asym_lut[33];
 uint32_t color_corr_lut[33];
 uint8_t i_control[2];
 uint16_t black_lvl;
 uint16_t white_lvl;
 uint8_t var;
 uint8_t limit_ampl;
 uint8_t i_dither;
 uint8_t slope_max;
 uint8_t slope_min;
 uint8_t dither_ctl;
 uint8_t format;
 uint8_t auto_size;
 uint16_t frame_w;
 uint16_t frame_h;
 uint8_t logo_v;
 uint8_t logo_h;
 uint32_t alpha;
 uint32_t alpha_base;
 uint32_t bl_lin_len;
 uint32_t bl_att_len;
 uint32_t *bl_lin;
 uint32_t *bl_lin_inv;
 uint32_t *bl_att_lut;
};



struct mdss_ad_cfg {
 uint32_t mode;
 uint32_t al_calib_lut[33];
 uint16_t backlight_min;
 uint16_t backlight_max;
 uint16_t backlight_scale;
 uint16_t amb_light_min;
 uint16_t filter[2];
 uint16_t calib[4];
 uint8_t strength_limit;
 uint8_t t_filter_recursion;
 uint16_t stab_itr;
 uint32_t bl_ctrl_mode;
};


struct mdss_ad_init_cfg {
 uint32_t ops;
 union {
  struct mdss_ad_init init;
  struct mdss_ad_cfg cfg;
 } params;
};


struct mdss_ad_input {
 uint32_t mode;
 union {
  uint32_t amb_light;
  uint32_t strength;
  uint32_t calib_bl;
 } in;
 uint32_t output;
};


struct mdss_calib_cfg {
 uint32_t ops;
 uint32_t calib_mask;
};

enum {
 mdp_op_pcc_cfg,
 mdp_op_csc_cfg,
 mdp_op_lut_cfg,
 mdp_op_qseed_cfg,
 mdp_bl_scale_cfg,
 mdp_op_pa_cfg,
 mdp_op_pa_v2_cfg,
 mdp_op_dither_cfg,
 mdp_op_gamut_cfg,
 mdp_op_calib_cfg,
 mdp_op_ad_cfg,
 mdp_op_ad_input,
 mdp_op_calib_mode,
 mdp_op_calib_buffer,
 mdp_op_calib_dcm_state,
 mdp_op_max,
 mdp_op_pp_init_cfg,
};

enum {
 WB_FORMAT_NV12,
 WB_FORMAT_RGB_565,
 WB_FORMAT_RGB_888,
 WB_FORMAT_xRGB_8888,
 WB_FORMAT_ARGB_8888,
 WB_FORMAT_BGRA_8888,
 WB_FORMAT_BGRX_8888,
 WB_FORMAT_ARGB_8888_INPUT_ALPHA
};

struct msmfb_mdp_pp {
 uint32_t op;
 union {
  struct mdp_pcc_cfg_data pcc_cfg_data;
  struct mdp_csc_cfg_data csc_cfg_data;
  struct mdp_lut_cfg_data lut_cfg_data;
  struct mdp_qseed_cfg_data qseed_cfg_data;
  struct mdp_bl_scale_data bl_scale_data;
  struct mdp_pa_cfg_data pa_cfg_data;
  struct mdp_pa_v2_cfg_data pa_v2_cfg_data;
  struct mdp_dither_cfg_data dither_cfg_data;
  struct mdp_gamut_cfg_data gamut_cfg_data;
  struct mdp_calib_config_data calib_cfg;
  struct mdss_ad_init_cfg ad_init_cfg;
  struct mdss_calib_cfg mdss_calib_cfg;
  struct mdss_ad_input ad_input;
  struct mdp_calib_config_buffer calib_buffer;
  struct mdp_calib_dcm_state calib_dcm;
  struct mdp_pp_init_data init_data;
 } data;
};


enum {
 metadata_op_none,
 metadata_op_base_blend,
 metadata_op_frame_rate,
 metadata_op_vic,
 metadata_op_wb_format,
 metadata_op_wb_secure,
 metadata_op_get_caps,
 metadata_op_crc,
 metadata_op_get_ion_fd,
 metadata_op_max
};

struct mdp_blend_cfg {
 uint32_t is_premultiplied;
};

struct mdp_mixer_cfg {
 uint32_t writeback_format;
 uint32_t alpha;
};

struct mdss_hw_caps {
 uint32_t mdp_rev;
 uint8_t rgb_pipes;
 uint8_t vig_pipes;
 uint8_t dma_pipes;
 uint8_t max_smp_cnt;
 uint8_t smp_per_pipe;
 uint32_t features;
};

struct msmfb_metadata {
 uint32_t op;
 uint32_t flags;
 union {
  struct mdp_misr misr_request;
  struct mdp_blend_cfg blend_cfg;
  struct mdp_mixer_cfg mixer_cfg;
  uint32_t panel_frame_rate;
  uint32_t video_info_code;
  struct mdss_hw_caps caps;
  uint8_t secure_en;
  int fbmem_ionfd;
 } data;
};





struct mdp_buf_sync {
 uint32_t flags;
 uint32_t acq_fen_fd_cnt;
 uint32_t session_id;
 int *acq_fen_fd;
 int *rel_fen_fd;
 int *retire_fen_fd;
};

struct mdp_async_blit_req_list {
 struct mdp_buf_sync sync;
 uint32_t count;
 struct mdp_blit_req req[];
};



struct mdp_display_commit {
 uint32_t flags;
 uint32_t wait_for_finish;
 struct fb_var_screeninfo var;
 struct mdp_rect l_roi;
 struct mdp_rect r_roi;
};
// # 1137 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/msm_mdp.h"
struct mdp_overlay_list {
 uint32_t num_overlays;
 struct mdp_overlay **overlay_list;
 uint32_t flags;
 uint32_t processed_overlays;
};

struct mdp_page_protection {
 uint32_t page_protection;
};


struct mdp_mixer_info {
 int pndx;
 int pnum;
 int ptype;
 int mixer_num;
 int z_order;
};



struct msmfb_mixer_info_req {
 int mixer_num;
 int cnt;
 struct mdp_mixer_info info[7];
};

enum {
 DISPLAY_SUBSYSTEM_ID,
 ROTATOR_SUBSYSTEM_ID,
};

enum {
 MDP_IOMMU_DOMAIN_CP,
 MDP_IOMMU_DOMAIN_NS,
};

enum {
 MDP_WRITEBACK_MIRROR_OFF,
 MDP_WRITEBACK_MIRROR_ON,
 MDP_WRITEBACK_MIRROR_PAUSE,
 MDP_WRITEBACK_MIRROR_RESUME,
};

enum {
 MDP_CSC_ITU_R_601,
 MDP_CSC_ITU_R_601_FR,
 MDP_CSC_ITU_R_709,
};
// # 42 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 2
// # 1 "bionic/libc/include/semaphore.h" 1 3 4
// # 34 "bionic/libc/include/semaphore.h" 3 4


struct timespec;

typedef struct {
  unsigned int count;



} sem_t;



int sem_destroy(sem_t*);
int sem_getvalue(sem_t*, int*);
int sem_init(sem_t*, int, unsigned int);
int sem_post(sem_t*);
int sem_timedwait(sem_t*, const struct timespec*);
int sem_trywait(sem_t*);
int sem_wait(sem_t*);


sem_t* sem_open(const char*, int, ...);
int sem_close(sem_t*);
int sem_unlink(const char*);


// # 43 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 2

// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h" 1
// # 33 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h" 1
// # 63 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-common.h" 1
// # 64 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h" 2
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h" 1
// # 93 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_power_line_frequency {
 V4L2_CID_POWER_LINE_FREQUENCY_DISABLED = 0,
 V4L2_CID_POWER_LINE_FREQUENCY_50HZ = 1,
 V4L2_CID_POWER_LINE_FREQUENCY_60HZ = 2,
 V4L2_CID_POWER_LINE_FREQUENCY_AUTO = 3,
};







enum v4l2_colorfx {
 V4L2_COLORFX_NONE = 0,
 V4L2_COLORFX_BW = 1,
 V4L2_COLORFX_SEPIA = 2,
 V4L2_COLORFX_NEGATIVE = 3,
 V4L2_COLORFX_EMBOSS = 4,
 V4L2_COLORFX_SKETCH = 5,
 V4L2_COLORFX_SKY_BLUE = 6,
 V4L2_COLORFX_GRASS_GREEN = 7,
 V4L2_COLORFX_SKIN_WHITEN = 8,
 V4L2_COLORFX_VIVID = 9,
 V4L2_COLORFX_AQUA = 10,
 V4L2_COLORFX_ART_FREEZE = 11,
 V4L2_COLORFX_SILHOUETTE = 12,
 V4L2_COLORFX_SOLARIZATION = 13,
 V4L2_COLORFX_ANTIQUE = 14,
 V4L2_COLORFX_SET_CBCR = 15,
};
// # 172 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_mpeg_stream_type {
 V4L2_MPEG_STREAM_TYPE_MPEG2_PS = 0,
 V4L2_MPEG_STREAM_TYPE_MPEG2_TS = 1,
 V4L2_MPEG_STREAM_TYPE_MPEG1_SS = 2,
 V4L2_MPEG_STREAM_TYPE_MPEG2_DVD = 3,
 V4L2_MPEG_STREAM_TYPE_MPEG1_VCD = 4,
 V4L2_MPEG_STREAM_TYPE_MPEG2_SVCD = 5,
};







enum v4l2_mpeg_stream_vbi_fmt {
 V4L2_MPEG_STREAM_VBI_FMT_NONE = 0,
 V4L2_MPEG_STREAM_VBI_FMT_IVTV = 1,
};



enum v4l2_mpeg_audio_sampling_freq {
 V4L2_MPEG_AUDIO_SAMPLING_FREQ_44100 = 0,
 V4L2_MPEG_AUDIO_SAMPLING_FREQ_48000 = 1,
 V4L2_MPEG_AUDIO_SAMPLING_FREQ_32000 = 2,
};

enum v4l2_mpeg_audio_encoding {
 V4L2_MPEG_AUDIO_ENCODING_LAYER_1 = 0,
 V4L2_MPEG_AUDIO_ENCODING_LAYER_2 = 1,
 V4L2_MPEG_AUDIO_ENCODING_LAYER_3 = 2,
 V4L2_MPEG_AUDIO_ENCODING_AAC = 3,
 V4L2_MPEG_AUDIO_ENCODING_AC3 = 4,
};

enum v4l2_mpeg_audio_l1_bitrate {
 V4L2_MPEG_AUDIO_L1_BITRATE_32K = 0,
 V4L2_MPEG_AUDIO_L1_BITRATE_64K = 1,
 V4L2_MPEG_AUDIO_L1_BITRATE_96K = 2,
 V4L2_MPEG_AUDIO_L1_BITRATE_128K = 3,
 V4L2_MPEG_AUDIO_L1_BITRATE_160K = 4,
 V4L2_MPEG_AUDIO_L1_BITRATE_192K = 5,
 V4L2_MPEG_AUDIO_L1_BITRATE_224K = 6,
 V4L2_MPEG_AUDIO_L1_BITRATE_256K = 7,
 V4L2_MPEG_AUDIO_L1_BITRATE_288K = 8,
 V4L2_MPEG_AUDIO_L1_BITRATE_320K = 9,
 V4L2_MPEG_AUDIO_L1_BITRATE_352K = 10,
 V4L2_MPEG_AUDIO_L1_BITRATE_384K = 11,
 V4L2_MPEG_AUDIO_L1_BITRATE_416K = 12,
 V4L2_MPEG_AUDIO_L1_BITRATE_448K = 13,
};

enum v4l2_mpeg_audio_l2_bitrate {
 V4L2_MPEG_AUDIO_L2_BITRATE_32K = 0,
 V4L2_MPEG_AUDIO_L2_BITRATE_48K = 1,
 V4L2_MPEG_AUDIO_L2_BITRATE_56K = 2,
 V4L2_MPEG_AUDIO_L2_BITRATE_64K = 3,
 V4L2_MPEG_AUDIO_L2_BITRATE_80K = 4,
 V4L2_MPEG_AUDIO_L2_BITRATE_96K = 5,
 V4L2_MPEG_AUDIO_L2_BITRATE_112K = 6,
 V4L2_MPEG_AUDIO_L2_BITRATE_128K = 7,
 V4L2_MPEG_AUDIO_L2_BITRATE_160K = 8,
 V4L2_MPEG_AUDIO_L2_BITRATE_192K = 9,
 V4L2_MPEG_AUDIO_L2_BITRATE_224K = 10,
 V4L2_MPEG_AUDIO_L2_BITRATE_256K = 11,
 V4L2_MPEG_AUDIO_L2_BITRATE_320K = 12,
 V4L2_MPEG_AUDIO_L2_BITRATE_384K = 13,
};

enum v4l2_mpeg_audio_l3_bitrate {
 V4L2_MPEG_AUDIO_L3_BITRATE_32K = 0,
 V4L2_MPEG_AUDIO_L3_BITRATE_40K = 1,
 V4L2_MPEG_AUDIO_L3_BITRATE_48K = 2,
 V4L2_MPEG_AUDIO_L3_BITRATE_56K = 3,
 V4L2_MPEG_AUDIO_L3_BITRATE_64K = 4,
 V4L2_MPEG_AUDIO_L3_BITRATE_80K = 5,
 V4L2_MPEG_AUDIO_L3_BITRATE_96K = 6,
 V4L2_MPEG_AUDIO_L3_BITRATE_112K = 7,
 V4L2_MPEG_AUDIO_L3_BITRATE_128K = 8,
 V4L2_MPEG_AUDIO_L3_BITRATE_160K = 9,
 V4L2_MPEG_AUDIO_L3_BITRATE_192K = 10,
 V4L2_MPEG_AUDIO_L3_BITRATE_224K = 11,
 V4L2_MPEG_AUDIO_L3_BITRATE_256K = 12,
 V4L2_MPEG_AUDIO_L3_BITRATE_320K = 13,
};

enum v4l2_mpeg_audio_mode {
 V4L2_MPEG_AUDIO_MODE_STEREO = 0,
 V4L2_MPEG_AUDIO_MODE_JOINT_STEREO = 1,
 V4L2_MPEG_AUDIO_MODE_DUAL = 2,
 V4L2_MPEG_AUDIO_MODE_MONO = 3,
};

enum v4l2_mpeg_audio_mode_extension {
 V4L2_MPEG_AUDIO_MODE_EXTENSION_BOUND_4 = 0,
 V4L2_MPEG_AUDIO_MODE_EXTENSION_BOUND_8 = 1,
 V4L2_MPEG_AUDIO_MODE_EXTENSION_BOUND_12 = 2,
 V4L2_MPEG_AUDIO_MODE_EXTENSION_BOUND_16 = 3,
};

enum v4l2_mpeg_audio_emphasis {
 V4L2_MPEG_AUDIO_EMPHASIS_NONE = 0,
 V4L2_MPEG_AUDIO_EMPHASIS_50_DIV_15_uS = 1,
 V4L2_MPEG_AUDIO_EMPHASIS_CCITT_J17 = 2,
};

enum v4l2_mpeg_audio_crc {
 V4L2_MPEG_AUDIO_CRC_NONE = 0,
 V4L2_MPEG_AUDIO_CRC_CRC16 = 1,
};



enum v4l2_mpeg_audio_ac3_bitrate {
 V4L2_MPEG_AUDIO_AC3_BITRATE_32K = 0,
 V4L2_MPEG_AUDIO_AC3_BITRATE_40K = 1,
 V4L2_MPEG_AUDIO_AC3_BITRATE_48K = 2,
 V4L2_MPEG_AUDIO_AC3_BITRATE_56K = 3,
 V4L2_MPEG_AUDIO_AC3_BITRATE_64K = 4,
 V4L2_MPEG_AUDIO_AC3_BITRATE_80K = 5,
 V4L2_MPEG_AUDIO_AC3_BITRATE_96K = 6,
 V4L2_MPEG_AUDIO_AC3_BITRATE_112K = 7,
 V4L2_MPEG_AUDIO_AC3_BITRATE_128K = 8,
 V4L2_MPEG_AUDIO_AC3_BITRATE_160K = 9,
 V4L2_MPEG_AUDIO_AC3_BITRATE_192K = 10,
 V4L2_MPEG_AUDIO_AC3_BITRATE_224K = 11,
 V4L2_MPEG_AUDIO_AC3_BITRATE_256K = 12,
 V4L2_MPEG_AUDIO_AC3_BITRATE_320K = 13,
 V4L2_MPEG_AUDIO_AC3_BITRATE_384K = 14,
 V4L2_MPEG_AUDIO_AC3_BITRATE_448K = 15,
 V4L2_MPEG_AUDIO_AC3_BITRATE_512K = 16,
 V4L2_MPEG_AUDIO_AC3_BITRATE_576K = 17,
 V4L2_MPEG_AUDIO_AC3_BITRATE_640K = 18,
};

enum v4l2_mpeg_audio_dec_playback {
 V4L2_MPEG_AUDIO_DEC_PLAYBACK_AUTO = 0,
 V4L2_MPEG_AUDIO_DEC_PLAYBACK_STEREO = 1,
 V4L2_MPEG_AUDIO_DEC_PLAYBACK_LEFT = 2,
 V4L2_MPEG_AUDIO_DEC_PLAYBACK_RIGHT = 3,
 V4L2_MPEG_AUDIO_DEC_PLAYBACK_MONO = 4,
 V4L2_MPEG_AUDIO_DEC_PLAYBACK_SWAPPED_STEREO = 5,
};




enum v4l2_mpeg_video_encoding {
 V4L2_MPEG_VIDEO_ENCODING_MPEG_1 = 0,
 V4L2_MPEG_VIDEO_ENCODING_MPEG_2 = 1,
 V4L2_MPEG_VIDEO_ENCODING_MPEG_4_AVC = 2,
};

enum v4l2_mpeg_video_aspect {
 V4L2_MPEG_VIDEO_ASPECT_1x1 = 0,
 V4L2_MPEG_VIDEO_ASPECT_4x3 = 1,
 V4L2_MPEG_VIDEO_ASPECT_16x9 = 2,
 V4L2_MPEG_VIDEO_ASPECT_221x100 = 3,
};





enum v4l2_mpeg_video_bitrate_mode {
 V4L2_MPEG_VIDEO_BITRATE_MODE_VBR = 0,
 V4L2_MPEG_VIDEO_BITRATE_MODE_CBR = 1,
};
// # 351 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_mpeg_video_header_mode {
 V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE = 0,
 V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME = 1,
 V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_I_FRAME = 2,

};





enum v4l2_mpeg_video_multi_slice_mode {
 V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_SINGLE = 0,
 V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_MB = 1,
 V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_BYTES = 2,
 V4L2_MPEG_VIDEO_MULTI_SLICE_GOB = 3,
};
// # 387 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_mpeg_video_h264_entropy_mode {
 V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CAVLC = 0,
 V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CABAC = 1,
};


enum v4l2_mpeg_video_h264_level {
 V4L2_MPEG_VIDEO_H264_LEVEL_1_0 = 0,
 V4L2_MPEG_VIDEO_H264_LEVEL_1B = 1,
 V4L2_MPEG_VIDEO_H264_LEVEL_1_1 = 2,
 V4L2_MPEG_VIDEO_H264_LEVEL_1_2 = 3,
 V4L2_MPEG_VIDEO_H264_LEVEL_1_3 = 4,
 V4L2_MPEG_VIDEO_H264_LEVEL_2_0 = 5,
 V4L2_MPEG_VIDEO_H264_LEVEL_2_1 = 6,
 V4L2_MPEG_VIDEO_H264_LEVEL_2_2 = 7,
 V4L2_MPEG_VIDEO_H264_LEVEL_3_0 = 8,
 V4L2_MPEG_VIDEO_H264_LEVEL_3_1 = 9,
 V4L2_MPEG_VIDEO_H264_LEVEL_3_2 = 10,
 V4L2_MPEG_VIDEO_H264_LEVEL_4_0 = 11,
 V4L2_MPEG_VIDEO_H264_LEVEL_4_1 = 12,
 V4L2_MPEG_VIDEO_H264_LEVEL_4_2 = 13,
 V4L2_MPEG_VIDEO_H264_LEVEL_5_0 = 14,
 V4L2_MPEG_VIDEO_H264_LEVEL_5_1 = 15,
 V4L2_MPEG_VIDEO_H264_LEVEL_5_2 = 16,
};



enum v4l2_mpeg_video_h264_loop_filter_mode {
 V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED = 0,
 V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_ENABLED = 1,
 V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED_AT_SLICE_BOUNDARY
         = 2,
};

enum v4l2_mpeg_video_h264_profile {
 V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE = 0,
 V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE = 1,
 V4L2_MPEG_VIDEO_H264_PROFILE_MAIN = 2,
 V4L2_MPEG_VIDEO_H264_PROFILE_EXTENDED = 3,
 V4L2_MPEG_VIDEO_H264_PROFILE_HIGH = 4,
 V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10 = 5,
 V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422 = 6,
 V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_PREDICTIVE = 7,
 V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10_INTRA = 8,
 V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422_INTRA = 9,
 V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_INTRA = 10,
 V4L2_MPEG_VIDEO_H264_PROFILE_CAVLC_444_INTRA = 11,
 V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_BASELINE = 12,
 V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_HIGH = 13,
 V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_HIGH_INTRA = 14,
 V4L2_MPEG_VIDEO_H264_PROFILE_STEREO_HIGH = 15,
 V4L2_MPEG_VIDEO_H264_PROFILE_MULTIVIEW_HIGH = 16,
 V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_HIGH = 17,
};




enum v4l2_mpeg_video_h264_vui_sar_idc {
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_UNSPECIFIED = 0,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_1x1 = 1,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_12x11 = 2,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_10x11 = 3,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_16x11 = 4,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_40x33 = 5,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_24x11 = 6,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_20x11 = 7,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_32x11 = 8,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_80x33 = 9,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_18x11 = 10,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_15x11 = 11,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_64x33 = 12,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_160x99 = 13,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_4x3 = 14,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_3x2 = 15,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_2x1 = 16,
 V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_EXTENDED = 17,
};



enum v4l2_mpeg_video_h264_sei_fp_arrangement_type {
 V4L2_MPEG_VIDEO_H264_SEI_FP_ARRANGEMENT_TYPE_CHECKERBOARD = 0,
 V4L2_MPEG_VIDEO_H264_SEI_FP_ARRANGEMENT_TYPE_COLUMN = 1,
 V4L2_MPEG_VIDEO_H264_SEI_FP_ARRANGEMENT_TYPE_ROW = 2,
 V4L2_MPEG_VIDEO_H264_SEI_FP_ARRANGEMENT_TYPE_SIDE_BY_SIDE = 3,
 V4L2_MPEG_VIDEO_H264_SEI_FP_ARRANGEMENT_TYPE_TOP_BOTTOM = 4,
 V4L2_MPEG_VIDEO_H264_SEI_FP_ARRANGEMENT_TYPE_TEMPORAL = 5,
};


enum v4l2_mpeg_video_h264_fmo_map_type {
 V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_INTERLEAVED_SLICES = 0,
 V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_SCATTERED_SLICES = 1,
 V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_FOREGROUND_WITH_LEFT_OVER = 2,
 V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_BOX_OUT = 3,
 V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_RASTER_SCAN = 4,
 V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_WIPE_SCAN = 5,
 V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_EXPLICIT = 6,
};


enum v4l2_mpeg_video_h264_fmo_change_dir {
 V4L2_MPEG_VIDEO_H264_FMO_CHANGE_DIR_RIGHT = 0,
 V4L2_MPEG_VIDEO_H264_FMO_CHANGE_DIR_LEFT = 1,
};






enum v4l2_mpeg_video_h264_hierarchical_coding_type {
 V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_B = 0,
 V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_P = 1,
};
// # 512 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_mpeg_video_mpeg4_level {
 V4L2_MPEG_VIDEO_MPEG4_LEVEL_0 = 0,
 V4L2_MPEG_VIDEO_MPEG4_LEVEL_0B = 1,
 V4L2_MPEG_VIDEO_MPEG4_LEVEL_1 = 2,
 V4L2_MPEG_VIDEO_MPEG4_LEVEL_2 = 3,
 V4L2_MPEG_VIDEO_MPEG4_LEVEL_3 = 4,
 V4L2_MPEG_VIDEO_MPEG4_LEVEL_3B = 5,
 V4L2_MPEG_VIDEO_MPEG4_LEVEL_4 = 6,
 V4L2_MPEG_VIDEO_MPEG4_LEVEL_5 = 7,
};

enum v4l2_mpeg_video_mpeg4_profile {
 V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE = 0,
 V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE = 1,
 V4L2_MPEG_VIDEO_MPEG4_PROFILE_CORE = 2,
 V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE_SCALABLE = 3,
 V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY = 4,
};






enum v4l2_mpeg_cx2341x_video_spatial_filter_mode {
 V4L2_MPEG_CX2341X_VIDEO_SPATIAL_FILTER_MODE_MANUAL = 0,
 V4L2_MPEG_CX2341X_VIDEO_SPATIAL_FILTER_MODE_AUTO = 1,
};


enum v4l2_mpeg_cx2341x_video_luma_spatial_filter_type {
 V4L2_MPEG_CX2341X_VIDEO_LUMA_SPATIAL_FILTER_TYPE_OFF = 0,
 V4L2_MPEG_CX2341X_VIDEO_LUMA_SPATIAL_FILTER_TYPE_1D_HOR = 1,
 V4L2_MPEG_CX2341X_VIDEO_LUMA_SPATIAL_FILTER_TYPE_1D_VERT = 2,
 V4L2_MPEG_CX2341X_VIDEO_LUMA_SPATIAL_FILTER_TYPE_2D_HV_SEPARABLE = 3,
 V4L2_MPEG_CX2341X_VIDEO_LUMA_SPATIAL_FILTER_TYPE_2D_SYM_NON_SEPARABLE = 4,
};

enum v4l2_mpeg_cx2341x_video_chroma_spatial_filter_type {
 V4L2_MPEG_CX2341X_VIDEO_CHROMA_SPATIAL_FILTER_TYPE_OFF = 0,
 V4L2_MPEG_CX2341X_VIDEO_CHROMA_SPATIAL_FILTER_TYPE_1D_HOR = 1,
};

enum v4l2_mpeg_cx2341x_video_temporal_filter_mode {
 V4L2_MPEG_CX2341X_VIDEO_TEMPORAL_FILTER_MODE_MANUAL = 0,
 V4L2_MPEG_CX2341X_VIDEO_TEMPORAL_FILTER_MODE_AUTO = 1,
};


enum v4l2_mpeg_cx2341x_video_median_filter_type {
 V4L2_MPEG_CX2341X_VIDEO_MEDIAN_FILTER_TYPE_OFF = 0,
 V4L2_MPEG_CX2341X_VIDEO_MEDIAN_FILTER_TYPE_HOR = 1,
 V4L2_MPEG_CX2341X_VIDEO_MEDIAN_FILTER_TYPE_VERT = 2,
 V4L2_MPEG_CX2341X_VIDEO_MEDIAN_FILTER_TYPE_HOR_VERT = 3,
 V4L2_MPEG_CX2341X_VIDEO_MEDIAN_FILTER_TYPE_DIAG = 4,
};
// # 580 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_mpeg_mfc51_video_frame_skip_mode {
 V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED = 0,
 V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT = 1,
 V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT = 2,
};

enum v4l2_mpeg_mfc51_video_force_frame_type {
 V4L2_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE_DISABLED = 0,
 V4L2_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE_I_FRAME = 1,
 V4L2_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE_NOT_CODED = 2,
};
// # 612 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_mpeg_vidc_video_divx_format_type {
 V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_4 = 0,
 V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_5 = 1,
 V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_6 = 2,
};






enum v4l2_mpeg_vidc_video_stream_format {
 V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_STARTCODES = 0,
 V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_ONE_NAL_PER_BUFFER = 1,
 V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_ONE_BYTE_LENGTH = 2,
 V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_TWO_BYTE_LENGTH = 3,
 V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_FOUR_BYTE_LENGTH = 4,
};


enum v4l2_mpeg_vidc_video_output_order {
 V4L2_MPEG_VIDC_VIDEO_OUTPUT_ORDER_DISPLAY = 0,
 V4L2_MPEG_VIDC_VIDEO_OUTPUT_ORDER_DECODE = 1,
};
// # 644 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_mpeg_vidc_video_rate_control {
 V4L2_CID_MPEG_VIDC_VIDEO_RATE_CONTROL_OFF = 0,
 V4L2_CID_MPEG_VIDC_VIDEO_RATE_CONTROL_VBR_VFR = 1,
 V4L2_CID_MPEG_VIDC_VIDEO_RATE_CONTROL_VBR_CFR = 2,
 V4L2_CID_MPEG_VIDC_VIDEO_RATE_CONTROL_CBR_VFR = 3,
 V4L2_CID_MPEG_VIDC_VIDEO_RATE_CONTROL_CBR_CFR = 4,
};


enum v4l2_mpeg_vidc_video_rotation {
 V4L2_CID_MPEG_VIDC_VIDEO_ROTATION_NONE = 0,
 V4L2_CID_MPEG_VIDC_VIDEO_ROTATION_90 = 1,
 V4L2_CID_MPEG_VIDC_VIDEO_ROTATION_180 = 2,
 V4L2_CID_MPEG_VIDC_VIDEO_ROTATION_270 = 3,
};


enum v4l2_mpeg_vidc_h264_cabac_model {
 V4L2_CID_MPEG_VIDC_VIDEO_H264_CABAC_MODEL_0 = 0,
 V4L2_CID_MPEG_VIDC_VIDEO_H264_CABAC_MODEL_1 = 1,
 V4L2_CID_MPEG_VIDC_VIDEO_H264_CABAC_MODEL_2 = 2,
};


enum v4l2_mpeg_vidc_video_intra_refresh_mode {
 V4L2_CID_MPEG_VIDC_VIDEO_INTRA_REFRESH_NONE = 0,
 V4L2_CID_MPEG_VIDC_VIDEO_INTRA_REFRESH_CYCLIC = 1,
 V4L2_CID_MPEG_VIDC_VIDEO_INTRA_REFRESH_ADAPTIVE = 2,
 V4L2_CID_MPEG_VIDC_VIDEO_INTRA_REFRESH_CYCLIC_ADAPTIVE = 3,
 V4L2_CID_MPEG_VIDC_VIDEO_INTRA_REFRESH_RANDOM = 4,
};





enum v4l2_mpeg_vidc_video_h263_profile {
 V4L2_MPEG_VIDC_VIDEO_H263_PROFILE_BASELINE = 0,
 V4L2_MPEG_VIDC_VIDEO_H263_PROFILE_H320CODING = 1,
 V4L2_MPEG_VIDC_VIDEO_H263_PROFILE_BACKWARDCOMPATIBLE = 2,
 V4L2_MPEG_VIDC_VIDEO_H263_PROFILE_ISWV2 = 3,
 V4L2_MPEG_VIDC_VIDEO_H263_PROFILE_ISWV3 = 4,
 V4L2_MPEG_VIDC_VIDEO_H263_PROFILE_HIGHCOMPRESSION = 5,
 V4L2_MPEG_VIDC_VIDEO_H263_PROFILE_INTERNET = 6,
 V4L2_MPEG_VIDC_VIDEO_H263_PROFILE_INTERLACE = 7,
 V4L2_MPEG_VIDC_VIDEO_H263_PROFILE_HIGHLATENCY = 8,
};


enum v4l2_mpeg_vidc_video_h263_level {
 V4L2_MPEG_VIDC_VIDEO_H263_LEVEL_1_0 = 0,
 V4L2_MPEG_VIDC_VIDEO_H263_LEVEL_2_0 = 1,
 V4L2_MPEG_VIDC_VIDEO_H263_LEVEL_3_0 = 2,
 V4L2_MPEG_VIDC_VIDEO_H263_LEVEL_4_0 = 3,
 V4L2_MPEG_VIDC_VIDEO_H263_LEVEL_4_5 = 4,
 V4L2_MPEG_VIDC_VIDEO_H263_LEVEL_5_0 = 5,
 V4L2_MPEG_VIDC_VIDEO_H263_LEVEL_6_0 = 6,
 V4L2_MPEG_VIDC_VIDEO_H263_LEVEL_7_0 = 7,
};



enum v4l2_mpeg_vidc_video_h264_au_delimiter {
 V4L2_MPEG_VIDC_VIDEO_H264_AU_DELIMITER_DISABLED = 0,
 V4L2_MPEG_VIDC_VIDEO_H264_AU_DELIMITER_ENABLED = 1
};


enum v4l2_mpeg_vidc_video_sync_frame_decode {
 V4L2_MPEG_VIDC_VIDEO_SYNC_FRAME_DECODE_DISABLE = 0,
 V4L2_MPEG_VIDC_VIDEO_SYNC_FRAME_DECODE_ENABLE = 1
};



enum v4l2_mpeg_vidc_extradata {
 V4L2_MPEG_VIDC_EXTRADATA_NONE = 0,
 V4L2_MPEG_VIDC_EXTRADATA_MB_QUANTIZATION = 1,
 V4L2_MPEG_VIDC_EXTRADATA_INTERLACE_VIDEO = 2,
 V4L2_MPEG_VIDC_EXTRADATA_VC1_FRAMEDISP = 3,
 V4L2_MPEG_VIDC_EXTRADATA_VC1_SEQDISP = 4,
 V4L2_MPEG_VIDC_EXTRADATA_TIMESTAMP = 5,
 V4L2_MPEG_VIDC_EXTRADATA_S3D_FRAME_PACKING = 6,
 V4L2_MPEG_VIDC_EXTRADATA_FRAME_RATE = 7,
 V4L2_MPEG_VIDC_EXTRADATA_PANSCAN_WINDOW = 8,
 V4L2_MPEG_VIDC_EXTRADATA_RECOVERY_POINT_SEI = 9,
 V4L2_MPEG_VIDC_EXTRADATA_MULTISLICE_INFO = 10,
 V4L2_MPEG_VIDC_EXTRADATA_NUM_CONCEALED_MB = 11,
 V4L2_MPEG_VIDC_EXTRADATA_METADATA_FILLER = 12,
 V4L2_MPEG_VIDC_EXTRADATA_INPUT_CROP = 13,
 V4L2_MPEG_VIDC_EXTRADATA_DIGITAL_ZOOM = 14,
 V4L2_MPEG_VIDC_EXTRADATA_ASPECT_RATIO = 15,
 V4L2_MPEG_VIDC_EXTRADATA_MPEG2_SEQDISP = 16,
 V4L2_MPEG_VIDC_EXTRADATA_STREAM_USERDATA = 17,
 V4L2_MPEG_VIDC_EXTRADATA_FRAME_QP = 18,
 V4L2_MPEG_VIDC_EXTRADATA_FRAME_BITS_INFO = 19,
 V4L2_MPEG_VIDC_EXTRADATA_LTR = 20,
 V4L2_MPEG_VIDC_EXTRADATA_METADATA_MBI = 21,


 V4L2_MPEG_VIDC_EXTRADATA_DISPLAY_COLOUR_SEI = 26,


 V4L2_MPEG_VIDC_EXTRADATA_CONTENT_LIGHT_LEVEL_SEI = 27,


 V4L2_MPEG_VIDC_EXTRADATA_VUI_DISPLAY = 28,


 V4L2_MPEG_VIDC_EXTRADATA_VPX_COLORSPACE = 29,
};


enum v4l2_mpeg_vidc_perf_level {
 V4L2_CID_MPEG_VIDC_PERF_LEVEL_NOMINAL = 0,
 V4L2_CID_MPEG_VIDC_PERF_LEVEL_PERFORMANCE = 1,
 V4L2_CID_MPEG_VIDC_PERF_LEVEL_TURBO = 2,
};
// # 770 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_mpeg_vidc_video_h264_vui_timing_info {
 V4L2_MPEG_VIDC_VIDEO_H264_VUI_TIMING_INFO_DISABLED = 0,
 V4L2_MPEG_VIDC_VIDEO_H264_VUI_TIMING_INFO_ENABLED = 1
};





enum v4l2_mpeg_vidc_video_alloc_mode_type {
 V4L2_MPEG_VIDC_VIDEO_STATIC = 0,
 V4L2_MPEG_VIDC_VIDEO_RING = 1,
 V4L2_MPEG_VIDC_VIDEO_DYNAMIC = 2,
};



enum v4l2_mpeg_vidc_video_assembly {
 V4L2_MPEG_VIDC_FRAME_ASSEMBLY_DISABLE = 0,
 V4L2_MPEG_VIDC_FRAME_ASSEMBLY_ENABLE = 1,
};



enum v4l2_mpeg_vidc_video_vp8_profile_level {
 V4L2_MPEG_VIDC_VIDEO_VP8_UNUSED,
 V4L2_MPEG_VIDC_VIDEO_VP8_VERSION_0,
 V4L2_MPEG_VIDC_VIDEO_VP8_VERSION_1,
 V4L2_MPEG_VIDC_VIDEO_VP8_VERSION_2,
 V4L2_MPEG_VIDC_VIDEO_VP8_VERSION_3,
};



enum v4l2_mpeg_vidc_video_h264_vui_bitstream_restrict {
 V4L2_MPEG_VIDC_VIDEO_H264_VUI_BITSTREAM_RESTRICT_DISABLED = 0,
 V4L2_MPEG_VIDC_VIDEO_H264_VUI_BITSTREAM_RESTRICT_ENABLED = 1
};



enum v4l2_mpeg_vidc_video_preserve_text_quality {
 V4L2_MPEG_VIDC_VIDEO_PRESERVE_TEXT_QUALITY_DISABLED = 0,
 V4L2_MPEG_VIDC_VIDEO_PRESERVE_TEXT_QUALITY_ENABLED = 1
};



enum v4l2_mpeg_vidc_video_deinterlace {
 V4L2_CID_MPEG_VIDC_VIDEO_DEINTERLACE_DISABLED = 0,
 V4L2_CID_MPEG_VIDC_VIDEO_DEINTERLACE_ENABLED = 1
};






enum v4l2_mpeg_vidc_video_decoder_multi_stream {
 V4L2_CID_MPEG_VIDC_VIDEO_STREAM_OUTPUT_PRIMARY = 0,
 V4L2_CID_MPEG_VIDC_VIDEO_STREAM_OUTPUT_SECONDARY = 1,
};




enum v4l2_mpeg_vidc_video_mpeg2_level {
 V4L2_MPEG_VIDC_VIDEO_MPEG2_LEVEL_0 = 0,
 V4L2_MPEG_VIDC_VIDEO_MPEG2_LEVEL_1 = 1,
 V4L2_MPEG_VIDC_VIDEO_MPEG2_LEVEL_2 = 2,
 V4L2_MPEG_VIDC_VIDEO_MPEG2_LEVEL_3 = 3,
};

enum v4l2_mpeg_vidc_video_mpeg2_profile {
 V4L2_MPEG_VIDC_VIDEO_MPEG2_PROFILE_SIMPLE = 0,
 V4L2_MPEG_VIDC_VIDEO_MPEG2_PROFILE_MAIN = 1,
 V4L2_MPEG_VIDC_VIDEO_MPEG2_PROFILE_422 = 2,
 V4L2_MPEG_VIDC_VIDEO_MPEG2_PROFILE_SNR_SCALABLE = 3,
 V4L2_MPEG_VIDC_VIDEO_MPEG2_PROFILE_SPATIAL_SCALABLE = 4,
 V4L2_MPEG_VIDC_VIDEO_MPEG2_PROFILE_HIGH = 5,
};





enum v4l2_mpeg_vidc_video_mvc_layout {
 V4L2_MPEG_VIDC_VIDEO_MVC_SEQUENTIAL = 0,
 V4L2_MPEG_VIDC_VIDEO_MVC_TOP_BOTTOM = 1
};
// # 869 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_mpeg_vidc_video_ltrmode {
 V4L2_MPEG_VIDC_VIDEO_LTR_MODE_DISABLE = 0,
 V4L2_MPEG_VIDC_VIDEO_LTR_MODE_MANUAL = 1,
 V4L2_MPEG_VIDC_VIDEO_LTR_MODE_PERIODIC = 2
};
// # 889 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_mpeg_vidc_video_rate_control_timestamp_mode {
 V4L2_MPEG_VIDC_VIDEO_RATE_CONTROL_TIMESTAMP_MODE_HONOR = 0,
 V4L2_MPEG_VIDC_VIDEO_RATE_CONTROL_TIMESTAMP_MODE_IGNORE = 1,
};



enum vl42_mpeg_vidc_video_enable_initial_qp {
 V4L2_CID_MPEG_VIDC_VIDEO_ENABLE_INITIAL_QP_IFRAME = 0x1,
 V4L2_CID_MPEG_VIDC_VIDEO_ENABLE_INITIAL_QP_PFRAME = 0x2,
 V4L2_CID_MPEG_VIDC_VIDEO_ENABLE_INITIAL_QP_BFRAME = 0x4,
};
// # 935 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum vl42_mpeg_vidc_video_vpx_error_resilience {
 V4L2_MPEG_VIDC_VIDEO_VPX_ERROR_RESILIENCE_DISABLED = 0,
 V4L2_MPEG_VIDC_VIDEO_VPX_ERROR_RESILIENCE_ENABLED = 1,
};



enum v4l2_mpeg_video_hevc_profile {
 V4L2_MPEG_VIDC_VIDEO_HEVC_PROFILE_MAIN = 0,
 V4L2_MPEG_VIDC_VIDEO_HEVC_PROFILE_MAIN10 = 1,
 V4L2_MPEG_VIDC_VIDEO_HEVC_PROFILE_MAIN_STILL_PIC = 2,
};



enum v4l2_mpeg_video_hevc_level {
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_1 = 0,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_1 = 1,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_2 = 2,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_2 = 3,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_2_1 = 4,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_2_1 = 5,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_3 = 6,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_3 = 7,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_3_1 = 8,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_3_1 = 9,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_4 = 10,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_4 = 11,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_4_1 = 12,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_4_1 = 13,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_5 = 14,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_5 = 15,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_5_1 = 16,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_5_1 = 17,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_5_2 = 18,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_5_2 = 19,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_6 = 20,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_6 = 21,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_6_1 = 22,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_6_1 = 23,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_MAIN_TIER_LEVEL_6_2 = 24,
 V4L2_MPEG_VIDC_VIDEO_HEVC_LEVEL_HIGH_TIER_LEVEL_6_2 = 25,
};




enum vl42_mpeg_vidc_video_h264_svc_nal {
 V4L2_CID_MPEG_VIDC_VIDEO_H264_NAL_SVC_DISABLED = 0,
 V4L2_CID_MPEG_VIDC_VIDEO_H264_NAL_SVC_ENABLED = 1,
};




enum v4l2_mpeg_vidc_video_perf_mode {
 V4L2_MPEG_VIDC_VIDEO_PERF_MAX_QUALITY = 1,
 V4L2_MPEG_VIDC_VIDEO_PERF_POWER_SAVE = 2
};
// # 1010 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_mpeg_vidc_video_priority {
 V4L2_MPEG_VIDC_VIDEO_PRIORITY_REALTIME_ENABLE = 0,
 V4L2_MPEG_VIDC_VIDEO_PRIORITY_REALTIME_DISABLE = 1,
};
// # 1024 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_cid_mpeg_vidc_video_full_range {
  V4L2_CID_MPEG_VIDC_VIDEO_FULL_RANGE_DISABLE = 0,
  V4L2_CID_MPEG_VIDC_VIDEO_FULL_RANGE_ENABLE = 1,
};
// # 1038 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_cid_mpeg_vidc_video_vpe_csc_type_enable {
 V4L2_CID_MPEG_VIDC_VIDEO_VPE_CSC_DISABLE = 0,
 V4L2_CID_MPEG_VIDC_VIDEO_VPE_CSC_ENABLE = 1
};







enum v4l2_exposure_auto_type {
 V4L2_EXPOSURE_AUTO = 0,
 V4L2_EXPOSURE_MANUAL = 1,
 V4L2_EXPOSURE_SHUTTER_PRIORITY = 2,
 V4L2_EXPOSURE_APERTURE_PRIORITY = 3
};
// # 1082 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_auto_n_preset_white_balance {
 V4L2_WHITE_BALANCE_MANUAL = 0,
 V4L2_WHITE_BALANCE_AUTO = 1,
 V4L2_WHITE_BALANCE_INCANDESCENT = 2,
 V4L2_WHITE_BALANCE_FLUORESCENT = 3,
 V4L2_WHITE_BALANCE_FLUORESCENT_H = 4,
 V4L2_WHITE_BALANCE_HORIZON = 5,
 V4L2_WHITE_BALANCE_DAYLIGHT = 6,
 V4L2_WHITE_BALANCE_FLASH = 7,
 V4L2_WHITE_BALANCE_CLOUDY = 8,
 V4L2_WHITE_BALANCE_SHADE = 9,
};






enum v4l2_iso_sensitivity_auto_type {
 V4L2_ISO_SENSITIVITY_MANUAL = 0,
 V4L2_ISO_SENSITIVITY_AUTO = 1,
};


enum v4l2_exposure_metering {
 V4L2_EXPOSURE_METERING_AVERAGE = 0,
 V4L2_EXPOSURE_METERING_CENTER_WEIGHTED = 1,
 V4L2_EXPOSURE_METERING_SPOT = 2,
 V4L2_EXPOSURE_METERING_MATRIX = 3,
};


enum v4l2_scene_mode {
 V4L2_SCENE_MODE_NONE = 0,
 V4L2_SCENE_MODE_BACKLIGHT = 1,
 V4L2_SCENE_MODE_BEACH_SNOW = 2,
 V4L2_SCENE_MODE_CANDLE_LIGHT = 3,
 V4L2_SCENE_MODE_DAWN_DUSK = 4,
 V4L2_SCENE_MODE_FALL_COLORS = 5,
 V4L2_SCENE_MODE_FIREWORKS = 6,
 V4L2_SCENE_MODE_LANDSCAPE = 7,
 V4L2_SCENE_MODE_NIGHT = 8,
 V4L2_SCENE_MODE_PARTY_INDOOR = 9,
 V4L2_SCENE_MODE_PORTRAIT = 10,
 V4L2_SCENE_MODE_SPORTS = 11,
 V4L2_SCENE_MODE_SUNSET = 12,
 V4L2_SCENE_MODE_TEXT = 13,
};
// # 1145 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_auto_focus_range {
 V4L2_AUTO_FOCUS_RANGE_AUTO = 0,
 V4L2_AUTO_FOCUS_RANGE_NORMAL = 1,
 V4L2_AUTO_FOCUS_RANGE_MACRO = 2,
 V4L2_AUTO_FOCUS_RANGE_INFINITY = 3,
};
// # 1179 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_preemphasis {
 V4L2_PREEMPHASIS_DISABLED = 0,
 V4L2_PREEMPHASIS_50_uS = 1,
 V4L2_PREEMPHASIS_75_uS = 2,
};
// # 1194 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_flash_led_mode {
 V4L2_FLASH_LED_MODE_NONE,
 V4L2_FLASH_LED_MODE_FLASH,
 V4L2_FLASH_LED_MODE_TORCH,
};


enum v4l2_flash_strobe_source {
 V4L2_FLASH_STROBE_SOURCE_SOFTWARE,
 V4L2_FLASH_STROBE_SOURCE_EXTERNAL,
};
// # 1233 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_jpeg_chroma_subsampling {
 V4L2_JPEG_CHROMA_SUBSAMPLING_444 = 0,
 V4L2_JPEG_CHROMA_SUBSAMPLING_422 = 1,
 V4L2_JPEG_CHROMA_SUBSAMPLING_420 = 2,
 V4L2_JPEG_CHROMA_SUBSAMPLING_411 = 3,
 V4L2_JPEG_CHROMA_SUBSAMPLING_410 = 4,
 V4L2_JPEG_CHROMA_SUBSAMPLING_GRAY = 5,
};
// # 1279 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_dv_tx_mode {
 V4L2_DV_TX_MODE_DVI_D = 0,
 V4L2_DV_TX_MODE_HDMI = 1,
};

enum v4l2_dv_rgb_range {
 V4L2_DV_RGB_RANGE_AUTO = 0,
 V4L2_DV_RGB_RANGE_LIMITED = 1,
 V4L2_DV_RGB_RANGE_FULL = 2,
};
// # 1297 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/v4l2-controls.h"
enum v4l2_deemphasis {
 V4L2_DEEMPHASIS_DISABLED = V4L2_PREEMPHASIS_DISABLED,
 V4L2_DEEMPHASIS_50_uS = V4L2_PREEMPHASIS_50_uS,
 V4L2_DEEMPHASIS_75_uS = V4L2_PREEMPHASIS_75_uS,
};
// # 65 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h" 2
// # 84 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
enum v4l2_field {
 V4L2_FIELD_ANY = 0,



 V4L2_FIELD_NONE = 1,
 V4L2_FIELD_TOP = 2,
 V4L2_FIELD_BOTTOM = 3,
 V4L2_FIELD_INTERLACED = 4,
 V4L2_FIELD_SEQ_TB = 5,

 V4L2_FIELD_SEQ_BT = 6,
 V4L2_FIELD_ALTERNATE = 7,

 V4L2_FIELD_INTERLACED_TB = 8,


 V4L2_FIELD_INTERLACED_BT = 9,


};
// # 126 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
enum v4l2_buf_type {
 V4L2_BUF_TYPE_VIDEO_CAPTURE = 1,
 V4L2_BUF_TYPE_VIDEO_OUTPUT = 2,
 V4L2_BUF_TYPE_VIDEO_OVERLAY = 3,
 V4L2_BUF_TYPE_VBI_CAPTURE = 4,
 V4L2_BUF_TYPE_VBI_OUTPUT = 5,
 V4L2_BUF_TYPE_SLICED_VBI_CAPTURE = 6,
 V4L2_BUF_TYPE_SLICED_VBI_OUTPUT = 7,


 V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY = 8,

 V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE = 9,
 V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE = 10,

 V4L2_BUF_TYPE_PRIVATE = 0x80,
};
// # 159 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
enum v4l2_tuner_type {
 V4L2_TUNER_RADIO = 1,
 V4L2_TUNER_ANALOG_TV = 2,
 V4L2_TUNER_DIGITAL_TV = 3,
};

enum v4l2_memory {
 V4L2_MEMORY_MMAP = 1,
 V4L2_MEMORY_USERPTR = 2,
 V4L2_MEMORY_OVERLAY = 3,
 V4L2_MEMORY_DMABUF = 4,
};


enum v4l2_colorspace {

 V4L2_COLORSPACE_SMPTE170M = 1,


 V4L2_COLORSPACE_SMPTE240M = 2,


 V4L2_COLORSPACE_REC709 = 3,


 V4L2_COLORSPACE_BT878 = 4,


 V4L2_COLORSPACE_470_SYSTEM_M = 5,
 V4L2_COLORSPACE_470_SYSTEM_BG = 6,





 V4L2_COLORSPACE_JPEG = 7,


 V4L2_COLORSPACE_SRGB = 8,
};

enum v4l2_priority {
 V4L2_PRIORITY_UNSET = 0,
 V4L2_PRIORITY_BACKGROUND = 1,
 V4L2_PRIORITY_INTERACTIVE = 2,
 V4L2_PRIORITY_RECORD = 3,
 V4L2_PRIORITY_DEFAULT = V4L2_PRIORITY_INTERACTIVE,
};

struct v4l2_rect {
 __s32 left;
 __s32 top;
 __s32 width;
 __s32 height;
};

struct v4l2_fract {
 __u32 numerator;
 __u32 denominator;
};
// # 231 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_capability {
 __u8 driver[16];
 __u8 card[32];
 __u8 bus_info[32];
 __u32 version;
 __u32 capabilities;
 __u32 device_caps;
 __u32 reserved[3];
};
// # 277 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_pix_format {
 __u32 width;
 __u32 height;
 __u32 pixelformat;
 __u32 field;
 __u32 bytesperline;
 __u32 sizeimage;
 __u32 colorspace;
 __u32 priv;
};
// # 451 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_fmtdesc {
 __u32 index;
 __u32 type;
 __u32 flags;
 __u8 description[32];
 __u32 pixelformat;
 __u32 reserved[4];
};
// # 468 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
enum v4l2_frmsizetypes {
 V4L2_FRMSIZE_TYPE_DISCRETE = 1,
 V4L2_FRMSIZE_TYPE_CONTINUOUS = 2,
 V4L2_FRMSIZE_TYPE_STEPWISE = 3,
};

struct v4l2_frmsize_discrete {
 __u32 width;
 __u32 height;
};

struct v4l2_frmsize_stepwise {
 __u32 min_width;
 __u32 max_width;
 __u32 step_width;
 __u32 min_height;
 __u32 max_height;
 __u32 step_height;
};

struct v4l2_frmsizeenum {
 __u32 index;
 __u32 pixel_format;
 __u32 type;

 union {
  struct v4l2_frmsize_discrete discrete;
  struct v4l2_frmsize_stepwise stepwise;
 };

 __u32 reserved[2];
};




enum v4l2_frmivaltypes {
 V4L2_FRMIVAL_TYPE_DISCRETE = 1,
 V4L2_FRMIVAL_TYPE_CONTINUOUS = 2,
 V4L2_FRMIVAL_TYPE_STEPWISE = 3,
};

struct v4l2_frmival_stepwise {
 struct v4l2_fract min;
 struct v4l2_fract max;
 struct v4l2_fract step;
};

struct v4l2_frmivalenum {
 __u32 index;
 __u32 pixel_format;
 __u32 width;
 __u32 height;
 __u32 type;

 union {
  struct v4l2_fract discrete;
  struct v4l2_frmival_stepwise stepwise;
 };

 __u32 reserved[2];
};





struct v4l2_timecode {
 __u32 type;
 __u32 flags;
 __u8 frames;
 __u8 seconds;
 __u8 minutes;
 __u8 hours;
 __u8 userbits[4];
};
// # 560 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_jpegcompression {
 int quality;

 int APPn;

 int APP_len;
 char APP_data[60];

 int COM_len;
 char COM_data[60];

 __u32 jpeg_markers;
// # 587 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
};




struct v4l2_requestbuffers {
 __u32 count;
 __u32 type;
 __u32 memory;
 __u32 reserved[2];
};
// # 619 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_plane {
 __u32 bytesused;
 __u32 length;
 union {
  __u32 mem_offset;
  unsigned long userptr;
  __s32 fd;
 } m;
 __u32 data_offset;
 __u32 reserved[11];
};
// # 662 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_buffer {
 __u32 index;
 __u32 type;
 __u32 bytesused;
 __u32 flags;
 __u32 field;
 struct timeval timestamp;
 struct v4l2_timecode timecode;
 __u32 sequence;


 __u32 memory;
 union {
  __u32 offset;
  unsigned long userptr;
  struct v4l2_plane *planes;
  __s32 fd;
 } m;
 __u32 length;
 __u32 reserved2;
 __u32 reserved;
};
// # 739 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_exportbuffer {
 __u32 type;
 __u32 index;
 __u32 plane;
 __u32 flags;
 __s32 fd;
 __u32 reserved[11];
};




struct v4l2_framebuffer {
 __u32 capability;
 __u32 flags;


 void *base;
 struct v4l2_pix_format fmt;
};
// # 777 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_clip {
 struct v4l2_rect c;
 struct v4l2_clip *next;
};

struct v4l2_window {
 struct v4l2_rect w;
 __u32 field;
 __u32 chromakey;
 struct v4l2_clip *clips;
 __u32 clipcount;
 void *bitmap;
 __u8 global_alpha;
};




struct v4l2_captureparm {
 __u32 capability;
 __u32 capturemode;
 struct v4l2_fract timeperframe;
 __u32 extendedmode;
 __u32 readbuffers;
 __u32 reserved[4];
};






struct v4l2_qcom_frameskip {
 __u64 maxframeinterval;
 __u8 fpsvariance;
};

struct v4l2_outputparm {
 __u32 capability;
 __u32 outputmode;
 struct v4l2_fract timeperframe;
 __u32 extendedmode;
 __u32 writebuffers;
 __u32 reserved[4];
};




struct v4l2_cropcap {
 __u32 type;
 struct v4l2_rect bounds;
 struct v4l2_rect defrect;
 struct v4l2_fract pixelaspect;
};

struct v4l2_crop {
 __u32 type;
 struct v4l2_rect c;
};
// # 851 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_selection {
 __u32 type;
 __u32 target;
 __u32 flags;
 struct v4l2_rect r;
 __u32 reserved[9];
};






typedef __u64 v4l2_std_id;
// # 992 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_standard {
 __u32 index;
 v4l2_std_id id;
 __u8 name[24];
 struct v4l2_fract frameperiod;
 __u32 framelines;
 __u32 reserved[4];
};
// # 1037 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_bt_timings {
 __u32 width;
 __u32 height;
 __u32 interlaced;
 __u32 polarities;
 __u64 pixelclock;
 __u32 hfrontporch;
 __u32 hsync;
 __u32 hbackporch;
 __u32 vfrontporch;
 __u32 vsync;
 __u32 vbackporch;
 __u32 il_vfrontporch;
 __u32 il_vsync;
 __u32 il_vbackporch;
 __u32 standards;
 __u32 flags;
 __u32 reserved[14];
} __attribute__ ((packed));
// # 1101 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_dv_timings {
 __u32 type;
 union {
  struct v4l2_bt_timings bt;
  __u32 reserved[32];
 };
} __attribute__ ((packed));
// # 1118 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_enum_dv_timings {
 __u32 index;
 __u32 reserved[3];
 struct v4l2_dv_timings timings;
};
// # 1135 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_bt_timings_cap {
 __u32 min_width;
 __u32 max_width;
 __u32 min_height;
 __u32 max_height;
 __u64 min_pixelclock;
 __u64 max_pixelclock;
 __u32 standards;
 __u32 capabilities;
 __u32 reserved[16];
} __attribute__ ((packed));
// # 1160 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_dv_timings_cap {
 __u32 type;
 __u32 reserved[3];
 union {
  struct v4l2_bt_timings_cap bt;
  __u32 raw_data[32];
 };
};





struct v4l2_input {
 __u32 index;
 __u8 name[32];
 __u32 type;
 __u32 audioset;
 __u32 tuner;
 v4l2_std_id std;
 __u32 status;
 __u32 capabilities;
 __u32 reserved[3];
};
// # 1221 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_output {
 __u32 index;
 __u8 name[32];
 __u32 type;
 __u32 audioset;
 __u32 modulator;
 v4l2_std_id std;
 __u32 capabilities;
 __u32 reserved[3];
};
// # 1244 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_control {
 __u32 id;
 __s32 value;
};

struct v4l2_ext_control {
 __u32 id;
 __u32 size;
 __u32 reserved2[1];
 union {
  __s32 value;
  __s64 value64;
  char *string;
 };
} __attribute__ ((packed));

struct v4l2_ext_controls {
 __u32 ctrl_class;
 __u32 count;
 __u32 error_idx;
 __u32 reserved[2];
 struct v4l2_ext_control *controls;
};





enum v4l2_ctrl_type {
 V4L2_CTRL_TYPE_INTEGER = 1,
 V4L2_CTRL_TYPE_BOOLEAN = 2,
 V4L2_CTRL_TYPE_MENU = 3,
 V4L2_CTRL_TYPE_BUTTON = 4,
 V4L2_CTRL_TYPE_INTEGER64 = 5,
 V4L2_CTRL_TYPE_CTRL_CLASS = 6,
 V4L2_CTRL_TYPE_STRING = 7,
 V4L2_CTRL_TYPE_BITMASK = 8,
 V4L2_CTRL_TYPE_INTEGER_MENU = 9,
};


struct v4l2_queryctrl {
 __u32 id;
 __u32 type;
 __u8 name[32];
 __s32 minimum;
 __s32 maximum;
 __s32 step;
 __s32 default_value;
 __u32 flags;
 __u32 reserved[2];
};


struct v4l2_querymenu {
 __u32 id;
 __u32 index;
 union {
  __u8 name[32];
  __s64 value;
 };
 __u32 reserved;
} __attribute__ ((packed));
// # 1330 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_tuner {
 __u32 index;
 __u8 name[32];
 __u32 type;
 __u32 capability;
 __u32 rangelow;
 __u32 rangehigh;
 __u32 rxsubchans;
 __u32 audmode;
 __s32 signal;
 __s32 afc;
 __u32 reserved[4];
};

struct v4l2_modulator {
 __u32 index;
 __u8 name[32];
 __u32 capability;
 __u32 rangelow;
 __u32 rangehigh;
 __u32 txsubchans;
 __u32 reserved[4];
};
// # 1385 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_frequency {
 __u32 tuner;
 __u32 type;
 __u32 frequency;
 __u32 reserved[8];
};





struct v4l2_frequency_band {
 __u32 tuner;
 __u32 type;
 __u32 index;
 __u32 capability;
 __u32 rangelow;
 __u32 rangehigh;
 __u32 modulation;
 __u32 reserved[9];
};

struct v4l2_hw_freq_seek {
 __u32 tuner;
 __u32 type;
 __u32 seek_upward;
 __u32 wrap_around;
 __u32 spacing;
 __u32 rangelow;
 __u32 rangehigh;
 __u32 reserved[5];
};





struct v4l2_rds_data {
 __u8 lsb;
 __u8 msb;
 __u8 block;
} __attribute__ ((packed));
// # 1442 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_audio {
 __u32 index;
 __u8 name[32];
 __u32 capability;
 __u32 mode;
 __u32 reserved[2];
};
// # 1457 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_audioout {
 __u32 index;
 __u8 name[32];
 __u32 capability;
 __u32 mode;
 __u32 reserved[2];
};
// # 1476 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_enc_idx_entry {
 __u64 offset;
 __u64 pts;
 __u32 length;
 __u32 flags;
 __u32 reserved[2];
};


struct v4l2_enc_idx {
 __u32 entries;
 __u32 entries_cap;
 __u32 reserved[4];
 struct v4l2_enc_idx_entry entry[(64)];
};
// # 1502 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_encoder_cmd {
 __u32 cmd;
 __u32 flags;
 union {
  struct {
   __u32 data[8];
  } raw;
 };
};
// # 1545 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_decoder_cmd {
 __u32 cmd;
 __u32 flags;
 union {
  struct {
   __u64 pts;
  } stop;

  struct {





   __s32 speed;
   __u32 format;
  } start;

  struct {
   __u32 data[16];
  } raw;
 };
};
// # 1578 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_vbi_format {
 __u32 sampling_rate;
 __u32 offset;
 __u32 samples_per_line;
 __u32 sample_format;
 __s32 start[2];
 __u32 count[2];
 __u32 flags;
 __u32 reserved[2];
};
// # 1600 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_sliced_vbi_format {
 __u16 service_set;




 __u16 service_lines[2][24];
 __u32 io_size;
 __u32 reserved[2];
};
// # 1624 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_sliced_vbi_cap {
 __u16 service_set;




 __u16 service_lines[2][24];
 __u32 type;
 __u32 reserved[3];
};

struct v4l2_sliced_vbi_data {
 __u32 id;
 __u32 field;
 __u32 line;
 __u32 reserved;
 __u8 data[48];
};
// # 1665 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_mpeg_vbi_itv0_line {
 __u8 id;
 __u8 data[42];
} __attribute__ ((packed));

struct v4l2_mpeg_vbi_itv0 {
 __le32 linemask[2];
 struct v4l2_mpeg_vbi_itv0_line line[35];
} __attribute__ ((packed));

struct v4l2_mpeg_vbi_ITV0 {
 struct v4l2_mpeg_vbi_itv0_line line[36];
} __attribute__ ((packed));




struct v4l2_mpeg_vbi_fmt_ivtv {
 __u8 magic[4];
 union {
  struct v4l2_mpeg_vbi_itv0 itv0;
  struct v4l2_mpeg_vbi_ITV0 ITV0;
 };
} __attribute__ ((packed));
// # 1701 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_plane_pix_format {
 __u32 sizeimage;
 __u16 bytesperline;
 __u16 reserved[7];
} __attribute__ ((packed));
// # 1717 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_pix_format_mplane {
 __u32 width;
 __u32 height;
 __u32 pixelformat;
 __u32 field;
 __u32 colorspace;

 struct v4l2_plane_pix_format plane_fmt[8];
 __u8 num_planes;
 __u8 reserved[11];
} __attribute__ ((packed));
// # 1739 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_format {
 __u32 type;
 union {
  struct v4l2_pix_format pix;
  struct v4l2_pix_format_mplane pix_mp;
  struct v4l2_window win;
  struct v4l2_vbi_format vbi;
  struct v4l2_sliced_vbi_format sliced;
  __u8 raw_data[200];
 } fmt;
};



struct v4l2_streamparm {
 __u32 type;
 union {
  struct v4l2_captureparm capture;
  struct v4l2_outputparm output;
  __u8 raw_data[200];
 } parm;
};
// # 1790 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_event_vsync {

 __u8 field;
} __attribute__ ((packed));






struct v4l2_event_ctrl {
 __u32 changes;
 __u32 type;
 union {
  __s32 value;
  __s64 value64;
 };
 __u32 flags;
 __s32 minimum;
 __s32 maximum;
 __s32 step;
 __s32 default_value;
};

struct v4l2_event_frame_sync {
 __u32 frame_sequence;
};

struct v4l2_event {
 __u32 type;
 union {
  struct v4l2_event_vsync vsync;
  struct v4l2_event_ctrl ctrl;
  struct v4l2_event_frame_sync frame_sync;
  __u8 data[64];
 } u;
 __u32 pending;
 __u32 sequence;
 struct timespec timestamp;
 __u32 id;
 __u32 reserved[8];
};




struct v4l2_event_subscription {
 __u32 type;
 __u32 id;
 __u32 flags;
 __u32 reserved[5];
};
// # 1859 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_dbg_match {
 __u32 type;
 union {
  __u32 addr;
  char name[32];
 };
} __attribute__ ((packed));

struct v4l2_dbg_register {
 struct v4l2_dbg_match match;
 __u32 size;
 __u64 reg;
 __u64 val;
} __attribute__ ((packed));


struct v4l2_dbg_chip_ident {
 struct v4l2_dbg_match match;
 __u32 ident;
 __u32 revision;
} __attribute__ ((packed));





struct v4l2_dbg_chip_info {
 struct v4l2_dbg_match match;
 char name[32];
 __u32 flags;
 __u32 reserved[32];
} __attribute__ ((packed));
// # 1901 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/videodev2.h"
struct v4l2_create_buffers {
 __u32 index;
 __u32 count;
 __u32 memory;
 struct v4l2_format format;
 __u32 reserved[8];
};
// # 34 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h" 2
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/media/msmb_camera.h" 1
// # 123 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/media/msmb_camera.h"
struct msm_v4l2_event_data {

 unsigned int command;

 unsigned int status;

 unsigned int session_id;

 unsigned int stream_id;

 unsigned int map_op;

 unsigned int map_buf_idx;

 unsigned int notify;

 unsigned int arg_value;

 unsigned int ret_value;

 unsigned int v4l2_event_type;

 unsigned int v4l2_event_id;

 unsigned int nop5;

 unsigned int nop6;

 unsigned int nop7;

 unsigned int nop8;

 unsigned int nop9;
};


struct msm_v4l2_format_data {
 enum v4l2_buf_type type;
 unsigned int width;
 unsigned int height;
 unsigned int pixelformat;
 unsigned char num_planes;
 unsigned int plane_sizes[8];
};
// # 194 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/media/msmb_camera.h"
enum smmu_attach_mode {
 NON_SECURE_MODE,
 SECURE_MODE,
 MAX_PROTECTION_MODE,
};

struct msm_camera_smmu_attach_type {
 enum smmu_attach_mode attach;
};
// # 35 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h" 2
// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_intf.h" 1
// # 33 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_intf.h"
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/media/msmb_isp.h" 1
// # 33 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/media/msmb_isp.h"
struct msm_vfe_cfg_cmd_list;

enum ISP_START_PIXEL_PATTERN {
 ISP_BAYER_RGRGRG,
 ISP_BAYER_GRGRGR,
 ISP_BAYER_BGBGBG,
 ISP_BAYER_GBGBGB,
 ISP_YUV_YCbYCr,
 ISP_YUV_YCrYCb,
 ISP_YUV_CbYCrY,
 ISP_YUV_CrYCbY,
 ISP_PIX_PATTERN_MAX
};

enum msm_vfe_plane_fmt {
 Y_PLANE,
 CB_PLANE,
 CR_PLANE,
 CRCB_PLANE,
 CBCR_PLANE,
 VFE_PLANE_FMT_MAX
};

enum msm_vfe_input_src {
 VFE_PIX_0,
 VFE_RAW_0,
 VFE_RAW_1,
 VFE_RAW_2,
 VFE_SRC_MAX,
};

enum msm_vfe_axi_stream_src {
 PIX_ENCODER,
 PIX_VIEWFINDER,
 PIX_VIDEO,
 CAMIF_RAW,
 IDEAL_RAW,
 RDI_INTF_0,
 RDI_INTF_1,
 RDI_INTF_2,
 VFE_AXI_SRC_MAX
};

enum msm_vfe_frame_skip_pattern {
 NO_SKIP,
 EVERY_2FRAME,
 EVERY_3FRAME,
 EVERY_4FRAME,
 EVERY_5FRAME,
 EVERY_6FRAME,
 EVERY_7FRAME,
 EVERY_8FRAME,
 EVERY_9FRAME,
 EVERY_10FRAME,
 EVERY_11FRAME,
 EVERY_12FRAME,
 EVERY_13FRAME,
 EVERY_14FRAME,
 EVERY_15FRAME,
 EVERY_16FRAME,
 EVERY_32FRAME,
 SKIP_ALL,
 MAX_SKIP,
};

enum msm_vfe_camif_input {
 CAMIF_DISABLED,
 CAMIF_PAD_REG_INPUT,
 CAMIF_MIDDI_INPUT,
 CAMIF_MIPI_INPUT,
};

struct msm_vfe_fetch_engine_cfg {
 uint32_t input_format;
 uint32_t buf_width;
 uint32_t buf_height;
 uint32_t fetch_width;
 uint32_t fetch_height;
 uint32_t x_offset;
 uint32_t y_offset;
 uint32_t buf_stride;
};

struct msm_vfe_camif_subsample_cfg {
 uint32_t irq_subsample_period;
 uint32_t irq_subsample_pattern;
 uint32_t pixel_skip;
 uint32_t line_skip;
};

struct msm_vfe_camif_cfg {
 uint32_t lines_per_frame;
 uint32_t pixels_per_line;
 uint32_t first_pixel;
 uint32_t last_pixel;
 uint32_t first_line;
 uint32_t last_line;
 uint32_t epoch_line0;
 uint32_t epoch_line1;
 enum msm_vfe_camif_input camif_input;
 struct msm_vfe_camif_subsample_cfg subsample_cfg;
};

enum msm_vfe_inputmux {
 CAMIF,
 TESTGEN,
 EXTERNAL_READ,
};

enum msm_vfe_stats_composite_group {
 STATS_COMPOSITE_GRP_NONE,
 STATS_COMPOSITE_GRP_1,
 STATS_COMPOSITE_GRP_2,
 STATS_COMPOSITE_GRP_MAX,
};

struct msm_vfe_pix_cfg {
 struct msm_vfe_camif_cfg camif_cfg;
 struct msm_vfe_fetch_engine_cfg fetch_engine_cfg;
 enum msm_vfe_inputmux input_mux;
 enum ISP_START_PIXEL_PATTERN pixel_pattern;
 uint32_t input_format;
};

struct msm_vfe_rdi_cfg {
 uint8_t cid;
 uint8_t frame_based;
};

struct msm_vfe_input_cfg {
 union {
  struct msm_vfe_pix_cfg pix_cfg;
  struct msm_vfe_rdi_cfg rdi_cfg;
 } d;
 enum msm_vfe_input_src input_src;
 uint32_t input_pix_clk;
};

struct msm_vfe_fetch_eng_start {
 uint32_t session_id;
 uint32_t stream_id;
 uint32_t buf_idx;
 uint32_t buf_addr;
};

struct msm_vfe_axi_plane_cfg {
 uint32_t output_width;
 uint32_t output_height;
 uint32_t output_stride;
 uint32_t output_scan_lines;
 uint32_t output_plane_format;
 uint32_t plane_addr_offset;
 uint8_t csid_src;
 uint8_t rdi_cid;
};

enum msm_stream_memory_input_t {
 MEMORY_INPUT_DISABLED,
 MEMORY_INPUT_ENABLED
};

struct msm_vfe_axi_stream_request_cmd {
 uint32_t session_id;
 uint32_t stream_id;
 uint32_t vt_enable;
 uint32_t output_format;
 enum msm_vfe_axi_stream_src stream_src;
 struct msm_vfe_axi_plane_cfg plane_cfg[3];

 uint32_t burst_count;
 uint32_t hfr_mode;
 uint8_t frame_base;

 uint32_t init_frame_drop;
 enum msm_vfe_frame_skip_pattern frame_skip_pattern;
 uint8_t buf_divert;

 uint32_t axi_stream_handle;
 uint32_t controllable_output;
 uint32_t burst_len;

 enum msm_stream_memory_input_t memory_input;
};

struct msm_vfe_axi_stream_release_cmd {
 uint32_t stream_handle;
};

enum msm_vfe_axi_stream_cmd {
 STOP_STREAM,
 START_STREAM,
 STOP_IMMEDIATELY,
};

struct msm_vfe_axi_stream_cfg_cmd {
 uint8_t num_streams;
 uint32_t stream_handle[7];
 enum msm_vfe_axi_stream_cmd cmd;
};

enum msm_vfe_axi_stream_update_type {
 ENABLE_STREAM_BUF_DIVERT,
 DISABLE_STREAM_BUF_DIVERT,
 UPDATE_STREAM_FRAMEDROP_PATTERN,
 UPDATE_STREAM_STATS_FRAMEDROP_PATTERN,
 UPDATE_STREAM_AXI_CONFIG,
 UPDATE_STREAM_REQUEST_FRAMES,
};

enum msm_vfe_iommu_type {
 IOMMU_ATTACH,
 IOMMU_DETACH,
};

struct msm_vfe_axi_stream_cfg_update_info {
 uint32_t stream_handle;
 uint32_t output_format;
 uint32_t request_frm_num;
 enum msm_vfe_frame_skip_pattern skip_pattern;
 struct msm_vfe_axi_plane_cfg plane_cfg[3];
};

struct msm_vfe_axi_halt_cmd {
 uint32_t stop_camif;
 uint32_t overflow_detected;
};

struct msm_vfe_axi_reset_cmd {
 uint32_t blocking;
 uint32_t frame_id;
};

struct msm_vfe_axi_restart_cmd {
 uint32_t enable_camif;
};

struct msm_vfe_axi_stream_update_cmd {
 uint32_t num_streams;
 enum msm_vfe_axi_stream_update_type update_type;
 struct msm_vfe_axi_stream_cfg_update_info update_info[7];
};

struct msm_vfe_smmu_attach_cmd {
 uint32_t security_mode;
 uint32_t iommu_attach_mode;
};

enum msm_isp_stats_type {
 MSM_ISP_STATS_AEC,
 MSM_ISP_STATS_AF,
 MSM_ISP_STATS_AWB,
 MSM_ISP_STATS_RS,
 MSM_ISP_STATS_CS,
 MSM_ISP_STATS_IHIST,
 MSM_ISP_STATS_SKIN,
 MSM_ISP_STATS_BG,
 MSM_ISP_STATS_BF,
 MSM_ISP_STATS_BE,
 MSM_ISP_STATS_BHIST,
 MSM_ISP_STATS_BF_SCALE,
 MSM_ISP_STATS_HDR_BE,
 MSM_ISP_STATS_HDR_BHIST,
 MSM_ISP_STATS_MAX
};

struct msm_vfe_stats_stream_request_cmd {
 uint32_t session_id;
 uint32_t stream_id;
 enum msm_isp_stats_type stats_type;
 uint32_t composite_flag;
 uint32_t framedrop_pattern;
 uint32_t init_frame_drop;
 uint32_t irq_subsample_pattern;
 uint32_t buffer_offset;
 uint32_t stream_handle;
};

struct msm_vfe_stats_stream_release_cmd {
 uint32_t stream_handle;
};
struct msm_vfe_stats_stream_cfg_cmd {
 uint8_t num_streams;
 uint32_t stream_handle[MSM_ISP_STATS_MAX];
 uint8_t enable;
 uint32_t stats_burst_len;
};

enum msm_vfe_reg_cfg_type {
 VFE_WRITE,
 VFE_WRITE_MB,
 VFE_READ,
 VFE_CFG_MASK,
 VFE_WRITE_DMI_16BIT,
 VFE_WRITE_DMI_32BIT,
 VFE_WRITE_DMI_64BIT,
 VFE_READ_DMI_16BIT,
 VFE_READ_DMI_32BIT,
 VFE_READ_DMI_64BIT,
 GET_MAX_CLK_RATE,
 GET_ISP_ID,
 VFE_HW_UPDATE_LOCK,
 VFE_HW_UPDATE_UNLOCK,
 SET_WM_UB_SIZE,
 SET_UB_POLICY,
};

struct msm_vfe_cfg_cmd2 {
 uint16_t num_cfg;
 uint16_t cmd_len;
 void *cfg_data;
 void *cfg_cmd;
};

struct msm_vfe_cfg_cmd_list {
 struct msm_vfe_cfg_cmd2 cfg_cmd;
 struct msm_vfe_cfg_cmd_list *next;
 uint32_t next_size;
};

struct msm_vfe_reg_rw_info {
 uint32_t reg_offset;
 uint32_t cmd_data_offset;
 uint32_t len;
};

struct msm_vfe_reg_mask_info {
 uint32_t reg_offset;
 uint32_t mask;
 uint32_t val;
};

struct msm_vfe_reg_dmi_info {
 uint32_t hi_tbl_offset;
 uint32_t lo_tbl_offset;
 uint32_t len;
};

struct msm_vfe_reg_cfg_cmd {
 union {
  struct msm_vfe_reg_rw_info rw_info;
  struct msm_vfe_reg_mask_info mask_info;
  struct msm_vfe_reg_dmi_info dmi_info;
 } u;

 enum msm_vfe_reg_cfg_type cmd_type;
};

enum msm_isp_buf_type {
 ISP_PRIVATE_BUF,
 ISP_SHARE_BUF,
 MAX_ISP_BUF_TYPE,
};

struct msm_isp_buf_request {
 uint32_t session_id;
 uint32_t stream_id;
 uint8_t num_buf;
 uint32_t handle;
 enum msm_isp_buf_type buf_type;
};

struct msm_isp_qbuf_plane {
 uint32_t addr;
 uint32_t offset;
};

struct msm_isp_qbuf_buffer {
 struct msm_isp_qbuf_plane planes[3];
 uint32_t num_planes;
};

struct msm_isp_qbuf_info {
 uint32_t handle;
 int32_t buf_idx;

 struct msm_isp_qbuf_buffer buffer;

 uint32_t dirty_buf;
};

struct msm_vfe_axi_src_state {
 enum msm_vfe_input_src input_src;
 uint32_t src_active;
 uint32_t src_frame_id;
};

enum msm_isp_event_idx {
 ISP_REG_UPDATE = 0,
 ISP_EPOCH_0 = 1,
 ISP_EPOCH_1 = 2,
 ISP_START_ACK = 3,
 ISP_STOP_ACK = 4,
 ISP_IRQ_VIOLATION = 5,
 ISP_WM_BUS_OVERFLOW = 6,
 ISP_STATS_OVERFLOW = 7,
 ISP_CAMIF_ERROR = 8,
 ISP_BUF_DONE = 9,
 ISP_FE_RD_DONE = 10,
 ISP_EVENT_MAX = 11
};
// # 462 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/media/msmb_isp.h"
struct msm_isp_buf_event {
 uint32_t session_id;
 uint32_t stream_id;
 uint32_t handle;
 uint32_t output_format;
 int8_t buf_idx;
};
struct msm_isp_stats_event {
 uint32_t stats_mask;
 uint8_t stats_buf_idxs[MSM_ISP_STATS_MAX];
};

struct msm_isp_stream_ack {
 uint32_t session_id;
 uint32_t stream_id;
 uint32_t handle;
};

struct msm_isp_error_info {

 uint32_t error_mask;
};

struct msm_isp_event_data {



 struct timeval timestamp;

 struct timeval mono_timestamp;
 enum msm_vfe_input_src input_intf;
 uint32_t frame_id;
 union {
  struct msm_isp_stats_event stats;
  struct msm_isp_buf_event buf_done;
  struct msm_isp_error_info error_info;
 } u;
 uint32_t is_skip_pproc;
};
// # 34 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_intf.h" 2

// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_types.h" 1
// # 92 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_types.h"
typedef enum {
    CAM_HAL_V1 = 1,
    CAM_HAL_V3 = 3
} cam_hal_version_t;

typedef enum {
    CAM_STATUS_SUCCESS,
    CAM_STATUS_FAILED,
    CAM_STATUS_INVALID_PARM,
    CAM_STATUS_NOT_SUPPORTED,
    CAM_STATUS_ACCEPTED,
    CAM_STATUS_MAX,
} cam_status_t;

typedef enum {
    CAM_POSITION_BACK,
    CAM_POSITION_FRONT
} cam_position_t;

typedef enum {
    CAM_FORMAT_JPEG = 0,
    CAM_FORMAT_YUV_420_NV12 = 1,
    CAM_FORMAT_YUV_420_NV21,
    CAM_FORMAT_YUV_420_NV21_ADRENO,
    CAM_FORMAT_YUV_420_YV12,
    CAM_FORMAT_YUV_422_NV16,
    CAM_FORMAT_YUV_422_NV61,
    CAM_FORMAT_YUV_420_NV12_VENUS,
// # 129 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_types.h"
    CAM_FORMAT_YUV_RAW_8BIT_YUYV,
    CAM_FORMAT_YUV_RAW_8BIT_YVYU,
    CAM_FORMAT_YUV_RAW_8BIT_UYVY,
    CAM_FORMAT_YUV_RAW_8BIT_VYUY,
// # 141 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_types.h"
    CAM_FORMAT_BAYER_QCOM_RAW_8BPP_GBRG,
    CAM_FORMAT_BAYER_QCOM_RAW_8BPP_GRBG,
    CAM_FORMAT_BAYER_QCOM_RAW_8BPP_RGGB,
    CAM_FORMAT_BAYER_QCOM_RAW_8BPP_BGGR,
    CAM_FORMAT_BAYER_QCOM_RAW_10BPP_GBRG,
    CAM_FORMAT_BAYER_QCOM_RAW_10BPP_GRBG,
    CAM_FORMAT_BAYER_QCOM_RAW_10BPP_RGGB,
    CAM_FORMAT_BAYER_QCOM_RAW_10BPP_BGGR,
    CAM_FORMAT_BAYER_QCOM_RAW_12BPP_GBRG,
    CAM_FORMAT_BAYER_QCOM_RAW_12BPP_GRBG,
    CAM_FORMAT_BAYER_QCOM_RAW_12BPP_RGGB,
    CAM_FORMAT_BAYER_QCOM_RAW_12BPP_BGGR,







    CAM_FORMAT_BAYER_MIPI_RAW_8BPP_GBRG,
    CAM_FORMAT_BAYER_MIPI_RAW_8BPP_GRBG,
    CAM_FORMAT_BAYER_MIPI_RAW_8BPP_RGGB,
    CAM_FORMAT_BAYER_MIPI_RAW_8BPP_BGGR,
    CAM_FORMAT_BAYER_MIPI_RAW_10BPP_GBRG,
    CAM_FORMAT_BAYER_MIPI_RAW_10BPP_GRBG,
    CAM_FORMAT_BAYER_MIPI_RAW_10BPP_RGGB,
    CAM_FORMAT_BAYER_MIPI_RAW_10BPP_BGGR,
    CAM_FORMAT_BAYER_MIPI_RAW_12BPP_GBRG,
    CAM_FORMAT_BAYER_MIPI_RAW_12BPP_GRBG,
    CAM_FORMAT_BAYER_MIPI_RAW_12BPP_RGGB,
    CAM_FORMAT_BAYER_MIPI_RAW_12BPP_BGGR,






    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_8BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_8BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_8BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_8BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_10BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_10BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_10BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_10BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_12BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_12BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_12BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_12BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_BGGR,


    CAM_FORMAT_JPEG_RAW_8BIT,
    CAM_FORMAT_META_RAW_8BIT,
    CAM_FORMAT_META_RAW_10BIT,





    CAM_FORMAT_BAYER_QCOM_RAW_14BPP_GBRG,
    CAM_FORMAT_BAYER_QCOM_RAW_14BPP_GRBG,
    CAM_FORMAT_BAYER_QCOM_RAW_14BPP_RGGB,
    CAM_FORMAT_BAYER_QCOM_RAW_14BPP_BGGR,
// # 242 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_types.h"
    CAM_FORMAT_BAYER_MIPI_RAW_14BPP_GBRG,
    CAM_FORMAT_BAYER_MIPI_RAW_14BPP_GRBG,
    CAM_FORMAT_BAYER_MIPI_RAW_14BPP_RGGB,
    CAM_FORMAT_BAYER_MIPI_RAW_14BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_14BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_14BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_14BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_14BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_14BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_14BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_14BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_14BPP_BGGR,





    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_14BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_14BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_14BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_14BPP_BGGR,


    CAM_FORMAT_YUV_420_NV12_UBWC,

    CAM_FORMAT_YUV_420_NV21_VENUS,

    CAM_FORMAT_MAX
} cam_format_t;

typedef enum {

    CAM_STREAM_TYPE_DEFAULT,
    CAM_STREAM_TYPE_PREVIEW,
    CAM_STREAM_TYPE_POSTVIEW,
    CAM_STREAM_TYPE_SNAPSHOT,
    CAM_STREAM_TYPE_VIDEO,


    CAM_STREAM_TYPE_IMPL_DEFINED,
    CAM_STREAM_TYPE_YUV,


    CAM_STREAM_TYPE_METADATA,
    CAM_STREAM_TYPE_RAW,
    CAM_STREAM_TYPE_OFFLINE_PROC,
    CAM_STREAM_TYPE_MAX,
} cam_stream_type_t;

typedef enum {
    CAM_PAD_NONE = 1,
    CAM_PAD_TO_2 = 2,
    CAM_PAD_TO_4 = 4,
    CAM_PAD_TO_WORD = CAM_PAD_TO_4,
    CAM_PAD_TO_8 = 8,
    CAM_PAD_TO_16 = 16,
    CAM_PAD_TO_32 = 32,
    CAM_PAD_TO_64 = 64,
    CAM_PAD_TO_1K = 1024,
    CAM_PAD_TO_2K = 2048,
    CAM_PAD_TO_4K = 4096,
    CAM_PAD_TO_8K = 8192
} cam_pad_format_t;

typedef enum {

    CAM_MAPPING_BUF_TYPE_CAPABILITY,
    CAM_MAPPING_BUF_TYPE_PARM_BUF,


    CAM_MAPPING_BUF_TYPE_STREAM_BUF,
    CAM_MAPPING_BUF_TYPE_STREAM_INFO,
    CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF,
    CAM_MAPPING_BUF_TYPE_OFFLINE_META_BUF,
    CAM_MAPPING_BUF_TYPE_MAX
} cam_mapping_buf_type;

typedef struct {
    cam_mapping_buf_type type;
    uint32_t stream_id;
    uint32_t frame_idx;
    int32_t plane_idx;


    uint32_t cookie;
    int fd;
    size_t size;
} cam_buf_map_type;

typedef struct {
    cam_mapping_buf_type type;
    uint32_t stream_id;
    uint32_t frame_idx;
    int32_t plane_idx;


    uint32_t cookie;
} cam_buf_unmap_type;

typedef enum {
    CAM_MAPPING_TYPE_FD_MAPPING,
    CAM_MAPPING_TYPE_FD_UNMAPPING,
    CAM_MAPPING_TYPE_MAX
} cam_mapping_type;

typedef struct {
    cam_mapping_type msg_type;
    union {
        cam_buf_map_type buf_map;
        cam_buf_unmap_type buf_unmap;
    } payload;
} cam_sock_packet_t;

typedef enum {
    CAM_MODE_2D = (1<<0),
    CAM_MODE_3D = (1<<1)
} cam_mode_t;

typedef struct {
    uint32_t len;
    uint32_t y_offset;
    uint32_t cbcr_offset;
} cam_sp_len_offset_t;

typedef struct{
    uint32_t len;
    uint32_t offset;
    int32_t offset_x;
    int32_t offset_y;
    int32_t stride;
    int32_t scanline;
    int32_t width;
    int32_t height;
} cam_mp_len_offset_t;

typedef struct {
    uint32_t width_padding;
    uint32_t height_padding;
    uint32_t plane_padding;
} cam_padding_info_t;

typedef struct {
    uint32_t num_planes;
    union {
        cam_sp_len_offset_t sp;
        cam_mp_len_offset_t mp[8];
    };
    uint32_t frame_len;
} cam_frame_len_offset_t;

typedef struct {
    int32_t width;
    int32_t height;
} cam_dimension_t;

typedef struct {
    cam_frame_len_offset_t plane_info;
} cam_stream_buf_plane_info_t;

typedef struct {
    float min_fps;
    float max_fps;
    float video_min_fps;
    float video_max_fps;
} cam_fps_range_t;


typedef enum {
    CAM_HFR_MODE_OFF,
    CAM_HFR_MODE_60FPS,
    CAM_HFR_MODE_90FPS,
    CAM_HFR_MODE_120FPS,
    CAM_HFR_MODE_150FPS,
    CAM_HFR_MODE_MAX
} cam_hfr_mode_t;

typedef struct {
    cam_hfr_mode_t mode;
    cam_dimension_t dim;
    uint8_t frame_skip;
    uint8_t livesnapshot_sizes_tbl_cnt;
    cam_dimension_t livesnapshot_sizes_tbl[24];
} cam_hfr_info_t;

typedef enum {
    CAM_WB_MODE_AUTO,
    CAM_WB_MODE_CUSTOM,
    CAM_WB_MODE_INCANDESCENT,
    CAM_WB_MODE_FLUORESCENT,
    CAM_WB_MODE_WARM_FLUORESCENT,
    CAM_WB_MODE_DAYLIGHT,
    CAM_WB_MODE_CLOUDY_DAYLIGHT,
    CAM_WB_MODE_TWILIGHT,
    CAM_WB_MODE_SHADE,
    CAM_WB_MODE_MANUAL,
    CAM_WB_MODE_OFF,
    CAM_WB_MODE_MAX
} cam_wb_mode_type;

typedef enum {
    CAM_ANTIBANDING_MODE_OFF,
    CAM_ANTIBANDING_MODE_60HZ,
    CAM_ANTIBANDING_MODE_50HZ,
    CAM_ANTIBANDING_MODE_AUTO,
    CAM_ANTIBANDING_MODE_AUTO_50HZ,
    CAM_ANTIBANDING_MODE_AUTO_60HZ,
    CAM_ANTIBANDING_MODE_MAX,
} cam_antibanding_mode_type;


typedef enum {
    CAM_ISO_MODE_AUTO,
    CAM_ISO_MODE_DEBLUR,
    CAM_ISO_MODE_100,
    CAM_ISO_MODE_200,
    CAM_ISO_MODE_400,
    CAM_ISO_MODE_800,
    CAM_ISO_MODE_1600,
    CAM_ISO_MODE_3200,
    CAM_ISO_MODE_MAX
} cam_iso_mode_type;

typedef enum {
    CAM_AEC_MODE_FRAME_AVERAGE,
    CAM_AEC_MODE_CENTER_WEIGHTED,
    CAM_AEC_MODE_SPOT_METERING,
    CAM_AEC_MODE_SMART_METERING,
    CAM_AEC_MODE_USER_METERING,
    CAM_AEC_MODE_SPOT_METERING_ADV,
    CAM_AEC_MODE_CENTER_WEIGHTED_ADV,
    CAM_AEC_MODE_MAX
} cam_auto_exposure_mode_type;

typedef enum {
    CAM_AE_MODE_OFF,
    CAM_AE_MODE_ON,
    CAM_AE_MODE_MAX
} cam_ae_mode_type;

typedef enum {
    CAM_FOCUS_ALGO_AUTO,
    CAM_FOCUS_ALGO_SPOT,
    CAM_FOCUS_ALGO_CENTER_WEIGHTED,
    CAM_FOCUS_ALGO_AVERAGE,
    CAM_FOCUS_ALGO_MAX
} cam_focus_algorithm_type;


typedef enum {
    CAM_FOCUS_MODE_AUTO,
    CAM_FOCUS_MODE_INFINITY,
    CAM_FOCUS_MODE_MACRO,
    CAM_FOCUS_MODE_FIXED,
    CAM_FOCUS_MODE_EDOF,
    CAM_FOCUS_MODE_CONTINOUS_VIDEO,
    CAM_FOCUS_MODE_CONTINOUS_PICTURE,
    CAM_FOCUS_MODE_MANUAL,
    CAM_FOCUS_MODE_MAX
} cam_focus_mode_type;

typedef enum {
    CAM_MANUAL_FOCUS_MODE_INDEX,
    CAM_MANUAL_FOCUS_MODE_DAC_CODE,
    CAM_MANUAL_FOCUS_MODE_RATIO,
    CAM_MANUAL_FOCUS_MODE_DIOPTER,
    CAM_MANUAL_FOCUS_MODE_MAX
} cam_manual_focus_mode_type;

typedef struct {
    cam_manual_focus_mode_type flag;
    union{
        int32_t af_manual_lens_position_index;
        int32_t af_manual_lens_position_dac;
        int32_t af_manual_lens_position_ratio;
        float af_manual_diopter;
    };
} cam_manual_focus_parm_t;

typedef enum {
    CAM_MANUAL_WB_MODE_CCT,
    CAM_MANUAL_WB_MODE_GAIN,
    CAM_MANUAL_WB_MODE_MAX
} cam_manual_wb_mode_type;

typedef struct {
    float r_gain;
    float g_gain;
    float b_gain;
} cam_awb_gain_t;

typedef struct {
    cam_manual_wb_mode_type type;
    union{
        int32_t cct;
        cam_awb_gain_t gains;
    };
} cam_manual_wb_parm_t;

typedef enum {
    CAM_SCENE_MODE_OFF,
    CAM_SCENE_MODE_AUTO,
    CAM_SCENE_MODE_LANDSCAPE,
    CAM_SCENE_MODE_SNOW,
    CAM_SCENE_MODE_BEACH,
    CAM_SCENE_MODE_SUNSET,
    CAM_SCENE_MODE_NIGHT,
    CAM_SCENE_MODE_PORTRAIT,
    CAM_SCENE_MODE_BACKLIGHT,
    CAM_SCENE_MODE_SPORTS,
    CAM_SCENE_MODE_ANTISHAKE,
    CAM_SCENE_MODE_FLOWERS,
    CAM_SCENE_MODE_CANDLELIGHT,
    CAM_SCENE_MODE_FIREWORKS,
    CAM_SCENE_MODE_PARTY,
    CAM_SCENE_MODE_NIGHT_PORTRAIT,
    CAM_SCENE_MODE_THEATRE,
    CAM_SCENE_MODE_ACTION,
    CAM_SCENE_MODE_AR,
    CAM_SCENE_MODE_FACE_PRIORITY,
    CAM_SCENE_MODE_BARCODE,
    CAM_SCENE_MODE_HDR,
    CAM_SCENE_MODE_MAX
} cam_scene_mode_type;

typedef enum {
    CAM_EFFECT_MODE_OFF,
    CAM_EFFECT_MODE_MONO,
    CAM_EFFECT_MODE_NEGATIVE,
    CAM_EFFECT_MODE_SOLARIZE,
    CAM_EFFECT_MODE_SEPIA,
    CAM_EFFECT_MODE_POSTERIZE,
    CAM_EFFECT_MODE_WHITEBOARD,
    CAM_EFFECT_MODE_BLACKBOARD,
    CAM_EFFECT_MODE_AQUA,
    CAM_EFFECT_MODE_EMBOSS,
    CAM_EFFECT_MODE_SKETCH,
    CAM_EFFECT_MODE_NEON,
    CAM_EFFECT_MODE_MAX
} cam_effect_mode_type;

typedef enum {
    CAM_FLASH_MODE_OFF,
    CAM_FLASH_MODE_AUTO,
    CAM_FLASH_MODE_ON,
    CAM_FLASH_MODE_TORCH,
    CAM_FLASH_MODE_MAX
} cam_flash_mode_t;

typedef enum {
    CAM_AEC_TRIGGER_IDLE,
    CAM_AEC_TRIGGER_START
} cam_aec_trigger_type_t;

typedef enum {
    CAM_AF_TRIGGER_IDLE,
    CAM_AF_TRIGGER_START,
    CAM_AF_TRIGGER_CANCEL
} cam_af_trigger_type_t;

typedef enum {
    CAM_AE_STATE_INACTIVE,
    CAM_AE_STATE_SEARCHING,
    CAM_AE_STATE_CONVERGED,
    CAM_AE_STATE_LOCKED,
    CAM_AE_STATE_FLASH_REQUIRED,
    CAM_AE_STATE_PRECAPTURE
} cam_ae_state_t;

typedef enum {
  CAM_CDS_MODE_OFF,
  CAM_CDS_MODE_ON,
  CAM_CDS_MODE_AUTO,
  CAM_CDS_MODE_MAX
} cam_cds_mode_type_t;

typedef struct {
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
} cam_rect_t;

typedef struct {
    cam_rect_t rect;
    int32_t weight;
} cam_area_t;

typedef enum {
    CAM_STREAMING_MODE_CONTINUOUS,
    CAM_STREAMING_MODE_BURST,
    CAM_STREAMING_MODE_BATCH,
    CAM_STREAMING_MODE_MAX
} cam_streaming_mode_t;

typedef enum {
    IS_TYPE_NONE,
    IS_TYPE_DIS,
    IS_TYPE_GA_DIS,
    IS_TYPE_EIS_1_0,
    IS_TYPE_EIS_2_0
} cam_is_type_t;




typedef enum {
    CAM_EVENT_TYPE_MAP_UNMAP_DONE = (1<<0),
    CAM_EVENT_TYPE_AUTO_FOCUS_DONE = (1<<1),
    CAM_EVENT_TYPE_ZOOM_DONE = (1<<2),
    CAM_EVENT_TYPE_REPROCESS_STAGE_DONE = (1<<3),
    CAM_EVENT_TYPE_DAEMON_DIED = (1<<4),
    CAM_EVENT_TYPE_INT_TAKE_PIC = (1<<5),
    CAM_EVENT_TYPE_MAX
} cam_event_type_t;

typedef enum {
    CAM_EXP_BRACKETING_OFF,
    CAM_EXP_BRACKETING_ON
} cam_bracket_mode;

typedef struct {
    cam_bracket_mode mode;
    char values[32];
} cam_exp_bracketing_t;

typedef struct {
  uint32_t num_frames;
  cam_exp_bracketing_t exp_val;
} cam_hdr_bracketing_info_t;

typedef struct {
    uint8_t chromatixData[21292];
    uint8_t snapchromatixData[21292];
    uint8_t common_chromatixData[42044];
} tune_chromatix_t;

typedef struct {
    uint8_t af_tuneData[5200];
} tune_autofocus_t;

typedef struct {
    uint8_t stepsize;
    uint8_t direction;
    int32_t num_steps;
    uint8_t ttype;
} tune_actuator_t;

typedef struct {
    uint8_t module;
    uint8_t type;
    int32_t value;
} tune_cmd_t;

typedef enum {
    CAM_AEC_ROI_OFF,
    CAM_AEC_ROI_ON
} cam_aec_roi_ctrl_t;

typedef enum {
    CAM_AEC_ROI_BY_INDEX,
    CAM_AEC_ROI_BY_COORDINATE,
} cam_aec_roi_type_t;

typedef struct {
    uint32_t x;
    uint32_t y;
} cam_coordinate_type_t;

typedef struct {
    int32_t numerator;
    int32_t denominator;
} cam_rational_type_t;

typedef struct {
    cam_aec_roi_ctrl_t aec_roi_enable;
    cam_aec_roi_type_t aec_roi_type;
    union {
        cam_coordinate_type_t coordinate[5];
        uint32_t aec_roi_idx[5];
    } cam_aec_roi_position;
} cam_set_aec_roi_t;

typedef struct {
    uint32_t frm_id;
    uint8_t num_roi;
    cam_rect_t roi[5];
    int32_t weight[5];
    uint8_t is_multiwindow;
} cam_roi_info_t;

typedef enum {
    CAM_WAVELET_DENOISE_YCBCR_PLANE,
    CAM_WAVELET_DENOISE_CBCR_ONLY,
    CAM_WAVELET_DENOISE_STREAMLINE_YCBCR,
    CAM_WAVELET_DENOISE_STREAMLINED_CBCR
} cam_denoise_process_type_t;

typedef struct {
    uint8_t denoise_enable;
    cam_denoise_process_type_t process_plates;
} cam_denoise_param_t;



typedef struct {
    uint32_t fd_mode;
    uint32_t num_fd;
} cam_fd_set_parm_t;

typedef enum {
    QCAMERA_FD_PREVIEW,
    QCAMERA_FD_SNAPSHOT
}qcamera_face_detect_type_t;

typedef struct {
    int8_t face_id;
    int8_t score;
    cam_rect_t face_boundary;
    cam_coordinate_type_t left_eye_center;
    cam_coordinate_type_t right_eye_center;
    cam_coordinate_type_t mouth_center;
    uint8_t smile_degree;
    uint8_t smile_confidence;
    uint8_t face_recognised;
    int8_t gaze_angle;
    int8_t updown_dir;
    int8_t leftright_dir;
    int8_t roll_dir;
    int8_t left_right_gaze;
    int8_t top_bottom_gaze;
    uint8_t blink_detected;
    uint8_t left_blink;
    uint8_t right_blink;
} cam_face_detection_info_t;

typedef struct {
    uint32_t frame_id;
    uint8_t num_faces_detected;
    cam_face_detection_info_t faces[5];
    qcamera_face_detect_type_t fd_type;
    cam_dimension_t fd_frame_dim;
} cam_face_detection_data_t;


typedef struct {
    uint32_t max_hist_value;
    uint32_t hist_buf[256];
} cam_histogram_data_t;

typedef struct {
    cam_histogram_data_t r_stats;
    cam_histogram_data_t b_stats;
    cam_histogram_data_t gr_stats;
    cam_histogram_data_t gb_stats;
} cam_bayer_hist_stats_t;

typedef enum {
    CAM_HISTOGRAM_TYPE_BAYER,
    CAM_HISTOGRAM_TYPE_YUV
} cam_histogram_type_t;

typedef struct {
    cam_histogram_type_t type;
    union {
        cam_bayer_hist_stats_t bayer_stats;
        cam_histogram_data_t yuv_stats;
    };
} cam_hist_stats_t;

enum cam_focus_distance_index{
  CAM_FOCUS_DISTANCE_NEAR_INDEX,
  CAM_FOCUS_DISTANCE_OPTIMAL_INDEX,
  CAM_FOCUS_DISTANCE_FAR_INDEX,
  CAM_FOCUS_DISTANCE_MAX_INDEX
};

typedef struct {
  float focus_distance[CAM_FOCUS_DISTANCE_MAX_INDEX];
} cam_focus_distances_info_t;

typedef struct {
  uint32_t scale;
  float diopter;
}cam_focus_pos_info_t ;
// # 839 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_types.h"
typedef enum {
    CAM_AF_COMPLETE_EXISTING_SWEEP,
    CAM_AF_DO_ONE_FULL_SWEEP,
    CAM_AF_START_CONTINUOUS_SWEEP
} cam_autofocus_cycle_t;

typedef enum {
    CAM_AF_SCANNING,
    CAM_AF_FOCUSED,
    CAM_AF_NOT_FOCUSED,
    CAM_AF_INACTIVE,
    CAM_AF_PASSIVE_SCANNING,
    CAM_AF_PASSIVE_FOCUSED,
    CAM_AF_PASSIVE_UNFOCUSED,
} cam_autofocus_state_t;

typedef struct {
    cam_autofocus_state_t focus_state;
    cam_focus_distances_info_t focus_dist;
    int32_t focus_pos;
    uint32_t focused_frame_idx;
    cam_focus_mode_type focus_mode;
} cam_auto_focus_data_t;

typedef struct {
  uint32_t is_hdr_scene;
  float hdr_confidence;
} cam_asd_hdr_scene_data_t;

typedef struct {
    uint32_t stream_id;
    cam_rect_t crop;
} cam_stream_crop_info_t;

typedef struct {
    uint8_t num_of_streams;
    cam_stream_crop_info_t crop_info[8];
} cam_crop_data_t;

typedef enum {
    DO_NOT_NEED_FUTURE_FRAME,
    NEED_FUTURE_FRAME,
} cam_prep_snapshot_state_t;

typedef struct {
    uint32_t min_frame_idx;
    uint32_t max_frame_idx;
} cam_frame_idx_range_t;

typedef enum {
  S_NORMAL = 0,
  S_SCENERY,
  S_PORTRAIT,
  S_PORTRAIT_BACKLIGHT,
  S_SCENERY_BACKLIGHT,
  S_BACKLIGHT,
  S_MAX,
} cam_auto_scene_t;

typedef struct {
   uint32_t meta_frame_id;
} cam_meta_valid_t;

typedef enum {
    CAM_SENSOR_RAW,
    CAM_SENSOR_YUV
} cam_sensor_t;

typedef struct {
    cam_flash_mode_t flash_mode;
    cam_sensor_t sens_type;
    float aperture_value;
    float focal_length;
    float f_number;
    int sensing_method;
    float crop_factor;
} cam_sensor_params_t;

typedef enum {
    CAM_METERING_MODE_UNKNOWN = 0,
    CAM_METERING_MODE_AVERAGE = 1,
    CAM_METERING_MODE_CENTER_WEIGHTED_AVERAGE = 2,
    CAM_METERING_MODE_SPOT = 3,
    CAM_METERING_MODE_MULTI_SPOT = 4,
    CAM_METERING_MODE_PATTERN = 5,
    CAM_METERING_MODE_PARTIAL = 6,
    CAM_METERING_MODE_OTHER = 255,
} cam_metering_mode_t;

typedef struct {
    float exp_time;
    float real_gain;
    int32_t iso_value;
    uint32_t flash_needed;
    uint32_t settled;
    uint32_t exp_index;
    uint32_t line_count;
    uint32_t metering_mode;
    uint32_t exposure_program;
    uint32_t exposure_mode;
    uint32_t scenetype;
    float brightness;
} cam_ae_params_t;

typedef struct {
    int32_t cct_value;
    cam_awb_gain_t rgb_gains;
    int32_t decision;
} cam_awb_params_t;

typedef struct {
    int32_t aec_debug_data_size;
    char aec_private_debug_data[(1720)];
} cam_ae_exif_debug_t;

typedef struct {
    int32_t awb_debug_data_size;
    char awb_private_debug_data[(7027)];
} cam_awb_exif_debug_t;

typedef struct {
    int32_t af_debug_data_size;
    char af_private_debug_data[(643)];
} cam_af_exif_debug_t;

typedef struct {
    int32_t asd_debug_data_size;
    char asd_private_debug_data[(100)];
} cam_asd_exif_debug_t;

typedef struct {
    int32_t bg_stats_buffer_size;
    int32_t bhist_stats_buffer_size;
    char stats_buffer_private_debug_data[(74756)];
} cam_stats_buffer_exif_debug_t;

typedef struct {
    uint32_t tuning_data_version;
    size_t tuning_sensor_data_size;
    size_t tuning_vfe_data_size;
    size_t tuning_cpp_data_size;
    size_t tuning_cac_data_size;
    uint8_t data[(0x10000 + 0x10000 + 0x10000 + 0x10000)];
}tuning_params_t;

typedef struct {
    cam_dimension_t dim;
    size_t size;
    char path[50];
} cam_int_evt_params_t;

typedef struct {
  uint8_t private_mobicat_af_data[1000];
} cam_chromatix_mobicat_af_t;

typedef struct {
  uint8_t private_isp_data[11500];
} cam_chromatix_lite_isp_t;

typedef struct {
  uint8_t private_pp_data[2000];
} cam_chromatix_lite_pp_t;

typedef struct {
  uint8_t private_stats_data[1000];
} cam_chromatix_lite_ae_stats_t;

typedef struct {
  uint8_t private_stats_data[1000];
} cam_chromatix_lite_awb_stats_t;

typedef struct {
  uint8_t private_stats_data[1000];
} cam_chromatix_lite_af_stats_t;

typedef struct {
    uint8_t is_stats_valid;
    cam_hist_stats_t stats_data;

    uint8_t is_faces_valid;
    cam_face_detection_data_t faces_data;

    uint8_t is_focus_valid;
    cam_auto_focus_data_t focus_data;

    uint8_t is_crop_valid;
    cam_crop_data_t crop_data;

    uint8_t is_prep_snapshot_done_valid;
    cam_prep_snapshot_state_t prep_snapshot_done_state;


    uint8_t is_good_frame_idx_range_valid;



    cam_frame_idx_range_t good_frame_idx_range;

    uint32_t is_hdr_scene_data_valid;
    cam_asd_hdr_scene_data_t hdr_scene_data;
    uint8_t is_asd_decision_valid;
    cam_auto_scene_t scene;

    char private_metadata[(1024)];


    uint8_t is_ae_params_valid;
    cam_ae_params_t ae_params;


    uint8_t is_awb_params_valid;
    cam_awb_params_t awb_params;


    uint8_t is_ae_exif_debug_valid;
    cam_ae_exif_debug_t ae_exif_debug_params;


    uint8_t is_awb_exif_debug_valid;
    cam_awb_exif_debug_t awb_exif_debug_params;


    uint8_t is_af_exif_debug_valid;
    cam_af_exif_debug_t af_exif_debug_params;


    uint8_t is_asd_exif_debug_valid;
    cam_asd_exif_debug_t asd_exif_debug_params;


    uint8_t is_stats_buffer_exif_debug_valid;
    cam_stats_buffer_exif_debug_t stats_buffer_exif_debug_params;


    uint8_t is_sensor_params_valid;
    cam_sensor_params_t sensor_params;

    uint8_t is_meta_invalid;
    cam_meta_valid_t meta_invalid_params;


    uint8_t is_preview_frame_skip_valid;
    cam_frame_idx_range_t preview_frame_skip_idx_range;


    uint8_t is_tuning_params_valid;
    tuning_params_t tuning_params;

    uint8_t is_chromatix_mobicat_af_valid;
    cam_chromatix_mobicat_af_t chromatix_mobicat_af_data;

    uint8_t is_chromatix_lite_isp_valid;
    cam_chromatix_lite_isp_t chromatix_lite_isp_data;

    uint8_t is_chromatix_lite_pp_valid;
    cam_chromatix_lite_pp_t chromatix_lite_pp_data;

    uint8_t is_chromatix_lite_ae_stats_valid;
    cam_chromatix_lite_ae_stats_t chromatix_lite_ae_stats_data;

    uint8_t is_chromatix_lite_awb_stats_valid;
    cam_chromatix_lite_awb_stats_t chromatix_lite_awb_stats_data;

    uint8_t is_chromatix_lite_af_stats_valid;
    cam_chromatix_lite_af_stats_t chromatix_lite_af_stats_data;


    uint8_t is_mobicat_ae_params_valid;
    cam_ae_exif_debug_t mobicat_ae_data;

    uint8_t is_mobicat_awb_params_valid;
    cam_awb_exif_debug_t mobicat_awb_data;

    uint8_t is_mobicat_af_params_valid;
    cam_af_exif_debug_t mobicat_af_data;

    uint8_t is_mobicat_asd_params_valid;
    cam_asd_exif_debug_t mobicat_asd_data;

    uint8_t is_mobicat_stats_params_valid;
    cam_stats_buffer_exif_debug_t mobicat_stats_buffer_data;

    uint8_t is_focus_pos_info_valid;
    cam_focus_pos_info_t cur_pos_info;


    uint8_t is_frame_id_reset;
} cam_metadata_info_t;

typedef enum {
    CAM_INTF_PARM_HAL_VERSION,

    CAM_INTF_PARM_ANTIBANDING,
    CAM_INTF_PARM_EXPOSURE_COMPENSATION,
    CAM_INTF_PARM_AEC_LOCK,
    CAM_INTF_PARM_FPS_RANGE,
    CAM_INTF_PARM_AWB_LOCK,
    CAM_INTF_PARM_WHITE_BALANCE,
    CAM_INTF_PARM_EFFECT,
    CAM_INTF_PARM_BESTSHOT_MODE,
    CAM_INTF_PARM_DIS_ENABLE,
    CAM_INTF_PARM_LED_MODE,
    CAM_INTF_META_HISTOGRAM,
    CAM_INTF_META_FACE_DETECTION,
    CAM_INTF_META_AUTOFOCUS_DATA,


    CAM_INTF_PARM_QUERY_FLASH4SNAP,
    CAM_INTF_PARM_EXPOSURE,
    CAM_INTF_PARM_SHARPNESS,
    CAM_INTF_PARM_CONTRAST,
    CAM_INTF_PARM_SATURATION,
    CAM_INTF_PARM_BRIGHTNESS,
    CAM_INTF_PARM_ISO,
    CAM_INTF_PARM_EXPOSURE_TIME,
    CAM_INTF_PARM_ZOOM,
    CAM_INTF_PARM_ROLLOFF,
    CAM_INTF_PARM_MODE,
    CAM_INTF_PARM_AEC_ALGO_TYPE,
    CAM_INTF_PARM_FOCUS_ALGO_TYPE,
    CAM_INTF_PARM_AEC_ROI,
    CAM_INTF_PARM_AF_ROI,
    CAM_INTF_PARM_FOCUS_MODE,
    CAM_INTF_PARM_MANUAL_FOCUS_POS,
    CAM_INTF_PARM_SCE_FACTOR,
    CAM_INTF_PARM_FD,
    CAM_INTF_PARM_MCE,
    CAM_INTF_PARM_HFR,
    CAM_INTF_PARM_REDEYE_REDUCTION,
    CAM_INTF_PARM_WAVELET_DENOISE,
    CAM_INTF_PARM_HISTOGRAM,
    CAM_INTF_PARM_ASD_ENABLE,
    CAM_INTF_PARM_RECORDING_HINT,
    CAM_INTF_PARM_HDR,
    CAM_INTF_PARM_MAX_DIMENSION,
    CAM_INTF_PARM_RAW_DIMENSION,
    CAM_INTF_PARM_FRAMESKIP,
    CAM_INTF_PARM_ZSL_MODE,
    CAM_INTF_PARM_HDR_NEED_1X,
    CAM_INTF_PARM_VIDEO_HDR,
    CAM_INTF_PARM_SENSOR_HDR,
    CAM_INTF_PARM_ROTATION,
    CAM_INTF_PARM_SCALE,
    CAM_INTF_PARM_VT,
    CAM_INTF_META_CROP_DATA,
    CAM_INTF_META_PREP_SNAPSHOT_DONE,
    CAM_INTF_META_GOOD_FRAME_IDX_RANGE,
    CAM_INTF_PARM_GET_CHROMATIX,
    CAM_INTF_PARM_SET_RELOAD_CHROMATIX,
    CAM_INTF_PARM_SET_AUTOFOCUSTUNING,
    CAM_INTF_PARM_GET_AFTUNE,
    CAM_INTF_PARM_SET_RELOAD_AFTUNE,
    CAM_INTF_PARM_SET_VFE_COMMAND,
    CAM_INTF_PARM_SET_PP_COMMAND,
    CAM_INTF_PARM_TINTLESS,
    CAM_INTF_PARM_CDS_MODE,
    CAM_INTF_PARM_WB_MANUAL,
    CAM_INTF_PARM_LONGSHOT_ENABLE,
    CAM_INTF_PARM_LOW_POWER_ENABLE,


    CAM_INTF_PARM_DO_REPROCESS,
    CAM_INTF_PARM_SET_BUNDLE,
    CAM_INTF_PARM_STREAM_FLIP,
    CAM_INTF_PARM_GET_OUTPUT_CROP,

    CAM_INTF_PARM_EZTUNE_CMD,
    CAM_INTF_PARM_INT_EVT,



    CAM_INTF_META_FRAME_NUMBER_VALID,

    CAM_INTF_META_COLOR_CORRECT_MODE,


    CAM_INTF_META_COLOR_CORRECT_TRANSFORM,




    CAM_INTF_META_FRAME_NUMBER,


    CAM_INTF_META_AEC_MODE,

    CAM_INTF_META_AEC_ROI,

    CAM_INTF_META_AEC_PRECAPTURE_TRIGGER,

    CAM_INTF_META_AEC_PRECAPTURE_ID,

    CAM_INTF_META_AEC_STATE,

    CAM_INTF_META_AF_ROI,

    CAM_INTF_META_AF_TRIGGER,

    CAM_INTF_META_AF_STATE,

    CAM_INTF_META_AF_TRIGGER_ID,

    CAM_INTF_META_AWB_REGIONS,

    CAM_INTF_META_AWB_STATE,


    CAM_INTF_META_CAPTURE_INTENT,



    CAM_INTF_META_MODE,


    CAM_INTF_META_DEMOSAIC,


    CAM_INTF_META_EDGE,


    CAM_INTF_META_SHARPNESS_STRENGTH,


    CAM_INTF_META_FLASH_POWER,

    CAM_INTF_META_FLASH_FIRING_TIME,

    CAM_INTF_META_FLASH_STATE,


    CAM_INTF_META_GEOMETRIC_MODE,

    CAM_INTF_META_GEOMETRIC_STRENGTH,


    CAM_INTF_META_HOTPIXEL_MODE,


    CAM_INTF_META_LENS_APERTURE,

    CAM_INTF_META_LENS_FILTERDENSITY,

    CAM_INTF_META_LENS_FOCAL_LENGTH,


    CAM_INTF_META_LENS_FOCUS_DISTANCE,

    CAM_INTF_META_LENS_FOCUS_RANGE,

    CAM_INTF_META_LENS_OPT_STAB_MODE,

    CAM_INTF_META_LENS_STATE,


    CAM_INTF_META_NOISE_REDUCTION_MODE,


    CAM_INTF_META_NOISE_REDUCTION_STRENGTH,



    CAM_INTF_META_SCALER_CROP_REGION,


    CAM_INTF_META_SENSOR_EXPOSURE_TIME,


    CAM_INTF_META_SENSOR_FRAME_DURATION,


    CAM_INTF_META_SENSOR_SENSITIVITY,

    CAM_INTF_META_SENSOR_TIMESTAMP,


    CAM_INTF_META_SHADING_MODE,


    CAM_INTF_META_SHADING_STRENGTH,


    CAM_INTF_META_STATS_FACEDETECT_MODE,

    CAM_INTF_META_STATS_HISTOGRAM_MODE,

    CAM_INTF_META_STATS_SHARPNESS_MAP_MODE,




    CAM_INTF_META_STATS_SHARPNESS_MAP,



    CAM_INTF_META_TONEMAP_CURVE_BLUE,

    CAM_INTF_META_TONEMAP_CURVE_GREEN,

    CAM_INTF_META_TONEMAP_CURVE_RED,

    CAM_INTF_META_TONEMAP_MODE,
    CAM_INTF_META_FLASH_MODE,
    CAM_INTF_META_ASD_HDR_SCENE_DATA,
    CAM_INTF_META_PRIVATE_DATA,
    CAM_INTF_PARM_STATS_DEBUG_MASK,
    CAM_INTF_PARM_ISP_DEBUG_MASK,
    CAM_INTF_PARM_ALGO_OPTIMIZATIONS_MASK,
    CAM_INTF_PARM_SENSOR_DEBUG_MASK,

    CAM_INTF_META_STREAM_ID,
    CAM_INTF_PARM_FOCUS_BRACKETING,
    CAM_INTF_PARM_MULTI_TOUCH_FOCUS_BRACKETING,
    CAM_INTF_PARM_FLASH_BRACKETING,
    CAM_INTF_PARM_GET_IMG_PROP,

    CAM_INTF_PARM_MAX
} cam_intf_parm_type_t;

typedef struct {
    uint32_t forced;
    union {
      uint32_t force_linecount_value;
      float force_gain_value;
      float force_snap_exp_value;
      float force_exp_value;
      uint32_t force_snap_linecount_value;
      float force_snap_gain_value;
    } u;
} cam_ez_force_params_t;

typedef enum {
    CAM_EZTUNE_CMD_STATUS,
    CAM_EZTUNE_CMD_AEC_ENABLE,
    CAM_EZTUNE_CMD_AWB_ENABLE,
    CAM_EZTUNE_CMD_AF_ENABLE,
    CAM_EZTUNE_CMD_AEC_FORCE_LINECOUNT,
    CAM_EZTUNE_CMD_AEC_FORCE_GAIN,
    CAM_EZTUNE_CMD_AEC_FORCE_EXP,
    CAM_EZTUNE_CMD_AEC_FORCE_SNAP_LC,
    CAM_EZTUNE_CMD_AEC_FORCE_SNAP_GAIN,
    CAM_EZTUNE_CMD_AEC_FORCE_SNAP_EXP,
    CAM_EZTUNE_CMD_AWB_MODE,
} cam_eztune_cmd_type_t;

typedef struct {
  cam_eztune_cmd_type_t cmd;
  union {
    int32_t running;
    int32_t aec_enable;
    int32_t awb_enable;
    int32_t af_enable;
    cam_ez_force_params_t ez_force_param;
    int32_t awb_mode;
  } u;
} cam_eztune_cmd_data_t;





typedef enum {
    CAM_INTF_METADATA_MAX
} cam_intf_metadata_type_t;

typedef enum {
    CAM_INTENT_CUSTOM,
    CAM_INTENT_PREVIEW,
    CAM_INTENT_STILL_CAPTURE,
    CAM_INTENT_VIDEO_RECORD,
    CAM_INTENT_VIDEO_SNAPSHOT,
    CAM_INTENT_ZERO_SHUTTER_LAG,
    CAM_INTENT_MAX,
} cam_intent_t;

typedef enum {


    CAM_CONTROL_OFF,



    CAM_CONTROL_AUTO,






    CAM_CONTROL_USE_SCENE_MODE,
    CAM_CONTROL_MAX
} cam_control_mode_t;

typedef enum {

    CAM_COLOR_CORRECTION_TRANSFORM_MATRIX,

    CAM_COLOR_CORRECTION_FAST,

    CAM_COLOR_CORRECTION_HIGH_QUALITY,
} cam_color_correct_mode_t;

typedef struct {

    float transform[3][3];
} cam_color_correct_matrix_t;
// # 1453 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_types.h"
typedef struct {
    size_t tonemap_points_cnt;
// # 1463 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_types.h"
    float tonemap_points[128][2];
} cam_tonemap_curve_t;

typedef enum {
    OFF,
    FAST,
    QUALITY,
} cam_quality_preference_t;

typedef enum {
    CAM_FLASH_CTRL_OFF,
    CAM_FLASH_CTRL_SINGLE,
    CAM_FLASH_CTRL_TORCH
} cam_flash_ctrl_t;

typedef struct {
    uint8_t ae_mode;
    uint8_t awb_mode;
    uint8_t af_mode;
} cam_scene_mode_overrides_t;

typedef struct {
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
} cam_crop_region_t;

typedef struct {



    int32_t sharpness[6][6];
} cam_sharpness_map_t;

typedef struct {
    int32_t min_value;
    int32_t max_value;
    int32_t def_value;
    int32_t step;
} cam_control_range_t;
// # 1534 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_types.h"
typedef enum {
    ROTATE_0 = 1<<0,
    ROTATE_90 = 1<<1,
    ROTATE_180 = 1<<2,
    ROTATE_270 = 1<<3,
} cam_rotation_t;

typedef enum {
    FLIP_NONE = 0,
    FLIP_H = 1,
    FLIP_V = 2,
    FLIP_V_H = 3,
} cam_flip_t;

typedef struct {
    uint32_t bundle_id;
    uint8_t num_of_streams;
    uint32_t stream_ids[4];
} cam_bundle_config_t;

typedef enum {
    CAM_ONLINE_REPROCESS_TYPE,
    CAM_OFFLINE_REPROCESS_TYPE,
} cam_reprocess_type_enum_t;

typedef enum {
    CAM_HDR_MODE_SINGLEFRAME,
    CAM_HDR_MODE_MULTIFRAME,
} cam_hdr_mode_enum_t;

typedef struct {
    uint32_t hdr_enable;
    uint32_t hdr_need_1x;
    cam_hdr_mode_enum_t hdr_mode;
} cam_hdr_param_t;

typedef struct {
    int32_t output_width;
    int32_t output_height;
} cam_scale_param_t;

typedef struct {
    uint8_t enable;
    uint8_t burst_count;
    int32_t focus_steps[5];
    uint8_t output_count;
} cam_af_bracketing_t;

typedef struct {
    uint8_t enable;
    uint8_t burst_count;
} cam_flash_bracketing_t;

typedef struct {
    uint8_t enable;
    uint8_t burst_count;
} cam_fssr_t;

typedef struct {
    uint8_t enable;
    uint8_t burst_count;
    uint8_t zoom_threshold;
} cam_opti_zoom_t;

typedef struct {
    uint8_t enable;
    uint32_t meta_max_size;
    uint32_t meta_header_size;
    uint32_t body_mask_width;
} cam_true_portrait_t;

typedef enum {
    CAM_FLASH_OFF,
    CAM_FLASH_ON
} cam_flash_value_t;

typedef struct {
    cam_sensor_t sens_type;
    cam_format_t native_format;
} cam_sensor_type_t;

typedef struct {

    uint32_t feature_mask;


    cam_denoise_param_t denoise2d;
    cam_rect_t input_crop;
    cam_rotation_t rotation;
    uint32_t flip;
    int32_t sharpness;
    int32_t effect;
    cam_hdr_param_t hdr_param;
    cam_scale_param_t scale_param;

    uint8_t zoom_level;
    cam_flash_value_t flash_value;
    cam_true_portrait_t tp_param;
} cam_pp_feature_config_t;

typedef struct {
    uint32_t input_stream_id;

    cam_stream_type_t input_stream_type;
} cam_pp_online_src_config_t;

typedef struct {

    cam_format_t input_fmt;


    cam_dimension_t input_dim;




    cam_stream_buf_plane_info_t input_buf_planes;


    uint8_t num_of_bufs;


    cam_stream_type_t input_type;
} cam_pp_offline_src_config_t;


typedef struct {

    cam_reprocess_type_enum_t pp_type;
    union {
        cam_pp_online_src_config_t online;
        cam_pp_offline_src_config_t offline;
    };


    cam_pp_feature_config_t pp_feature_config;
} cam_stream_reproc_config_t;

typedef struct {
    uint8_t crop_enabled;
    cam_rect_t input_crop;
} cam_crop_param_t;

typedef struct {
    uint8_t trigger;
    int32_t trigger_id;
} cam_trigger_t;

typedef struct {
    cam_denoise_param_t denoise;
    cam_crop_param_t crop;
    uint32_t flip;
    uint32_t uv_upsample;
    int32_t sharpness;
} cam_per_frame_pp_config_t;

typedef enum {
    CAM_OPT_STAB_OFF,
    CAM_OPT_STAB_ON,
    CAM_OPT_STAB_MAX
} cam_optical_stab_modes_t;

typedef enum {
    CAM_FILTER_ARRANGEMENT_RGGB,
    CAM_FILTER_ARRANGEMENT_GRBG,
    CAM_FILTER_ARRANGEMENT_GBRG,
    CAM_FILTER_ARRANGEMENT_BGGR,



    CAM_FILTER_ARRANGEMENT_RGB
} cam_color_filter_arrangement_t;

typedef enum {
    CAM_AF_STATE_INACTIVE,
    CAM_AF_STATE_PASSIVE_SCAN,
    CAM_AF_STATE_PASSIVE_FOCUSED,
    CAM_AF_STATE_ACTIVE_SCAN,
    CAM_AF_STATE_FOCUSED_LOCKED,
    CAM_AF_STATE_NOT_FOCUSED_LOCKED,
    CAM_AF_STATE_PASSIVE_UNFOCUSED
} cam_af_state_t;

typedef enum {
    CAM_AWB_STATE_INACTIVE,
    CAM_AWB_STATE_SEARCHING,
    CAM_AWB_STATE_CONVERGED,
    CAM_AWB_STATE_LOCKED
} cam_awb_state_t;
// # 36 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_intf.h" 2





typedef enum {

    CAM_PRIV_PARM = (0x08000000 + 14),

    CAM_PRIV_DO_AUTO_FOCUS,

    CAM_PRIV_CANCEL_AUTO_FOCUS,

    CAM_PRIV_PREPARE_SNAPSHOT,

    CAM_PRIV_STREAM_INFO_SYNC,

    CAM_PRIV_STREAM_PARM,

    CAM_PRIV_START_ZSL_SNAPSHOT,

    CAM_PRIV_STOP_ZSL_SNAPSHOT,
} cam_private_ioctl_enum_t;


typedef struct{
    cam_hal_version_t version;

    cam_position_t position;

    uint8_t auto_hdr_supported;


    size_t supported_iso_modes_cnt;
    cam_iso_mode_type supported_iso_modes[CAM_ISO_MODE_MAX];


    int32_t min_iso;
    int32_t max_iso;


    uint64_t min_exposure_time;
    uint64_t max_exposure_time;


    int32_t near_end_distance;


    size_t supported_flash_modes_cnt;
    cam_flash_mode_t supported_flash_modes[CAM_FLASH_MODE_MAX];

    size_t zoom_ratio_tbl_cnt;
    uint32_t zoom_ratio_tbl[79];


    size_t supported_effects_cnt;
    cam_effect_mode_type supported_effects[CAM_EFFECT_MODE_MAX];


    size_t supported_scene_modes_cnt;
    cam_scene_mode_type supported_scene_modes[CAM_SCENE_MODE_MAX];


    size_t supported_aec_modes_cnt;
    cam_auto_exposure_mode_type supported_aec_modes[CAM_AEC_MODE_MAX];

    size_t fps_ranges_tbl_cnt;
    cam_fps_range_t fps_ranges_tbl[24];


    size_t supported_antibandings_cnt;
    cam_antibanding_mode_type supported_antibandings[CAM_ANTIBANDING_MODE_MAX];


    size_t supported_white_balances_cnt;
    cam_wb_mode_type supported_white_balances[CAM_WB_MODE_MAX];


    int32_t min_wb_cct;
    int32_t max_wb_cct;


    float min_wb_gain;
    float max_wb_gain;


    size_t supported_focus_modes_cnt;
    cam_focus_mode_type supported_focus_modes[CAM_FOCUS_MODE_MAX];


    float min_focus_pos[CAM_MANUAL_FOCUS_MODE_MAX];
    float max_focus_pos[CAM_MANUAL_FOCUS_MODE_MAX];

    int32_t exposure_compensation_min;
    int32_t exposure_compensation_max;
    int32_t exposure_compensation_default;
    float exposure_compensation_step;
    cam_rational_type_t exp_compensation_step;

    uint8_t video_stablization_supported;

    size_t picture_sizes_tbl_cnt;
    cam_dimension_t picture_sizes_tbl[24];



    int32_t modes_supported;
    uint32_t sensor_mount_angle;

    float focal_length;
    float hor_view_angle;
    float ver_view_angle;

    size_t preview_sizes_tbl_cnt;
    cam_dimension_t preview_sizes_tbl[24];

    size_t video_sizes_tbl_cnt;
    cam_dimension_t video_sizes_tbl[24];

    size_t livesnapshot_sizes_tbl_cnt;
    cam_dimension_t livesnapshot_sizes_tbl[24];

    size_t vhdr_livesnapshot_sizes_tbl_cnt;
    cam_dimension_t vhdr_livesnapshot_sizes_tbl[24];

    size_t hfr_tbl_cnt;
    cam_hfr_info_t hfr_tbl[CAM_HFR_MODE_MAX];


    size_t supported_preview_fmt_cnt;
    cam_format_t supported_preview_fmts[CAM_FORMAT_MAX];


    size_t supported_picture_fmt_cnt;
    cam_format_t supported_picture_fmts[CAM_FORMAT_MAX];


    cam_dimension_t raw_dim;
    size_t supported_raw_fmt_cnt;
    cam_format_t supported_raw_fmts[CAM_FORMAT_MAX];


    size_t supported_focus_algos_cnt;
    cam_focus_algorithm_type supported_focus_algos[CAM_FOCUS_ALGO_MAX];


    uint8_t auto_wb_lock_supported;
    uint8_t zoom_supported;
    uint8_t smooth_zoom_supported;
    uint8_t auto_exposure_lock_supported;
    uint8_t video_snapshot_supported;

    uint8_t max_num_roi;
    uint8_t max_num_focus_areas;
    uint8_t max_num_metering_areas;
    uint8_t max_zoom_step;


    cam_control_range_t brightness_ctrl;
    cam_control_range_t sharpness_ctrl;
    cam_control_range_t contrast_ctrl;
    cam_control_range_t saturation_ctrl;
    cam_control_range_t sce_ctrl;


    cam_hdr_bracketing_info_t hdr_bracketing_setting;

    uint32_t qcom_supported_feature_mask;

    cam_padding_info_t padding_info;
    uint32_t min_num_pp_bufs;



    float min_focus_distance;
    float hyper_focal_distance;

    float focal_lengths[1];
    uint8_t focal_lengths_count;

    float apertures[1];
    uint8_t apertures_count;

    float filter_densities[1];
    uint8_t filter_densities_count;

    uint8_t optical_stab_modes[CAM_OPT_STAB_MAX];
    uint8_t optical_stab_modes_count;

    cam_dimension_t lens_shading_map_size;
    float lens_shading_map[3 * 6 *
              6];

    cam_dimension_t geo_correction_map_size;
    float geo_correction_map[2 * 3 * 6 *
              6];

    float lens_position[3];


    int64_t exposure_time_range[2];


    int64_t max_frame_duration;

    cam_color_filter_arrangement_t color_arrangement;

    float sensor_physical_size[2];



    cam_dimension_t pixel_array_size;


    cam_rect_t active_array_size;


    int32_t white_level;



    int32_t black_level_pattern[4];


    int64_t flash_charge_duration;



    int32_t max_tone_map_curve_points;


    size_t supported_scalar_format_cnt;
    cam_format_t supported_scalar_fmts[CAM_FORMAT_MAX];



    int64_t raw_min_duration;

    size_t supported_sizes_tbl_cnt;
    cam_dimension_t supported_sizes_tbl[24];





    int64_t min_duration[24];

    uint32_t max_face_detection_count;



    uint8_t is_sw_wnr;

    uint8_t histogram_supported;

    int32_t histogram_size;

    int32_t max_histogram_count;

    cam_dimension_t sharpness_map_size;


    int32_t max_sharpness_map_value;

    cam_scene_mode_overrides_t scene_mode_overrides[CAM_SCENE_MODE_MAX];


    size_t supported_ae_modes_cnt;
    cam_ae_mode_type supported_ae_modes[CAM_AE_MODE_MAX];


    size_t scale_picture_sizes_cnt;
    cam_dimension_t scale_picture_sizes[8];

    uint8_t flash_available;

    cam_rational_type_t base_gain_factor;

    cam_af_bracketing_t ubifocus_af_bracketing_need;
    cam_af_bracketing_t refocus_af_bracketing_need;

    cam_opti_zoom_t opti_zoom_settings_need;

    cam_true_portrait_t true_portrait_settings_need;

    cam_fssr_t fssr_settings_need;

    cam_af_bracketing_t mtf_af_bracketing_parm;

    cam_sensor_type_t sensor_type;

    uint8_t low_power_mode_supported;

    uint8_t use_pix_for_SOC;
} cam_capability_t;

typedef enum {
    CAM_STREAM_CONSUMER_DISPLAY,
    CAM_STREAM_CONSUMER_VIDEO_ENC,
    CAM_STREAM_CONSUMER_JPEG_ENC,
} cam_stream_consumer_t;

typedef enum {
    CAM_STREAM_PARAM_TYPE_DO_REPROCESS = CAM_INTF_PARM_DO_REPROCESS,
    CAM_STREAM_PARAM_TYPE_SET_BUNDLE_INFO = CAM_INTF_PARM_SET_BUNDLE,
    CAM_STREAM_PARAM_TYPE_SET_FLIP = CAM_INTF_PARM_STREAM_FLIP,
    CAM_STREAM_PARAM_SET_STREAM_CONSUMER,
    CAM_STREAM_PARAM_TYPE_GET_OUTPUT_CROP = CAM_INTF_PARM_GET_OUTPUT_CROP,
    CAM_STREAM_PARAM_TYPE_GET_IMG_PROP = CAM_INTF_PARM_GET_IMG_PROP,
    CAM_STREAM_PARAM_TYPE_MAX
} cam_stream_param_type_e;

typedef struct {
    uint32_t buf_index;

    uint32_t frame_idx;
    int32_t ret_val;

    uint8_t meta_present;
    uint32_t meta_stream_handle;
    uint32_t meta_buf_index;

    cam_per_frame_pp_config_t frame_pp_config;
} cam_reprocess_param;

typedef struct {
    uint32_t flip_mask;
} cam_flip_mode_t;


typedef struct {
    cam_rect_t crop;
    cam_dimension_t input;
    cam_dimension_t output;
    char name[32];
    uint32_t is_raw_image;
    cam_format_t format;
    uint32_t analysis_image;
    uint32_t size;
} cam_stream_img_prop_t;

typedef struct {
    cam_stream_param_type_e type;
    union {
        cam_reprocess_param reprocess;
        cam_bundle_config_t bundleInfo;
        cam_flip_mode_t flipInfo;
        cam_stream_consumer_t consumer;
        cam_crop_data_t outputCrop;
        cam_stream_img_prop_t imgProp;
    };
} cam_stream_parm_buffer_t;


typedef struct {

    uint32_t stream_svr_id;


    cam_stream_type_t stream_type;


    cam_format_t fmt;


    cam_dimension_t dim;




    cam_stream_buf_plane_info_t buf_planes;


    uint32_t num_bufs;


    cam_streaming_mode_t streaming_mode;


    uint8_t num_of_burst;


    cam_pp_feature_config_t pp_config;


    cam_stream_reproc_config_t reprocess_config;

    cam_stream_parm_buffer_t parm_buf;


    cam_is_type_t is_type;

} cam_stream_info_t;
// # 456 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_intf.h"
typedef union {



    int32_t member_variable_CAM_INTF_PARM_HAL_VERSION[ 1 ];

    int32_t member_variable_CAM_INTF_PARM_ANTIBANDING[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_EXPOSURE_COMPENSATION[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_AEC_LOCK[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_AEC_ENABLE[ 1 ];
    cam_fps_range_t member_variable_CAM_INTF_PARM_FPS_RANGE[ 1 ];
    uint8_t member_variable_CAM_INTF_PARM_FOCUS_MODE[ 1 ];
    cam_manual_focus_parm_t member_variable_CAM_INTF_PARM_MANUAL_FOCUS_POS[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_AWB_LOCK[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_AWB_ENABLE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_AF_ENABLE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_WHITE_BALANCE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_EFFECT[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_BESTSHOT_MODE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_DIS_ENABLE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_LED_MODE[ 1 ];


    int32_t member_variable_CAM_INTF_PARM_QUERY_FLASH4SNAP[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_EXPOSURE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_SHARPNESS[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_CONTRAST[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_SATURATION[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_BRIGHTNESS[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_ISO[ 1 ];
    uint64_t member_variable_CAM_INTF_PARM_EXPOSURE_TIME[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_ZOOM[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_ROLLOFF[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_MODE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_AEC_ALGO_TYPE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_FOCUS_ALGO_TYPE[ 1 ];
    cam_set_aec_roi_t member_variable_CAM_INTF_PARM_AEC_ROI[ 1 ];
    cam_roi_info_t member_variable_CAM_INTF_PARM_AF_ROI[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_SCE_FACTOR[ 1 ];
    cam_fd_set_parm_t member_variable_CAM_INTF_PARM_FD[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_MCE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_HFR[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_REDEYE_REDUCTION[ 1 ];
    cam_denoise_param_t member_variable_CAM_INTF_PARM_WAVELET_DENOISE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_HISTOGRAM[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_ASD_ENABLE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_RECORDING_HINT[ 1 ];
    cam_hdr_param_t member_variable_CAM_INTF_PARM_HDR[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_FRAMESKIP[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_ZSL_MODE[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_HDR_NEED_1X[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_LOCK_CAF[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_VIDEO_HDR[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_VT[ 1 ];
    tune_chromatix_t member_variable_CAM_INTF_PARM_GET_CHROMATIX[ 1 ];
    tune_chromatix_t member_variable_CAM_INTF_PARM_SET_RELOAD_CHROMATIX[ 1 ];
    tune_autofocus_t member_variable_CAM_INTF_PARM_GET_AFTUNE[ 1 ];
    tune_autofocus_t member_variable_CAM_INTF_PARM_SET_RELOAD_AFTUNE[ 1 ];
    tune_actuator_t member_variable_CAM_INTF_PARM_SET_AUTOFOCUSTUNING[ 1 ];
    tune_cmd_t member_variable_CAM_INTF_PARM_SET_VFE_COMMAND[ 1 ];
    tune_cmd_t member_variable_CAM_INTF_PARM_SET_PP_COMMAND[ 1 ];
    cam_dimension_t member_variable_CAM_INTF_PARM_MAX_DIMENSION[ 1 ];
    cam_dimension_t member_variable_CAM_INTF_PARM_RAW_DIMENSION[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_TINTLESS[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_CDS_MODE[ 1 ];
    cam_manual_wb_parm_t member_variable_CAM_INTF_PARM_WB_MANUAL[ 1 ];
    cam_eztune_cmd_data_t member_variable_CAM_INTF_PARM_EZTUNE_CMD[ 1 ];
    int8_t member_variable_CAM_INTF_PARM_LONGSHOT_ENABLE[ 1 ];
    int8_t member_variable_CAM_INTF_PARM_LOW_POWER_ENABLE[ 1 ];


    uint32_t member_variable_CAM_INTF_META_FRAME_NUMBER[ 1 ];
    uint8_t member_variable_CAM_INTF_META_COLOR_CORRECT_MODE[ 1 ];
    cam_color_correct_matrix_t member_variable_CAM_INTF_META_COLOR_CORRECT_TRANSFORM[ 1 ];
    uint8_t member_variable_CAM_INTF_META_AEC_MODE[ 1 ];
    cam_area_t member_variable_CAM_INTF_META_AEC_ROI[ 5 ];
    cam_trigger_t member_variable_CAM_INTF_META_AEC_PRECAPTURE_TRIGGER[ 1 ];
    cam_area_t member_variable_CAM_INTF_META_AF_ROI[ 5 ];
    cam_trigger_t member_variable_CAM_INTF_META_AF_TRIGGER[ 1 ];
    cam_area_t member_variable_CAM_INTF_META_AWB_REGIONS[ 5 ];
    uint8_t member_variable_CAM_INTF_META_CAPTURE_INTENT[ 1 ];
    uint8_t member_variable_CAM_INTF_META_MODE[ 1 ];
    int32_t member_variable_CAM_INTF_META_DEMOSAIC[ 1 ];
    int32_t member_variable_CAM_INTF_META_EDGE[ 1 ];
    int32_t member_variable_CAM_INTF_META_SHARPNESS_STRENGTH[ 1 ];
    uint8_t member_variable_CAM_INTF_META_FLASH_POWER[ 1 ];
    int64_t member_variable_CAM_INTF_META_FLASH_FIRING_TIME[ 1 ];
    uint8_t member_variable_CAM_INTF_META_GEOMETRIC_MODE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_GEOMETRIC_STRENGTH[ 1 ];
    uint8_t member_variable_CAM_INTF_META_HOTPIXEL_MODE[ 1 ];
    float member_variable_CAM_INTF_META_LENS_APERTURE[ 1 ];
    float member_variable_CAM_INTF_META_LENS_FILTERDENSITY[ 1 ];
    float member_variable_CAM_INTF_META_LENS_FOCAL_LENGTH[ 1 ];
    float member_variable_CAM_INTF_META_LENS_FOCUS_DISTANCE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_LENS_OPT_STAB_MODE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_NOISE_REDUCTION_MODE[ 1 ];
    int32_t member_variable_CAM_INTF_META_NOISE_REDUCTION_STRENGTH[ 1 ];
    cam_crop_region_t member_variable_CAM_INTF_META_SCALER_CROP_REGION[ 1 ];
    int64_t member_variable_CAM_INTF_META_SENSOR_EXPOSURE_TIME[ 1 ];
    int64_t member_variable_CAM_INTF_META_SENSOR_FRAME_DURATION[ 1 ];
    int32_t member_variable_CAM_INTF_META_SENSOR_SENSITIVITY[ 1 ];
    uint8_t member_variable_CAM_INTF_META_SHADING_MODE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_SHADING_STRENGTH[ 1 ];
    uint8_t member_variable_CAM_INTF_META_STATS_FACEDETECT_MODE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_STATS_HISTOGRAM_MODE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_STATS_SHARPNESS_MAP_MODE[ 1 ];
    cam_tonemap_curve_t member_variable_CAM_INTF_META_TONEMAP_CURVE_BLUE[ 1 ];
    cam_tonemap_curve_t member_variable_CAM_INTF_META_TONEMAP_CURVE_GREEN[ 1 ];
    cam_tonemap_curve_t member_variable_CAM_INTF_META_TONEMAP_CURVE_RED[ 1 ];
    uint8_t member_variable_CAM_INTF_META_TONEMAP_MODE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_FLASH_MODE[ 1 ];
    uint32_t member_variable_CAM_INTF_PARM_STATS_DEBUG_MASK[ 1 ];
    uint32_t member_variable_CAM_INTF_PARM_ISP_DEBUG_MASK[ 1 ];
    uint32_t member_variable_CAM_INTF_PARM_ALGO_OPTIMIZATIONS_MASK[ 1 ];
    uint32_t member_variable_CAM_INTF_PARM_SENSOR_DEBUG_MASK[ 1 ];
    cam_af_bracketing_t member_variable_CAM_INTF_PARM_FOCUS_BRACKETING[ 1 ];
    cam_af_bracketing_t member_variable_CAM_INTF_PARM_MULTI_TOUCH_FOCUS_BRACKETING[ 1 ];
    cam_flash_bracketing_t member_variable_CAM_INTF_PARM_FLASH_BRACKETING[ 1 ];
} parm_type_t;

typedef union {




    cam_hist_stats_t member_variable_CAM_INTF_META_HISTOGRAM[ 1 ];
    cam_face_detection_data_t member_variable_CAM_INTF_META_FACE_DETECTION[ 1 ];
    cam_auto_focus_data_t member_variable_CAM_INTF_META_AUTOFOCUS_DATA[ 1 ];


    cam_crop_data_t member_variable_CAM_INTF_META_CROP_DATA[ 1 ];
    int32_t member_variable_CAM_INTF_META_PREP_SNAPSHOT_DONE[ 1 ];
    cam_frame_idx_range_t member_variable_CAM_INTF_META_GOOD_FRAME_IDX_RANGE[ 1 ];

    int32_t member_variable_CAM_INTF_META_FRAME_NUMBER_VALID[ 1 ];
    uint32_t member_variable_CAM_INTF_META_FRAME_NUMBER[ 1 ];
    uint8_t member_variable_CAM_INTF_META_COLOR_CORRECT_MODE[ 1 ];
    int32_t member_variable_CAM_INTF_META_AEC_PRECAPTURE_ID[ 1 ];
    cam_area_t member_variable_CAM_INTF_META_AEC_ROI[ 5 ];
    uint8_t member_variable_CAM_INTF_META_AEC_STATE[ 1 ];
    uint8_t member_variable_CAM_INTF_PARM_FOCUS_MODE[ 1 ];
    cam_area_t member_variable_CAM_INTF_META_AF_ROI[ 5 ];
    uint8_t member_variable_CAM_INTF_META_AF_STATE[ 1 ];
    int32_t member_variable_CAM_INTF_META_AF_TRIGGER_ID[ 1 ];
    int32_t member_variable_CAM_INTF_PARM_WHITE_BALANCE[ 1 ];
    cam_area_t member_variable_CAM_INTF_META_AWB_REGIONS[ 5 ];
    uint8_t member_variable_CAM_INTF_META_AWB_STATE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_MODE[ 1 ];
    int32_t member_variable_CAM_INTF_META_EDGE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_FLASH_POWER[ 1 ];
    int64_t member_variable_CAM_INTF_META_FLASH_FIRING_TIME[ 1 ];
    uint8_t member_variable_CAM_INTF_META_FLASH_MODE[ 1 ];
    int32_t member_variable_CAM_INTF_META_FLASH_STATE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_HOTPIXEL_MODE[ 1 ];
    float member_variable_CAM_INTF_META_LENS_APERTURE[ 1 ];
    float member_variable_CAM_INTF_META_LENS_FILTERDENSITY[ 1 ];
    float member_variable_CAM_INTF_META_LENS_FOCAL_LENGTH[ 1 ];
    float member_variable_CAM_INTF_META_LENS_FOCUS_DISTANCE[ 1 ];
    float member_variable_CAM_INTF_META_LENS_FOCUS_RANGE[ 2 ];
    uint8_t member_variable_CAM_INTF_META_LENS_OPT_STAB_MODE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_LENS_FOCUS_STATE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_NOISE_REDUCTION_MODE[ 1 ];
    cam_crop_region_t member_variable_CAM_INTF_META_SCALER_CROP_REGION[ 1 ];
    int64_t member_variable_CAM_INTF_META_SENSOR_EXPOSURE_TIME[ 1 ];
    int64_t member_variable_CAM_INTF_META_SENSOR_FRAME_DURATION[ 1 ];
    int32_t member_variable_CAM_INTF_META_SENSOR_SENSITIVITY[ 1 ];
    struct timeval member_variable_CAM_INTF_META_SENSOR_TIMESTAMP[ 1 ];
    uint8_t member_variable_CAM_INTF_META_SHADING_MODE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_STATS_FACEDETECT_MODE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_STATS_HISTOGRAM_MODE[ 1 ];
    uint8_t member_variable_CAM_INTF_META_STATS_SHARPNESS_MAP_MODE[ 1 ];
    cam_sharpness_map_t member_variable_CAM_INTF_META_STATS_SHARPNESS_MAP[ 3 ];
    cam_asd_hdr_scene_data_t member_variable_CAM_INTF_META_ASD_HDR_SCENE_DATA[ 1 ];
    char member_variable_CAM_INTF_META_PRIVATE_DATA[ (1024) ];

} metadata_type_t;



typedef struct {
    metadata_type_t data;
    uint8_t next_flagged_entry;
} metadata_entry_type_t;

typedef struct {
    uint8_t first_flagged_entry;
    metadata_entry_type_t entry[CAM_INTF_PARM_MAX];
} metadata_buffer_t;

typedef struct {
    parm_type_t data;
    uint8_t next_flagged_entry;
} parm_entry_type_t;


typedef struct {
    cam_intf_parm_type_t entry_type;
    size_t size;
    size_t aligned_size;
    char data[1];
} parm_entry_type_new_t;

typedef struct {
    uint8_t first_flagged_entry;
    parm_entry_type_t entry[CAM_INTF_PARM_MAX];
} parm_buffer_t;

typedef struct {
    size_t num_entry;
    size_t tot_rem_size;
    size_t curr_size;
    char entry[1];
} parm_buffer_new_t;




void *POINTER_OF_PARAM(cam_intf_parm_type_t PARAM_ID,
                    void *TABLE_PTR);
// # 36 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h" 2
// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_queue.h" 1
// # 30 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_queue.h"
// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_list.h" 1
// # 36 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_list.h"
// # 1 "prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9/include/stddef.h" 1 3 4
// # 37 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_list.h" 2







struct cam_list {
  struct cam_list *next, *prev;
};

static inline void cam_list_init(struct cam_list *ptr)
{
  ptr->next = ptr;
  ptr->prev = ptr;
}

static inline void cam_list_add_tail_node(struct cam_list *item,
  struct cam_list *head)
{
  struct cam_list *prev = head->prev;

  head->prev = item;
  item->next = head;
  item->prev = prev;
  prev->next = item;
}

static inline void cam_list_insert_before_node(struct cam_list *item,
  struct cam_list *node)
{
  item->next = node;
  item->prev = node->prev;
  item->prev->next = item;
  node->prev = item;
}

static inline void cam_list_del_node(struct cam_list *ptr)
{
  struct cam_list *prev = ptr->prev;
  struct cam_list *next = ptr->next;

  next->prev = ptr->prev;
  prev->next = ptr->next;
  ptr->next = ptr;
  ptr->prev = ptr;
}
// # 31 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/cam_queue.h" 2

typedef struct {
    struct cam_list list;
    void *data;
} cam_node_t;

typedef struct {
    cam_node_t head;
    uint32_t size;
    pthread_mutex_t lock;
} cam_queue_t;

static inline int32_t cam_queue_init(cam_queue_t *queue)
{
    pthread_mutex_init(&queue->lock, ((void *)0));
    cam_list_init(&queue->head.list);
    queue->size = 0;
    return 0;
}

static inline int32_t cam_queue_enq(cam_queue_t *queue, void *data)
{
    cam_node_t *node =
        (cam_node_t *)malloc(sizeof(cam_node_t));
    if (((void *)0) == node) {
        return -1;
    }

    memset(node, 0, sizeof(cam_node_t));
    node->data = data;

    pthread_mutex_lock(&queue->lock);
    cam_list_add_tail_node(&node->list, &queue->head.list);
    queue->size++;
    pthread_mutex_unlock(&queue->lock);

    return 0;
}

static inline void *cam_queue_deq(cam_queue_t *queue)
{
    cam_node_t *node = ((void *)0);
    void *data = ((void *)0);
    struct cam_list *head = ((void *)0);
    struct cam_list *pos = ((void *)0);

    pthread_mutex_lock(&queue->lock);
    head = &queue->head.list;
    pos = head->next;
    if (pos != head) {
        node = ({ const typeof(((cam_node_t *)0)->list) *__mptr = (pos); (cam_node_t *)((char *)__mptr - __builtin_offsetof (cam_node_t, list));});
        cam_list_del_node(&node->list);
        queue->size--;
    }
    pthread_mutex_unlock(&queue->lock);

    if (((void *)0) != node) {
        data = node->data;
        free(node);
    }

    return data;
}

static inline int32_t cam_queue_flush(cam_queue_t *queue)
{
    cam_node_t *node = ((void *)0);
    struct cam_list *head = ((void *)0);
    struct cam_list *pos = ((void *)0);

    pthread_mutex_lock(&queue->lock);
    head = &queue->head.list;
    pos = head->next;

    while(pos != head) {
        node = ({ const typeof(((cam_node_t *)0)->list) *__mptr = (pos); (cam_node_t *)((char *)__mptr - __builtin_offsetof (cam_node_t, list));});
        pos = pos->next;
        cam_list_del_node(&node->list);
        queue->size--;




        if (((void *)0) != node->data) {
            free(node->data);
        }
        free(node);

    }
    queue->size = 0;
    pthread_mutex_unlock(&queue->lock);
    return 0;
}

static inline int32_t cam_queue_deinit(cam_queue_t *queue)
{
    cam_queue_flush(queue);
    pthread_mutex_destroy(&queue->lock);
    return 0;
}
// # 37 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h" 2
// # 95 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
typedef struct {
    uint32_t stream_id;
    cam_stream_type_t stream_type;
    uint32_t buf_idx;
    uint8_t is_uv_subsampled;
    struct timespec ts;
    uint32_t frame_idx;
    int8_t num_planes;
    struct v4l2_plane planes[8];
    int fd;
    void *buffer;
    size_t frame_len;
    void *mem_info;
} mm_camera_buf_def_t;
// # 120 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
typedef struct {
    uint32_t camera_handle;
    uint32_t ch_id;
    uint32_t num_bufs;
    mm_camera_buf_def_t* bufs[4];
} mm_camera_super_buf_t;







typedef struct {
    cam_event_type_t server_event_type;
    uint32_t status;
} mm_camera_event_t;







typedef void (*mm_camera_event_notify_t)(uint32_t camera_handle,
                                         mm_camera_event_t *evt,
                                         void *user_data);






typedef void (*mm_camera_buf_notify_t) (mm_camera_super_buf_t *bufs,
                                        void *user_data);
// # 166 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
typedef int32_t (*map_stream_buf_op_t) (uint32_t frame_idx,
                                        int32_t plane_idx,
                                        int fd,
                                        size_t size,
                                        void *userdata);
// # 181 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
typedef int32_t (*unmap_stream_buf_op_t) (uint32_t frame_idx,
                                          int32_t plane_idx,
                                          void *userdata);
// # 192 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
typedef struct {
    map_stream_buf_op_t map_ops;
    unmap_stream_buf_op_t unmap_ops;
    void *userdata;
} mm_camera_map_unmap_ops_tbl_t;
// # 206 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
typedef struct {
  void *user_data;
  int32_t (*get_bufs) (cam_frame_len_offset_t *offset,
                       uint8_t *num_bufs,
                       uint8_t **initial_reg_flag,
                       mm_camera_buf_def_t **bufs,
                       mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                       void *user_data);
  int32_t (*put_bufs) (mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                       void *user_data);
  int32_t (*invalidate_buf)(uint32_t index, void *user_data);
  int32_t (*clean_invalidate_buf)(uint32_t index, void *user_data);
} mm_camera_stream_mem_vtbl_t;
// # 229 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
typedef struct {
    cam_stream_info_t *stream_info;
    cam_padding_info_t padding_info;
    mm_camera_stream_mem_vtbl_t mem_vtbl;
    mm_camera_buf_notify_t stream_cb;
    void *userdata;
} mm_camera_stream_config_t;
// # 245 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
typedef enum {
    MM_CAMERA_SUPER_BUF_NOTIFY_BURST = 0,
    MM_CAMERA_SUPER_BUF_NOTIFY_CONTINUOUS,
    MM_CAMERA_SUPER_BUF_NOTIFY_MAX
} mm_camera_super_buf_notify_mode_t;
// # 262 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
typedef enum {
    MM_CAMERA_SUPER_BUF_PRIORITY_NORMAL = 0,
    MM_CAMERA_SUPER_BUF_PRIORITY_FOCUS,
    MM_CAMERA_SUPER_BUF_PRIORITY_EXPOSURE_BRACKETING,
    MM_CAMERA_SUPER_BUF_PRIORITY_MAX
} mm_camera_super_buf_priority_t;
// # 279 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
typedef enum {
   MM_CAMERA_AF_BRACKETING = 0,
   MM_CAMERA_AE_BRACKETING,
   MM_CAMERA_FLASH_BRACKETING,
   MM_CAMERA_MTF_BRACKETING,
   MM_CAMERA_ZOOM_1X,
} mm_camera_advanced_capture_t;
// # 300 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
typedef struct {
    mm_camera_super_buf_notify_mode_t notify_mode;
    uint8_t water_mark;
    uint8_t look_back;
    uint8_t post_frame_skip;
    uint8_t max_unmatched_frames;
    mm_camera_super_buf_priority_t priority;
} mm_camera_channel_attr_t;

typedef struct {







    int32_t (*query_capability) (uint32_t camera_handle);
// # 327 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*register_event_notify) (uint32_t camera_handle,
                                      mm_camera_event_notify_t evt_cb,
                                      void *user_data);






    int32_t (*close_camera) (uint32_t camera_handle);
// # 350 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*map_buf) (uint32_t camera_handle,
                        uint8_t buf_type,
                        int fd,
                        size_t size);
// # 365 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*unmap_buf) (uint32_t camera_handle,
                          uint8_t buf_type);
// # 379 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*set_parms) (uint32_t camera_handle,
                          void *parms);
// # 393 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*get_parms) (uint32_t camera_handle,
                          void *parms);
// # 403 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*do_auto_focus) (uint32_t camera_handle);







    int32_t (*cancel_auto_focus) (uint32_t camera_handle);
// # 422 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*prepare_snapshot) (uint32_t camera_handle,
                                 int32_t do_af_flag);
// # 432 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*start_zsl_snapshot) (uint32_t camera_handle, uint32_t ch_id);
// # 441 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*stop_zsl_snapshot) (uint32_t camera_handle, uint32_t ch_id);
// # 453 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    uint32_t (*add_channel) (uint32_t camera_handle,
                             mm_camera_channel_attr_t *attr,
                             mm_camera_buf_notify_t channel_cb,
                             void *userdata);







    int32_t (*delete_channel) (uint32_t camera_handle,
                               uint32_t ch_id);
// # 475 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*get_bundle_info) (uint32_t camera_handle,
                                uint32_t ch_id,
                                cam_bundle_config_t *bundle_info);






    uint32_t (*add_stream) (uint32_t camera_handle,
                            uint32_t ch_id);
// # 494 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*delete_stream) (uint32_t camera_handle,
                              uint32_t ch_id,
                              uint32_t stream_id);
// # 506 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*link_stream) (uint32_t camera_handle,
          uint32_t ch_id,
          uint32_t stream_id,
          uint32_t linked_ch_id);
// # 519 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*config_stream) (uint32_t camera_handle,
                              uint32_t ch_id,
                              uint32_t stream_id,
                              mm_camera_stream_config_t *config);
// # 542 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*map_stream_buf) (uint32_t camera_handle,
                               uint32_t ch_id,
                               uint32_t stream_id,
                               uint8_t buf_type,
                               uint32_t buf_idx,
                               int32_t plane_idx,
                               int fd,
                               size_t size);
// # 567 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*unmap_stream_buf) (uint32_t camera_handle,
                                 uint32_t ch_id,
                                 uint32_t stream_id,
                                 uint8_t buf_type,
                                 uint32_t buf_idx,
                                 int32_t plane_idx);
// # 586 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*set_stream_parms) (uint32_t camera_handle,
                                 uint32_t ch_id,
                                 uint32_t s_id,
                                 cam_stream_parm_buffer_t *parms);
// # 603 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*get_stream_parms) (uint32_t camera_handle,
                                 uint32_t ch_id,
                                 uint32_t s_id,
                                 cam_stream_parm_buffer_t *parms);
// # 615 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*start_channel) (uint32_t camera_handle,
                              uint32_t ch_id);
// # 625 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*stop_channel) (uint32_t camera_handle,
                             uint32_t ch_id);
// # 636 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*qbuf) (uint32_t camera_handle,
                     uint32_t ch_id,
                     mm_camera_buf_def_t *buf);







    int32_t (*get_queued_buf_count) (uint32_t camera_handle,
            uint32_t ch_id,
            uint32_t stream_id);
// # 658 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*request_super_buf) (uint32_t camera_handle,
                                  uint32_t ch_id,
                                  uint32_t num_buf_requested);
// # 670 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*cancel_super_buf_request) (uint32_t camera_handle,
                                         uint32_t ch_id);
// # 683 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*flush_super_buf_queue) (uint32_t camera_handle,
                                      uint32_t ch_id, uint32_t frame_idx);
// # 694 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
    int32_t (*configure_notify_mode) (uint32_t camera_handle,
                                      uint32_t ch_id,
                                      mm_camera_super_buf_notify_mode_t notify_mode);
// # 709 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_camera_interface.h"
     int32_t (*process_advanced_capture) (uint32_t camera_handle,
                                          mm_camera_advanced_capture_t type,
                                          uint32_t ch_id,
                                          int8_t start_flag);
} mm_camera_ops_t;






typedef struct {
    uint32_t camera_handle;
    mm_camera_ops_t *ops;
} mm_camera_vtbl_t;


uint8_t get_num_of_cameras();


mm_camera_vtbl_t * camera_open(uint8_t camera_idx);
struct camera_info *get_cam_info(uint32_t camera_id);


int32_t mm_stream_calc_offset_preview(cam_stream_info_t *stream_info,
        cam_dimension_t *dim,
        cam_padding_info_t *padding,
        cam_stream_buf_plane_info_t *buf_planes);

int32_t mm_stream_calc_offset_post_view(cam_format_t fmt,
        cam_dimension_t *dim,
        cam_stream_buf_plane_info_t *buf_planes);

int32_t mm_stream_calc_offset_snapshot(cam_stream_info_t *stream_info,
        cam_dimension_t *dim,
        cam_padding_info_t *padding,
        cam_stream_buf_plane_info_t *buf_planes);

int32_t mm_stream_calc_offset_raw(cam_format_t fmt,
        cam_dimension_t *dim,
        cam_padding_info_t *padding,
        cam_stream_buf_plane_info_t *buf_planes);

int32_t mm_stream_calc_offset_video(cam_dimension_t *dim,
        cam_stream_buf_plane_info_t *buf_planes);

int32_t mm_stream_calc_offset_metadata(cam_dimension_t *dim,
        cam_padding_info_t *padding,
        cam_stream_buf_plane_info_t *buf_planes);

int32_t mm_stream_calc_offset_postproc(cam_stream_info_t *stream_info,
        cam_padding_info_t *padding,
        cam_stream_buf_plane_info_t *buf_planes);

uint8_t check_cam_access(uint8_t camera_idx);
// # 45 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 2
// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_jpeg_interface.h" 1
// # 32 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_jpeg_interface.h"
// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qomx_core/QOMX_JpegExtensions.h" 1
// # 35 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qomx_core/QOMX_JpegExtensions.h"
// # 1 "frameworks/native/include/media/openmax/OMX_Image.h" 1
// # 59 "frameworks/native/include/media/openmax/OMX_Image.h"
// # 1 "frameworks/native/include/media/openmax/OMX_IVCommon.h" 1
// # 59 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
// # 1 "frameworks/native/include/media/openmax/OMX_Core.h" 1
// # 59 "frameworks/native/include/media/openmax/OMX_Core.h"
// # 1 "frameworks/native/include/media/openmax/OMX_Index.h" 1
// # 58 "frameworks/native/include/media/openmax/OMX_Index.h"
// # 1 "frameworks/native/include/media/openmax/OMX_Types.h" 1
// # 153 "frameworks/native/include/media/openmax/OMX_Types.h"
typedef unsigned char OMX_U8;


typedef signed char OMX_S8;


typedef unsigned short OMX_U16;


typedef signed short OMX_S16;


typedef uint32_t OMX_U32;


typedef int32_t OMX_S32;
// # 196 "frameworks/native/include/media/openmax/OMX_Types.h"
typedef unsigned long long OMX_U64;


typedef signed long long OMX_S64;
// # 209 "frameworks/native/include/media/openmax/OMX_Types.h"
typedef enum OMX_BOOL {
    OMX_FALSE = 0,
    OMX_TRUE = !OMX_FALSE,
    OMX_BOOL_MAX = 0x7FFFFFFF
} OMX_BOOL;
// # 237 "frameworks/native/include/media/openmax/OMX_Types.h"
typedef void* OMX_PTR;






typedef char* OMX_STRING;






typedef unsigned char* OMX_BYTE;







typedef unsigned char OMX_UUIDTYPE[128];




typedef enum OMX_DIRTYPE
{
    OMX_DirInput,
    OMX_DirOutput,
    OMX_DirMax = 0x7FFFFFFF
} OMX_DIRTYPE;




typedef enum OMX_ENDIANTYPE
{
    OMX_EndianBig,
    OMX_EndianLittle,
    OMX_EndianMax = 0x7FFFFFFF
} OMX_ENDIANTYPE;
// # 290 "frameworks/native/include/media/openmax/OMX_Types.h"
typedef enum OMX_NUMERICALDATATYPE
{
    OMX_NumericalDataSigned,
    OMX_NumericalDataUnsigned,
    OMX_NumericalDataFloat = 0x7F000001,
    OMX_NumercialDataMax = 0x7FFFFFFF
} OMX_NUMERICALDATATYPE;



typedef struct OMX_BU32 {
    OMX_U32 nValue;
    OMX_U32 nMin;
    OMX_U32 nMax;
} OMX_BU32;



typedef struct OMX_BS32 {
    OMX_S32 nValue;
    OMX_S32 nMin;
    OMX_S32 nMax;
} OMX_BS32;
// # 328 "frameworks/native/include/media/openmax/OMX_Types.h"
typedef OMX_S64 OMX_TICKS;
// # 341 "frameworks/native/include/media/openmax/OMX_Types.h"
typedef OMX_PTR OMX_HANDLETYPE;

typedef struct OMX_MARKTYPE
{
    OMX_HANDLETYPE hMarkTargetComponent;


    OMX_PTR pMarkData;


} OMX_MARKTYPE;





typedef OMX_PTR OMX_NATIVE_DEVICETYPE;



typedef OMX_PTR OMX_NATIVE_WINDOWTYPE;
// # 374 "frameworks/native/include/media/openmax/OMX_Types.h"
typedef union OMX_VERSIONTYPE
{
    struct
    {
        OMX_U8 nVersionMajor;
        OMX_U8 nVersionMinor;
        OMX_U8 nRevision;
        OMX_U8 nStep;
    } s;
    OMX_U32 nVersion;


} OMX_VERSIONTYPE;
// # 59 "frameworks/native/include/media/openmax/OMX_Index.h" 2
// # 75 "frameworks/native/include/media/openmax/OMX_Index.h"
typedef enum OMX_INDEXTYPE {

    OMX_IndexComponentStartUnused = 0x01000000,
    OMX_IndexParamPriorityMgmt,
    OMX_IndexParamAudioInit,
    OMX_IndexParamImageInit,
    OMX_IndexParamVideoInit,
    OMX_IndexParamOtherInit,
    OMX_IndexParamNumAvailableStreams,
    OMX_IndexParamActiveStream,
    OMX_IndexParamSuspensionPolicy,
    OMX_IndexParamComponentSuspended,
    OMX_IndexConfigCapturing,
    OMX_IndexConfigCaptureMode,
    OMX_IndexAutoPauseAfterCapture,
    OMX_IndexParamContentURI,
    OMX_IndexParamCustomContentPipe,
    OMX_IndexParamDisableResourceConcealment,
    OMX_IndexConfigMetadataItemCount,
    OMX_IndexConfigContainerNodeCount,
    OMX_IndexConfigMetadataItem,
    OMX_IndexConfigCounterNodeID,
    OMX_IndexParamMetadataFilterType,
    OMX_IndexParamMetadataKeyFilter,
    OMX_IndexConfigPriorityMgmt,
    OMX_IndexParamStandardComponentRole,

    OMX_IndexPortStartUnused = 0x02000000,
    OMX_IndexParamPortDefinition,
    OMX_IndexParamCompBufferSupplier,
    OMX_IndexReservedStartUnused = 0x03000000,


    OMX_IndexAudioStartUnused = 0x04000000,
    OMX_IndexParamAudioPortFormat,
    OMX_IndexParamAudioPcm,
    OMX_IndexParamAudioAac,
    OMX_IndexParamAudioRa,
    OMX_IndexParamAudioMp3,
    OMX_IndexParamAudioAdpcm,
    OMX_IndexParamAudioG723,
    OMX_IndexParamAudioG729,
    OMX_IndexParamAudioAmr,
    OMX_IndexParamAudioWma,
    OMX_IndexParamAudioSbc,
    OMX_IndexParamAudioMidi,
    OMX_IndexParamAudioGsm_FR,
    OMX_IndexParamAudioMidiLoadUserSound,
    OMX_IndexParamAudioG726,
    OMX_IndexParamAudioGsm_EFR,
    OMX_IndexParamAudioGsm_HR,
    OMX_IndexParamAudioPdc_FR,
    OMX_IndexParamAudioPdc_EFR,
    OMX_IndexParamAudioPdc_HR,
    OMX_IndexParamAudioTdma_FR,
    OMX_IndexParamAudioTdma_EFR,
    OMX_IndexParamAudioQcelp8,
    OMX_IndexParamAudioQcelp13,
    OMX_IndexParamAudioEvrc,
    OMX_IndexParamAudioSmv,
    OMX_IndexParamAudioVorbis,
    OMX_IndexParamAudioFlac,

    OMX_IndexConfigAudioMidiImmediateEvent,
    OMX_IndexConfigAudioMidiControl,
    OMX_IndexConfigAudioMidiSoundBankProgram,
    OMX_IndexConfigAudioMidiStatus,
    OMX_IndexConfigAudioMidiMetaEvent,
    OMX_IndexConfigAudioMidiMetaEventData,
    OMX_IndexConfigAudioVolume,
    OMX_IndexConfigAudioBalance,
    OMX_IndexConfigAudioChannelMute,
    OMX_IndexConfigAudioMute,
    OMX_IndexConfigAudioLoudness,
    OMX_IndexConfigAudioEchoCancelation,
    OMX_IndexConfigAudioNoiseReduction,
    OMX_IndexConfigAudioBass,
    OMX_IndexConfigAudioTreble,
    OMX_IndexConfigAudioStereoWidening,
    OMX_IndexConfigAudioChorus,
    OMX_IndexConfigAudioEqualizer,
    OMX_IndexConfigAudioReverberation,
    OMX_IndexConfigAudioChannelVolume,


    OMX_IndexImageStartUnused = 0x05000000,
    OMX_IndexParamImagePortFormat,
    OMX_IndexParamFlashControl,
    OMX_IndexConfigFocusControl,
    OMX_IndexParamQFactor,
    OMX_IndexParamQuantizationTable,
    OMX_IndexParamHuffmanTable,
    OMX_IndexConfigFlashControl,


    OMX_IndexVideoStartUnused = 0x06000000,
    OMX_IndexParamVideoPortFormat,
    OMX_IndexParamVideoQuantization,
    OMX_IndexParamVideoFastUpdate,
    OMX_IndexParamVideoBitrate,
    OMX_IndexParamVideoMotionVector,
    OMX_IndexParamVideoIntraRefresh,
    OMX_IndexParamVideoErrorCorrection,
    OMX_IndexParamVideoVBSMC,
    OMX_IndexParamVideoMpeg2,
    OMX_IndexParamVideoMpeg4,
    OMX_IndexParamVideoWmv,
    OMX_IndexParamVideoRv,
    OMX_IndexParamVideoAvc,
    OMX_IndexParamVideoH263,
    OMX_IndexParamVideoProfileLevelQuerySupported,
    OMX_IndexParamVideoProfileLevelCurrent,
    OMX_IndexConfigVideoBitrate,
    OMX_IndexConfigVideoFramerate,
    OMX_IndexConfigVideoIntraVOPRefresh,
    OMX_IndexConfigVideoIntraMBRefresh,
    OMX_IndexConfigVideoMBErrorReporting,
    OMX_IndexParamVideoMacroblocksPerFrame,
    OMX_IndexConfigVideoMacroBlockErrorMap,
    OMX_IndexParamVideoSliceFMO,
    OMX_IndexConfigVideoAVCIntraPeriod,
    OMX_IndexConfigVideoNalSize,


    OMX_IndexCommonStartUnused = 0x07000000,
    OMX_IndexParamCommonDeblocking,
    OMX_IndexParamCommonSensorMode,
    OMX_IndexParamCommonInterleave,
    OMX_IndexConfigCommonColorFormatConversion,
    OMX_IndexConfigCommonScale,
    OMX_IndexConfigCommonImageFilter,
    OMX_IndexConfigCommonColorEnhancement,
    OMX_IndexConfigCommonColorKey,
    OMX_IndexConfigCommonColorBlend,
    OMX_IndexConfigCommonFrameStabilisation,
    OMX_IndexConfigCommonRotate,
    OMX_IndexConfigCommonMirror,
    OMX_IndexConfigCommonOutputPosition,
    OMX_IndexConfigCommonInputCrop,
    OMX_IndexConfigCommonOutputCrop,
    OMX_IndexConfigCommonDigitalZoom,
    OMX_IndexConfigCommonOpticalZoom,
    OMX_IndexConfigCommonWhiteBalance,
    OMX_IndexConfigCommonExposure,
    OMX_IndexConfigCommonContrast,
    OMX_IndexConfigCommonBrightness,
    OMX_IndexConfigCommonBacklight,
    OMX_IndexConfigCommonGamma,
    OMX_IndexConfigCommonSaturation,
    OMX_IndexConfigCommonLightness,
    OMX_IndexConfigCommonExclusionRect,
    OMX_IndexConfigCommonDithering,
    OMX_IndexConfigCommonPlaneBlend,
    OMX_IndexConfigCommonExposureValue,
    OMX_IndexConfigCommonOutputSize,
    OMX_IndexParamCommonExtraQuantData,
    OMX_IndexConfigCommonFocusRegion,
    OMX_IndexConfigCommonFocusStatus,
    OMX_IndexConfigCommonTransitionEffect,


    OMX_IndexOtherStartUnused = 0x08000000,
    OMX_IndexParamOtherPortFormat,
    OMX_IndexConfigOtherPower,
    OMX_IndexConfigOtherStats,



    OMX_IndexTimeStartUnused = 0x09000000,
    OMX_IndexConfigTimeScale,
    OMX_IndexConfigTimeClockState,
    OMX_IndexConfigTimeActiveRefClock,
    OMX_IndexConfigTimeCurrentMediaTime,
    OMX_IndexConfigTimeCurrentWallTime,
    OMX_IndexConfigTimeCurrentAudioReference,
    OMX_IndexConfigTimeCurrentVideoReference,
    OMX_IndexConfigTimeMediaTimeRequest,
    OMX_IndexConfigTimeClientStartTime,
    OMX_IndexConfigTimePosition,
    OMX_IndexConfigTimeSeekMode,


    OMX_IndexKhronosExtensions = 0x6F000000,

    OMX_IndexVendorStartUnused = 0x7F000000,





    OMX_IndexMax = 0x7FFFFFFF

} OMX_INDEXTYPE;
// # 60 "frameworks/native/include/media/openmax/OMX_Core.h" 2






typedef enum OMX_COMMANDTYPE
{
    OMX_CommandStateSet,
    OMX_CommandFlush,
    OMX_CommandPortDisable,
    OMX_CommandPortEnable,
    OMX_CommandMarkBuffer,
    OMX_CommandKhronosExtensions = 0x6F000000,
    OMX_CommandVendorStartUnused = 0x7F000000,
    OMX_CommandMax = 0X7FFFFFFF
} OMX_COMMANDTYPE;
// # 109 "frameworks/native/include/media/openmax/OMX_Core.h"
typedef enum OMX_STATETYPE
{
    OMX_StateInvalid,


    OMX_StateLoaded,




    OMX_StateIdle,


    OMX_StateExecuting,

    OMX_StatePause,
    OMX_StateWaitForResources,


    OMX_StateKhronosExtensions = 0x6F000000,
    OMX_StateVendorStartUnused = 0x7F000000,
    OMX_StateMax = 0X7FFFFFFF
} OMX_STATETYPE;
// # 143 "frameworks/native/include/media/openmax/OMX_Core.h"
typedef enum OMX_ERRORTYPE
{
  OMX_ErrorNone = 0,


  OMX_ErrorInsufficientResources = (OMX_S32) 0x80001000,


  OMX_ErrorUndefined = (OMX_S32) 0x80001001,


  OMX_ErrorInvalidComponentName = (OMX_S32) 0x80001002,


  OMX_ErrorComponentNotFound = (OMX_S32) 0x80001003,



  OMX_ErrorInvalidComponent = (OMX_S32) 0x80001004,


  OMX_ErrorBadParameter = (OMX_S32) 0x80001005,


  OMX_ErrorNotImplemented = (OMX_S32) 0x80001006,


  OMX_ErrorUnderflow = (OMX_S32) 0x80001007,


  OMX_ErrorOverflow = (OMX_S32) 0x80001008,


  OMX_ErrorHardware = (OMX_S32) 0x80001009,


  OMX_ErrorInvalidState = (OMX_S32) 0x8000100A,


  OMX_ErrorStreamCorrupt = (OMX_S32) 0x8000100B,


  OMX_ErrorPortsNotCompatible = (OMX_S32) 0x8000100C,



  OMX_ErrorResourcesLost = (OMX_S32) 0x8000100D,


  OMX_ErrorNoMore = (OMX_S32) 0x8000100E,


  OMX_ErrorVersionMismatch = (OMX_S32) 0x8000100F,


  OMX_ErrorNotReady = (OMX_S32) 0x80001010,


  OMX_ErrorTimeout = (OMX_S32) 0x80001011,


  OMX_ErrorSameState = (OMX_S32) 0x80001012,



  OMX_ErrorResourcesPreempted = (OMX_S32) 0x80001013,





  OMX_ErrorPortUnresponsiveDuringAllocation = (OMX_S32) 0x80001014,





  OMX_ErrorPortUnresponsiveDuringDeallocation = (OMX_S32) 0x80001015,





  OMX_ErrorPortUnresponsiveDuringStop = (OMX_S32) 0x80001016,


  OMX_ErrorIncorrectStateTransition = (OMX_S32) 0x80001017,


  OMX_ErrorIncorrectStateOperation = (OMX_S32) 0x80001018,


  OMX_ErrorUnsupportedSetting = (OMX_S32) 0x80001019,


  OMX_ErrorUnsupportedIndex = (OMX_S32) 0x8000101A,


  OMX_ErrorBadPortIndex = (OMX_S32) 0x8000101B,


  OMX_ErrorPortUnpopulated = (OMX_S32) 0x8000101C,


  OMX_ErrorComponentSuspended = (OMX_S32) 0x8000101D,


  OMX_ErrorDynamicResourcesUnavailable = (OMX_S32) 0x8000101E,



  OMX_ErrorMbErrorsInFrame = (OMX_S32) 0x8000101F,


  OMX_ErrorFormatNotDetected = (OMX_S32) 0x80001020,


  OMX_ErrorContentPipeOpenFailed = (OMX_S32) 0x80001021,


  OMX_ErrorContentPipeCreationFailed = (OMX_S32) 0x80001022,


  OMX_ErrorSeperateTablesUsed = (OMX_S32) 0x80001023,


  OMX_ErrorTunnelingUnsupported = (OMX_S32) 0x80001024,

  OMX_ErrorKhronosExtensions = (OMX_S32)0x8F000000,
  OMX_ErrorVendorStartUnused = (OMX_S32)0x90000000,
  OMX_ErrorMax = 0x7FFFFFFF
} OMX_ERRORTYPE;


typedef OMX_ERRORTYPE (* OMX_COMPONENTINITTYPE)( OMX_HANDLETYPE hComponent);


typedef struct OMX_COMPONENTREGISTERTYPE
{
  const char * pName;
  OMX_COMPONENTINITTYPE pInitialize;
} OMX_COMPONENTREGISTERTYPE;


extern OMX_COMPONENTREGISTERTYPE OMX_ComponentRegistered[];


typedef struct OMX_PRIORITYMGMTTYPE {
 OMX_U32 nSize;
 OMX_VERSIONTYPE nVersion;
 OMX_U32 nGroupPriority;
 OMX_U32 nGroupID;
} OMX_PRIORITYMGMTTYPE;





typedef struct OMX_PARAM_COMPONENTROLETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 cRole[128];
} OMX_PARAM_COMPONENTROLETYPE;
// # 415 "frameworks/native/include/media/openmax/OMX_Core.h"
typedef struct OMX_BUFFERHEADERTYPE
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8* pBuffer;

    OMX_U32 nAllocLen;
    OMX_U32 nFilledLen;

    OMX_U32 nOffset;

    OMX_PTR pAppPrivate;

    OMX_PTR pPlatformPrivate;

    OMX_PTR pInputPortPrivate;

    OMX_PTR pOutputPortPrivate;

    OMX_HANDLETYPE hMarkTargetComponent;

    OMX_PTR pMarkData;


    OMX_U32 nTickCount;
// # 448 "frameworks/native/include/media/openmax/OMX_Core.h"
 OMX_TICKS nTimeStamp;






  OMX_U32 nFlags;
  OMX_U32 nOutputPortIndex;

  OMX_U32 nInputPortIndex;

} OMX_BUFFERHEADERTYPE;







typedef enum OMX_EXTRADATATYPE
{
   OMX_ExtraDataNone = 0,
   OMX_ExtraDataQuantization,
   OMX_ExtraDataKhronosExtensions = 0x6F000000,
   OMX_ExtraDataVendorStartUnused = 0x7F000000,
   OMX_ExtraDataMax = 0x7FFFFFFF
} OMX_EXTRADATATYPE;


typedef struct OMX_OTHER_EXTRADATATYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_EXTRADATATYPE eType;
    OMX_U32 nDataSize;
    OMX_U8 data[1];
} OMX_OTHER_EXTRADATATYPE;


typedef struct OMX_PORT_PARAM_TYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPorts;
    OMX_U32 nStartPortNumber;
} OMX_PORT_PARAM_TYPE;


typedef enum OMX_EVENTTYPE
{
    OMX_EventCmdComplete,
    OMX_EventError,
    OMX_EventMark,
    OMX_EventPortSettingsChanged,
    OMX_EventBufferFlag,
    OMX_EventResourcesAcquired,


    OMX_EventComponentResumed,
    OMX_EventDynamicResourcesAvailable,
    OMX_EventPortFormatDetected,
    OMX_EventKhronosExtensions = 0x6F000000,
    OMX_EventVendorStartUnused = 0x7F000000,
// # 529 "frameworks/native/include/media/openmax/OMX_Core.h"
    OMX_EventOutputRendered = 0x7F000001,
// # 544 "frameworks/native/include/media/openmax/OMX_Core.h"
    OMX_EventDataSpaceChanged,
    OMX_EventMax = 0x7FFFFFFF
} OMX_EVENTTYPE;

typedef struct OMX_CALLBACKTYPE
{
// # 581 "frameworks/native/include/media/openmax/OMX_Core.h"
   OMX_ERRORTYPE (*EventHandler)(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_EVENTTYPE eEvent,
        OMX_U32 nData1,
        OMX_U32 nData2,
        OMX_PTR pEventData);
// # 611 "frameworks/native/include/media/openmax/OMX_Core.h"
    OMX_ERRORTYPE (*EmptyBufferDone)(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_BUFFERHEADERTYPE* pBuffer);
// # 640 "frameworks/native/include/media/openmax/OMX_Core.h"
    OMX_ERRORTYPE (*FillBufferDone)(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_BUFFERHEADERTYPE* pBuffer);

} OMX_CALLBACKTYPE;





typedef enum OMX_BUFFERSUPPLIERTYPE
{
    OMX_BufferSupplyUnspecified = 0x0,

    OMX_BufferSupplyInput,
    OMX_BufferSupplyOutput,
    OMX_BufferSupplyKhronosExtensions = 0x6F000000,
    OMX_BufferSupplyVendorStartUnused = 0x7F000000,
    OMX_BufferSupplyMax = 0x7FFFFFFF
} OMX_BUFFERSUPPLIERTYPE;





typedef struct OMX_PARAM_BUFFERSUPPLIERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BUFFERSUPPLIERTYPE eBufferSupplier;
} OMX_PARAM_BUFFERSUPPLIERTYPE;
// # 686 "frameworks/native/include/media/openmax/OMX_Core.h"
typedef struct OMX_TUNNELSETUPTYPE
{
    OMX_U32 nTunnelFlags;
    OMX_BUFFERSUPPLIERTYPE eSupplier;
} OMX_TUNNELSETUPTYPE;
// # 1246 "frameworks/native/include/media/openmax/OMX_Core.h"
extern OMX_ERRORTYPE OMX_Init(void);
// # 1261 "frameworks/native/include/media/openmax/OMX_Core.h"
extern OMX_ERRORTYPE OMX_Deinit(void);
// # 1301 "frameworks/native/include/media/openmax/OMX_Core.h"
extern OMX_ERRORTYPE OMX_ComponentNameEnum(
    OMX_STRING cComponentName,
    OMX_U32 nNameLength,
    OMX_U32 nIndex);
// # 1334 "frameworks/native/include/media/openmax/OMX_Core.h"
extern OMX_ERRORTYPE OMX_GetHandle(
    OMX_HANDLETYPE* pHandle,
    OMX_STRING cComponentName,
    OMX_PTR pAppData,
    OMX_CALLBACKTYPE* pCallBacks);
// # 1356 "frameworks/native/include/media/openmax/OMX_Core.h"
extern OMX_ERRORTYPE OMX_FreeHandle(
    OMX_HANDLETYPE hComponent);
// # 1409 "frameworks/native/include/media/openmax/OMX_Core.h"
extern OMX_ERRORTYPE OMX_SetupTunnel(
    OMX_HANDLETYPE hOutput,
    OMX_U32 nPortOutput,
    OMX_HANDLETYPE hInput,
    OMX_U32 nPortInput);


extern OMX_ERRORTYPE OMX_GetContentPipe(
    OMX_HANDLETYPE *hPipe,
    OMX_STRING szURI);
// # 1447 "frameworks/native/include/media/openmax/OMX_Core.h"
extern OMX_ERRORTYPE OMX_GetComponentsOfRole (
    OMX_STRING role,
    OMX_U32 *pNumComps,
    OMX_U8 **compNames);
// # 1477 "frameworks/native/include/media/openmax/OMX_Core.h"
extern OMX_ERRORTYPE OMX_GetRolesOfComponent (
    OMX_STRING compName,
    OMX_U32 *pNumRoles,
    OMX_U8 **roles);
// # 60 "frameworks/native/include/media/openmax/OMX_IVCommon.h" 2
// # 105 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef enum OMX_COLOR_FORMATTYPE {
    OMX_COLOR_FormatUnused,
    OMX_COLOR_FormatMonochrome,
    OMX_COLOR_Format8bitRGB332,
    OMX_COLOR_Format12bitRGB444,
    OMX_COLOR_Format16bitARGB4444,
    OMX_COLOR_Format16bitARGB1555,
    OMX_COLOR_Format16bitRGB565,
    OMX_COLOR_Format16bitBGR565,
    OMX_COLOR_Format18bitRGB666,
    OMX_COLOR_Format18bitARGB1665,
    OMX_COLOR_Format19bitARGB1666,
    OMX_COLOR_Format24bitRGB888,
    OMX_COLOR_Format24bitBGR888,
    OMX_COLOR_Format24bitARGB1887,
    OMX_COLOR_Format25bitARGB1888,
    OMX_COLOR_Format32bitBGRA8888,
    OMX_COLOR_Format32bitARGB8888,
    OMX_COLOR_FormatYUV411Planar,
    OMX_COLOR_FormatYUV411PackedPlanar,
    OMX_COLOR_FormatYUV420Planar,
    OMX_COLOR_FormatYUV420PackedPlanar,
    OMX_COLOR_FormatYUV420SemiPlanar,
    OMX_COLOR_FormatYUV422Planar,
    OMX_COLOR_FormatYUV422PackedPlanar,
    OMX_COLOR_FormatYUV422SemiPlanar,
    OMX_COLOR_FormatYCbYCr,
    OMX_COLOR_FormatYCrYCb,
    OMX_COLOR_FormatCbYCrY,
    OMX_COLOR_FormatCrYCbY,
    OMX_COLOR_FormatYUV444Interleaved,
    OMX_COLOR_FormatRawBayer8bit,
    OMX_COLOR_FormatRawBayer10bit,
    OMX_COLOR_FormatRawBayer8bitcompressed,
    OMX_COLOR_FormatL2,
    OMX_COLOR_FormatL4,
    OMX_COLOR_FormatL8,
    OMX_COLOR_FormatL16,
    OMX_COLOR_FormatL24,
    OMX_COLOR_FormatL32,
    OMX_COLOR_FormatYUV420PackedSemiPlanar,
    OMX_COLOR_FormatYUV422PackedSemiPlanar,
    OMX_COLOR_Format18BitBGR666,
    OMX_COLOR_Format24BitARGB6666,
    OMX_COLOR_Format24BitABGR6666,
    OMX_COLOR_FormatKhronosExtensions = 0x6F000000,
    OMX_COLOR_FormatVendorStartUnused = 0x7F000000,







    OMX_COLOR_FormatAndroidOpaque = 0x7F000789,
    OMX_COLOR_Format32BitRGBA8888 = 0x7F00A000,





    OMX_COLOR_FormatYUV420Flexible = 0x7F420888,

    OMX_TI_COLOR_FormatYUV420PackedSemiPlanar = 0x7F000100,
    OMX_QCOM_COLOR_FormatYVU420SemiPlanar = 0x7FA30C00,
    OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka = 0x7FA30C03,
    OMX_SEC_COLOR_FormatNV12Tiled = 0x7FC00002,
    OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar32m = 0x7FA30C04,
    OMX_COLOR_FormatMax = 0x7FFFFFFF
} OMX_COLOR_FORMATTYPE;







typedef struct OMX_CONFIG_COLORCONVERSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 xColorMatrix[3][3];
    OMX_S32 xColorOffset[4];
}OMX_CONFIG_COLORCONVERSIONTYPE;







typedef struct OMX_CONFIG_SCALEFACTORTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 xWidth;
    OMX_S32 xHeight;
}OMX_CONFIG_SCALEFACTORTYPE;





typedef enum OMX_IMAGEFILTERTYPE {
    OMX_ImageFilterNone,
    OMX_ImageFilterNoise,
    OMX_ImageFilterEmboss,
    OMX_ImageFilterNegative,
    OMX_ImageFilterSketch,
    OMX_ImageFilterOilPaint,
    OMX_ImageFilterHatch,
    OMX_ImageFilterGpen,
    OMX_ImageFilterAntialias,
    OMX_ImageFilterDeRing,
    OMX_ImageFilterSolarize,
    OMX_ImageFilterKhronosExtensions = 0x6F000000,
    OMX_ImageFilterVendorStartUnused = 0x7F000000,
    OMX_ImageFilterMax = 0x7FFFFFFF
} OMX_IMAGEFILTERTYPE;
// # 235 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_IMAGEFILTERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGEFILTERTYPE eImageFilter;
} OMX_CONFIG_IMAGEFILTERTYPE;
// # 256 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_COLORENHANCEMENTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bColorEnhancement;
    OMX_U8 nCustomizedU;
    OMX_U8 nCustomizedV;
} OMX_CONFIG_COLORENHANCEMENTTYPE;
// # 276 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_COLORKEYTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nARGBColor;
    OMX_U32 nARGBMask;
} OMX_CONFIG_COLORKEYTYPE;
// # 298 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef enum OMX_COLORBLENDTYPE {
    OMX_ColorBlendNone,
    OMX_ColorBlendAlphaConstant,
    OMX_ColorBlendAlphaPerPixel,
    OMX_ColorBlendAlternate,
    OMX_ColorBlendAnd,
    OMX_ColorBlendOr,
    OMX_ColorBlendInvert,
    OMX_ColorBlendKhronosExtensions = 0x6F000000,
    OMX_ColorBlendVendorStartUnused = 0x7F000000,
    OMX_ColorBlendMax = 0x7FFFFFFF
} OMX_COLORBLENDTYPE;
// # 322 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_COLORBLENDTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nRGBAlphaConstant;
    OMX_COLORBLENDTYPE eColorBlend;
} OMX_CONFIG_COLORBLENDTYPE;
// # 341 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_FRAMESIZETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
} OMX_FRAMESIZETYPE;
// # 359 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_ROTATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nRotation;
} OMX_CONFIG_ROTATIONTYPE;
// # 376 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef enum OMX_MIRRORTYPE {
    OMX_MirrorNone = 0,
    OMX_MirrorVertical,
    OMX_MirrorHorizontal,
    OMX_MirrorBoth,
    OMX_MirrorKhronosExtensions = 0x6F000000,
    OMX_MirrorVendorStartUnused = 0x7F000000,
    OMX_MirrorMax = 0x7FFFFFFF
} OMX_MIRRORTYPE;
// # 396 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_MIRRORTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_MIRRORTYPE eMirror;
} OMX_CONFIG_MIRRORTYPE;
// # 414 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_POINTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nX;
    OMX_S32 nY;
} OMX_CONFIG_POINTTYPE;
// # 435 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_RECTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nLeft;
    OMX_S32 nTop;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
} OMX_CONFIG_RECTTYPE;
// # 455 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_PARAM_DEBLOCKINGTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDeblocking;
} OMX_PARAM_DEBLOCKINGTYPE;
// # 472 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_FRAMESTABTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bStab;
} OMX_CONFIG_FRAMESTABTYPE;
// # 487 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef enum OMX_WHITEBALCONTROLTYPE {
    OMX_WhiteBalControlOff = 0,
    OMX_WhiteBalControlAuto,
    OMX_WhiteBalControlSunLight,
    OMX_WhiteBalControlCloudy,
    OMX_WhiteBalControlShade,
    OMX_WhiteBalControlTungsten,
    OMX_WhiteBalControlFluorescent,
    OMX_WhiteBalControlIncandescent,
    OMX_WhiteBalControlFlash,
    OMX_WhiteBalControlHorizon,
    OMX_WhiteBalControlKhronosExtensions = 0x6F000000,
    OMX_WhiteBalControlVendorStartUnused = 0x7F000000,
    OMX_WhiteBalControlMax = 0x7FFFFFFF
} OMX_WHITEBALCONTROLTYPE;
// # 513 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_WHITEBALCONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_WHITEBALCONTROLTYPE eWhiteBalControl;
} OMX_CONFIG_WHITEBALCONTROLTYPE;





typedef enum OMX_EXPOSURECONTROLTYPE {
    OMX_ExposureControlOff = 0,
    OMX_ExposureControlAuto,
    OMX_ExposureControlNight,
    OMX_ExposureControlBackLight,
    OMX_ExposureControlSpotLight,
    OMX_ExposureControlSports,
    OMX_ExposureControlSnow,
    OMX_ExposureControlBeach,
    OMX_ExposureControlLargeAperture,
    OMX_ExposureControlSmallApperture,
    OMX_ExposureControlKhronosExtensions = 0x6F000000,
    OMX_ExposureControlVendorStartUnused = 0x7F000000,
    OMX_ExposureControlMax = 0x7FFFFFFF
} OMX_EXPOSURECONTROLTYPE;
// # 550 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_EXPOSURECONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_EXPOSURECONTROLTYPE eExposureControl;
} OMX_CONFIG_EXPOSURECONTROLTYPE;
// # 569 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_PARAM_SENSORMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nFrameRate;
    OMX_BOOL bOneShot;
    OMX_FRAMESIZETYPE sFrameSize;
} OMX_PARAM_SENSORMODETYPE;
// # 588 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_CONTRASTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nContrast;
} OMX_CONFIG_CONTRASTTYPE;
// # 605 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_BRIGHTNESSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nBrightness;
} OMX_CONFIG_BRIGHTNESSTYPE;
// # 624 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_BACKLIGHTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nBacklight;
    OMX_U32 nTimeout;
} OMX_CONFIG_BACKLIGHTTYPE;
// # 642 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_GAMMATYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nGamma;
} OMX_CONFIG_GAMMATYPE;
// # 660 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_SATURATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nSaturation;
} OMX_CONFIG_SATURATIONTYPE;
// # 678 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_LIGHTNESSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nLightness;
} OMX_CONFIG_LIGHTNESSTYPE;
// # 699 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_PLANEBLENDTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nDepth;
    OMX_U32 nAlpha;
} OMX_CONFIG_PLANEBLENDTYPE;
// # 721 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_PARAM_INTERLEAVETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_U32 nInterleavePortIndex;
} OMX_PARAM_INTERLEAVETYPE;





typedef enum OMX_TRANSITIONEFFECTTYPE {
    OMX_EffectNone,
    OMX_EffectFadeFromBlack,
    OMX_EffectFadeToBlack,
    OMX_EffectUnspecifiedThroughConstantColor,
    OMX_EffectDissolve,
    OMX_EffectWipe,
    OMX_EffectUnspecifiedMixOfTwoScenes,
    OMX_EffectKhronosExtensions = 0x6F000000,
    OMX_EffectVendorStartUnused = 0x7F000000,
    OMX_EffectMax = 0x7FFFFFFF
} OMX_TRANSITIONEFFECTTYPE;
// # 756 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_TRANSITIONEFFECTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TRANSITIONEFFECTTYPE eEffect;
} OMX_CONFIG_TRANSITIONEFFECTTYPE;







typedef enum OMX_DATAUNITTYPE {
    OMX_DataUnitCodedPicture,
    OMX_DataUnitVideoSegment,
    OMX_DataUnitSeveralSegments,
    OMX_DataUnitArbitraryStreamSection,
    OMX_DataUnitKhronosExtensions = 0x6F000000,
    OMX_DataUnitVendorStartUnused = 0x7F000000,
    OMX_DataUnitMax = 0x7FFFFFFF
} OMX_DATAUNITTYPE;







typedef enum OMX_DATAUNITENCAPSULATIONTYPE {
    OMX_DataEncapsulationElementaryStream,
    OMX_DataEncapsulationGenericPayload,
    OMX_DataEncapsulationRtpPayload,
    OMX_DataEncapsulationKhronosExtensions = 0x6F000000,
    OMX_DataEncapsulationVendorStartUnused = 0x7F000000,
    OMX_DataEncapsulationMax = 0x7FFFFFFF
} OMX_DATAUNITENCAPSULATIONTYPE;





typedef struct OMX_PARAM_DATAUNITTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_DATAUNITTYPE eUnitType;
    OMX_DATAUNITENCAPSULATIONTYPE eEncapsulationType;
} OMX_PARAM_DATAUNITTYPE;





typedef enum OMX_DITHERTYPE {
    OMX_DitherNone,
    OMX_DitherOrdered,
    OMX_DitherErrorDiffusion,
    OMX_DitherOther,
    OMX_DitherKhronosExtensions = 0x6F000000,
    OMX_DitherVendorStartUnused = 0x7F000000,
    OMX_DitherMax = 0x7FFFFFFF
} OMX_DITHERTYPE;





typedef struct OMX_CONFIG_DITHERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_DITHERTYPE eDither;
} OMX_CONFIG_DITHERTYPE;

typedef struct OMX_CONFIG_CAPTUREMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bContinuous;

    OMX_BOOL bFrameLimited;




    OMX_U32 nFrameLimit;

} OMX_CONFIG_CAPTUREMODETYPE;

typedef enum OMX_METERINGTYPE {

    OMX_MeteringModeAverage,
    OMX_MeteringModeSpot,
    OMX_MeteringModeMatrix,

    OMX_MeteringKhronosExtensions = 0x6F000000,
    OMX_MeteringVendorStartUnused = 0x7F000000,
    OMX_EVModeMax = 0x7fffffff
} OMX_METERINGTYPE;

typedef struct OMX_CONFIG_EXPOSUREVALUETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_METERINGTYPE eMetering;
    OMX_S32 xEVCompensation;
    OMX_U32 nApertureFNumber;
    OMX_BOOL bAutoAperture;
    OMX_U32 nShutterSpeedMsec;
    OMX_BOOL bAutoShutterSpeed;
    OMX_U32 nSensitivity;
    OMX_BOOL bAutoSensitivity;
} OMX_CONFIG_EXPOSUREVALUETYPE;
// # 888 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_CONFIG_FOCUSREGIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bCenter;
    OMX_BOOL bLeft;
    OMX_BOOL bRight;
    OMX_BOOL bTop;
    OMX_BOOL bBottom;
    OMX_BOOL bTopLeft;
    OMX_BOOL bTopRight;
    OMX_BOOL bBottomLeft;
    OMX_BOOL bBottomRight;
} OMX_CONFIG_FOCUSREGIONTYPE;




typedef enum OMX_FOCUSSTATUSTYPE {
    OMX_FocusStatusOff = 0,
    OMX_FocusStatusRequest,
    OMX_FocusStatusReached,
    OMX_FocusStatusUnableToReach,
    OMX_FocusStatusLost,
    OMX_FocusStatusKhronosExtensions = 0x6F000000,
    OMX_FocusStatusVendorStartUnused = 0x7F000000,
    OMX_FocusStatusMax = 0x7FFFFFFF
} OMX_FOCUSSTATUSTYPE;
// # 935 "frameworks/native/include/media/openmax/OMX_IVCommon.h"
typedef struct OMX_PARAM_FOCUSSTATUSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_FOCUSSTATUSTYPE eFocusStatus;
    OMX_BOOL bCenterStatus;
    OMX_BOOL bLeftStatus;
    OMX_BOOL bRightStatus;
    OMX_BOOL bTopStatus;
    OMX_BOOL bBottomStatus;
    OMX_BOOL bTopLeftStatus;
    OMX_BOOL bTopRightStatus;
    OMX_BOOL bBottomLeftStatus;
    OMX_BOOL bBottomRightStatus;
} OMX_PARAM_FOCUSSTATUSTYPE;
// # 60 "frameworks/native/include/media/openmax/OMX_Image.h" 2
// # 70 "frameworks/native/include/media/openmax/OMX_Image.h"
typedef enum OMX_IMAGE_CODINGTYPE {
    OMX_IMAGE_CodingUnused,
    OMX_IMAGE_CodingAutoDetect,
    OMX_IMAGE_CodingJPEG,
    OMX_IMAGE_CodingJPEG2K,
    OMX_IMAGE_CodingEXIF,
    OMX_IMAGE_CodingTIFF,
    OMX_IMAGE_CodingGIF,
    OMX_IMAGE_CodingPNG,
    OMX_IMAGE_CodingLZW,
    OMX_IMAGE_CodingBMP,
    OMX_IMAGE_CodingKhronosExtensions = 0x6F000000,
    OMX_IMAGE_CodingVendorStartUnused = 0x7F000000,
    OMX_IMAGE_CodingMax = 0x7FFFFFFF
} OMX_IMAGE_CODINGTYPE;
// # 128 "frameworks/native/include/media/openmax/OMX_Image.h"
typedef struct OMX_IMAGE_PORTDEFINITIONTYPE {
    OMX_STRING cMIMEType;
    OMX_NATIVE_DEVICETYPE pNativeRender;
    OMX_U32 nFrameWidth;
    OMX_U32 nFrameHeight;
    OMX_S32 nStride;
    OMX_U32 nSliceHeight;
    OMX_BOOL bFlagErrorConcealment;
    OMX_IMAGE_CODINGTYPE eCompressionFormat;
    OMX_COLOR_FORMATTYPE eColorFormat;
    OMX_NATIVE_WINDOWTYPE pNativeWindow;
} OMX_IMAGE_PORTDEFINITIONTYPE;
// # 157 "frameworks/native/include/media/openmax/OMX_Image.h"
typedef struct OMX_IMAGE_PARAM_PORTFORMATTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIndex;
    OMX_IMAGE_CODINGTYPE eCompressionFormat;
    OMX_COLOR_FORMATTYPE eColorFormat;
} OMX_IMAGE_PARAM_PORTFORMATTYPE;
// # 173 "frameworks/native/include/media/openmax/OMX_Image.h"
typedef enum OMX_IMAGE_FLASHCONTROLTYPE {
    OMX_IMAGE_FlashControlOn = 0,
    OMX_IMAGE_FlashControlOff,
    OMX_IMAGE_FlashControlAuto,
    OMX_IMAGE_FlashControlRedEyeReduction,
    OMX_IMAGE_FlashControlFillin,
    OMX_IMAGE_FlashControlTorch,
    OMX_IMAGE_FlashControlKhronosExtensions = 0x6F000000,
    OMX_IMAGE_FlashControlVendorStartUnused = 0x7F000000,
    OMX_IMAGE_FlashControlMax = 0x7FFFFFFF
} OMX_IMAGE_FLASHCONTROLTYPE;
// # 195 "frameworks/native/include/media/openmax/OMX_Image.h"
typedef struct OMX_IMAGE_PARAM_FLASHCONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGE_FLASHCONTROLTYPE eFlashControl;
} OMX_IMAGE_PARAM_FLASHCONTROLTYPE;





typedef enum OMX_IMAGE_FOCUSCONTROLTYPE {
    OMX_IMAGE_FocusControlOn = 0,
    OMX_IMAGE_FocusControlOff,
    OMX_IMAGE_FocusControlAuto,
    OMX_IMAGE_FocusControlAutoLock,
    OMX_IMAGE_FocusControlKhronosExtensions = 0x6F000000,
    OMX_IMAGE_FocusControlVendorStartUnused = 0x7F000000,
    OMX_IMAGE_FocusControlMax = 0x7FFFFFFF
} OMX_IMAGE_FOCUSCONTROLTYPE;
// # 229 "frameworks/native/include/media/openmax/OMX_Image.h"
typedef struct OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGE_FOCUSCONTROLTYPE eFocusControl;
    OMX_U32 nFocusSteps;
    OMX_U32 nFocusStepIndex;
} OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE;
// # 254 "frameworks/native/include/media/openmax/OMX_Image.h"
typedef struct OMX_IMAGE_PARAM_QFACTORTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nQFactor;
} OMX_IMAGE_PARAM_QFACTORTYPE;





typedef enum OMX_IMAGE_QUANTIZATIONTABLETYPE {
    OMX_IMAGE_QuantizationTableLuma = 0,
    OMX_IMAGE_QuantizationTableChroma,
    OMX_IMAGE_QuantizationTableChromaCb,
    OMX_IMAGE_QuantizationTableChromaCr,
    OMX_IMAGE_QuantizationTableKhronosExtensions = 0x6F000000,
    OMX_IMAGE_QuantizationTableVendorStartUnused = 0x7F000000,
    OMX_IMAGE_QuantizationTableMax = 0x7FFFFFFF
} OMX_IMAGE_QUANTIZATIONTABLETYPE;
// # 292 "frameworks/native/include/media/openmax/OMX_Image.h"
typedef struct OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGE_QUANTIZATIONTABLETYPE eQuantizationTable;
    OMX_U8 nQuantizationMatrix[64];
} OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE;






typedef enum OMX_IMAGE_HUFFMANTABLETYPE {
    OMX_IMAGE_HuffmanTableAC = 0,
    OMX_IMAGE_HuffmanTableDC,
    OMX_IMAGE_HuffmanTableACLuma,
    OMX_IMAGE_HuffmanTableACChroma,
    OMX_IMAGE_HuffmanTableDCLuma,
    OMX_IMAGE_HuffmanTableDCChroma,
    OMX_IMAGE_HuffmanTableKhronosExtensions = 0x6F000000,
    OMX_IMAGE_HuffmanTableVendorStartUnused = 0x7F000000,
    OMX_IMAGE_HuffmanTableMax = 0x7FFFFFFF
} OMX_IMAGE_HUFFMANTABLETYPE;
// # 330 "frameworks/native/include/media/openmax/OMX_Image.h"
typedef struct OMX_IMAGE_PARAM_HUFFMANTTABLETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGE_HUFFMANTABLETYPE eHuffmanTable;
    OMX_U8 nNumberOfHuffmanCodeOfLength[16];
    OMX_U8 nHuffmanTable[256];
}OMX_IMAGE_PARAM_HUFFMANTTABLETYPE;
// # 36 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qomx_core/QOMX_JpegExtensions.h" 2
// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qexif/qexif.h" 1
// # 35 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qexif/qexif.h"
struct exif_info_t;
typedef struct exif_info_t * exif_info_obj_t;


typedef uint32_t exif_tag_id_t;



typedef struct
{
    uint32_t num;
    uint32_t denom;

} rat_t;


typedef struct
{
    int32_t num;
    int32_t denom;

} srat_t;


typedef enum
{
    EXIF_BYTE = 1,
    EXIF_ASCII = 2,
    EXIF_SHORT = 3,
    EXIF_LONG = 4,
    EXIF_RATIONAL = 5,
    EXIF_UNDEFINED = 7,
    EXIF_SLONG = 9,
    EXIF_SRATIONAL = 10
} exif_tag_type_t;




typedef struct
{


    exif_tag_type_t type;
// # 91 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qexif/qexif.h"
    uint8_t copy;
// # 102 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qexif/qexif.h"
    uint32_t count;






    union
    {
        char *_ascii;
        uint8_t *_bytes;
        uint8_t _byte;
        uint16_t *_shorts;
        uint16_t _short;
        uint32_t *_longs;
        uint32_t _long;
        rat_t *_rats;
        rat_t _rat;
        uint8_t *_undefined;
        int32_t *_slongs;
        int32_t _slong;
        srat_t *_srats;
        srat_t _srat;

    } data;

} exif_tag_entry_t;






typedef enum
{

    GPS_VERSION_ID = 0,
    GPS_LATITUDE_REF,
    GPS_LATITUDE,
    GPS_LONGITUDE_REF,
    GPS_LONGITUDE,
    GPS_ALTITUDE_REF,
    GPS_ALTITUDE,
    GPS_TIMESTAMP,
    GPS_SATELLITES,
    GPS_STATUS,
    GPS_MEASUREMODE,
    GPS_DOP,
    GPS_SPEED_REF,
    GPS_SPEED,
    GPS_TRACK_REF,
    GPS_TRACK,
    GPS_IMGDIRECTION_REF,
    GPS_IMGDIRECTION,
    GPS_MAPDATUM,
    GPS_DESTLATITUDE_REF,
    GPS_DESTLATITUDE,
    GPS_DESTLONGITUDE_REF,
    GPS_DESTLONGITUDE,
    GPS_DESTBEARING_REF,
    GPS_DESTBEARING,
    GPS_DESTDISTANCE_REF,
    GPS_DESTDISTANCE,
    GPS_PROCESSINGMETHOD,
    GPS_AREAINFORMATION,
    GPS_DATESTAMP,
    GPS_DIFFERENTIAL,


    NEW_SUBFILE_TYPE,
    SUBFILE_TYPE,
    IMAGE_WIDTH,
    IMAGE_LENGTH,
    BITS_PER_SAMPLE,
    COMPRESSION,
    PHOTOMETRIC_INTERPRETATION,
    THRESH_HOLDING,
    CELL_WIDTH,
    CELL_HEIGHT,
    FILL_ORDER,
    DOCUMENT_NAME,
    IMAGE_DESCRIPTION,
    MAKE,
    MODEL,
    STRIP_OFFSETS,
    ORIENTATION,
    SAMPLES_PER_PIXEL,
    ROWS_PER_STRIP,
    STRIP_BYTE_COUNTS,
    MIN_SAMPLE_VALUE,
    MAX_SAMPLE_VALUE,
    X_RESOLUTION,
    Y_RESOLUTION,
    PLANAR_CONFIGURATION,
    PAGE_NAME,
    X_POSITION,
    Y_POSITION,
    FREE_OFFSET,
    FREE_BYTE_COUNTS,
    GRAY_RESPONSE_UNIT,
    GRAY_RESPONSE_CURVE,
    T4_OPTION,
    T6_OPTION,
    RESOLUTION_UNIT,
    PAGE_NUMBER,
    TRANSFER_FUNCTION,
    SOFTWARE,
    DATE_TIME,
    ARTIST,
    HOST_COMPUTER,
    PREDICTOR,
    WHITE_POINT,
    PRIMARY_CHROMATICITIES,
    COLOR_MAP,
    HALFTONE_HINTS,
    TILE_WIDTH,
    TILE_LENGTH,
    TILE_OFFSET,
    TILE_BYTE_COUNTS,
    INK_SET,
    INK_NAMES,
    NUMBER_OF_INKS,
    DOT_RANGE,
    TARGET_PRINTER,
    EXTRA_SAMPLES,
    SAMPLE_FORMAT,
    TRANSFER_RANGE,
    JPEG_PROC,
    JPEG_INTERCHANGE_FORMAT,
    JPEG_INTERCHANGE_FORMAT_LENGTH,
    JPEG_RESTART_INTERVAL,
    JPEG_LOSSLESS_PREDICTORS,
    JPEG_POINT_TRANSFORMS,
    JPEG_Q_TABLES,
    JPEG_DC_TABLES,
    JPEG_AC_TABLES,
    YCBCR_COEFFICIENTS,
    YCBCR_SUB_SAMPLING,
    YCBCR_POSITIONING,
    REFERENCE_BLACK_WHITE,
    GAMMA,
    ICC_PROFILE_DESCRIPTOR,
    SRGB_RENDERING_INTENT,
    IMAGE_TITLE,
    COPYRIGHT,
    EXIF_IFD,
    ICC_PROFILE,
    GPS_IFD,



    TN_IMAGE_WIDTH,
    TN_IMAGE_LENGTH,
    TN_BITS_PER_SAMPLE,
    TN_COMPRESSION,
    TN_PHOTOMETRIC_INTERPRETATION,
    TN_IMAGE_DESCRIPTION,
    TN_MAKE,
    TN_MODEL,
    TN_STRIP_OFFSETS,
    TN_ORIENTATION,
    TN_SAMPLES_PER_PIXEL,
    TN_ROWS_PER_STRIP,
    TN_STRIP_BYTE_COUNTS,
    TN_X_RESOLUTION,
    TN_Y_RESOLUTION,
    TN_PLANAR_CONFIGURATION,
    TN_RESOLUTION_UNIT,
    TN_TRANSFER_FUNCTION,
    TN_SOFTWARE,
    TN_DATE_TIME,
    TN_ARTIST,
    TN_WHITE_POINT,
    TN_PRIMARY_CHROMATICITIES,
    TN_JPEGINTERCHANGE_FORMAT,
    TN_JPEGINTERCHANGE_FORMAT_L,
    TN_YCBCR_COEFFICIENTS,
    TN_YCBCR_SUB_SAMPLING,
    TN_YCBCR_POSITIONING,
    TN_REFERENCE_BLACK_WHITE,
    TN_COPYRIGHT,


    EXPOSURE_TIME,
    F_NUMBER,
    EXPOSURE_PROGRAM,
    SPECTRAL_SENSITIVITY,
    ISO_SPEED_RATING,
    OECF,
    EXIF_VERSION,
    EXIF_DATE_TIME_ORIGINAL,
    EXIF_DATE_TIME_DIGITIZED,
    EXIF_COMPONENTS_CONFIG,
    EXIF_COMPRESSED_BITS_PER_PIXEL,
    SHUTTER_SPEED,
    APERTURE,
    BRIGHTNESS,
    EXPOSURE_BIAS_VALUE,
    MAX_APERTURE,
    SUBJECT_DISTANCE,
    METERING_MODE,
    LIGHT_SOURCE,
    FLASH,
    FOCAL_LENGTH,
    SUBJECT_AREA,
    EXIF_MAKER_NOTE,
    EXIF_USER_COMMENT,
    SUBSEC_TIME,
    SUBSEC_TIME_ORIGINAL,
    SUBSEC_TIME_DIGITIZED,
    EXIF_FLASHPIX_VERSION,
    EXIF_COLOR_SPACE,
    EXIF_PIXEL_X_DIMENSION,
    EXIF_PIXEL_Y_DIMENSION,
    RELATED_SOUND_FILE,
    INTEROP,
    FLASH_ENERGY,
    SPATIAL_FREQ_RESPONSE,
    FOCAL_PLANE_X_RESOLUTION,
    FOCAL_PLANE_Y_RESOLUTION,
    FOCAL_PLANE_RESOLUTION_UNIT,
    SUBJECT_LOCATION,
    EXPOSURE_INDEX,
    SENSING_METHOD,
    FILE_SOURCE,
    SCENE_TYPE,
    CFA_PATTERN,
    CUSTOM_RENDERED,
    EXPOSURE_MODE,
    WHITE_BALANCE,
    DIGITAL_ZOOM_RATIO,
    FOCAL_LENGTH_35MM,
    SCENE_CAPTURE_TYPE,
    GAIN_CONTROL,
    CONTRAST,
    SATURATION,
    SHARPNESS,
    DEVICE_SETTINGS_DESCRIPTION,
    SUBJECT_DISTANCE_RANGE,
    IMAGE_UID,
    PIM,

    EXIF_TAG_MAX_OFFSET

} exif_tag_offset_t;
// # 37 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qomx_core/QOMX_JpegExtensions.h" 2
// # 45 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qomx_core/QOMX_JpegExtensions.h"
typedef enum {
 OMX_EVENT_THUMBNAIL_DROPPED = OMX_EventVendorStartUnused+1
} QOMX_IMAGE_EXT_EVENTS;
// # 71 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qomx_core/QOMX_JpegExtensions.h"
typedef enum {

  QOMX_IMAGE_EXT_EXIF = 0x07F00000,


  QOMX_IMAGE_EXT_THUMBNAIL = 0x07F00001,


  QOMX_IMAGE_EXT_BUFFER_OFFSET = 0x07F00002,


  QOMX_IMAGE_EXT_MOBICAT = 0x07F00003,


  QOMX_IMAGE_EXT_ENCODING_MODE = 0x07F00004,


  QOMX_IMAGE_EXT_WORK_BUFFER = 0x07F00005,


  QOMX_IMAGE_EXT_METADATA = 0x07F00008,


  QOMX_IMAGE_EXT_META_ENC_KEY = 0x07F00009,


  QOMX_IMAGE_EXT_MEM_OPS = 0x07F0000A,


  QOMX_IMAGE_EXT_JPEG_SPEED = 0x07F000B,

} QOMX_IMAGE_EXT_INDEXTYPE;
// # 113 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qomx_core/QOMX_JpegExtensions.h"
typedef struct {
  OMX_U32 fd;
  OMX_U32 offset;
} QOMX_BUFFER_INFO;







typedef struct{
  exif_tag_entry_t tag_entry;
  exif_tag_id_t tag_id;
} QEXIF_INFO_DATA;
// # 137 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qomx_core/QOMX_JpegExtensions.h"
typedef struct {
  QEXIF_INFO_DATA *exif_data;
  OMX_U32 numOfEntries;
} QOMX_EXIF_INFO;
// # 160 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qomx_core/QOMX_JpegExtensions.h"
typedef struct {
  OMX_U32 yOffset;
  OMX_U32 cbcrOffset[2];
  OMX_U32 cbcrStartOffset[2];
} QOMX_YUV_FRAME_INFO;
// # 183 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qomx_core/QOMX_JpegExtensions.h"
typedef struct {
  OMX_U32 input_width;
  OMX_U32 input_height;
  OMX_U8 scaling_enabled;
  OMX_CONFIG_RECTTYPE crop_info;
  OMX_U32 output_width;
  OMX_U32 output_height;
  QOMX_YUV_FRAME_INFO tmbOffset;
  OMX_U32 rotation;
} QOMX_THUMBNAIL_INFO;






typedef struct {
  OMX_U8 *mobicatData;
  OMX_U32 mobicatDataLength;
} QOMX_MOBICAT;







typedef struct {
  int fd;
  uint8_t *vaddr;
  uint32_t length;
} QOMX_WORK_BUFFER;





typedef struct {
  OMX_U8 *metadata;
  OMX_U32 metaPayloadSize;
} QOMX_METADATA;





typedef struct {
  OMX_U8 *metaKey;
  OMX_U32 keyLen;
} QOMX_META_ENC_KEY;





typedef enum QOMX_IMG_COLOR_FORMATTYPE {
  OMX_QCOM_IMG_COLOR_FormatYVU420SemiPlanar = OMX_COLOR_FormatVendorStartUnused + 0x300,
  OMX_QCOM_IMG_COLOR_FormatYVU422SemiPlanar,
  OMX_QCOM_IMG_COLOR_FormatYVU422SemiPlanar_h1v2,
  OMX_QCOM_IMG_COLOR_FormatYUV422SemiPlanar_h1v2,
  OMX_QCOM_IMG_COLOR_FormatYVU444SemiPlanar,
  OMX_QCOM_IMG_COLOR_FormatYUV444SemiPlanar,
  OMX_QCOM_IMG_COLOR_FormatYVU420Planar,
  OMX_QCOM_IMG_COLOR_FormatYVU422Planar,
  OMX_QCOM_IMG_COLOR_FormatYVU422Planar_h1v2,
  OMX_QCOM_IMG_COLOR_FormatYUV422Planar_h1v2,
  OMX_QCOM_IMG_COLOR_FormatYVU444Planar,
  OMX_QCOM_IMG_COLOR_FormatYUV444Planar
} QOMX_IMG_COLOR_FORMATTYPE;






typedef enum {
  OMX_Serial_Encoding,
  OMX_Parallel_Encoding
} QOMX_ENCODING_MODE;
// # 272 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../../../mm-image-codec/qomx_core/QOMX_JpegExtensions.h"
typedef struct {
  void *handle;
  void *mem_hdl;
  int8_t isheap;
  size_t size;
  void *vaddr;
  int fd;
} omx_jpeg_ouput_buf_t;






typedef struct {
  int (*get_memory)( omx_jpeg_ouput_buf_t *p_out_buf);
} QOMX_MEM_OPS;





typedef enum {
  QOMX_JPEG_SPEED_MODE_NORMAL,
  QOMX_JPEG_SPEED_MODE_HIGH
} QOMX_JPEG_SPEED_MODE;






typedef struct {
  QOMX_JPEG_SPEED_MODE speedMode;
} QOMX_JPEG_SPEED;
// # 33 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/../common/mm_jpeg_interface.h" 2






typedef enum {
  MM_JPEG_FMT_YUV,
  MM_JPEG_FMT_BITSTREAM
} mm_jpeg_format_t;

typedef enum {
   FLASH_NOT_FIRED,
   FLASH_FIRED
}exif_flash_fired_sate_t;

typedef enum {
   NO_STROBE_RETURN_DETECT = 0x00,
   STROBE_RESERVED = 0x01,
   STROBE_RET_LIGHT_NOT_DETECT = 0x02,
   STROBE_RET_LIGHT_DETECT = 0x03
}exif_strobe_state_t;

typedef enum {
   CAMERA_FLASH_UNKNOWN = 0x00,
   CAMERA_FLASH_COMPULSORY = 0x08,
   CAMERA_FLASH_SUPRESSION = 0x10,
   CAMERA_FLASH_AUTO = 0x18
}exif_flash_mode_t;

typedef enum {
   FLASH_FUNC_PRESENT = 0x00,
   NO_FLASH_FUNC = 0x20
}exif_flash_func_pre_t;

typedef enum {
   NO_REDEYE_MODE = 0x00,
   REDEYE_MODE = 0x40
}exif_redeye_t;

typedef struct {
  cam_ae_exif_debug_t ae_debug_params;
  cam_awb_exif_debug_t awb_debug_params;
  cam_af_exif_debug_t af_debug_params;
  cam_asd_exif_debug_t asd_debug_params;
  cam_stats_buffer_exif_debug_t stats_debug_params;
  uint8_t ae_debug_params_valid;
  uint8_t awb_debug_params_valid;
  uint8_t af_debug_params_valid;
  uint8_t asd_debug_params_valid;
  uint8_t stats_debug_params_valid;
} mm_jpeg_debug_exif_params_t;

typedef struct {
  cam_ae_params_t ae_params;
  cam_auto_focus_data_t af_params;
  uint8_t af_mobicat_params[1000];
  cam_awb_params_t awb_params;
  cam_auto_scene_t scene;
  mm_jpeg_debug_exif_params_t *debug_params;
  cam_sensor_params_t sensor_params;
  cam_flash_mode_t ui_flash_mode;
  exif_flash_func_pre_t flash_presence;
  exif_redeye_t red_eye;
} mm_jpeg_exif_params_t;

typedef struct {
  uint32_t sequence;
  uint8_t *buf_vaddr;
  int fd;
  size_t buf_size;
  mm_jpeg_format_t format;
  cam_frame_len_offset_t offset;
  uint32_t index;
} mm_jpeg_buf_t;

typedef struct {
  uint8_t *buf_vaddr;
  int fd;
  size_t buf_filled_len;
} mm_jpeg_output_t;

typedef enum {
  MM_JPEG_COLOR_FORMAT_YCRCBLP_H2V2,
  MM_JPEG_COLOR_FORMAT_YCBCRLP_H2V2,
  MM_JPEG_COLOR_FORMAT_YCRCBLP_H2V1,
  MM_JPEG_COLOR_FORMAT_YCBCRLP_H2V1,
  MM_JPEG_COLOR_FORMAT_YCRCBLP_H1V2,
  MM_JPEG_COLOR_FORMAT_YCBCRLP_H1V2,
  MM_JPEG_COLOR_FORMAT_YCRCBLP_H1V1,
  MM_JPEG_COLOR_FORMAT_YCBCRLP_H1V1,
  MM_JPEG_COLOR_FORMAT_MONOCHROME,
  MM_JPEG_COLOR_FORMAT_BITSTREAM_H2V2,
  MM_JPEG_COLOR_FORMAT_BITSTREAM_H2V1,
  MM_JPEG_COLOR_FORMAT_BITSTREAM_H1V2,
  MM_JPEG_COLOR_FORMAT_BITSTREAM_H1V1,
  MM_JPEG_COLOR_FORMAT_MAX
} mm_jpeg_color_format;

typedef enum {
  JPEG_JOB_STATUS_DONE = 0,
  JPEG_JOB_STATUS_ERROR
} jpeg_job_status_t;

typedef void (*jpeg_encode_callback_t)(jpeg_job_status_t status,
  uint32_t client_hdl,
  uint32_t jobId,
  mm_jpeg_output_t *p_output,
  void *userData);

typedef struct {

  cam_dimension_t src_dim;


  cam_dimension_t dst_dim;


  cam_rect_t crop;
} mm_jpeg_dim_t;

typedef struct {

  uint32_t num_src_bufs;


  uint32_t num_tmb_bufs;


  uint32_t num_dst_bufs;


  uint32_t encode_thumbnail;


  mm_jpeg_buf_t src_main_buf[(24)];


  mm_jpeg_buf_t src_thumb_buf[(24)];


  mm_jpeg_buf_t dest_buf[(24)];


  mm_jpeg_color_format color_format;


  mm_jpeg_color_format thumb_color_format;


  uint32_t quality;

  jpeg_encode_callback_t jpeg_cb;
  void* userdata;


  mm_jpeg_dim_t thumb_dim;


  uint32_t rotation;


  uint32_t thumb_rotation;


  mm_jpeg_dim_t main_dim;


  uint32_t burst_mode;


  int (*get_memory)( omx_jpeg_ouput_buf_t *p_out_buf);


  int (*put_memory)( omx_jpeg_ouput_buf_t *p_out_buf);
} mm_jpeg_encode_params_t;

typedef struct {

  uint32_t num_src_bufs;


  uint32_t num_dst_bufs;


  mm_jpeg_buf_t src_main_buf[(24)];


  mm_jpeg_buf_t dest_buf[(24)];


  mm_jpeg_color_format color_format;

  jpeg_encode_callback_t jpeg_cb;
  void* userdata;

} mm_jpeg_decode_params_t;

typedef struct {

  int32_t src_index;
  int32_t dst_index;
  uint32_t thumb_index;
  mm_jpeg_dim_t thumb_dim;


  uint32_t rotation;


  mm_jpeg_dim_t main_dim;


  uint32_t session_id;


  cam_metadata_info_t *p_metadata;



  QOMX_EXIF_INFO exif_info;


  mm_jpeg_exif_params_t cam_exif_params;


  mm_jpeg_buf_t work_buf;
} mm_jpeg_encode_job_t;

typedef struct {

  int32_t src_index;
  int32_t dst_index;
  uint32_t tmb_dst_index;


  uint32_t rotation;


  mm_jpeg_dim_t main_dim;


  uint32_t session_id;
} mm_jpeg_decode_job_t;

typedef enum {
  JPEG_JOB_TYPE_ENCODE,
  JPEG_JOB_TYPE_DECODE,
  JPEG_JOB_TYPE_MAX
} mm_jpeg_job_type_t;

typedef struct {
  mm_jpeg_job_type_t job_type;
  union {
    mm_jpeg_encode_job_t encode_job;
    mm_jpeg_decode_job_t decode_job;
  };
} mm_jpeg_job_t;

typedef struct {
  uint32_t w;
  uint32_t h;
} mm_dimension;

typedef struct {

  int (*start_job)(mm_jpeg_job_t* job, uint32_t* job_id);


  int (*abort_job)(uint32_t job_id);


  int (*create_session)(uint32_t client_hdl,
    mm_jpeg_encode_params_t *p_params, uint32_t *p_session_id);


  int (*destroy_session)(uint32_t session_id);


  int (*close) (uint32_t clientHdl);
} mm_jpeg_ops_t;

typedef struct {

  int (*start_job)(mm_jpeg_job_t* job, uint32_t* job_id);


  int (*abort_job)(uint32_t job_id);


  int (*create_session)(uint32_t client_hdl,
    mm_jpeg_decode_params_t *p_params, uint32_t *p_session_id);


  int (*destroy_session)(uint32_t session_id);


  int (*close) (uint32_t clientHdl);
} mm_jpegdec_ops_t;





uint32_t jpeg_open(mm_jpeg_ops_t *ops, mm_dimension picture_size);





uint32_t jpegdec_open(mm_jpegdec_ops_t *ops);
// # 46 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 2
// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_socket.h" 1
// # 39 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_socket.h"
// # 1 "bionic/libc/include/termios.h" 1 3 4
// # 36 "bionic/libc/include/termios.h" 3 4



speed_t cfgetispeed(const struct termios*);
speed_t cfgetospeed(const struct termios*);
void cfmakeraw(struct termios*);
int cfsetispeed(struct termios*, speed_t);
int cfsetospeed(struct termios*, speed_t);
int cfsetspeed(struct termios*, speed_t);
int tcdrain(int);
int tcflow(int, int);
int tcflush(int, int);
int tcgetattr(int, struct termios*);
pid_t tcgetsid(int);
int tcsendbreak(int, int);
int tcsetattr(int, int, const struct termios*);





// # 40 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_socket.h" 2
// # 1 "bionic/libc/include/assert.h" 1 3 4
// # 62 "bionic/libc/include/assert.h" 3 4

__attribute__((__noreturn__)) void __assert(const char *, int, const char *) __attribute__((__noreturn__));
__attribute__((__noreturn__)) void __assert2(const char *, int, const char *, const char *) __attribute__((__noreturn__));

// # 41 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_socket.h" 2







// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/socket.h" 1
// # 11 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/socket.h"
typedef unsigned short __kernel_sa_family_t;

struct __kernel_sockaddr_storage {
 __kernel_sa_family_t ss_family;

 char __data[128 - sizeof(unsigned short)];


} __attribute__ ((aligned((__alignof__ (struct sockaddr *)))));
// # 49 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_socket.h" 2
// # 1 "bionic/libc/include/arpa/inet.h" 1 3 4
// # 34 "bionic/libc/include/arpa/inet.h" 3 4
// # 1 "bionic/libc/include/netinet/in.h" 1 3 4
// # 32 "bionic/libc/include/netinet/in.h" 3 4
// # 1 "bionic/libc/include/endian.h" 1 3 4
// # 31 "bionic/libc/include/endian.h" 3 4
// # 1 "bionic/libc/include/sys/endian.h" 1 3 4
// # 51 "bionic/libc/include/sys/endian.h" 3 4

uint32_t htonl(uint32_t) __attribute__((__const__));
uint16_t htons(uint16_t) __attribute__((__const__));
uint32_t ntohl(uint32_t) __attribute__((__const__));
uint16_t ntohs(uint16_t) __attribute__((__const__));

// # 32 "bionic/libc/include/endian.h" 2 3 4
// # 33 "bionic/libc/include/netinet/in.h" 2 3 4
// # 1 "bionic/libc/include/netinet/in6.h" 1 3 4
// # 32 "bionic/libc/include/netinet/in6.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/in6.h" 1 3 4
// # 30 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/in6.h" 3 4
struct in6_addr {
 union {
  __u8 u6_addr8[16];
  __be16 u6_addr16[8];
  __be32 u6_addr32[4];
 } in6_u;



};

struct sockaddr_in6 {
 unsigned short int sin6_family;
 __be16 sin6_port;
 __be32 sin6_flowinfo;
 struct in6_addr sin6_addr;
 __u32 sin6_scope_id;
};

struct ipv6_mreq {

 struct in6_addr ipv6mr_multiaddr;


 int ipv6mr_ifindex;
};



struct in6_flowlabel_req {
 struct in6_addr flr_dst;
 __be32 flr_label;
 __u8 flr_action;
 __u8 flr_share;
 __u16 flr_flags;
 __u16 flr_expires;
 __u16 flr_linger;
 __u32 __flr_pad;

};
// # 33 "bionic/libc/include/netinet/in6.h" 2 3 4
// # 34 "bionic/libc/include/netinet/in.h" 2 3 4

// # 1 "bionic/libc/include/sys/socket.h" 1 3 4
// # 37 "bionic/libc/include/sys/socket.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/socket.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/socket.h" 1 3 4



// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/sockios.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/sockios.h" 1 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/sockios.h" 2 3 4
// # 5 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm-generic/socket.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/socket.h" 2 3 4
// # 38 "bionic/libc/include/sys/socket.h" 2 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/sockios.h" 1 3 4
// # 21 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/sockios.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/sockios.h" 1 3 4
// # 22 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/sockios.h" 2 3 4
// # 39 "bionic/libc/include/sys/socket.h" 2 3 4


// # 1 "bionic/libc/kernel/uapi/linux/compiler.h" 1 3 4
// # 42 "bionic/libc/include/sys/socket.h" 2 3 4




typedef unsigned short sa_family_t;

struct timespec;
// # 66 "bionic/libc/include/sys/socket.h" 3 4
enum {
  SHUT_RD = 0,

  SHUT_WR,

  SHUT_RDWR

};

struct sockaddr {
  sa_family_t sa_family;
  char sa_data[14];
};

struct linger {
  int l_onoff;
  int l_linger;
};

struct msghdr {
  void* msg_name;
  socklen_t msg_namelen;
  struct iovec* msg_iov;
  size_t msg_iovlen;
  void* msg_control;
  size_t msg_controllen;
  int msg_flags;
};

struct mmsghdr {
  struct msghdr msg_hdr;
  unsigned int msg_len;
};

struct cmsghdr {
  size_t cmsg_len;
  int cmsg_level;
  int cmsg_type;
};
// # 116 "bionic/libc/include/sys/socket.h" 3 4
struct cmsghdr* __cmsg_nxthdr(struct msghdr*, struct cmsghdr*);





struct ucred {
  pid_t pid;
  uid_t uid;
  gid_t gid;
};
// # 271 "bionic/libc/include/sys/socket.h" 3 4
extern int accept(int, struct sockaddr*, socklen_t*);
extern int accept4(int, struct sockaddr*, socklen_t*, int);
extern int bind(int, const struct sockaddr*, int);
extern int connect(int, const struct sockaddr*, socklen_t);
extern int getpeername(int, struct sockaddr*, socklen_t*);
extern int getsockname(int, struct sockaddr*, socklen_t*);
extern int getsockopt(int, int, int, void*, socklen_t*);
extern int listen(int, int);
extern int recvmmsg(int, struct mmsghdr*, unsigned int, int, const struct timespec*);
extern int recvmsg(int, struct msghdr*, int);
extern int sendmmsg(int, const struct mmsghdr*, unsigned int, int);
extern int sendmsg(int, const struct msghdr*, int);
extern int setsockopt(int, int, int, const void*, socklen_t);
extern int shutdown(int, int);
extern int socket(int, int, int);
extern int socketpair(int, int, int, int*);

extern ssize_t send(int, const void*, size_t, int);
extern ssize_t recv(int, void*, size_t, int);

extern ssize_t sendto(int, const void*, size_t, int, const struct sockaddr*, socklen_t);
extern ssize_t recvfrom(int, void*, size_t, int, const struct sockaddr*, socklen_t*);

extern void __recvfrom_error(void) __attribute__((__error__("recvfrom called with size bigger than buffer")));
extern ssize_t __recvfrom_chk(int, void*, size_t, size_t, int, const struct sockaddr*, socklen_t*);
extern ssize_t __recvfrom_real(int, void*, size_t, int, const struct sockaddr*, socklen_t*) __asm__("recvfrom");



extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
ssize_t recvfrom(int fd, void* buf, size_t len, int flags, const struct sockaddr* src_addr, socklen_t* addr_len) {
  size_t bos = __builtin_object_size((buf), 0);


  if (bos == ((size_t) -1)) {
    return __recvfrom_real(fd, buf, len, flags, src_addr, addr_len);
  }

  if (__builtin_constant_p(len) && (len <= bos)) {
    return __recvfrom_real(fd, buf, len, flags, src_addr, addr_len);
  }

  if (__builtin_constant_p(len) && (len > bos)) {
    __recvfrom_error();
  }


  return __recvfrom_chk(fd, buf, len, bos, flags, src_addr, addr_len);
}

extern __inline__ __attribute__((__always_inline__)) __attribute__((gnu_inline)) __attribute__((__artificial__))
ssize_t recv(int socket, void* buf, size_t len, int flags) {
  return recvfrom(socket, buf, len, flags, ((void *)0), 0);
}






// # 36 "bionic/libc/include/netinet/in.h" 2 3 4

// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/in.h" 1 3 4
// # 25 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/in.h" 3 4
enum {
  IPPROTO_IP = 0,
  IPPROTO_ICMP = 1,
  IPPROTO_IGMP = 2,
  IPPROTO_IPIP = 4,
  IPPROTO_TCP = 6,
  IPPROTO_EGP = 8,
  IPPROTO_PUP = 12,
  IPPROTO_UDP = 17,
  IPPROTO_IDP = 22,
  IPPROTO_DCCP = 33,
  IPPROTO_RSVP = 46,
  IPPROTO_GRE = 47,

  IPPROTO_IPV6 = 41,

  IPPROTO_ESP = 50,
  IPPROTO_AH = 51,
  IPPROTO_BEETPH = 94,
  IPPROTO_PIM = 103,

  IPPROTO_COMP = 108,
  IPPROTO_SCTP = 132,
  IPPROTO_UDPLITE = 136,

  IPPROTO_RAW = 255,
  IPPROTO_MAX
};



struct in_addr {
 __be32 s_addr;
};
// # 125 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/in.h" 3 4
struct ip_mreq {
 struct in_addr imr_multiaddr;
 struct in_addr imr_interface;
};

struct ip_mreqn {
 struct in_addr imr_multiaddr;
 struct in_addr imr_address;
 int imr_ifindex;
};

struct ip_mreq_source {
 __be32 imr_multiaddr;
 __be32 imr_interface;
 __be32 imr_sourceaddr;
};

struct ip_msfilter {
 __be32 imsf_multiaddr;
 __be32 imsf_interface;
 __u32 imsf_fmode;
 __u32 imsf_numsrc;
 __be32 imsf_slist[1];
};





struct group_req {
 __u32 gr_interface;
 struct __kernel_sockaddr_storage gr_group;
};

struct group_source_req {
 __u32 gsr_interface;
 struct __kernel_sockaddr_storage gsr_group;
 struct __kernel_sockaddr_storage gsr_source;
};

struct group_filter {
 __u32 gf_interface;
 struct __kernel_sockaddr_storage gf_group;
 __u32 gf_fmode;
 __u32 gf_numsrc;
 struct __kernel_sockaddr_storage gf_slist[1];
};





struct in_pktinfo {
 int ipi_ifindex;
 struct in_addr ipi_spec_dst;
 struct in_addr ipi_addr;
};



struct sockaddr_in {
  __kernel_sa_family_t sin_family;
  __be16 sin_port;
  struct in_addr sin_addr;


  unsigned char __pad[16 - sizeof(short int) -
   sizeof(unsigned short int) - sizeof(struct in_addr)];
};
// # 250 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/in.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/byteorder.h" 1 3 4
// # 21 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/byteorder.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/byteorder/little_endian.h" 1 3 4
// # 12 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/byteorder/little_endian.h" 3 4
// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/swab.h" 1 3 4





// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/swab.h" 1 3 4
// # 26 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/swab.h" 3 4
static __inline__ __u32 __arch_swab32(__u32 x)
{
 __u32 t;
// # 40 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/swab.h" 3 4
  t = x ^ ((x << 16) | (x >> 16));

 x = (x << 24) | (x >> 8);
 t &= ~0x00FF0000;
 x ^= (t >> 8);

 return x;
}
// # 7 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/swab.h" 2 3 4
// # 46 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/swab.h" 3 4
static __inline__ __u16 __fswab16(__u16 val)
{





 return ((__u16)( (((__u16)(val) & (__u16)0x00ffU) << 8) | (((__u16)(val) & (__u16)0xff00U) >> 8)));

}

static __inline__ __u32 __fswab32(__u32 val)
{



 return __arch_swab32(val);



}

static __inline__ __u64 __fswab64(__u64 val)
{





 __u32 h = val >> 32;
 __u32 l = val & ((1ULL << 32) - 1);
 return (((__u64)__fswab32(l)) << 32) | ((__u64)(__fswab32(h)));



}

static __inline__ __u32 __fswahw32(__u32 val)
{



 return ((__u32)( (((__u32)(val) & (__u32)0x0000ffffUL) << 16) | (((__u32)(val) & (__u32)0xffff0000UL) >> 16)));

}

static __inline__ __u32 __fswahb32(__u32 val)
{



 return ((__u32)( (((__u32)(val) & (__u32)0x00ff00ffUL) << 8) | (((__u32)(val) & (__u32)0xff00ff00UL) >> 8)));

}
// # 154 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/swab.h" 3 4
static __inline__ __u16 __swab16p(const __u16 *p)
{



 return (__builtin_constant_p((__u16)(*p)) ? ((__u16)( (((__u16)(*p) & (__u16)0x00ffU) << 8) | (((__u16)(*p) & (__u16)0xff00U) >> 8))) : __fswab16(*p));

}





static __inline__ __u32 __swab32p(const __u32 *p)
{



 return (__builtin_constant_p((__u32)(*p)) ? ((__u32)( (((__u32)(*p) & (__u32)0x000000ffUL) << 24) | (((__u32)(*p) & (__u32)0x0000ff00UL) << 8) | (((__u32)(*p) & (__u32)0x00ff0000UL) >> 8) | (((__u32)(*p) & (__u32)0xff000000UL) >> 24))) : __fswab32(*p));

}





static __inline__ __u64 __swab64p(const __u64 *p)
{



 return (__builtin_constant_p((__u64)(*p)) ? ((__u64)( (((__u64)(*p) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(*p) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(*p) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(*p) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(*p) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(*p) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(*p) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(*p) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(*p));

}







static __inline__ __u32 __swahw32p(const __u32 *p)
{



 return (__builtin_constant_p((__u32)(*p)) ? ((__u32)( (((__u32)(*p) & (__u32)0x0000ffffUL) << 16) | (((__u32)(*p) & (__u32)0xffff0000UL) >> 16))) : __fswahw32(*p));

}







static __inline__ __u32 __swahb32p(const __u32 *p)
{



 return (__builtin_constant_p((__u32)(*p)) ? ((__u32)( (((__u32)(*p) & (__u32)0x00ff00ffUL) << 8) | (((__u32)(*p) & (__u32)0xff00ff00UL) >> 8))) : __fswahb32(*p));

}





static __inline__ void __swab16s(__u16 *p)
{



 *p = __swab16p(p);

}




static __inline__ void __swab32s(__u32 *p)
{



 *p = __swab32p(p);

}





static __inline__ void __swab64s(__u64 *p)
{



 *p = __swab64p(p);

}







static __inline__ void __swahw32s(__u32 *p)
{



 *p = __swahw32p(p);

}







static __inline__ void __swahb32s(__u32 *p)
{



 *p = __swahb32p(p);

}
// # 13 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/byteorder/little_endian.h" 2 3 4
// # 43 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/byteorder/little_endian.h" 3 4
static __inline__ __le64 __cpu_to_le64p(const __u64 *p)
{
 return (__le64)*p;
}
static __inline__ __u64 __le64_to_cpup(const __le64 *p)
{
 return (__u64)*p;
}
static __inline__ __le32 __cpu_to_le32p(const __u32 *p)
{
 return (__le32)*p;
}
static __inline__ __u32 __le32_to_cpup(const __le32 *p)
{
 return (__u32)*p;
}
static __inline__ __le16 __cpu_to_le16p(const __u16 *p)
{
 return (__le16)*p;
}
static __inline__ __u16 __le16_to_cpup(const __le16 *p)
{
 return (__u16)*p;
}
static __inline__ __be64 __cpu_to_be64p(const __u64 *p)
{
 return (__be64)__swab64p(p);
}
static __inline__ __u64 __be64_to_cpup(const __be64 *p)
{
 return __swab64p((__u64 *)p);
}
static __inline__ __be32 __cpu_to_be32p(const __u32 *p)
{
 return (__be32)__swab32p(p);
}
static __inline__ __u32 __be32_to_cpup(const __be32 *p)
{
 return __swab32p((__u32 *)p);
}
static __inline__ __be16 __cpu_to_be16p(const __u16 *p)
{
 return (__be16)__swab16p(p);
}
static __inline__ __u16 __be16_to_cpup(const __be16 *p)
{
 return __swab16p((__u16 *)p);
}
// # 22 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/asm/byteorder.h" 2 3 4
// # 251 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/in.h" 2 3 4
// # 38 "bionic/libc/include/netinet/in.h" 2 3 4

// # 1 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/ipv6.h" 1 3 4
// # 19 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/ipv6.h" 3 4
struct in6_pktinfo {
 struct in6_addr ipi6_addr;
 int ipi6_ifindex;
};

struct ip6_mtuinfo {
 struct sockaddr_in6 ip6m_addr;
 __u32 ip6m_mtu;
};

struct in6_ifreq {
 struct in6_addr ifr6_addr;
 __u32 ifr6_prefixlen;
 int ifr6_ifindex;
};
// # 42 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/ipv6.h" 3 4
struct ipv6_rt_hdr {
 __u8 nexthdr;
 __u8 hdrlen;
 __u8 type;
 __u8 segments_left;





};


struct ipv6_opt_hdr {
 __u8 nexthdr;
 __u8 hdrlen;



} __attribute__((packed));
// # 73 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/ipv6.h" 3 4
struct rt0_hdr {
 struct ipv6_rt_hdr rt_hdr;
 __u32 reserved;
 struct in6_addr addr[0];


};





struct rt2_hdr {
 struct ipv6_rt_hdr rt_hdr;
 __u32 reserved;
 struct in6_addr addr;


};





struct ipv6_destopt_hao {
 __u8 type;
 __u8 length;
 struct in6_addr addr;
} __attribute__((packed));
// # 110 "out/target/product/msm8909/obj/KERNEL_OBJ/usr/include/linux/ipv6.h" 3 4
struct ipv6hdr {

 __u8 priority:4,
    version:4;






 __u8 flow_lbl[3];

 __be16 payload_len;
 __u8 nexthdr;
 __u8 hop_limit;

 struct in6_addr saddr;
 struct in6_addr daddr;
};



enum {
 DEVCONF_FORWARDING = 0,
 DEVCONF_HOPLIMIT,
 DEVCONF_MTU6,
 DEVCONF_ACCEPT_RA,
 DEVCONF_ACCEPT_REDIRECTS,
 DEVCONF_AUTOCONF,
 DEVCONF_DAD_TRANSMITS,
 DEVCONF_RTR_SOLICITS,
 DEVCONF_RTR_SOLICIT_INTERVAL,
 DEVCONF_RTR_SOLICIT_DELAY,
 DEVCONF_USE_TEMPADDR,
 DEVCONF_TEMP_VALID_LFT,
 DEVCONF_TEMP_PREFERED_LFT,
 DEVCONF_REGEN_MAX_RETRY,
 DEVCONF_MAX_DESYNC_FACTOR,
 DEVCONF_MAX_ADDRESSES,
 DEVCONF_FORCE_MLD_VERSION,
 DEVCONF_ACCEPT_RA_DEFRTR,
 DEVCONF_ACCEPT_RA_PINFO,
 DEVCONF_ACCEPT_RA_RTR_PREF,
 DEVCONF_RTR_PROBE_INTERVAL,
 DEVCONF_ACCEPT_RA_RT_INFO_MAX_PLEN,
 DEVCONF_PROXY_NDP,
 DEVCONF_OPTIMISTIC_DAD,
 DEVCONF_ACCEPT_SOURCE_ROUTE,
 DEVCONF_MC_FORWARDING,
 DEVCONF_DISABLE_IPV6,
 DEVCONF_ACCEPT_DAD,
 DEVCONF_FORCE_TLLAO,
 DEVCONF_NDISC_NOTIFY,
 DEVCONF_ACCEPT_RA_PREFIX_ROUTE,
 DEVCONF_ACCEPT_RA_RT_TABLE,
 DEVCONF_ACCEPT_RA_MTU,
 DEVCONF_USE_OPTIMISTIC,
 DEVCONF_USE_OIF_ADDRS_ONLY,
 DEVCONF_MLDV1_UNSOLICITED_REPORT_INTERVAL,
 DEVCONF_MLDV2_UNSOLICITED_REPORT_INTERVAL,
 DEVCONF_MAX
};
// # 40 "bionic/libc/include/netinet/in.h" 2 3 4







typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

int bindresvport(int, struct sockaddr_in*);

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;


// # 35 "bionic/libc/include/arpa/inet.h" 2 3 4



in_addr_t inet_addr(const char*);
int inet_aton(const char*, struct in_addr*);
in_addr_t inet_lnaof(struct in_addr);
struct in_addr inet_makeaddr(in_addr_t, in_addr_t);
in_addr_t inet_netof(struct in_addr);
in_addr_t inet_network(const char*);
char* inet_ntoa(struct in_addr);
const char* inet_ntop(int, const void*, char*, socklen_t);
unsigned int inet_nsap_addr(const char*, unsigned char*, int);
char* inet_nsap_ntoa(int, const unsigned char*, char*);
int inet_pton(int, const char*, void*);


// # 50 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_socket.h" 2
// # 1 "system/core/include/utils/Log.h" 1 3 4
// # 31 "system/core/include/utils/Log.h" 3 4
// # 1 "system/core/include/cutils/log.h" 1 3 4
// # 1 "system/core/include/log/log.h" 1 3 4
// # 37 "system/core/include/log/log.h" 3 4
// # 1 "system/core/include/log/logd.h" 1 3 4
// # 23 "system/core/include/log/logd.h" 3 4
// # 1 "system/core/include/android/log.h" 1 3 4
// # 79 "system/core/include/android/log.h" 3 4
typedef enum android_LogPriority {
    ANDROID_LOG_UNKNOWN = 0,
    ANDROID_LOG_DEFAULT,
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_FATAL,
    ANDROID_LOG_SILENT,
} android_LogPriority;




void __android_log_close();




int __android_log_write(int prio, const char *tag, const char *text);




int __android_log_print(int prio, const char *tag, const char *fmt, ...)
// # 113 "system/core/include/android/log.h" 3 4
    __attribute__ ((format(printf, 3, 4)))


    ;





int __android_log_vprint(int prio, const char *tag,
                         const char *fmt, va_list ap);





void __android_log_assert(const char *cond, const char *tag,
                          const char *fmt, ...)

    __attribute__ ((noreturn))







    __attribute__ ((format(printf, 3, 4)))


    ;
// # 24 "system/core/include/log/logd.h" 2 3 4
// # 36 "system/core/include/log/logd.h" 3 4
// # 1 "system/core/include/log/uio.h" 1 3 4
// # 22 "system/core/include/log/uio.h" 3 4
// # 1 "bionic/libc/include/sys/uio.h" 1 3 4
// # 35 "bionic/libc/include/sys/uio.h" 3 4


int readv(int, const struct iovec*, int);
int writev(int, const struct iovec*, int);
// # 57 "bionic/libc/include/sys/uio.h" 3 4

// # 23 "system/core/include/log/uio.h" 2 3 4
// # 37 "system/core/include/log/logd.h" 2 3 4





int __android_log_bwrite(int32_t tag, const void *payload, size_t len);
int __android_log_btwrite(int32_t tag, char type, const void *payload,
    size_t len);
int __android_log_bswrite(int32_t tag, const char *payload);

int __android_log_security_bwrite(int32_t tag, const void *payload, size_t len);
int __android_log_security_bswrite(int32_t tag, const char *payload);
// # 38 "system/core/include/log/log.h" 2 3 4
// # 489 "system/core/include/log/log.h" 3 4
typedef enum {

    EVENT_TYPE_LIST_STOP = '\n',
    EVENT_TYPE_UNKNOWN = '?',


    EVENT_TYPE_INT = 0,
    EVENT_TYPE_LONG = 1,
    EVENT_TYPE_STRING = 2,
    EVENT_TYPE_LIST = 3,
    EVENT_TYPE_FLOAT = 4,
} AndroidEventLogType;
// # 530 "system/core/include/log/log.h" 3 4
typedef enum log_id {
    LOG_ID_MIN = 0,


    LOG_ID_MAIN = 0,

    LOG_ID_RADIO = 1,

    LOG_ID_EVENTS = 2,
    LOG_ID_SYSTEM = 3,
    LOG_ID_CRASH = 4,
    LOG_ID_SECURITY = 5,
    LOG_ID_KERNEL = 6,


    LOG_ID_MAX
} log_id_t;
// # 557 "system/core/include/log/log.h" 3 4
typedef struct android_log_context_internal *android_log_context;




typedef struct {
    AndroidEventLogType type;
    uint16_t complete;
    uint16_t len;
    union {
        int32_t int32;
        int64_t int64;
        char *string;
        float float32;
    } data;
} android_log_list_element;





android_log_context create_android_logger(uint32_t tag);







int android_log_write_list_begin(android_log_context ctx);
int android_log_write_list_end(android_log_context ctx);

int android_log_write_int32(android_log_context ctx, int32_t value);
int android_log_write_int64(android_log_context ctx, int64_t value);
int android_log_write_string8(android_log_context ctx, const char *value);
int android_log_write_string8_len(android_log_context ctx,
                                  const char *value, size_t maxlen);
int android_log_write_float32(android_log_context ctx, float value);



int android_log_write_list(android_log_context ctx, log_id_t id);




android_log_context create_android_log_parser(const char *msg, size_t len);

android_log_list_element android_log_read_next(android_log_context ctx);
android_log_list_element android_log_peek_next(android_log_context ctx);


int android_log_destroy(android_log_context *ctx);
// # 678 "system/core/include/log/log.h" 3 4
int __android_log_is_loggable(int prio, const char *tag, int default_prio);

int __android_log_security();

int __android_log_error_write(int tag, const char *subTag, int32_t uid, const char *data,
                              uint32_t dataLen);




int __android_log_buf_write(int bufID, int prio, const char *tag, const char *text);
int __android_log_buf_print(int bufID, int prio, const char *tag, const char *fmt, ...)

    __attribute__((__format__(printf, 4, 5)))

    ;
// # 1 "system/core/include/cutils/log.h" 2 3 4
// # 32 "system/core/include/utils/Log.h" 2 3 4
// # 51 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_socket.h" 2
// # 83 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_socket.h"
typedef struct {
  char data[128];
} tuneserver_misc_cmd;

typedef enum {
  TUNESERVER_RECV_COMMAND = 1,
  TUNESERVER_RECV_PAYLOAD_SIZE,
  TUNESERVER_RECV_PAYLOAD,
  TUNESERVER_RECV_RESPONSE,
  TUNESERVERER_RECV_INVALID,
} tuneserver_recv_cmd_t;

typedef struct {
  uint16_t current_cmd;
  tuneserver_recv_cmd_t next_recv_code;
  uint32_t next_recv_len;
  void *recv_buf;
  uint32_t recv_len;
  uint32_t send_len;
  void *send_buf;
} tuneserver_protocol_t;

typedef enum {
  TUNE_PREV_RECV_COMMAND = 1,
  TUNE_PREV_RECV_NEWCNKSIZE,
  TUNE_PREV_RECV_INVALID
} tune_prev_cmd_t;

typedef struct _eztune_preview_protocol_t {
  uint16_t current_cmd;
  tune_prev_cmd_t next_recv_code;
  uint32_t next_recv_len;
  int32_t send_len;
  char* send_buf;
  uint32_t send_buf_size;
  uint32_t new_cnk_size;
  uint32_t new_cmd_available;
} prserver_protocol_t;

typedef union {
  struct sockaddr addr;
  struct sockaddr_in addr_in;
} mm_qcamera_sock_addr_t;

int eztune_server_start(void *lib_handle);
// # 47 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h" 2
// # 95 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_app.h"
typedef enum {
    TUNE_CMD_INIT,
    TUNE_CMD_GET_LIST,
    TUNE_CMD_GET_PARAMS,
    TUNE_CMD_SET_PARAMS,
    TUNE_CMD_MISC,
    TUNE_CMD_DEINIT,
} mm_camera_tune_cmd_t;

typedef enum {
    TUNE_PREVCMD_INIT,
    TUNE_PREVCMD_SETDIM,
    TUNE_PREVCMD_GETINFO,
    TUNE_PREVCMD_GETCHUNKSIZE,
    TUNE_PREVCMD_GETFRAME,
    TUNE_PREVCMD_UNSUPPORTED,
    TUNE_PREVCMD_DEINIT,
} mm_camera_tune_prevcmd_t;

typedef void (*cam_stream_user_cb) (mm_camera_buf_def_t *frame);
typedef void (*prev_callback) (mm_camera_buf_def_t *preview_frame);


typedef struct {
  char *send_buf;
  uint32_t send_len;
  void *next;
} eztune_prevcmd_rsp;

typedef struct {
    int (*command_process) (void *recv, mm_camera_tune_cmd_t cmd,
      void *param, char *send_buf, uint32_t send_len);
    int (*prevcommand_process) (void *recv, mm_camera_tune_prevcmd_t cmd,
      void *param, char **send_buf, uint32_t *send_len);
    void (*prevframe_callback) (mm_camera_buf_def_t *preview_frame);
} mm_camera_tune_func_t;

typedef struct {
    mm_camera_tune_func_t *func_tbl;
    void *lib_handle;
}mm_camera_tuning_lib_params_t;

typedef enum {
    MM_CAMERA_OK,
    MM_CAMERA_E_GENERAL,
    MM_CAMERA_E_NO_MEMORY,
    MM_CAMERA_E_NOT_SUPPORTED,
    MM_CAMERA_E_INVALID_INPUT,
    MM_CAMERA_E_INVALID_OPERATION,
    MM_CAMERA_E_ENCODE,
    MM_CAMERA_E_BUFFER_REG,
    MM_CAMERA_E_PMEM_ALLOC,
    MM_CAMERA_E_CAPTURE_FAILED,
    MM_CAMERA_E_CAPTURE_TIMEOUT,
} mm_camera_status_type_t;

typedef enum {
    MM_CHANNEL_TYPE_ZSL,
    MM_CHANNEL_TYPE_CAPTURE,
    MM_CHANNEL_TYPE_PREVIEW,
    MM_CHANNEL_TYPE_SNAPSHOT,
    MM_CHANNEL_TYPE_VIDEO,
    MM_CHANNEL_TYPE_RDI,
    MM_CHANNEL_TYPE_REPROCESS,
    MM_CHANNEL_TYPE_MAX
} mm_camera_channel_type_t;

typedef struct {
    int fd;
    int main_ion_fd;
    ion_user_handle_t handle;
    size_t size;
    void * data;
} mm_camera_app_meminfo_t;

typedef struct {
    mm_camera_buf_def_t buf;
    mm_camera_app_meminfo_t mem_info;
} mm_camera_app_buf_t;

typedef struct {
    uint32_t s_id;
    mm_camera_stream_config_t s_config;
    cam_frame_len_offset_t offset;
    uint8_t num_of_bufs;
    uint32_t multipleOf;
    mm_camera_app_buf_t s_bufs[(24)];
    mm_camera_app_buf_t s_info_buf;
} mm_camera_stream_t;

typedef struct {
    uint32_t ch_id;
    uint8_t num_streams;
    mm_camera_stream_t streams[4];
} mm_camera_channel_t;

typedef void (*release_data_fn)(void* data, void *user_data);

typedef struct {
    struct cam_list list;
    void* data;
} camera_q_node;

typedef struct {
    camera_q_node m_head;
    int m_size;
    pthread_mutex_t m_lock;
    release_data_fn m_dataFn;
    void * m_userData;
} mm_camera_queue_t;

typedef struct {
    uint16_t user_input_display_width;
    uint16_t user_input_display_height;
} USER_INPUT_DISPLAY_T;

typedef struct {
    mm_camera_vtbl_t *cam;
    uint8_t num_channels;
    mm_camera_channel_t channels[MM_CHANNEL_TYPE_MAX];
    mm_jpeg_ops_t jpeg_ops;
    uint32_t jpeg_hdl;
    mm_camera_app_buf_t cap_buf;
    mm_camera_app_buf_t parm_buf;

    uint32_t current_jpeg_sess_id;
    mm_camera_super_buf_t* current_job_frames;
    uint32_t current_job_id;
    mm_camera_app_buf_t jpeg_buf;

    int fb_fd;
    struct fb_var_screeninfo vinfo;
    struct mdp_overlay data_overlay;
    uint32_t slice_size;
    uint32_t buffer_width, buffer_height;
    uint32_t buffer_size;
    cam_format_t buffer_format;
    uint32_t frame_size;
    uint32_t frame_count;
    int encodeJpeg;
    int zsl_enabled;
    int8_t focus_supported;
    cam_stream_user_cb user_preview_cb;
    cam_stream_user_cb user_metadata_cb;
    parm_buffer_new_t *params_buffer;
    USER_INPUT_DISPLAY_T preview_resolution;


    int8_t enable_reproc;
    int32_t reproc_sharpness;
    cam_denoise_param_t reproc_wnr;
    int8_t enable_CAC;
    mm_camera_queue_t pp_frames;
    mm_camera_stream_t *reproc_stream;
    cam_metadata_info_t *metadata;
    int8_t is_chromatix_reload;
    tune_chromatix_t tune_data;
} mm_camera_test_obj_t;

typedef struct {
  void *ptr;
  void* ptr_jpeg;

  uint8_t (*get_num_of_cameras) ();
  mm_camera_vtbl_t *(*mm_camera_open) (uint8_t camera_idx);
  uint32_t (*jpeg_open) (mm_jpeg_ops_t *ops, mm_dimension picture_size);

} hal_interface_lib_t;

typedef struct {
    uint8_t num_cameras;
    hal_interface_lib_t hal_lib;
} mm_camera_app_t;

typedef struct {
    uint32_t width;
    uint32_t height;
} mm_camera_lib_snapshot_params;

typedef enum {
    MM_CAMERA_LIB_NO_ACTION = 0,
    MM_CAMERA_LIB_RAW_CAPTURE,
    MM_CAMERA_LIB_JPEG_CAPTURE,
    MM_CAMERA_LIB_SET_FOCUS_MODE,
    MM_CAMERA_LIB_DO_AF,
    MM_CAMERA_LIB_CANCEL_AF,
    MM_CAMERA_LIB_LOCK_AE,
    MM_CAMERA_LIB_UNLOCK_AE,
    MM_CAMERA_LIB_LOCK_AWB,
    MM_CAMERA_LIB_UNLOCK_AWB,
    MM_CAMERA_LIB_GET_CHROMATIX,
    MM_CAMERA_LIB_SET_RELOAD_CHROMATIX,
    MM_CAMERA_LIB_GET_AFTUNE,
    MM_CAMERA_LIB_SET_RELOAD_AFTUNE,
    MM_CAMERA_LIB_SET_AUTOFOCUS_TUNING,
    MM_CAMERA_LIB_ZSL_ENABLE,
    MM_CAMERA_LIB_EV,
    MM_CAMERA_LIB_ANTIBANDING,
    MM_CAMERA_LIB_SET_VFE_COMMAND,
    MM_CAMERA_LIB_SET_POSTPROC_COMMAND,
    MM_CAMERA_LIB_SET_3A_COMMAND,
    MM_CAMERA_LIB_AEC_ENABLE,
    MM_CAMERA_LIB_AEC_DISABLE,
    MM_CAMERA_LIB_AF_ENABLE,
    MM_CAMERA_LIB_AF_DISABLE,
    MM_CAMERA_LIB_AWB_ENABLE,
    MM_CAMERA_LIB_AWB_DISABLE,
    MM_CAMERA_LIB_AEC_FORCE_LC,
    MM_CAMERA_LIB_AEC_FORCE_GAIN,
    MM_CAMERA_LIB_AEC_FORCE_EXP,
    MM_CAMERA_LIB_AEC_FORCE_SNAP_LC,
    MM_CAMERA_LIB_AEC_FORCE_SNAP_GAIN,
    MM_CAMERA_LIB_AEC_FORCE_SNAP_EXP,
    MM_CAMERA_LIB_WB,
    MM_CAMERA_LIB_EXPOSURE_METERING,
    MM_CAMERA_LIB_BRIGHTNESS,
    MM_CAMERA_LIB_CONTRAST,
    MM_CAMERA_LIB_SATURATION,
    MM_CAMERA_LIB_SHARPNESS,
    MM_CAMERA_LIB_ISO,
    MM_CAMERA_LIB_ZOOM,
    MM_CAMERA_LIB_BESTSHOT,
    MM_CAMERA_LIB_FLASH,
    MM_CAMERA_LIB_FPS_RANGE,
    MM_CAMERA_LIB_WNR_ENABLE,
    MM_CAMERA_LIB_SET_TINTLESS,
} mm_camera_lib_commands;

typedef struct {
    int32_t stream_width, stream_height;
    cam_focus_mode_type af_mode;
} mm_camera_lib_params;

typedef struct {
  tuneserver_protocol_t *proto;
  int clientsocket_id;
  prserver_protocol_t *pr_proto;
  int pr_clientsocket_id;
  mm_camera_tuning_lib_params_t tuning_params;
} tuningserver_t;

typedef struct {
    mm_camera_app_t app_ctx;
    mm_camera_test_obj_t test_obj;
    mm_camera_lib_params current_params;
    int stream_running;
    tuningserver_t tsctrl;
} mm_camera_lib_ctx;

typedef mm_camera_lib_ctx mm_camera_lib_handle;

typedef int (*mm_app_test_t) (mm_camera_app_t *cam_apps);
typedef struct {
    mm_app_test_t f;
    int r;
} mm_app_tc_t;

extern int mm_app_unit_test_entry(mm_camera_app_t *cam_app);
extern int mm_app_dual_test_entry(mm_camera_app_t *cam_app);
extern void mm_app_dump_frame(mm_camera_buf_def_t *frame,
                              char *name,
                              char *ext,
                              uint32_t frame_idx);
extern void mm_app_dump_jpeg_frame(const void * data,
                                   size_t size,
                                   char* name,
                                   char* ext,
                                   uint32_t index);
extern int mm_camera_app_timedwait(uint8_t seconds);
extern int mm_camera_app_wait();
extern void mm_camera_app_done();
extern int mm_app_alloc_bufs(mm_camera_app_buf_t* app_bufs,
                             cam_frame_len_offset_t *frame_offset_info,
                             uint8_t num_bufs,
                             uint8_t is_streambuf,
                             size_t multipleOf);
extern int mm_app_release_bufs(uint8_t num_bufs,
                               mm_camera_app_buf_t* app_bufs);
extern int mm_app_stream_initbuf(cam_frame_len_offset_t *frame_offset_info,
                                 uint8_t *num_bufs,
                                 uint8_t **initial_reg_flag,
                                 mm_camera_buf_def_t **bufs,
                                 mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                                 void *user_data);
extern int32_t mm_app_stream_deinitbuf(mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                                       void *user_data);
extern int mm_app_cache_ops(mm_camera_app_meminfo_t *mem_info, int cmd);
extern int32_t mm_app_stream_clean_invalidate_buf(uint32_t index, void *user_data);
extern int32_t mm_app_stream_invalidate_buf(uint32_t index, void *user_data);
extern int mm_app_open(mm_camera_app_t *cam_app,
                       int cam_id,
                       mm_camera_test_obj_t *test_obj);
extern int mm_app_close(mm_camera_test_obj_t *test_obj);
extern mm_camera_channel_t * mm_app_add_channel(
                                         mm_camera_test_obj_t *test_obj,
                                         mm_camera_channel_type_t ch_type,
                                         mm_camera_channel_attr_t *attr,
                                         mm_camera_buf_notify_t channel_cb,
                                         void *userdata);
extern int mm_app_del_channel(mm_camera_test_obj_t *test_obj,
                              mm_camera_channel_t *channel);
extern mm_camera_stream_t * mm_app_add_stream(mm_camera_test_obj_t *test_obj,
                                              mm_camera_channel_t *channel);
extern int mm_app_del_stream(mm_camera_test_obj_t *test_obj,
                             mm_camera_channel_t *channel,
                             mm_camera_stream_t *stream);
extern int mm_app_config_stream(mm_camera_test_obj_t *test_obj,
                                mm_camera_channel_t *channel,
                                mm_camera_stream_t *stream,
                                mm_camera_stream_config_t *config);
extern int mm_app_start_channel(mm_camera_test_obj_t *test_obj,
                                mm_camera_channel_t *channel);
extern int mm_app_stop_channel(mm_camera_test_obj_t *test_obj,
                               mm_camera_channel_t *channel);
extern mm_camera_channel_t *mm_app_get_channel_by_type(
                                    mm_camera_test_obj_t *test_obj,
                                    mm_camera_channel_type_t ch_type);

extern int mm_app_start_preview(mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_preview(mm_camera_test_obj_t *test_obj);
extern int mm_app_start_preview_zsl(mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_preview_zsl(mm_camera_test_obj_t *test_obj);
extern mm_camera_channel_t * mm_app_add_preview_channel(
                                mm_camera_test_obj_t *test_obj);
extern mm_camera_stream_t * mm_app_add_raw_stream(mm_camera_test_obj_t *test_obj,
                                                mm_camera_channel_t *channel,
                                                mm_camera_buf_notify_t stream_cb,
                                                void *userdata,
                                                uint8_t num_bufs,
                                                uint8_t num_burst);
extern int mm_app_stop_and_del_channel(mm_camera_test_obj_t *test_obj,
                                       mm_camera_channel_t *channel);
extern mm_camera_channel_t * mm_app_add_snapshot_channel(
                                               mm_camera_test_obj_t *test_obj);
extern mm_camera_stream_t * mm_app_add_snapshot_stream(
                                                mm_camera_test_obj_t *test_obj,
                                                mm_camera_channel_t *channel,
                                                mm_camera_buf_notify_t stream_cb,
                                                void *userdata,
                                                uint8_t num_bufs,
                                                uint8_t num_burst);
extern mm_camera_stream_t * mm_app_add_metadata_stream(mm_camera_test_obj_t *test_obj,
                                               mm_camera_channel_t *channel,
                                               mm_camera_buf_notify_t stream_cb,
                                               void *userdata,
                                               uint8_t num_bufs);
extern int mm_app_start_record_preview(mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_record_preview(mm_camera_test_obj_t *test_obj);
extern int mm_app_start_record(mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_record(mm_camera_test_obj_t *test_obj);
extern int mm_app_start_live_snapshot(mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_live_snapshot(mm_camera_test_obj_t *test_obj);
extern int mm_app_start_capture(mm_camera_test_obj_t *test_obj,
                                uint8_t num_snapshots);
extern int mm_app_stop_capture(mm_camera_test_obj_t *test_obj);
extern int mm_app_start_capture_raw(mm_camera_test_obj_t *test_obj,
                                    uint8_t num_snapshots);
extern int mm_app_stop_capture_raw(mm_camera_test_obj_t *test_obj);
extern int mm_app_start_rdi(mm_camera_test_obj_t *test_obj, uint8_t num_burst);
extern int mm_app_stop_rdi(mm_camera_test_obj_t *test_obj);
extern int mm_app_initialize_fb(mm_camera_test_obj_t *test_obj);
extern int mm_app_close_fb(mm_camera_test_obj_t *test_obj);
extern int mm_app_fb_write(mm_camera_test_obj_t *test_obj, char *buffer);
extern int mm_app_overlay_display(mm_camera_test_obj_t *test_obj, int bufferFd);
extern int mm_app_allocate_ion_memory(mm_camera_app_buf_t *buf, unsigned int ion_type);
extern int mm_app_deallocate_ion_memory(mm_camera_app_buf_t *buf);
extern int mm_app_set_params(mm_camera_test_obj_t *test_obj,
                      cam_intf_parm_type_t param_type,
                      int32_t value);
extern int mm_app_set_preview_fps_range(mm_camera_test_obj_t *test_obj,
                        cam_fps_range_t *fpsRange);
extern int mm_app_set_face_detection(mm_camera_test_obj_t *test_obj,
                        cam_fd_set_parm_t *fd_set_parm);
extern int mm_app_set_metadata_usercb(mm_camera_test_obj_t *test_obj,
                      cam_stream_user_cb usercb);
extern int mm_app_set_params_impl(mm_camera_test_obj_t *test_obj,
                   cam_intf_parm_type_t param_type,
                   uint32_t param_len,
                   void* param_val);
extern int mm_app_set_flash_mode(mm_camera_test_obj_t *test_obj,
                        cam_flash_mode_t flashMode);



int mm_camera_lib_open(mm_camera_lib_handle *handle, int cam_id);
int mm_camera_lib_get_caps(mm_camera_lib_handle *handle,
                           cam_capability_t *caps);
int mm_camera_lib_start_stream(mm_camera_lib_handle *handle);
int mm_camera_lib_send_command(mm_camera_lib_handle *handle,
                               mm_camera_lib_commands cmd,
                               void *data, void *out_data);
int mm_camera_lib_stop_stream(mm_camera_lib_handle *handle);
int mm_camera_lib_number_of_cameras(mm_camera_lib_handle *handle);
int mm_camera_lib_close(mm_camera_lib_handle *handle);
int32_t mm_camera_load_tuninglibrary(
  mm_camera_tuning_lib_params_t *tuning_param);
int mm_camera_lib_set_preview_usercb(
  mm_camera_lib_handle *handle, cam_stream_user_cb cb);


int mm_app_start_regression_test(int run_tc);
int mm_app_load_hal(mm_camera_app_t *my_cam_app);

extern int createEncodingSession(mm_camera_test_obj_t *test_obj,
                          mm_camera_stream_t *m_stream,
                          mm_camera_buf_def_t *m_frame);
extern int encodeData(mm_camera_test_obj_t *test_obj, mm_camera_super_buf_t* recvd_frame,
               mm_camera_stream_t *m_stream);
extern int mm_app_take_picture(mm_camera_test_obj_t *test_obj, uint8_t);

extern mm_camera_channel_t * mm_app_add_reprocess_channel(mm_camera_test_obj_t *test_obj,
                                                   mm_camera_stream_t *source_stream);
extern int mm_app_start_reprocess(mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_reprocess(mm_camera_test_obj_t *test_obj);
extern int mm_app_do_reprocess(mm_camera_test_obj_t *test_obj,
        mm_camera_buf_def_t *frame,
        uint32_t meta_idx,
        mm_camera_super_buf_t *super_buf,
        mm_camera_stream_t *src_meta);
extern void mm_app_release_ppinput(void *data, void *user_data);

extern int mm_camera_queue_init(mm_camera_queue_t *queue,
                         release_data_fn data_rel_fn,
                         void *user_data);
extern int mm_qcamera_queue_release(mm_camera_queue_t *queue);
extern int mm_qcamera_queue_isempty(mm_camera_queue_t *queue);
extern int mm_qcamera_queue_enqueue(mm_camera_queue_t *queue, void *data);
extern void* mm_qcamera_queue_dequeue(mm_camera_queue_t *queue,
                                      int bFromHead);
extern void mm_qcamera_queue_flush(mm_camera_queue_t *queue);
// # 15 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2
// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_dbg.h" 1
// # 16 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2
// # 1 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc/mm_qcamera_socket.h" 1
// # 17 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c" 2

#include "victor_camera.h"

typedef struct cameraobj_t
{
  mm_camera_lib_handle lib_handle;

} CameraObj;


// # 47 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c"
int enableAFR(mm_camera_lib_handle *lib_handle)
{
    size_t i, j;
    float max_range = 0.0f;
    cam_capability_t cap;
    int rc = MM_CAMERA_OK;

    if ( ((void *)0) == lib_handle ) {
        return MM_CAMERA_E_INVALID_INPUT;
    }

    rc = mm_camera_lib_get_caps(lib_handle, &cap);
    if ( MM_CAMERA_OK != rc ) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:mm_camera_lib_get_caps() err=%d\n", __PRETTY_FUNCTION__, rc));
        return rc;
    }

    for( i = 0, j = 0 ; i < cap.fps_ranges_tbl_cnt ; i++ ) {
        if ( max_range < (cap.fps_ranges_tbl[i].max_fps - cap.fps_ranges_tbl[i].min_fps) ) {
            j = i;
        }
    }

    rc = mm_camera_lib_send_command(lib_handle,
                                    MM_CAMERA_LIB_FPS_RANGE,
                                    &cap.fps_ranges_tbl[j],
                                    ((void *)0));

    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s : FPS range [%5.2f:%5.2f] rc = %d", __PRETTY_FUNCTION__, cap.fps_ranges_tbl[j].min_fps, cap.fps_ranges_tbl[j].max_fps, rc))



                 ;

    return rc;
}

mm_camera_stream_t * mm_app_add_raw_stream(mm_camera_test_obj_t *test_obj,
                                                mm_camera_channel_t *channel,
                                                mm_camera_buf_notify_t stream_cb,
                                                void *userdata,
                                                uint8_t num_bufs,
                                                uint8_t num_burst)
{
    int rc = MM_CAMERA_OK;
    mm_camera_stream_t *stream = ((void *)0);
    cam_capability_t *cam_cap = (cam_capability_t *)(test_obj->cap_buf.buf.buffer);

    stream = mm_app_add_stream(test_obj, channel);
    if (((void *)0) == stream) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: add stream failed\n", __PRETTY_FUNCTION__));
        return ((void *)0);
    }

    stream->s_config.mem_vtbl.get_bufs = mm_app_stream_initbuf;
    stream->s_config.mem_vtbl.put_bufs = mm_app_stream_deinitbuf;
    stream->s_config.mem_vtbl.invalidate_buf = mm_app_stream_invalidate_buf;
    stream->s_config.mem_vtbl.user_data = (void *)stream;
    stream->s_config.stream_cb = stream_cb;
    stream->s_config.userdata = userdata;
    stream->num_of_bufs = num_bufs;

    stream->s_config.stream_info = (cam_stream_info_t *)stream->s_info_buf.buf.buffer;
    memset(stream->s_config.stream_info, 0, sizeof(cam_stream_info_t));
    stream->s_config.stream_info->stream_type = CAM_STREAM_TYPE_RAW;
    if (num_burst == 0) {
        stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_CONTINUOUS;
    } else {
        stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_BURST;
        stream->s_config.stream_info->num_of_burst = num_burst;
    }
    stream->s_config.stream_info->fmt = test_obj->buffer_format;
    if ( test_obj->buffer_width == 0 || test_obj->buffer_height == 0 ) {
        stream->s_config.stream_info->dim.width = 1024;
        stream->s_config.stream_info->dim.height = 768;
    } else {
        stream->s_config.stream_info->dim.width = (int32_t)test_obj->buffer_width;
        stream->s_config.stream_info->dim.height = (int32_t)test_obj->buffer_height;
    }
    stream->s_config.padding_info = cam_cap->padding_info;

    rc = mm_app_config_stream(test_obj, channel, stream, &stream->s_config);
    if (MM_CAMERA_OK != rc) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:config preview stream err=%d\n", __PRETTY_FUNCTION__, rc));
        return ((void *)0);
    }

    return stream;
}

int mm_camera_lib_get_caps(mm_camera_lib_handle *handle,
                           cam_capability_t *caps)
{
    int rc = MM_CAMERA_OK;

    if ( ((void *)0) == handle ) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", " %s : Invalid handle", __PRETTY_FUNCTION__));
        rc = MM_CAMERA_E_INVALID_INPUT;
        goto EXIT;
    }

    if ( ((void *)0) == caps ) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", " %s : Invalid capabilities structure", __PRETTY_FUNCTION__));
        rc = MM_CAMERA_E_INVALID_INPUT;
        goto EXIT;
    }

    *caps = *( (cam_capability_t *) handle->test_obj.cap_buf.mem_info.data );

EXIT:

    return rc;
}

static pthread_mutex_t app_mutex;
static int thread_status = 0;
static pthread_cond_t app_cond_v;





int mm_camera_app_wait()
{
    int rc = 0;
    pthread_mutex_lock(&app_mutex);
    if(0 == thread_status){
        pthread_cond_wait(&app_cond_v, &app_mutex);
    }
    thread_status = 0;
    pthread_mutex_unlock(&app_mutex);
    return rc;
}

void mm_camera_app_done()
{
  pthread_mutex_lock(&app_mutex);
  thread_status = 1;
  pthread_cond_signal(&app_cond_v);
  pthread_mutex_unlock(&app_mutex);
}


int mm_app_load_hal(mm_camera_app_t *my_cam_app)
{
    memset(&my_cam_app->hal_lib, 0, sizeof(hal_interface_lib_t));
    my_cam_app->hal_lib.ptr = dlopen("libmmcamera_interface.so", RTLD_NOW);
    my_cam_app->hal_lib.ptr_jpeg = dlopen("libmmjpeg_interface.so", RTLD_NOW);
    if (!my_cam_app->hal_lib.ptr || !my_cam_app->hal_lib.ptr_jpeg) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s Error opening HAL library %s\n", __PRETTY_FUNCTION__, dlerror()));
        return -MM_CAMERA_E_GENERAL;
    }
    *(void **)&(my_cam_app->hal_lib.get_num_of_cameras) =
        dlsym(my_cam_app->hal_lib.ptr, "get_num_of_cameras");
    *(void **)&(my_cam_app->hal_lib.mm_camera_open) =
        dlsym(my_cam_app->hal_lib.ptr, "camera_open");
    *(void **)&(my_cam_app->hal_lib.jpeg_open) =
        dlsym(my_cam_app->hal_lib.ptr_jpeg, "jpeg_open");

    if (my_cam_app->hal_lib.get_num_of_cameras == ((void *)0) ||
        my_cam_app->hal_lib.mm_camera_open == ((void *)0) ||
        my_cam_app->hal_lib.jpeg_open == ((void *)0)) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s Error loading HAL sym %s\n", __PRETTY_FUNCTION__, dlerror()));
        return -MM_CAMERA_E_GENERAL;
    }

    my_cam_app->num_cameras = my_cam_app->hal_lib.get_num_of_cameras();
    do{}while(0);

    return MM_CAMERA_OK;
}

int mm_app_allocate_ion_memory(mm_camera_app_buf_t *buf, unsigned int ion_type)
{
    int rc = MM_CAMERA_OK;
    struct ion_handle_data handle_data;
    struct ion_allocation_data alloc;
    struct ion_fd_data ion_info_fd;
    int main_ion_fd = 0;
    void *data = ((void *)0);

    main_ion_fd = open("/dev/ion", 00000000);
    if (main_ion_fd <= 0) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "Ion dev open failed %s\n", strerror((*__errno()))));
        goto ION_OPEN_FAILED;
    }

    memset(&alloc, 0, sizeof(alloc));
    alloc.len = buf->mem_info.size;

    alloc.len = (alloc.len + 4095U) & (~4095U);
    alloc.align = 4096;
    alloc.flags = 1;
    alloc.heap_id_mask = ion_type;
    rc = ioctl(main_ion_fd, (((2U|1U) << (((0 +8)+8)+14)) | ((('I')) << (0 +8)) | (((0)) << 0) | ((((sizeof(struct ion_allocation_data)))) << ((0 +8)+8))), &alloc);
    if (rc < 0) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "ION allocation failed\n"));
        goto ION_ALLOC_FAILED;
    }

    memset(&ion_info_fd, 0, sizeof(ion_info_fd));
    ion_info_fd.handle = alloc.handle;
    rc = ioctl(main_ion_fd, (((2U|1U) << (((0 +8)+8)+14)) | ((('I')) << (0 +8)) | (((4)) << 0) | ((((sizeof(struct ion_fd_data)))) << ((0 +8)+8))), &ion_info_fd);
    if (rc < 0) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "ION map failed %s\n", strerror((*__errno()))));
        goto ION_MAP_FAILED;
    }

    data = mmap(((void *)0),
                alloc.len,
                0x1 | 0x2,
                0x01,
                ion_info_fd.fd,
                0);

    if (data == ((void *)-1)) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "ION_MMAP_FAILED: %s (%d)\n", strerror((*__errno())), (*__errno())));
        goto ION_MAP_FAILED;
    }
    buf->mem_info.main_ion_fd = main_ion_fd;
    buf->mem_info.fd = ion_info_fd.fd;
    buf->mem_info.handle = ion_info_fd.handle;
    buf->mem_info.size = alloc.len;
    buf->mem_info.data = data;
    return MM_CAMERA_OK;

ION_MAP_FAILED:
    memset(&handle_data, 0, sizeof(handle_data));
    handle_data.handle = ion_info_fd.handle;
    ioctl(main_ion_fd, (((2U|1U) << (((0 +8)+8)+14)) | ((('I')) << (0 +8)) | (((1)) << 0) | ((((sizeof(struct ion_handle_data)))) << ((0 +8)+8))), &handle_data);
ION_ALLOC_FAILED:
    close(main_ion_fd);
ION_OPEN_FAILED:
    return -MM_CAMERA_E_GENERAL;
}

int mm_app_deallocate_ion_memory(mm_camera_app_buf_t *buf)
{
  struct ion_handle_data handle_data;
  int rc = 0;

  rc = munmap(buf->mem_info.data, buf->mem_info.size);

  if (buf->mem_info.fd > 0) {
      close(buf->mem_info.fd);
      buf->mem_info.fd = 0;
  }

  if (buf->mem_info.main_ion_fd > 0) {
      memset(&handle_data, 0, sizeof(handle_data));
      handle_data.handle = buf->mem_info.handle;
      ioctl(buf->mem_info.main_ion_fd, (((2U|1U) << (((0 +8)+8)+14)) | ((('I')) << (0 +8)) | (((1)) << 0) | ((((sizeof(struct ion_handle_data)))) << ((0 +8)+8))), &handle_data);
      close(buf->mem_info.main_ion_fd);
      buf->mem_info.main_ion_fd = 0;
  }
  return rc;
}


int mm_app_cache_ops(mm_camera_app_meminfo_t *mem_info,
                     int cmd)
{
    struct ion_flush_data cache_inv_data;
    struct ion_custom_data custom_data;
    int ret = MM_CAMERA_OK;


    if (((void *)0) == mem_info) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: mem_info is NULL, return here", __PRETTY_FUNCTION__));
        return -MM_CAMERA_E_GENERAL;
    }

    memset(&cache_inv_data, 0, sizeof(cache_inv_data));
    memset(&custom_data, 0, sizeof(custom_data));
    cache_inv_data.vaddr = mem_info->data;
    cache_inv_data.fd = mem_info->fd;
    cache_inv_data.handle = mem_info->handle;
    cache_inv_data.length = (unsigned int)mem_info->size;
    custom_data.cmd = (unsigned int)cmd;
    custom_data.arg = (unsigned long)&cache_inv_data;

    do{}while(0)


                               ;
    if(mem_info->main_ion_fd > 0) {
        if(ioctl(mem_info->main_ion_fd, (((2U|1U) << (((0 +8)+8)+14)) | ((('I')) << (0 +8)) | (((6)) << 0) | ((((sizeof(struct ion_custom_data)))) << ((0 +8)+8))), &custom_data) < 0) {
            ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Cache Invalidate failed\n", __PRETTY_FUNCTION__));
            ret = -MM_CAMERA_E_GENERAL;
        }
    }


    return ret;
}

void mm_app_dump_frame(mm_camera_buf_def_t *frame,
                       char *name,
                       char *ext,
                       uint32_t frame_idx)
{
  static int seq = 0;

    char file_name[64];
    int file_fd;
    int i;
    int offset = 0;
    if ( frame != ((void *)0)) {
      snprintf(file_name, sizeof(file_name), "/data/misc/camera/test/%s_%04d.%s", name, frame_idx, ext);

        file_fd = open(file_name, 00000002 | 00000100, 0777);
        if (file_fd < 0) {
            ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: cannot open file %s \n", __PRETTY_FUNCTION__, file_name));
        } else {
            for (i = 0; i < frame->num_planes; i++) {
                do{}while(0)

                                                                          ;
                write(file_fd,
                      (uint8_t *)frame->buffer + offset,
                      frame->planes[i].length);
                offset += (int)frame->planes[i].length;
            }

            close(file_fd);
            do{}while(0);
        }
    }
}

void mm_app_dump_jpeg_frame(const void * data, size_t size, char* name,
        char* ext, uint32_t index)
{
    char buf[64];
    int file_fd;
    if ( data != ((void *)0)) {
        snprintf(buf, sizeof(buf), "/data/misc/camera/test/%s_%u.%s", name, index, ext);
        do{}while(0);
        file_fd = open(buf, 00000002 | 00000100, 0777);
        write(file_fd, data, size);
        close(file_fd);
    }
}

int mm_app_alloc_bufs(mm_camera_app_buf_t* app_bufs,
                      cam_frame_len_offset_t *frame_offset_info,
                      uint8_t num_bufs,
                      uint8_t is_streambuf,
                      size_t multipleOf)
{
    uint32_t i, j;
    unsigned int ion_type = 0x1 << ION_SYSTEM_HEAP_ID;

    if (is_streambuf) {
        ion_type |= 0x1 << ION_SYSTEM_HEAP_ID;
    }

    for (i = 0; i < num_bufs ; i++) {
        if ( 0 < multipleOf ) {
            size_t m = frame_offset_info->frame_len / multipleOf;
            if ( ( frame_offset_info->frame_len % multipleOf ) != 0 ) {
                m++;
            }
            app_bufs[i].mem_info.size = m * multipleOf;
        } else {
            app_bufs[i].mem_info.size = frame_offset_info->frame_len;
        }
        mm_app_allocate_ion_memory(&app_bufs[i], ion_type);

        app_bufs[i].buf.buf_idx = i;
        app_bufs[i].buf.num_planes = (int8_t)frame_offset_info->num_planes;
        app_bufs[i].buf.fd = app_bufs[i].mem_info.fd;
        app_bufs[i].buf.frame_len = app_bufs[i].mem_info.size;
        app_bufs[i].buf.buffer = app_bufs[i].mem_info.data;
        app_bufs[i].buf.mem_info = (void *)&app_bufs[i].mem_info;



        app_bufs[i].buf.planes[0].length = frame_offset_info->mp[0].len;
        app_bufs[i].buf.planes[0].m.userptr =
            (long unsigned int)app_bufs[i].buf.fd;
        app_bufs[i].buf.planes[0].data_offset = frame_offset_info->mp[0].offset;
        app_bufs[i].buf.planes[0].reserved[0] = 0;
        for (j = 1; j < (uint8_t)frame_offset_info->num_planes; j++) {
            app_bufs[i].buf.planes[j].length = frame_offset_info->mp[j].len;
            app_bufs[i].buf.planes[j].m.userptr =
                (long unsigned int)app_bufs[i].buf.fd;
            app_bufs[i].buf.planes[j].data_offset = frame_offset_info->mp[j].offset;
            app_bufs[i].buf.planes[j].reserved[0] =
                app_bufs[i].buf.planes[j-1].reserved[0] +
                app_bufs[i].buf.planes[j-1].length;
        }
    }
    do{}while(0);
    return MM_CAMERA_OK;
}

int mm_app_release_bufs(uint8_t num_bufs,
                        mm_camera_app_buf_t* app_bufs)
{
    int i, rc = MM_CAMERA_OK;

    do{}while(0);

    for (i = 0; i < num_bufs; i++) {
        rc = mm_app_deallocate_ion_memory(&app_bufs[i]);
    }
    memset(app_bufs, 0, num_bufs * sizeof(mm_camera_app_buf_t));
    do{}while(0);
    return rc;
}

int mm_app_stream_initbuf(cam_frame_len_offset_t *frame_offset_info,
                          uint8_t *num_bufs,
                          uint8_t **initial_reg_flag,
                          mm_camera_buf_def_t **bufs,
                          mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                          void *user_data)
{
    mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
    mm_camera_buf_def_t *pBufs = ((void *)0);
    uint8_t *reg_flags = ((void *)0);
    int i, rc;

    stream->offset = *frame_offset_info;

    do{}while(0)




                                         ;

    pBufs = (mm_camera_buf_def_t *)malloc(sizeof(mm_camera_buf_def_t) * stream->num_of_bufs);
    reg_flags = (uint8_t *)malloc(sizeof(uint8_t) * stream->num_of_bufs);
    if (pBufs == ((void *)0) || reg_flags == ((void *)0)) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: No mem for bufs", __PRETTY_FUNCTION__));
        if (pBufs != ((void *)0)) {
            free(pBufs);
        }
        if (reg_flags != ((void *)0)) {
            free(reg_flags);
        }
        return -1;
    }
    if (stream->num_of_bufs > (24)) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: more stream buffers per stream than allowed", __PRETTY_FUNCTION__));
        return -1;
    }

    rc = mm_app_alloc_bufs(&stream->s_bufs[0],
                           frame_offset_info,
                           stream->num_of_bufs,
                           1,
                           stream->multipleOf);

    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: mm_stream_alloc_bufs err = %d", __PRETTY_FUNCTION__, rc));
        free(pBufs);
        free(reg_flags);
        return rc;
    }

    for (i = 0; i < stream->num_of_bufs; i++) {

        pBufs[i] = stream->s_bufs[i].buf;
        reg_flags[i] = 1;
        rc = ops_tbl->map_ops(pBufs[i].buf_idx,
                              -1,
                              pBufs[i].fd,
                              (uint32_t)pBufs[i].frame_len,
                              ops_tbl->userdata);
        if (rc != MM_CAMERA_OK) {
            ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: mapping buf[%d] err = %d", __PRETTY_FUNCTION__, i, rc));
            break;
        }
    }

    if (rc != MM_CAMERA_OK) {
        int j;
        for (j=0; j>i; j++) {
            ops_tbl->unmap_ops(pBufs[j].buf_idx, -1, ops_tbl->userdata);
        }
        mm_app_release_bufs(stream->num_of_bufs, &stream->s_bufs[0]);
        free(pBufs);
        free(reg_flags);
        return rc;
    }

    *num_bufs = stream->num_of_bufs;
    *bufs = pBufs;
    *initial_reg_flag = reg_flags;

    do{}while(0);
    return rc;
}

int32_t mm_app_stream_deinitbuf(mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                                void *user_data)
{
    mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
    int i;

    for (i = 0; i < stream->num_of_bufs ; i++) {

        ops_tbl->unmap_ops(stream->s_bufs[i].buf.buf_idx, -1, ops_tbl->userdata);
    }

    mm_app_release_bufs(stream->num_of_bufs, &stream->s_bufs[0]);

    do{}while(0);
    return 0;
}

int32_t mm_app_stream_clean_invalidate_buf(uint32_t index, void *user_data)
{
    mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
    return mm_app_cache_ops(&stream->s_bufs[index].mem_info,
      (((2U|1U) << (((0 +8)+8)+14)) | ((('M')) << (0 +8)) | (((2)) << 0) | ((((sizeof(struct ion_flush_data)))) << ((0 +8)+8))));
}

int32_t mm_app_stream_invalidate_buf(uint32_t index, void *user_data)
{
    mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
    return mm_app_cache_ops(&stream->s_bufs[index].mem_info, (((2U|1U) << (((0 +8)+8)+14)) | ((('M')) << (0 +8)) | (((1)) << 0) | ((((sizeof(struct ion_flush_data)))) << ((0 +8)+8))));
}

static void notify_evt_cb(uint32_t camera_handle,
                          mm_camera_event_t *evt,
                          void *user_data)
{
    mm_camera_test_obj_t *test_obj =
        (mm_camera_test_obj_t *)user_data;
    if (test_obj == ((void *)0) || test_obj->cam->camera_handle != camera_handle) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Not a valid test obj", __PRETTY_FUNCTION__));
        return;
    }

    do{}while(0);
    switch (evt->server_event_type) {
       case CAM_EVENT_TYPE_AUTO_FOCUS_DONE:
           do{}while(0);
           break;
       case CAM_EVENT_TYPE_ZOOM_DONE:
           do{}while(0);
           break;
       default:
           break;
    }

    do{}while(0);
}

int mm_app_open(mm_camera_app_t *cam_app,
                int cam_id,
                mm_camera_test_obj_t *test_obj)
{
    int32_t rc;
    cam_frame_len_offset_t offset_info;

    do{}while(0);

    test_obj->cam = cam_app->hal_lib.mm_camera_open((uint8_t)cam_id);
    if(test_obj->cam == ((void *)0)) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:dev open error\n", __PRETTY_FUNCTION__));
        return -MM_CAMERA_E_GENERAL;
    }

    do{}while(0);


    memset(&offset_info, 0, sizeof(offset_info));
    offset_info.frame_len = sizeof(cam_capability_t);

    rc = mm_app_alloc_bufs(&test_obj->cap_buf,
                           &offset_info,
                           1,
                           0,
                           0);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:alloc buf for capability error\n", __PRETTY_FUNCTION__));
        goto error_after_cam_open;
    }


    rc = test_obj->cam->ops->map_buf(test_obj->cam->camera_handle,
                                     CAM_MAPPING_BUF_TYPE_CAPABILITY,
                                     test_obj->cap_buf.mem_info.fd,
                                     test_obj->cap_buf.mem_info.size);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:map for capability error\n", __PRETTY_FUNCTION__));
        goto error_after_cap_buf_alloc;
    }


    memset(&offset_info, 0, sizeof(offset_info));
    offset_info.frame_len = 1048576U;
    rc = mm_app_alloc_bufs(&test_obj->parm_buf,
                           &offset_info,
                           1,
                           0,
                           0);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:alloc buf for getparm_buf error\n", __PRETTY_FUNCTION__));
        goto error_after_cap_buf_map;
    }


    rc = test_obj->cam->ops->map_buf(test_obj->cam->camera_handle,
                                     CAM_MAPPING_BUF_TYPE_PARM_BUF,
                                     test_obj->parm_buf.mem_info.fd,
                                     test_obj->parm_buf.mem_info.size);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:map getparm_buf error\n", __PRETTY_FUNCTION__));
        goto error_after_getparm_buf_alloc;
    }
    test_obj->params_buffer = (parm_buffer_new_t*) test_obj->parm_buf.mem_info.data;
    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "\n%s params_buffer=%p\n",__PRETTY_FUNCTION__,test_obj->params_buffer));

    rc = test_obj->cam->ops->register_event_notify(test_obj->cam->camera_handle,
                                                   notify_evt_cb,
                                                   test_obj);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: failed register_event_notify", __PRETTY_FUNCTION__));
        rc = -MM_CAMERA_E_GENERAL;
        goto error_after_getparm_buf_map;
    }

    rc = test_obj->cam->ops->query_capability(test_obj->cam->camera_handle);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: failed query_capability", __PRETTY_FUNCTION__));
        rc = -MM_CAMERA_E_GENERAL;
        goto error_after_getparm_buf_map;
    }
    memset(&test_obj->jpeg_ops, 0, sizeof(mm_jpeg_ops_t));
    mm_dimension pic_size;
    memset(&pic_size, 0, sizeof(mm_dimension));
    pic_size.w = 4000;
    pic_size.h = 3000;
    test_obj->jpeg_hdl = cam_app->hal_lib.jpeg_open(&test_obj->jpeg_ops,pic_size);
    if (test_obj->jpeg_hdl == 0) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: jpeg lib open err", __PRETTY_FUNCTION__));
        rc = -MM_CAMERA_E_GENERAL;
        goto error_after_getparm_buf_map;
    }

    return rc;

error_after_getparm_buf_map:
    test_obj->cam->ops->unmap_buf(test_obj->cam->camera_handle,
                                  CAM_MAPPING_BUF_TYPE_PARM_BUF);
error_after_getparm_buf_alloc:
    mm_app_release_bufs(1, &test_obj->parm_buf);
error_after_cap_buf_map:
    test_obj->cam->ops->unmap_buf(test_obj->cam->camera_handle,
                                  CAM_MAPPING_BUF_TYPE_CAPABILITY);
error_after_cap_buf_alloc:
    mm_app_release_bufs(1, &test_obj->cap_buf);
error_after_cam_open:
    test_obj->cam->ops->close_camera(test_obj->cam->camera_handle);
    test_obj->cam = ((void *)0);
    return rc;
}

int mm_app_close(mm_camera_test_obj_t *test_obj)
{
    int32_t rc = MM_CAMERA_OK;

    if (test_obj == ((void *)0) || test_obj->cam ==((void *)0)) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: cam not opened", __PRETTY_FUNCTION__));
        return -MM_CAMERA_E_GENERAL;
    }


    rc = test_obj->cam->ops->unmap_buf(test_obj->cam->camera_handle,
                                       CAM_MAPPING_BUF_TYPE_CAPABILITY);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: unmap capability buf failed, rc=%d", __PRETTY_FUNCTION__, rc));
    }


    rc = test_obj->cam->ops->unmap_buf(test_obj->cam->camera_handle,
                                       CAM_MAPPING_BUF_TYPE_PARM_BUF);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: unmap setparm buf failed, rc=%d", __PRETTY_FUNCTION__, rc));
    }

    rc = test_obj->cam->ops->close_camera(test_obj->cam->camera_handle);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: close camera failed, rc=%d", __PRETTY_FUNCTION__, rc));
    }
    test_obj->cam = ((void *)0);


    if (test_obj->jpeg_hdl && test_obj->jpeg_ops.close) {
        rc = test_obj->jpeg_ops.close(test_obj->jpeg_hdl);
        test_obj->jpeg_hdl = 0;
        if (rc != MM_CAMERA_OK) {
            ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: close jpeg failed, rc=%d", __PRETTY_FUNCTION__, rc));
        }
    }


    rc = mm_app_release_bufs(1, &test_obj->cap_buf);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: release capability buf failed, rc=%d", __PRETTY_FUNCTION__, rc));
    }


    rc = mm_app_release_bufs(1, &test_obj->parm_buf);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: release setparm buf failed, rc=%d", __PRETTY_FUNCTION__, rc));
    }

    return MM_CAMERA_OK;
}

mm_camera_channel_t * mm_app_add_channel(mm_camera_test_obj_t *test_obj,
                                         mm_camera_channel_type_t ch_type,
                                         mm_camera_channel_attr_t *attr,
                                         mm_camera_buf_notify_t channel_cb,
                                         void *userdata)
{
    uint32_t ch_id = 0;
    mm_camera_channel_t *channel = ((void *)0);

    ch_id = test_obj->cam->ops->add_channel(test_obj->cam->camera_handle,
                                            attr,
                                            channel_cb,
                                            userdata);
    if (ch_id == 0) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: add channel failed", __PRETTY_FUNCTION__));
        return ((void *)0);
    }
    channel = &test_obj->channels[ch_type];
    channel->ch_id = ch_id;
    return channel;
}

int mm_app_del_channel(mm_camera_test_obj_t *test_obj,
                       mm_camera_channel_t *channel)
{
    test_obj->cam->ops->delete_channel(test_obj->cam->camera_handle,
                                       channel->ch_id);
    memset(channel, 0, sizeof(mm_camera_channel_t));
    return MM_CAMERA_OK;
}

mm_camera_stream_t * mm_app_add_stream(mm_camera_test_obj_t *test_obj,
                                       mm_camera_channel_t *channel)
{
    mm_camera_stream_t *stream = ((void *)0);
    int rc = MM_CAMERA_OK;
    cam_frame_len_offset_t offset_info;

    stream = &(channel->streams[channel->num_streams++]);
    stream->s_id = test_obj->cam->ops->add_stream(test_obj->cam->camera_handle,
                                                  channel->ch_id);
    if (stream->s_id == 0) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: add stream failed", __PRETTY_FUNCTION__));
        return ((void *)0);
    }

    stream->multipleOf = test_obj->slice_size;


    memset(&offset_info, 0, sizeof(offset_info));
    offset_info.frame_len = sizeof(cam_stream_info_t);

    rc = mm_app_alloc_bufs(&stream->s_info_buf,
                           &offset_info,
                           1,
                           0,
                           0);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:alloc buf for stream_info error\n", __PRETTY_FUNCTION__));
        test_obj->cam->ops->delete_stream(test_obj->cam->camera_handle,
                                          channel->ch_id,
                                          stream->s_id);
        stream->s_id = 0;
        return ((void *)0);
    }


    rc = test_obj->cam->ops->map_stream_buf(test_obj->cam->camera_handle,
                                            channel->ch_id,
                                            stream->s_id,
                                            CAM_MAPPING_BUF_TYPE_STREAM_INFO,
                                            0,
                                            -1,
                                            stream->s_info_buf.mem_info.fd,
                                            (uint32_t)stream->s_info_buf.mem_info.size);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:map setparm_buf error\n", __PRETTY_FUNCTION__));
        mm_app_deallocate_ion_memory(&stream->s_info_buf);
        test_obj->cam->ops->delete_stream(test_obj->cam->camera_handle,
                                          channel->ch_id,
                                          stream->s_id);
        stream->s_id = 0;
        return ((void *)0);
    }

    return stream;
}

int mm_app_del_stream(mm_camera_test_obj_t *test_obj,
                      mm_camera_channel_t *channel,
                      mm_camera_stream_t *stream)
{
    test_obj->cam->ops->unmap_stream_buf(test_obj->cam->camera_handle,
                                         channel->ch_id,
                                         stream->s_id,
                                         CAM_MAPPING_BUF_TYPE_STREAM_INFO,
                                         0,
                                         -1);
    mm_app_deallocate_ion_memory(&stream->s_info_buf);
    test_obj->cam->ops->delete_stream(test_obj->cam->camera_handle,
                                      channel->ch_id,
                                      stream->s_id);
    memset(stream, 0, sizeof(mm_camera_stream_t));
    return MM_CAMERA_OK;
}

mm_camera_channel_t *mm_app_get_channel_by_type(mm_camera_test_obj_t *test_obj,
                                                mm_camera_channel_type_t ch_type)
{
    return &test_obj->channels[ch_type];
}

int mm_app_config_stream(mm_camera_test_obj_t *test_obj,
                         mm_camera_channel_t *channel,
                         mm_camera_stream_t *stream,
                         mm_camera_stream_config_t *config)
{
    return test_obj->cam->ops->config_stream(test_obj->cam->camera_handle,
                                             channel->ch_id,
                                             stream->s_id,
                                             config);
}

int mm_app_start_channel(mm_camera_test_obj_t *test_obj,
                         mm_camera_channel_t *channel)
{
    return test_obj->cam->ops->start_channel(test_obj->cam->camera_handle,
                                             channel->ch_id);
}

int mm_app_stop_channel(mm_camera_test_obj_t *test_obj,
                        mm_camera_channel_t *channel)
{
    return test_obj->cam->ops->stop_channel(test_obj->cam->camera_handle,
                                            channel->ch_id);
}


int initBatchUpdate(mm_camera_test_obj_t *test_obj)
{
    parm_buffer_new_t *param_buf = ( parm_buffer_new_t * ) test_obj->parm_buf.mem_info.data;

    memset(param_buf, 0, sizeof(1048576U));
    param_buf->num_entry = 0;
    param_buf->curr_size = 0;
    param_buf->tot_rem_size = 1048576U - sizeof(parm_buffer_new_t);

    return MM_CAMERA_OK;
}

int commitSetBatch(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;
    parm_buffer_new_t *param_buf = (parm_buffer_new_t *)test_obj->parm_buf.mem_info.data;

    if (param_buf->num_entry > 0) {
        rc = test_obj->cam->ops->set_parms(test_obj->cam->camera_handle, param_buf);
        ((void)__android_log_print(ANDROID_LOG_DEBUG, "mm-camera-test", "%s: commitSetBatch done",__PRETTY_FUNCTION__));
    }

    return rc;
}

int AddSetParmEntryToBatch(mm_camera_test_obj_t *test_obj,
                           cam_intf_parm_type_t paramType,
                           uint32_t paramLength,
                           void *paramValue)
{
    uint32_t j = 0;
    parm_buffer_new_t *param_buf = (parm_buffer_new_t *) test_obj->parm_buf.mem_info.data;
    uint32_t num_entry = param_buf->num_entry;
    uint32_t size_req = paramLength + sizeof(parm_entry_type_new_t);
    uint32_t aligned_size_req = (size_req + 3U) & (~3U);
    parm_entry_type_new_t *curr_param = (parm_entry_type_new_t *)&param_buf->entry[0];
// # 947 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c"
    for (j = 0; j < num_entry; j++) {
      if (paramType == curr_param->entry_type) {
        ((void)__android_log_print(ANDROID_LOG_DEBUG, "mm-camera-test", "%s:Batch parameter overwrite for param: %d", __PRETTY_FUNCTION__, paramType))
                                                                    ;
        break;
      }
      curr_param = (parm_entry_type_new_t *)((char *)curr_param + curr_param->aligned_size);
    }


    if (j == num_entry) {
      if (aligned_size_req > param_buf->tot_rem_size) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:Batch buffer running out of size, commit and resend",__PRETTY_FUNCTION__));
        commitSetBatch(test_obj);
        initBatchUpdate(test_obj);
      }

      curr_param = (parm_entry_type_new_t *)(&param_buf->entry[0] +
                                                  param_buf->curr_size);
      param_buf->curr_size += aligned_size_req;
      param_buf->tot_rem_size -= aligned_size_req;
      param_buf->num_entry++;
    }

    curr_param->entry_type = paramType;
    curr_param->size = (size_t)paramLength;
    curr_param->aligned_size = aligned_size_req;
    memcpy(&curr_param->data[0], paramValue, paramLength);
    ((void)__android_log_print(ANDROID_LOG_DEBUG, "mm-camera-test", "%s: num_entry: %d, paramType: %d, paramLength: %d, aligned_size_req: %d", __PRETTY_FUNCTION__, param_buf->num_entry, paramType, paramLength, aligned_size_req))
                                                                                     ;

    return MM_CAMERA_OK;
}



int setFPSRange(mm_camera_test_obj_t *test_obj, cam_fps_range_t range)
{
    int rc = MM_CAMERA_OK;

    rc = initBatchUpdate(test_obj);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Batch camera parameter update failed\n", __PRETTY_FUNCTION__));
        goto ERROR;
    }

    rc = AddSetParmEntryToBatch(test_obj,
                                CAM_INTF_PARM_FPS_RANGE,
                                sizeof(cam_fps_range_t),
                                &range);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: FPS range parameter not added to batch\n", __PRETTY_FUNCTION__));
        goto ERROR;
    }

    rc = commitSetBatch(test_obj);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Batch parameters commit failed\n", __PRETTY_FUNCTION__));
        goto ERROR;
    }

    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: FPS Range set to: [%5.2f:%5.2f]", __PRETTY_FUNCTION__, range.min_fps, range.max_fps))


                              ;

ERROR:
    return rc;
}


int setScene(mm_camera_test_obj_t *test_obj, cam_scene_mode_type scene)
{
    int rc = MM_CAMERA_OK;

    rc = initBatchUpdate(test_obj);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Batch camera parameter update failed\n", __PRETTY_FUNCTION__));
        goto ERROR;
    }

    int32_t value = scene;

    rc = AddSetParmEntryToBatch(test_obj,
                                CAM_INTF_PARM_BESTSHOT_MODE,
                                sizeof(value),
                                &value);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Scene parameter not added to batch\n", __PRETTY_FUNCTION__));
        goto ERROR;
    }

    rc = commitSetBatch(test_obj);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Batch parameters commit failed\n", __PRETTY_FUNCTION__));
        goto ERROR;
    }

    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Scene set to: %d", __PRETTY_FUNCTION__, value));

ERROR:
    return rc;
}



camera_cb user_frame_callback = ((void *)0);
void camera_install_callback(camera_cb cb)
{
  user_frame_callback = cb;
}





static void downsample_frame(uint64_t in[360*2][214], int len, uint8_t out[360][640]) {
  int x = 0, y = 0, i = 0;
  for (y = 0; y < 360; y++)
    {
    for (x = 0; x < 214; x++)
      {
 unsigned long long p = in[y*2][x], q = in[y*2+1][x];
 int g;
 g = (((p>>00) & 1023) + ((p>>10) & 1023) + ((q>>00) & 1023) + ((q>>10) & 1023)) >> 2;
 out[y][x*3+0] = (g > 255) ? 255 : g;
 g = (((p>>20) & 1023) + ((p>>30) & 1023) + ((q>>20) & 1023) + ((q>>30) & 1023)) >> 2;
 out[y][x*3+1] = (g > 255) ? 255 : g;
 g = (((p>>40) & 1023) + ((p>>50) & 1023) + ((q>>40) & 1023) + ((q>>50) & 1023)) >> 2;
 out[y][x*3+2] = (g > 255) ? 255 : g;
      }
    }
}


static uint8_t raw_buffer1[360 +1][640];
static uint8_t raw_buffer2[360 +1][640];


static void mm_app_snapshot_notify_cb_raw(mm_camera_super_buf_t *bufs,
                                          void *user_data)
{
  static int frameid = 0;
    int rc;
    uint32_t i = 0;
    mm_camera_test_obj_t *pme = (mm_camera_test_obj_t *)user_data;
    mm_camera_channel_t *channel = ((void *)0);
    mm_camera_stream_t *m_stream = ((void *)0);
    mm_camera_buf_def_t *m_frame = ((void *)0);

    do{}while(0);


    for (i = 0; i < MM_CHANNEL_TYPE_MAX; i++) {
        if (pme->channels[i].ch_id == bufs->ch_id) {
            channel = &pme->channels[i];
            break;
        }
    }
    if (((void *)0) == channel) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Wrong channel id (%d)", __PRETTY_FUNCTION__, bufs->ch_id));
        rc = -1;
        goto EXIT;
    }


    for (i = 0; i < channel->num_streams; i++) {
        if (channel->streams[i].s_config.stream_info->stream_type == CAM_STREAM_TYPE_RAW) {
            m_stream = &channel->streams[i];
            break;
        }
    }
    if (((void *)0) == m_stream) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: cannot find snapshot stream", __PRETTY_FUNCTION__));
        rc = -1;
        goto EXIT;
    }


//    printf("%d bufs in stream\n", bufs->num_bufs);

    if (user_frame_callback) {
 for (i = 0; i < bufs->num_bufs; i++) {
     if (bufs->bufs[i]->stream_id == m_stream->s_id) {
  m_frame = bufs->bufs[i];

  downsample_frame((uint64_t (*)[214])m_frame->buffer,
     m_frame->planes[0].length,

     raw_buffer1);

  user_frame_callback((uint8_t*)raw_buffer1, 640, 360);
  frameid++;

      }
 }
    }
    if (((void *)0) == m_frame) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: main frame is NULL", __PRETTY_FUNCTION__));
        rc = -1;
        goto EXIT;
    }





EXIT:
    for (i=0; i<bufs->num_bufs; i++) {
        if (MM_CAMERA_OK != pme->cam->ops->qbuf(bufs->camera_handle,
                                                bufs->ch_id,
                                                bufs->bufs[i])) {
            ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Failed in Qbuf\n", __PRETTY_FUNCTION__));
        }
    }






    do{}while(0);
}



int mm_app_start_capture_raw(mm_camera_test_obj_t *test_obj, uint8_t num_snapshots)
{
    int32_t rc = MM_CAMERA_OK;
    mm_camera_channel_t *channel = ((void *)0);
    mm_camera_stream_t *s_main = ((void *)0);
    mm_camera_channel_attr_t attr;

    memset(&attr, 0, sizeof(mm_camera_channel_attr_t));
    attr.notify_mode = MM_CAMERA_SUPER_BUF_NOTIFY_BURST;

    attr.max_unmatched_frames = 2;
    channel = mm_app_add_channel(test_obj,
                                 MM_CHANNEL_TYPE_CAPTURE,
                                 &attr,
                                 mm_app_snapshot_notify_cb_raw,
                                 test_obj);
    if (((void *)0) == channel) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: add channel failed", __PRETTY_FUNCTION__));
        return -MM_CAMERA_E_GENERAL;
    }

    test_obj->buffer_format = CAM_FORMAT_BAYER_QCOM_RAW_10BPP_GBRG;
    s_main = mm_app_add_raw_stream(test_obj,
                                   channel,
                                   mm_app_snapshot_notify_cb_raw,
                                   test_obj,
                                   num_snapshots,
                                   num_snapshots);
    if (((void *)0) == s_main) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: add main snapshot stream failed\n", __PRETTY_FUNCTION__));
        mm_app_del_channel(test_obj, channel);
        return rc;
    }

    rc = mm_app_start_channel(test_obj, channel);
    if (MM_CAMERA_OK != rc) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:start zsl failed rc=%d\n", __PRETTY_FUNCTION__, rc));
        mm_app_del_stream(test_obj, channel, s_main);
        mm_app_del_channel(test_obj, channel);
        return rc;
    }

    return rc;
}

int mm_app_stop_capture_raw(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;
    mm_camera_channel_t *ch = ((void *)0);
    int i;

    ch = mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_CAPTURE);

    rc = mm_app_stop_channel(test_obj, ch);
    if (MM_CAMERA_OK != rc) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:stop recording failed rc=%d\n", __PRETTY_FUNCTION__, rc));
    }

    for ( i = 0 ; i < ch->num_streams ; i++ ) {
        mm_app_del_stream(test_obj, ch, &ch->streams[i]);
    }

    mm_app_del_channel(test_obj, ch);

    return rc;
}


int mm_camera_lib_open(mm_camera_lib_handle *handle, int cam_id)
{
    int rc = MM_CAMERA_OK;

    if ( ((void *)0) == handle ) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", " %s : Invalid handle", __PRETTY_FUNCTION__));
        rc = MM_CAMERA_E_INVALID_INPUT;
        goto EXIT;
    }

    memset(handle, 0, sizeof(mm_camera_lib_handle));
    rc = mm_app_load_hal(&handle->app_ctx);
    if( MM_CAMERA_OK != rc ) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:mm_app_init err\n", __PRETTY_FUNCTION__));
        goto EXIT;
    }

    handle->test_obj.buffer_width = 640;
    handle->test_obj.buffer_height = 480;
    handle->test_obj.buffer_format = CAM_FORMAT_YUV_420_NV21;
    handle->current_params.stream_width = 1024;
    handle->current_params.stream_height = 768;
    handle->current_params.af_mode = CAM_FOCUS_MODE_AUTO;
    rc = mm_app_open(&handle->app_ctx, (uint8_t)cam_id, &handle->test_obj);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:mm_app_open() cam_idx=%d, err=%d\n", __PRETTY_FUNCTION__, cam_id, rc))
                                        ;
        goto EXIT;
    }


    rc = MM_CAMERA_OK;
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: mm_app_initialize_fb() cam_idx=%d, err=%d\n", __PRETTY_FUNCTION__, cam_id, rc))
                                        ;
        goto EXIT;
    }

EXIT:

    return rc;
}






int mm_camera_lib_send_command(mm_camera_lib_handle *handle,
                               mm_camera_lib_commands cmd,
                               void *in_data, void *out_data)
{
    uint32_t width, height;
    int rc = MM_CAMERA_OK;
    cam_capability_t *camera_cap = ((void *)0);
    mm_camera_lib_snapshot_params *dim = ((void *)0);

    if ( ((void *)0) == handle ) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", " %s : Invalid handle", __PRETTY_FUNCTION__));
        rc = MM_CAMERA_E_INVALID_INPUT;
        goto EXIT;
    }

    camera_cap = (cam_capability_t *) handle->test_obj.cap_buf.mem_info.data;

    switch(cmd) {
        case MM_CAMERA_LIB_FPS_RANGE:
            if ( ((void *)0) != in_data ) {
                cam_fps_range_t range = *(( cam_fps_range_t * )in_data);
                rc = setFPSRange(&handle->test_obj, range);
                if (rc != MM_CAMERA_OK) {
                        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: setFPSRange() err=%d\n", __PRETTY_FUNCTION__, rc))
                                                ;
                        goto EXIT;
                }
            }
            break;
// # 1332 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c"
        case MM_CAMERA_LIB_BESTSHOT:
            if ( ((void *)0) != in_data ) {
                cam_scene_mode_type scene = *(( int * )in_data);
                rc = setScene(&handle->test_obj, scene);
                if (rc != MM_CAMERA_OK) {
                        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: setScene() err=%d\n", __PRETTY_FUNCTION__, rc))
                                                ;
                        goto EXIT;
                }
            }
            break;
// # 1465 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c"
        case MM_CAMERA_LIB_RAW_CAPTURE:


            width = handle->test_obj.buffer_width;
            height = handle->test_obj.buffer_height;
            handle->test_obj.buffer_width =
                    (uint32_t)camera_cap->raw_dim.width;
            handle->test_obj.buffer_height =
                    (uint32_t)camera_cap->raw_dim.height;
            handle->test_obj.buffer_format = CAM_FORMAT_BAYER_QCOM_RAW_10BPP_GBRG;
            ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: MM_CAMERA_LIB_RAW_CAPTURE %dx%d\n", __PRETTY_FUNCTION__, camera_cap->raw_dim.width, camera_cap->raw_dim.height))


                                                  ;

//     printf("Start capture Raw\n");
            rc = mm_app_start_capture_raw(&handle->test_obj, 1);
            if (rc != MM_CAMERA_OK) {
                ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: mm_app_start_capture() err=%d\n", __PRETTY_FUNCTION__, rc))
                                        ;
                goto EXIT;
            }


            break;
// # 1643 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/victor_camera.c"
      case MM_CAMERA_LIB_NO_ACTION:
 break;
        default:
   printf("YOu missed commmand %d\n",cmd);
   exit(-5);
            break;
    };

EXIT:

    return rc;
}
int mm_camera_lib_number_of_cameras(mm_camera_lib_handle *handle)
{
    int rc = 0;

    if ( ((void *)0) == handle ) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", " %s : Invalid handle", __PRETTY_FUNCTION__));
        goto EXIT;
    }

    rc = handle->app_ctx.num_cameras;

EXIT:

    return rc;
}

int mm_camera_lib_close(mm_camera_lib_handle *handle)
{
    int rc = MM_CAMERA_OK;

    if ( ((void *)0) == handle ) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", " %s : Invalid handle", __PRETTY_FUNCTION__));
        rc = MM_CAMERA_E_INVALID_INPUT;
        goto EXIT;
    }


    rc = MM_CAMERA_OK;
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:mm_app_close_fb() err=%d\n", __PRETTY_FUNCTION__, rc))
                                ;
        goto EXIT;
    }

    rc = mm_app_close(&handle->test_obj);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:mm_app_close() err=%d\n", __PRETTY_FUNCTION__, rc))
                                ;
        goto EXIT;
    }

EXIT:
    return rc;
}

int mm_camera_lib_set_preview_usercb(
   mm_camera_lib_handle *handle, cam_stream_user_cb cb)
{
    if (handle->test_obj.user_preview_cb != ((void *)0)) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s, already set preview callbacks\n", __PRETTY_FUNCTION__));
        return -1;
    }
    handle->test_obj.user_preview_cb = *cb;
    return 0;
}

int mm_app_set_preview_fps_range(mm_camera_test_obj_t *test_obj,
                        cam_fps_range_t *fpsRange)
{
    int rc = MM_CAMERA_OK;
    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: preview fps range: min=%f, max=%f.", __PRETTY_FUNCTION__, fpsRange->min_fps, fpsRange->max_fps))
                                             ;
    rc = setFPSRange(test_obj, *fpsRange);

    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: add_parm_entry_tobatch failed !!", __PRETTY_FUNCTION__));
        return rc;
    }

    return rc;
}

int mm_app_set_params_impl(mm_camera_test_obj_t *test_obj,
                      cam_intf_parm_type_t param_type,
                      uint32_t param_len,
                      void* param_val)
{
    int rc = MM_CAMERA_OK;

    rc = initBatchUpdate(test_obj);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Batch camera parameter update failed\n", __PRETTY_FUNCTION__));
        goto ERROR;
    }

    rc = AddSetParmEntryToBatch(test_obj,
                                param_type,
                                param_len,
                                param_val);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: parameter-type %d not added to batch\n", __PRETTY_FUNCTION__, param_type));
        goto ERROR;
    }

    rc = commitSetBatch(test_obj);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Batch parameters commit failed\n", __PRETTY_FUNCTION__));
        goto ERROR;
    }

ERROR:
    return rc;
}

int mm_app_set_face_detection(mm_camera_test_obj_t *test_obj,
                        cam_fd_set_parm_t *fd_set_parm)
{
    if (test_obj == ((void *)0) || fd_set_parm == ((void *)0)) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s, invalid params!", __PRETTY_FUNCTION__));
        return MM_CAMERA_E_INVALID_INPUT;
    }

    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: mode = %d, num_fd = %d", __PRETTY_FUNCTION__, fd_set_parm->fd_mode, fd_set_parm->num_fd))
                                                    ;

    return mm_app_set_params_impl(test_obj, CAM_INTF_PARM_FD,
                              sizeof(cam_fd_set_parm_t),
                              fd_set_parm);
}

int mm_app_set_flash_mode(mm_camera_test_obj_t *test_obj,
                cam_flash_mode_t flashMode)
{
    int rc = MM_CAMERA_OK;

    if (test_obj == ((void *)0)) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s, invalid params!", __PRETTY_FUNCTION__));
        return MM_CAMERA_E_INVALID_INPUT;
    }

    rc = initBatchUpdate(test_obj);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Batch camera parameter update failed\n", __PRETTY_FUNCTION__));
        goto ERROR;
    }

    int32_t value = flashMode;
    rc = AddSetParmEntryToBatch(test_obj,
                                CAM_INTF_PARM_LED_MODE,
                                sizeof(value),
                                &value);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Flash parameter not added to batch\n", __PRETTY_FUNCTION__));
        goto ERROR;
    }

    rc = commitSetBatch(test_obj);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Batch parameters commit failed\n", __PRETTY_FUNCTION__));
        goto ERROR;
    }

    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: Flash set to: %d", __PRETTY_FUNCTION__, value));

ERROR:
    return rc;
}

int mm_app_set_metadata_usercb(mm_camera_test_obj_t *test_obj,
                        cam_stream_user_cb usercb)
{
    if (test_obj == ((void *)0) || usercb == ((void *)0)) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s, invalid params!", __PRETTY_FUNCTION__));
        return MM_CAMERA_E_INVALID_INPUT;
    }

    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s, set user metadata callback, addr: %p\n", __PRETTY_FUNCTION__, usercb));

    if (test_obj->user_metadata_cb != ((void *)0)) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s, already set user metadata callback", __PRETTY_FUNCTION__));
    }
    test_obj->user_metadata_cb = usercb;

    return 0;
}




CameraHandle camera_alloc(void)
{
   return malloc(sizeof(CameraObj));
}


int camera_init(CameraObj* camera)
{
    int rc = 0;
    uint8_t previewing = 0;
    int isZSL = 0;
    uint8_t wnr_enabled = 0;
    mm_camera_lib_handle lib_handle;
    int num_cameras;


    cam_scene_mode_type default_scene= CAM_SCENE_MODE_OFF;
    int set_tintless= 0;

    mm_camera_test_obj_t test_obj;
    memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));


    rc = mm_camera_lib_open(&lib_handle, 0);
    if (rc != MM_CAMERA_OK) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:mm_camera_lib_open() err=%d\n", __PRETTY_FUNCTION__, rc));
        return -1;
    }


    num_cameras = mm_camera_lib_number_of_cameras(&lib_handle);
    if ( num_cameras <= 0 ) {
        ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: No camera sensors reported!", __PRETTY_FUNCTION__));
        rc = -1;
 return rc;
    }


    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "Starting eztune Server \n"));


    rc = enableAFR(&lib_handle);
    if (rc != MM_CAMERA_OK) {
       ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:enableAFR() err=%d\n", __PRETTY_FUNCTION__, rc));
       return -1;
    }

    rc = mm_camera_lib_send_command(&lib_handle,
                                     MM_CAMERA_LIB_BESTSHOT,
                                     &default_scene,
                                     ((void *)0));
    if (rc != MM_CAMERA_OK) {
       ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:mm_camera_lib_send_command() err=%d\n", __PRETTY_FUNCTION__, rc));
       return -1;
    }

    camera->lib_handle = lib_handle;
    return 0;
}

extern void camera_install_callback(camera_cb cb);

int camera_start(CameraObj* camera, camera_cb cb)
{
  int rc = MM_CAMERA_OK;
  camera_install_callback(cb);

  rc = mm_camera_lib_send_command(&(camera->lib_handle),
      MM_CAMERA_LIB_RAW_CAPTURE,
      ((void *)0),
      ((void *)0));
  if (rc != MM_CAMERA_OK) {
    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s:mm_camera_lib_send_command() err=%d\n", __PRETTY_FUNCTION__, rc));
    return rc;
  }
  return rc;
}

int camera_stop(CameraObj* camera)
{
  int rc;
  mm_camera_app_done();

//     printf("Wait\n");
  mm_camera_app_wait();

//  printf("Stop capture Raw\n");
  rc = mm_app_stop_capture_raw(&(camera->lib_handle.test_obj));
  if (rc != MM_CAMERA_OK) {
    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: mm_app_stop_capture() err=%d\n", __PRETTY_FUNCTION__, rc))
                     ;
  }

  return rc;
}

int camera_cleanup(CameraObj* camera)
{
  mm_camera_lib_close(&camera->lib_handle);
  return 0;
}


#define AS_APPLICATION
#ifdef AS_APPLICATION

// # 31 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/mm_qcamera_main_menu.c" 2
// # 1101 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/mm_qcamera_main_menu.c"
int my_camera_callback(const uint8_t *frame, int width, int height)
{
  char file_name[64];
  static int frame_idx = 0;
//  printf("hey, I just got %d bytes!\n", width);
  const char* name = "main";
  const char* ext = "gray";
  int file_fd;

  snprintf(file_name, sizeof(file_name), "/data/misc/camera/test/%s_%04d.%s", name, frame_idx++, ext);

  file_fd = open(file_name, 00000002 | 00000100, 0777);
  if (file_fd < 0) {
    ((void)__android_log_print(ANDROID_LOG_ERROR, "mm-camera-test", "%s: cannot open file %s \n", __PRETTY_FUNCTION__, file_name));
  } else {
    write(file_fd,
   frame,
   width * height);
  }

  close(file_fd);
  do{}while(0);

  return 0;
}




int main()
{
    int rc = 0;

    CameraHandle camera  = camera_alloc();
    printf("Camera Capturer\n");

    camera_init(camera);

    printf("Starting\n");
    camera_start(camera, my_camera_callback);

    printf("Sleeping\n");
    sleep(2.0);
    printf("Done Sleeping\n");

    camera_stop(camera);

    camera_cleanup(camera);

    free(camera);
    
    printf("Exiting application\n");

    return rc;
}
// # 1627 "hardware/qcom/camera/QCamera2/stack/mm-camera-test/src/mm_qcamera_main_menu.c"

#endif
