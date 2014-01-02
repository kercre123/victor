#ifndef SWC_WHOAMI_H_
#define SWC_WHOAMI_H_


enum Process {
    PROCESS_DEBUGGER = 0,
    PROCESS_SEBI,
    #ifdef MYRIAD2
    PROCESS_LEON_OS,
    PROCESS_LEON_RT,
    #else
    PROCESS_LEON,
    #endif
    PROCESS_SHAVE0,
    PROCESS_SHAVE1,
    PROCESS_SHAVE2,
    PROCESS_SHAVE3,
    PROCESS_SHAVE4,
    PROCESS_SHAVE5,
    PROCESS_SHAVE6,
    PROCESS_SHAVE7,
    #ifdef MYRIAD2
    PROCESS_SHAVE8,
    PROCESS_SHAVE9,
    PROCESS_SHAVE10,
    PROCESS_SHAVE11,
    #endif
    NUMBER_OF_PROCESSES,
};

#ifdef __llvm__
    #define SHAVE_CORE
#else
    #ifdef __GNUC__
        #define LEON_CORE
    #else
        #error "I don't know what type of core i'm going to run on"
    #endif
#endif


static inline enum Process swcWhoAmI()
{
    #ifdef LEON_CORE
        // We are one of the leon processors
        #ifdef MYRIAD2
            u32 cacheConfiguration;
            asm volatile("lda [%1] 2, %0" : "=r"(cacheConfiguration) : "r"(0x8));
            if (((cacheConfiguration >> 20) & 0xf) == 4)
                return PROCESS_LEON_OS;
            return PROCESS_LEON_RT;
        #else
            return PROCESS_LEON;
        #endif
    #endif
    #ifdef SHAVE_CORE
        // We are a shave
        int shaveNumber;
        asm volatile("cmu.cpti %0, P_SVID\n\t" : "=r"(shaveNumber));
        return PROCESS_SHAVE0 + shaveNumber;
    #endif
}

#endif
