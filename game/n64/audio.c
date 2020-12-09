#include "game/n64/audio.h"

#include "assets/pak.h"
#include "assets/track.h"
#include "base/base.h"
#include "base/fixup.h"
#include "base/n64/os.h" // osTvType
#include "base/n64/scheduler.h"
#include "base/pak/pak.h"
#include "game/n64/defs.h"
#include "game/n64/system.h"
#include "game/n64/task.h"

enum {
    // Number of frames in an audio buffer.
    AUDIO_BUFSZ = 1024,

    AUDIO_HEAP_SIZE = 256 * 1024,
    AUDIO_CLIST_SIZE = 4096,
    AUDIO_BUFFER_LENGTH = 2048,
    AUDIO_MAX_VOICES = 4,
    AUDIO_MAX_UPDATES = 64,
    AUDIO_EVT_COUNT = 32,
    AUDIO_DMA_COUNT = 8,
    AUDIO_DMA_BUFSZ = 2 * 1024,
};

// =============================================================================
// Audio DMA
// =============================================================================

struct audio_dmainfo {
    uint32_t age;
    uint32_t offset;
};

static struct audio_dmainfo audio_dma[AUDIO_DMA_COUNT];

static u8 audio_dmabuf[AUDIO_DMA_COUNT][AUDIO_DMA_BUFSZ]
    __attribute__((aligned(16), section("uninit")));

static OSIoMesg audio_dmamsg[AUDIO_DMA_COUNT];

static OSMesgQueue audio_dmaqueue;
static OSMesg audio_dmaqueue_buffer[AUDIO_DMA_COUNT];
static unsigned audio_dmanext, audio_dmanactive;

static s32 audio_dma_callback(s32 addr, s32 len, void *state) {
    (void)state;
    struct audio_dmainfo *restrict dma = audio_dma;
    uint32_t astart = addr, aend = astart + len;

    // If these samples are already buffered, return the buffer.
    int oldest = 0;
    uint32_t oldest_age = 0;
    for (int i = 0; i < AUDIO_DMA_COUNT; i++) {
        if (dma[i].age > oldest_age) {
            oldest = i;
            oldest_age = dma[i].age;
        }
        uint32_t dstart = dma[i].offset, dend = dstart + AUDIO_DMA_BUFSZ;
        if (dstart <= astart && aend <= dend) {
            dma[i].age = 0;
            uint32_t offset = astart - dstart;
            return K0_TO_PHYS(audio_dmabuf[i] + offset);
        }
    }

    // Otherwise, use the oldest buffer to start a new DMA.
    if (oldest_age == 0 || audio_dmanactive >= AUDIO_DMA_COUNT) {
        // If the buffer is in use, don't bother.
        fatal_error("DMA buffer in use"); // FIXME: not in release builds
        return K0_TO_PHYS(audio_dmabuf[oldest]);
    }
    uint32_t dma_addr = astart & ~1u;
    OSIoMesg *restrict mesg = &audio_dmamsg[audio_dmanext];
    audio_dmanext = (audio_dmanext + 1) % AUDIO_DMA_COUNT;
    audio_dmanactive++;
    *mesg = (OSIoMesg){
        .hdr = {.pri = OS_MESG_PRI_NORMAL, .retQueue = &audio_dmaqueue},
        .dramAddr = audio_dmabuf[oldest],
        .devAddr = dma_addr,
        .size = AUDIO_DMA_BUFSZ,
    };
    osEPiStartDma(rom_handle, mesg, OS_READ);
    dma[oldest] = (struct audio_dmainfo){
        .age = 0,
        .offset = dma_addr,
    };
    return K0_TO_PHYS(audio_dmabuf[oldest] + (astart & 1u));
}

static ALDMAproc audio_dma_new(void *arg) {
    (void)arg;
    return audio_dma_callback;
}

// =============================================================================
// Audio Initialization
// =============================================================================

static u8 audio_heap[AUDIO_HEAP_SIZE]
    __attribute__((section("uninit"), aligned(16)));
static ALHeap audio_hp;
static ALGlobals audio_globals;
static ALSndPlayer audio_sndp;

struct audio_header {
    unsigned lead_in;
    unsigned loop_length;
    ALWaveTable wavetable;
};

union audio_tracktablebuf {
    struct audio_header header;
    uint8_t data[208];
};

