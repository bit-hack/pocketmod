/* See end of file for license */

#ifndef POCKETMOD_H_INCLUDED
#define POCKETMOD_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pocketmod_context pocketmod_context;
typedef struct pocketmod_events pocketmod_events;

int32_t pocketmod_init      (pocketmod_events *e, FILE* file, const void *data, int32_t size, int32_t rate);
int32_t pocketmod_render    (void *buffer, int32_t size);
int32_t pocketmod_loop_count();
int32_t pocketmod_tick      ();

#ifndef POCKETMOD_MAX_CHANNELS
#define POCKETMOD_MAX_CHANNELS 32
#endif

#ifndef POCKETMOD_MAX_SAMPLES
#define POCKETMOD_MAX_SAMPLES 31
#endif


typedef struct
{
    uint8_t  index;              /* Sample index                            */
    int8_t  *data;               /* Sample data buffer                      */
    uint32_t length;             /* Data length (in bytes)                  */
    int32_t  loop_start;         /* Loop start pos (unused)                 */
    int32_t  loop_length;        /* Loop length                             */
    int32_t  loop_end;           /* Loop end pos                            */
    uint8_t  finetune;           /* 0 - 15 */
    uint8_t  volume;             /* 0 - 64 */
} _pocketmod_sample;

typedef struct
{
    uint8_t  index;              /* Channel index                           */
    uint8_t  dirty;              /* Pitch/volume dirty flags                */
    uint8_t  sample;             /* Sample number (0..31)                   */
    uint8_t  volume;             /* Base volume without tremolo (0..64)     */
    uint8_t  balance;            /* Stereo balance (0..255)                 */
    uint16_t period;             /* Note period (113..856)                  */
    uint16_t delayed;            /* Delayed note period (113..856)          */
    uint16_t target;             /* Target period (for tone portamento)     */
    uint8_t  finetune;           /* Note finetune (0..15)                   */
    uint8_t  loop_count;         /* E6x loop counter                        */
    uint8_t  loop_line;          /* E6x target line                         */
    uint8_t  lfo_step;           /* Vibrato/tremolo LFO step counter        */
    uint8_t  lfo_type[2];        /* LFO type for vibrato/tremolo            */
    uint8_t  effect;             /* Current effect (0x0..0xf or 0xe0..0xef) */
    uint8_t  param;              /* Raw effect parameter value              */
    uint8_t  param3;             /* Parameter memory for 3xx                */
    uint8_t  param4;             /* Parameter memory for 4xy                */
    uint8_t  param7;             /* Parameter memory for 7xy                */
    uint8_t  param9;             /* Parameter memory for 9xx                */
    uint8_t  paramE1;            /* Parameter memory for E1x                */
    uint8_t  paramE2;            /* Parameter memory for E2x                */
    uint8_t  paramEA;            /* Parameter memory for EAx                */
    uint8_t  paramEB;            /* Parameter memory for EBx                */
    uint8_t  real_volume;        /* Volume (with tremolo adjustment)        */
    float    position;           /* Position in sample data buffer          */
    float    increment;          /* Position increment per output sample    */
} _pocketmod_chan;

typedef void (*pm_upload_sample_t)(_pocketmod_sample* sample);
typedef void (*pm_sample_set_t)   (_pocketmod_chan* ch, _pocketmod_sample* sample);
typedef void (*pm_position_set_t) (_pocketmod_chan* ch, float position);
typedef void (*pm_period_set_t)   (_pocketmod_chan* ch, float period);
typedef void (*pm_volume_set_t)   (_pocketmod_chan* ch, uint8_t value);
typedef void (*pm_balance_set_t)  (_pocketmod_chan* ch, uint8_t balance);

struct pocketmod_events
{
  /* Notify event callbacks */
  pm_upload_sample_t on_upload_sample;
  pm_sample_set_t    on_sample_set;
  pm_position_set_t  on_position_set;
  pm_period_set_t    on_period_set;
  pm_volume_set_t    on_volume_set;
  pm_balance_set_t   on_balance_set;
};

struct pocketmod_context
{
    /* Event notifiers */
    pocketmod_events *events;

    /* Read-only song data */
    _pocketmod_sample samples[POCKETMOD_MAX_SAMPLES];
    uint8_t  order[128];         /* Pattern order table                     */
    uint32_t pattern_addr;
    uint8_t *patterns;           /* Start of pattern data                   */
    uint8_t  length;             /* Patterns in the order (1..128)          */
    uint8_t  reset;              /* Pattern to loop back to (0..127)        */
    uint8_t  num_patterns;       /* Patterns in the file (1..128)           */
    uint8_t  num_samples;        /* Sample count (15 or 31)                 */
    uint8_t  num_channels;       /* Channel count (1..32)                   */

    /* Timing variables */
    int32_t samples_per_second;  /* Sample rate (set by user)               */
    int32_t ticks_per_line;      /* A.K.A. song speed (initially 6)         */
    float   samples_per_tick;    /* Depends on sample rate and BPM          */
    float   ticks_per_second;    /* ticks per second rate                   */

    /* Loop detection state */
    uint8_t visited[16];         /* Bit mask of previously visited patterns */
    int32_t loop_count;          /* How many times the song has looped      */

    /* Render state */
    _pocketmod_chan channels[POCKETMOD_MAX_CHANNELS];
    uint8_t  pattern_delay;      /* EEx pattern delay counter               */
    uint32_t lfo_rng;            /* RNG used for the random LFO waveform    */

    /* Position in song (from least to most granular) */
    int8_t  pattern;             /* Current pattern in order                */
    int8_t  line;                /* Current line in pattern                 */
    int16_t tick;                /* Current tick in line                    */
    float   sample;              /* Current sample in tick                  */
};

#ifdef __cplusplus
}
#endif

#endif /* #ifndef POCKETMOD_H_INCLUDED */

/*******************************************************************************

MIT License

Copyright (c) 2018 rombankzero

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*******************************************************************************/
