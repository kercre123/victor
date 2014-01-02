#include "diskio_wrapper_Coalescer.h"
#include "ffconf.h"
#include "stdio.h"
#include "stdlib.h"
#include "swcLeonUtils.h"

static DSTATUS Coalescer_disk_initialize (void *private) {
    struct coalescer *p = (struct coalescer *)private;
    p->count = -1;
    p->buffer_dirty = 0;
    // truncate buffer size:
    DSTATUS ret = p->wrappedFunctions->disk_initialize(p->private);
    /* now let's find the sector size */
    if (p->underlying_sector_size == 0) {
        WORD ssize = 0;
        p->wrappedFunctions->disk_ioctl(p->private, GET_SECTOR_SIZE, &ssize);
        if (ssize != 0)
            p->underlying_sector_size = ssize;
        if (p->underlying_sector_size == 0)
            p->underlying_sector_size = 512;
    }
    if (p->presented_sector_size == 0)
        p->presented_sector_size = 512;
    p->buffer_size = (p->buffer_size / p->underlying_sector_size) * p->underlying_sector_size;
    return ret;
}

static DSTATUS Coalescer_disk_status (void *private) {
    struct coalescer *p = (struct coalescer *)private;
    return p->wrappedFunctions->disk_status(p->private);
}

static Coalescer_flush(struct coalescer *p);

static DRESULT Coalescer_disk_ioctl (void * private, BYTE Command, void * Buffer) {
    struct coalescer *p = (struct coalescer *)private;
    switch (Command) {
    case CTRL_SYNC:
        Coalescer_flush(p);
        return p->wrappedFunctions->disk_ioctl(p->private, Command, Buffer);
        break;
    case GET_SECTOR_SIZE:
        *((WORD *) Buffer) = p->presented_sector_size;
        return RES_OK;
        break;
    case GET_BLOCK_SIZE:
    case GET_SECTOR_COUNT: {
        DRESULT res = p->wrappedFunctions->disk_ioctl(p->private, Command, Buffer);
        u32 sectorCount = *((DWORD *) Buffer);
        sectorCount *= p->underlying_sector_size;
        sectorCount /= p->presented_sector_size;
        *((DWORD *) Buffer) = sectorCount;
        return res;
        break;
    }
    case CTRL_ERASE_SECTOR: {
        u32 StartSector = ((DWORD *) Buffer)[0];
        u32 EndSector = ((DWORD *) Buffer)[1];
        StartSector *= p->presented_sector_size;
        EndSector *= p->presented_sector_size;
        StartSector /= p->underlying_sector_size;
        EndSector /= p->underlying_sector_size;
        ((DWORD *) Buffer)[0] = StartSector;
        ((DWORD *) Buffer)[1] = EndSector;
        return p->wrappedFunctions->disk_ioctl(p->private, Command, Buffer);
        break;
    }
    default:
        return p->wrappedFunctions->disk_ioctl(p->private, Command, Buffer);
    }
}

static Coalescer_flush(struct coalescer *p) {
    if (p->buffer_dirty) {
        u32 dirty_tail = p->count % p->underlying_sector_size;
        if (dirty_tail!=0) {
            // we have to read back the last sector to be able to write an entire underlying sector
            u32 usec = (p->pos + p->count)/p->underlying_sector_size;
            DRESULT ret = p->wrappedFunctions->disk_read(p->private, p->underlying_sector_buf, usec, 1, p->preferred_endianness);
            swcLeonMemCpy(((char *)p->buffer) + dirty_tail, p->preferred_endianness,
                    ((char *)p->underlying_sector_buf) + dirty_tail, p->preferred_endianness,
                    p->underlying_sector_size - dirty_tail);
            p->count = p->count - dirty_tail + p->underlying_sector_size;
        }

        u32 towrite_spos = (p->buffer_dirty_from + p->pos) / p->underlying_sector_size;
        u32 towrite_pos = towrite_spos * p->underlying_sector_size;
        u32 towrite_pos_inbuf = towrite_pos - p->pos;
        u32 towrite_scount = (p->pos + p->count - towrite_pos) / p->underlying_sector_size;
        DRESULT ret = p->wrappedFunctions->disk_write(p->private, p->buffer, towrite_spos, towrite_scount, p->preferred_endianness);
    }
    p->buffer_dirty = 0;
}