static void audio_adpcm_fixup(ALADPCMWaveInfo *restrict w, uintptr_t base,
                              size_t size) {
    w->book = pointer_fixup(w->book, base, size);
    w->loop = pointer_fixup(w->loop, base, size);
}

static void audio_raw16_fixup(ALRAWWaveInfo *restrict w, uintptr_t base,
                              size_t size) {
    w->loop = pointer_fixup(w->loop, base, size);
}

static void audio_track_fixup(union audio_tracktablebuf *p, pak_track asset) {
    const uintptr_t base = (uintptr_t)p;
    const size_t size = sizeof(union audio_tracktablebuf);
    ALWaveTable *restrict tbl = &p->header.wavetable;
    int obj = pak_track_object(asset) + 1;
    tbl->base = (u8 *)pak_objects[obj].offset;
    tbl->len = pak_objects[obj].size;
    switch (tbl->type) {
    case AL_ADPCM_WAVE:
        audio_adpcm_fixup(&tbl->waveInfo.adpcmWave, base, size);
        break;
    case AL_RAW16_WAVE:
        audio_raw16_fixup(&tbl->waveInfo.rawWave, base, size);
        break;
    }
}

static union audio_tracktablebuf audio_trackbuf ASSET;

void audio_init(struct game_system *restrict sys) {
    pak_track asset = TRACK_RISING_TIDE;

    // Mark all DMA buffers as "old" so they get used.
    for (int i = 0; i < AUDIO_DMA_COUNT; i++) {
        audio_dma[i].age = 1;
    }

    osCreateMesgQueue(&audio_dmaqueue, audio_dmaqueue_buffer,
                      ARRAY_COUNT(audio_dmaqueue_buffer));

    int audio_rate = osAiSetFrequency(AUDIO_SAMPLERATE);
    if (osTvType == OS_TV_PAL) {
        sys->time.samples_per_frame = (audio_rate + 25) / 50;
    } else {
        sys->time.samples_per_frame = (audio_rate * 1001 + 30000) / 60000;
    }

    alHeapInit(&audio_hp, audio_heap, sizeof(audio_heap));
    ALSynConfig scfg = {
        .maxVVoices = AUDIO_MAX_VOICES,
        .maxPVoices = AUDIO_MAX_VOICES,
        .maxUpdates = AUDIO_MAX_UPDATES,
        .dmaproc = audio_dma_new,
        .heap = &audio_hp,
        .outputRate = audio_rate,
        .fxType = AL_FX_SMALLROOM,
    };
    alInit(&audio_globals, &scfg);

    ALSndpConfig pcfg = {
        .maxSounds = AUDIO_MAX_VOICES,
        .maxEvents = AUDIO_EVT_COUNT,
        .heap = &audio_hp,
    };
    alSndpNew(&audio_sndp, &pcfg);
    static ALEnvelope sndenv = {
        .attackVolume = 127,
        .decayVolume = 127,
        .decayTime = -1,
    };
    pak_load_asset_sync(&audio_trackbuf, sizeof(audio_trackbuf),
                        pak_track_object(asset));
    audio_track_fixup(&audio_trackbuf, asset);

    static ALSound snd = {
        .envelope = &sndenv,
        .wavetable = &audio_trackbuf.header.wavetable,
        .samplePan = AL_PAN_CENTER,
        .sampleVolume = AL_VOL_FULL,
        .flags = 1,
    };
    ALSndId sndid = alSndpAllocate(&audio_sndp, &snd);
    alSndpSetSound(&audio_sndp, sndid);
    alSndpSetPitch(&audio_sndp, 1.0f);
    alSndpSetPan(&audio_sndp, 64);
    alSndpSetVol(&audio_sndp, 30000);
    alSndpPlay(&audio_sndp);
}

// =============================================================================
// Audio Timing Updates
// =============================================================================

// Update the audio subsystem.
void audio_update(struct game_system *restrict sys) {
    unsigned track_offset =
        sys->time.current_frame_sample - sys->time.track_start;
    if (track_offset > 60 * 60 * AUDIO_SAMPLERATE) {
        // Paranoia? If sample goes backwards for some reason?
        fatal_error("Audio desynchronized");
    }
    unsigned end = sys->time.track_loop == 0
                       ? audio_trackbuf.header.lead_in
                       : audio_trackbuf.header.loop_length;
    if (track_offset >= end) {
        sys->time.track_start += end;
        sys->time.track_loop++;
    }
    sys->time.track_pos = track_offset;
}

// =============================================================================
// Audio Scheduling
// =============================================================================

static struct scheduler_task audio_tasks[3] __attribute__((section("uninit")));
static Acmd audio_cmdlist[2][AUDIO_CLIST_SIZE];
static int16_t audio_buffers[3][2 * AUDIO_BUFSZ]
    __attribute__((aligned(16), section("uninit")));

// Get the resource mask for the given task.
static unsigned audio_taskmask(int i) {
    return 1u << i;
}

// Get the resource mask for the given framebuffer.
static unsigned audio_buffermask(int i) {
    return 4u << i;
}

void audio_frame(struct game_system *restrict sys,
                 struct audio_state *restrict st, struct scheduler *sc,
                 OSMesgQueue *queue) {
    // Return finished DMA messages.
    {
        int nactive = audio_dmanactive;
        for (;;) {
            OSMesg mesg;
            int r = osRecvMesg(&audio_dmaqueue, &mesg, OS_MESG_NOBLOCK);
            if (r == -1) {
                break;
            }
            nactive--;
        }
        audio_dmanactive = nactive;
    }

    // Increase the age of all sample buffers.
    for (int i = 0; i < AUDIO_DMA_COUNT; i++) {
        audio_dma[i].age++;
    }

    // Create the command list.
    int16_t *buffer = audio_buffers[st->current_buffer];
    s32 cmdlen = 0;
    Acmd *al_start = audio_cmdlist[st->current_task];
    Acmd *al_end =
        alAudioFrame(al_start, &cmdlen, (s16 *)K0_TO_PHYS(buffer), AUDIO_BUFSZ);
    if (al_end - al_start > AUDIO_CLIST_SIZE) {
        fatal_error("Audio command list overrun\nsize=%td", al_end - al_start);
    }

    // Create and sumbit the task.
    struct scheduler_task *task = &audio_tasks[st->current_task];
    if (cmdlen == 0) {
        bzero(buffer, 4 * AUDIO_BUFSZ);
        osWritebackDCache(buffer, 4 * AUDIO_BUFSZ);
        task->flags = SCHEDULER_TASK_AUDIOBUFFER;
    } else {
        task->flags = SCHEDULER_TASK_AUDIO | SCHEDULER_TASK_AUDIOBUFFER;
        task->task = (OSTask){{
            .type = M_AUDTASK,
            .flags = OS_TASK_DP_WAIT,
            .ucode_boot = (u64 *)rspbootTextStart,
            .ucode_boot_size =
                (uintptr_t)rspbootTextEnd - (uintptr_t)rspbootTextStart,
            .ucode_data = (u64 *)aspMainDataStart,
            .ucode_data_size = SP_UCODE_DATA_SIZE,
            .ucode = (u64 *)aspMainTextStart,
            .ucode_size = SP_UCODE_SIZE,
            .dram_stack = NULL,
            .dram_stack_size = 0,
            .data_ptr = (u64 *)al_start,
            .data_size = sizeof(Acmd) * (al_end - al_start),
        }};
        osWritebackDCache(al_start, sizeof(Acmd) * (al_end - al_start));
    }
    task->done_queue = queue;
    task->done_mesg = event_pack((struct event_data){
        .type = EVENT_AUDIO,
        .value = audio_taskmask(st->current_task),
    });
    task->data.audiobuffer = (struct scheduler_audiobuffer){
        .ptr = buffer,
        .size = 4 * AUDIO_BUFSZ,
        .sample = sys->time.current_sample,
        .done_queue = queue,
        .done_mesg = event_pack((struct event_data){
            .type = EVENT_AUDIO,
            .value = audio_buffermask(st->current_buffer),
        }),
    };
    scheduler_submit(sc, task);

    // Update the task state.
    st->busy |=
        audio_taskmask(st->current_task) | audio_buffermask(st->current_buffer);
    st->current_task++;
    if (st->current_task == 2) {
        st->current_task = 0;
    }
    st->current_buffer++;
    if (st->current_buffer == 3) {
        st->current_buffer = 0;
    }
    st->wait =
        audio_taskmask(st->current_task) | audio_buffermask(st->current_buffer);
    sys->time.current_sample += AUDIO_BUFSZ;
}