static DRESULT Coalescer_disk_read (void *private, BYTE *Buffer, DWORD SectorNumber, BYTE SectorCount, pointer_type PointerType) {
    struct coalescer *p = (struct coalescer *)private;
    //printf("Coalescer_disk_read: %d %d big?:%d\n", SectorNumber, SectorCount, PointerType==be_pointer);
    do {
        u32 req_pos = SectorNumber * p->presented_sector_size;
        u32 req_count = SectorCount * p->presented_sector_size;
        if (p->count != -1 && req_pos >= p->pos && req_pos + p->presented_sector_size <= p->pos + p->count) {
            // at least one presented-sector is available in the buffer
            // now let's calculate how many presented-sectors are available
            u32 psects = (p->pos + p->count - req_pos) / p->presented_sector_size;
            if (psects > SectorCount) psects = SectorCount;
            u32 req_pos_in_buf = req_pos - p->pos;
            swcLeonMemCpy(Buffer, PointerType,
                    ((char *)p->buffer) + req_pos_in_buf, p->preferred_endianness,
                    psects * p->presented_sector_size);
            Buffer += psects * p->presented_sector_size;
            SectorCount -= psects;
            SectorNumber += psects;
            continue;
        } else if (SectorCount) {
            // now we fill the buffer
            if (p->buffer_dirty) {
                Coalescer_flush(p);
            }
            u32 req_usec_pos = req_pos / p->underlying_sector_size;
            u32 req_usec_count = p->buffer_size / p->underlying_sector_size;
            p->count = -1;
            DRESULT ret = p->wrappedFunctions->disk_read(p->private, p->buffer, req_usec_pos, req_usec_count, p->preferred_endianness);
            if (ret!=RES_OK) return ret;
            p->pos = req_usec_pos * p->underlying_sector_size;
            p->count = req_usec_count * p->underlying_sector_size;
        } else { return RES_ERROR; }
    } while (SectorCount);
    return RES_OK;
}

#if	_READONLY == 0
static DRESULT Coalescer_disk_write (void *private, const BYTE *Buffer, DWORD SectorNumber, BYTE SectorCount, pointer_type PointerType) {
    struct coalescer *p = (struct coalescer *)private;
    //printf("Coalescer_disk_write: %d %d big?:%d\n", SectorNumber, SectorCount, PointerType==be_pointer);
    do {
        u32 req_pos = SectorNumber * p->presented_sector_size;
        u32 req_count = SectorCount * p->presented_sector_size;
        if (p->count != -1 && req_pos >= p->pos && req_pos <= p->pos + p->count) {
            // if we are appending to the buffer (or overwriting)
            u32 psects = (p->pos + p->buffer_size - req_pos) / p->presented_sector_size;
            if (psects == 0) {
                Coalescer_flush(p);
                p->count = -1;
                continue; // buffer is full
            }
            if (psects > SectorCount) psects = SectorCount;
            u32 req_pos_in_buf = req_pos - p->pos;
            swcLeonMemCpy(((char *)p->buffer) + req_pos_in_buf, p->preferred_endianness,
                    Buffer, PointerType,
                    psects * p->presented_sector_size);
            Buffer += psects * p->presented_sector_size;
            SectorCount -= psects;
            SectorNumber += psects;
            s32 new_count = req_pos - p->pos + psects * p->presented_sector_size;
            if (new_count > p->count) p->count = new_count;
            if (!p->buffer_dirty || req_pos_in_buf < p->buffer_dirty_from)
                p->buffer_dirty_from = req_pos_in_buf;
            if (!p->buffer_dirty)
                p->count = new_count;
            p->buffer_dirty = 1;

            continue;
        } else {
            Coalescer_flush(p);
            if (0==req_pos % p->underlying_sector_size) {
                p->count = 0; // this allow appending
                p->pos = req_pos;
                // make a zero-length buffer, so we can later append to it
                continue;
            } else {
                // the buffer is unaligned in respect to the underlying sector size
                // so we have to read an underlying sector first.
                u32 load_pos = (req_pos / p->underlying_sector_size) * p->underlying_sector_size;
                u32 load_usec_pos = load_pos / p->underlying_sector_size;
                DRESULT ret = p->wrappedFunctions->disk_read(p->private, p->buffer, load_usec_pos, 1, p->preferred_endianness);
                if (ret!=RES_OK) return ret;
                p->pos = load_usec_pos * p->underlying_sector_size;
                p->count = p->underlying_sector_size;
            }
        }
    } while (SectorCount);
    return RES_OK;
}
#endif

const tyDiskIoFunctions CoalescerWrapperFunctions = {
    .disk_initialize = Coalescer_disk_initialize,
    .disk_status = Coalescer_disk_status,
    .disk_read = Coalescer_disk_read,
    #if	_READONLY == 0
         // _READONLY is (re)defined in ffconf.h
        .disk_write = Coalescer_disk_write,
    #endif
    .disk_ioctl = Coalescer_disk_ioctl,
    .private = NULL
};
