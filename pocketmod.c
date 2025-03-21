#include <stdio.h>
#include <stdlib.h>

#include "pocketmod.h"


#define POCKETMOD_NO_INTERPOLATION

/* Memorize a parameter unless the new value is zero */
#define POCKETMOD_MEM(dst, src) do { \
        (dst) = (src) ? (src) : (dst); \
    } while (0)

/* Same thing, but memorize each nibble separately */
#define POCKETMOD_MEM2(dst, src) do { \
        (dst) = (((src) & 0x0f) ? ((src) & 0x0f) : ((dst) & 0x0f)) \
              | (((src) & 0xf0) ? ((src) & 0xf0) : ((dst) & 0xf0)); \
    } while (0)

/* Channel dirty flags */
#define POCKETMOD_PITCH  0x01
#define POCKETMOD_VOLUME 0x02

/* The size of one sample in bytes */
#define POCKETMOD_SAMPLE_SIZE sizeof(int16_t[2])

/* Finetune adjustment table. Three octaves for each finetune setting. */
static const int8_t _pocketmod_finetune[16][36] = {
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    { -6, -6, -5, -5, -4, -3, -3, -3, -3, -3, -3, -3, -3, -3, -2, -3, -2, -2, -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0},
    {-12,-12,-10,-11, -8, -8, -7, -7, -6, -6, -6, -6, -6, -6, -5, -5, -4, -4, -4, -3, -3, -3, -3, -2, -3, -3, -2, -3, -3, -2, -2, -2, -2, -2, -2, -1},
    {-18,-17,-16,-16,-13,-12,-12,-11,-10,-10,-10, -9, -9, -9, -8, -8, -7, -6, -6, -5, -5, -5, -5, -4, -5, -4, -3, -4, -4, -3, -3, -3, -3, -2, -2, -2},
    {-24,-23,-21,-21,-18,-17,-16,-15,-14,-13,-13,-12,-12,-12,-11,-10, -9, -8, -8, -7, -7, -7, -7, -6, -6, -6, -5, -5, -5, -4, -4, -4, -4, -3, -3, -3},
    {-30,-29,-26,-26,-23,-21,-20,-19,-18,-17,-17,-16,-15,-14,-13,-13,-11,-11,-10, -9, -9, -9, -8, -7, -8, -7, -6, -6, -6, -5, -5, -5, -5, -4, -4, -4},
    {-36,-34,-32,-31,-27,-26,-24,-23,-22,-21,-20,-19,-18,-17,-16,-15,-14,-13,-12,-11,-11,-10,-10, -9, -9, -9, -7, -8, -7, -6, -6, -6, -6, -5, -5, -4},
    {-42,-40,-37,-36,-32,-30,-29,-27,-25,-24,-23,-22,-21,-20,-18,-18,-16,-15,-14,-13,-13,-12,-12,-10,-10,-10, -9, -9, -9, -8, -7, -7, -7, -6, -6, -5},
    { 51, 48, 46, 42, 42, 38, 36, 34, 32, 30, 24, 27, 25, 24, 23, 21, 21, 19, 18, 17, 16, 15, 14, 14, 12, 12, 12, 10, 10, 10,  9,  8,  8,  8,  7,  7},
    { 44, 42, 40, 37, 37, 35, 32, 31, 29, 27, 25, 24, 22, 21, 20, 19, 18, 17, 16, 15, 15, 14, 13, 12, 11, 10, 10,  9,  9,  9,  8,  7,  7,  7,  6,  6},
    { 38, 36, 34, 32, 31, 30, 28, 27, 25, 24, 22, 21, 19, 18, 17, 16, 16, 15, 14, 13, 13, 12, 11, 11,  9,  9,  9,  8,  7,  7,  7,  6,  6,  6,  5,  5},
    { 31, 30, 29, 26, 26, 25, 24, 22, 21, 20, 18, 17, 16, 15, 14, 13, 13, 12, 12, 11, 11, 10,  9,  9,  8,  7,  8,  7,  6,  6,  6,  5,  5,  5,  5,  5},
    { 25, 24, 23, 21, 21, 20, 19, 18, 17, 16, 14, 14, 13, 12, 11, 10, 11, 10, 10,  9,  9,  8,  7,  7,  6,  6,  6,  5,  5,  5,  5,  4,  4,  4,  3,  4},
    { 19, 18, 17, 16, 16, 15, 15, 14, 13, 12, 11, 10,  9,  9,  9,  8,  8, 18,  7,  7,  7,  6,  5,  6,  5,  4,  5,  4,  4,  4,  4,  3,  3,  3,  3,  3},
    { 12, 12, 12, 10, 11, 11, 10, 10,  9,  8,  7,  7,  6,  6,  6,  5,  6,  5,  5,  5,  5,  4,  4,  4,  3,  3,  3,  3,  2,  3,  3,  2,  2,  2,  2,  2},
    {  6,  6,  6,  5,  6,  6,  6,  5,  5,  5,  4,  4,  3,  3,  3,  3,  3,  3,  3,  3,  3,  2,  2,  2,  2,  1,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1}
};

/* Min/max helper functions */
static int32_t _pocketmod_min(int32_t x, int32_t y) { return x < y ? x : y; }
static int32_t _pocketmod_max(int32_t x, int32_t y) { return x > y ? x : y; }

static pocketmod_context c;
static FILE* file;

static uint8_t read_u8() {
  uint8_t out = 0;
  fread(&out, sizeof(out), 1, file);
  return out;
}

static uint16_t read_u16() {
  const uint16_t b0 = read_u8();
  const uint16_t b1 = read_u8();
  return (b0 << 8) | b1;
}

static void file_seek_to(uint32_t offset) {
  fseek(file, offset, SEEK_SET);
}

static void file_skip(uint32_t bytes) {
  fseek(file, bytes, SEEK_CUR);
}

/* Clamp a volume value to the 0..64 range */
static int32_t _pocketmod_clamp_volume(int32_t x)
{
    x = _pocketmod_max(x, 0x00);
    x = _pocketmod_min(x, 0x40);
    return x;
}

/* Zero out a block of memory */
static void _pocketmod_zero(void *data, int32_t size)
{
    char *byte = data, *end = byte + size;
    while (byte != end) { *byte++ = 0; }
}

/* Convert a period (at finetune = 0) to a note index in 0..35 */
static int32_t _pocketmod_period_to_note(int32_t period)
{
    switch (period) {
        case 856: return  0; case 808: return  1; case 762: return  2;
        case 720: return  3; case 678: return  4; case 640: return  5;
        case 604: return  6; case 570: return  7; case 538: return  8;
        case 508: return  9; case 480: return 10; case 453: return 11;
        case 428: return 12; case 404: return 13; case 381: return 14;
        case 360: return 15; case 339: return 16; case 320: return 17;
        case 302: return 18; case 285: return 19; case 269: return 20;
        case 254: return 21; case 240: return 22; case 226: return 23;
        case 214: return 24; case 202: return 25; case 190: return 26;
        case 180: return 27; case 170: return 28; case 160: return 29;
        case 151: return 30; case 143: return 31; case 135: return 32;
        case 127: return 33; case 120: return 34; case 113: return 35;
        default: return 0;
    }
}

/* Table-based sine wave oscillator */
static int32_t _pocketmod_sin(int32_t step)
{
    /* round(sin(x * pi / 32) * 255) for x in 0..15 */
    static const uint8_t sin[16] = {
        0x00, 0x19, 0x32, 0x4a, 0x62, 0x78, 0x8e, 0xa2,
        0xb4, 0xc5, 0xd4, 0xe0, 0xec, 0xf4, 0xfa, 0xfe
    };
    int32_t x = sin[step & 0x0f];
    x = (step & 0x1f) < 0x10 ? x : 0xff - x;
    return step < 0x20 ? x : -x;
}

/* Oscillators for vibrato/tremolo effects */
static int32_t _pocketmod_lfo(_pocketmod_chan *ch, int32_t step)
{
    switch (ch->lfo_type[ch->effect == 7] & 3) {
        case 0: return _pocketmod_sin(step & 0x3f);         /* Sine   */
        case 1: return 0xff - ((step & 0x3f) << 3);         /* Saw    */
        case 2: return (step & 0x3f) < 0x20 ? 0xff : -0xff; /* Square */
        case 3: return (c.lfo_rng & 0x1ff) - 0xff;         /* Random */
        default: return 0; /* Hush little compiler */
    }
}

static void _pocketmod_upload_sample(_pocketmod_sample* sample, uint32_t sample_offset) {

  if (c.events) {
    c.events->on_upload_sample(sample);
  }
}

static void _pocketmod_sample_set(_pocketmod_chan* ch, int32_t sample) {

  ch->sample = sample;

  if (c.events) {
    if (sample >= 1) {
      c.events->on_sample_set(ch, c.samples + (sample - 1));
    }
    else {
      c.events->on_sample_set(ch, NULL);
    }
  }
}

static void _pocketmod_position_set(_pocketmod_chan* ch, float position) {

  ch->position = position;

  if (c.events) {
    c.events->on_position_set(ch, position);
  }
}

static void _pocketmod_period_set(_pocketmod_chan* ch, float period) {

  // note: 3546895 is the PAL colorclock rate
  // the replay rate is:
  //    replay_rate = (3546895 / period)
  // see https://aminet.net/package/driver/audio/MaxReplayTest section 2

  // note: period counter is 16bits wide

  ch->increment = 3546894.6f / (period * c.samples_per_second);

  if (c.events) {
    c.events->on_period_set(ch, period);
  }
}

static void _pocketmod_volume_set(_pocketmod_chan* ch, uint8_t value) {

  // note: volume can be up 0->63

  // note: volume is done using a 6bit pwm chopper circuit
  // https://www.linusakesson.net/music/paulimba/index.php

  ch->real_volume = value;

  if (c.events) {
    c.events->on_volume_set(ch, value);
  }
}

static void _pocketmod_balance_set(_pocketmod_chan* ch, uint8_t balance) {

  ch->balance = balance;

  if (c.events) {
    c.events->on_balance_set(ch, balance);
  }
}

static void _pocketmod_update_pitch(_pocketmod_chan *ch)
{
    /* Don't do anything if the period is zero */
    ch->increment = 0.0f;
    if (ch->period) {
        float period = ch->period;

        /* Apply vibrato (if active) */
        if (ch->effect == 0x4 || ch->effect == 0x6) {
            int32_t step = (ch->param4 >> 4) * ch->lfo_step;
            int32_t rate = ch->param4 & 0x0f;
            period += _pocketmod_lfo(ch, step) * rate / 128.0f;

        /* Apply arpeggio (if active) */
        } else if (ch->effect == 0x0 && ch->param) {
            static const float arpeggio[16] = { /* 2^(X/12) for X in 0..15 */
                1.000000f, 1.059463f, 1.122462f, 1.189207f,
                1.259921f, 1.334840f, 1.414214f, 1.498307f,
                1.587401f, 1.681793f, 1.781797f, 1.887749f,
                2.000000f, 2.118926f, 2.244924f, 2.378414f
            };
            int32_t step = (ch->param >> ((2 - c.tick % 3) << 2)) & 0x0f;
            period /= arpeggio[step];
        }

        /* Calculate sample buffer position increment */
        _pocketmod_period_set(ch, period);
    }

    /* Clear the pitch dirty flag */
    ch->dirty &= ~POCKETMOD_PITCH;
}

static void _pocketmod_update_volume(_pocketmod_chan *ch)
{
    int32_t volume = ch->volume;
    if (ch->effect == 0x7) {
        int32_t step = ch->lfo_step * (ch->param7 >> 4);
        volume += _pocketmod_lfo(ch, step) * (ch->param7 & 0x0f) >> 6;
    }
    _pocketmod_volume_set(ch, _pocketmod_clamp_volume(volume));
    ch->dirty &= ~POCKETMOD_VOLUME;
}

static void _pocketmod_pitch_slide(_pocketmod_chan *ch, int32_t amount)
{
    int32_t max = 856 + _pocketmod_finetune[ch->finetune][ 0];
    int32_t min = 113 + _pocketmod_finetune[ch->finetune][35];
    ch->period += amount;
    ch->period = _pocketmod_max(ch->period, min);
    ch->period = _pocketmod_min(ch->period, max);
    ch->dirty |= POCKETMOD_PITCH;
}

static void _pocketmod_volume_slide(_pocketmod_chan *ch, int32_t param)
{
    /* Undocumented quirk: If both x and y are nonzero, then the value of x */
    /* takes precedence. (Yes, there are songs that rely on this behavior.) */
    int32_t change = (param & 0xf0) ? (param >> 4) : -(param & 0x0f);
    ch->volume = _pocketmod_clamp_volume(ch->volume + change);
    ch->dirty |= POCKETMOD_VOLUME;
}

static void _pocketmod_next_line()
{
    uint8_t (*data)[4];
    int32_t i, pos, pattern_break = -1;

    /* When entering a new pattern order index, mark it as "visited" */
    if (c.line == 0) {
        c.visited[c.pattern >> 3] |= 1 << (c.pattern & 7);
    }

    /* Move to the next pattern if this was the last line */
    if (++c.line == 64) {
        if (++c.pattern == c.length) {
            c.pattern = c.reset;
        }
        c.line = 0;
    }

    /* Find the pattern data for the current line */
    pos = (c.order[c.pattern] * 64 + c.line) * c.num_channels * 4;
    data = (uint8_t(*)[4]) (c.patterns + pos);
    for (i = 0; i < c.num_channels; i++) {

        /* Decode columns */
        int32_t sample = (data[i][0] & 0xf0) | (data[i][2] >> 4);
        int32_t period = ((data[i][0] & 0x0f) << 8) | data[i][1];
        int32_t effect = ((data[i][2] & 0x0f) << 8) | data[i][3];

        /* Memorize effect parameter values */
        _pocketmod_chan *ch = &c.channels[i];
        ch->effect = (effect >> 8) != 0xe ? (effect >> 8)   : (effect >> 4);
        ch->param  = (effect >> 8) != 0xe ? (effect & 0xff) : (effect & 0x0f);

        /* Set sample */
        if (sample) {
            if (sample <= POCKETMOD_MAX_SAMPLES) {
              _pocketmod_sample_set(ch, sample);
                ch->finetune = c.samples[sample-1].finetune;
                ch->volume   = c.samples[sample-1].volume;
                if (ch->effect != 0xED) {
                    ch->dirty |= POCKETMOD_VOLUME;
                }
            } else {
                _pocketmod_sample_set(ch, 0);
            }
        }

        /* Set note */
        if (period) {
            int32_t note = _pocketmod_period_to_note(period);
            period += _pocketmod_finetune[ch->finetune][note];
            if (ch->effect != 0x3) {
                if (ch->effect != 0xED) {
                    ch->period = period;
                    ch->dirty |= POCKETMOD_PITCH;
                    _pocketmod_position_set(ch, 0.0f);  // reset position
                    ch->lfo_step = 0;
                } else {
                    ch->delayed = period;
                }
            }
        }

        /* Handle pattern effects */
        switch (ch->effect) {

            /* Memorize parameters */
            case 0x3:  POCKETMOD_MEM (ch->param3,  ch->param); /* Fall through */
            case 0x5:  POCKETMOD_MEM (ch->target,  period);    break;
            case 0x4:  POCKETMOD_MEM2(ch->param4,  ch->param); break;
            case 0x7:  POCKETMOD_MEM2(ch->param7,  ch->param); break;
            case 0xE1: POCKETMOD_MEM (ch->paramE1, ch->param); break;
            case 0xE2: POCKETMOD_MEM (ch->paramE2, ch->param); break;
            case 0xEA: POCKETMOD_MEM (ch->paramEA, ch->param); break;
            case 0xEB: POCKETMOD_MEM (ch->paramEB, ch->param); break;

            /* 8xx: Set stereo balance (nonstandard) */
            case 0x8: {
                _pocketmod_balance_set(ch, ch->param);
            } break;

            /* 9xx: Set sample offset */
            case 0x9: {
                if (period != 0 || sample != 0) {
                    ch->param9 = ch->param ? ch->param : ch->param9;
                    _pocketmod_position_set(ch, (float)(ch->param9 << 8));
                }
            } break;

            /* Bxx: Jump to pattern */
            case 0xB: {
                c.pattern = ch->param < c.length ? ch->param : 0;
                c.line = -1;
            } break;

            /* Cxx: Set volume */
            case 0xC: {
                ch->volume = _pocketmod_clamp_volume(ch->param);
                ch->dirty |= POCKETMOD_VOLUME;
            } break;

            /* Dxy: Pattern break */
            case 0xD: {
                pattern_break = (ch->param >> 4) * 10 + (ch->param & 15);
            } break;

            /* E4x: Set vibrato waveform */
            case 0xE4: {
                ch->lfo_type[0] = ch->param;
            } break;

            /* E5x: Set sample finetune */
            case 0xE5: {
                ch->finetune = ch->param;
                ch->dirty |= POCKETMOD_PITCH;
            } break;

            /* E6x: Pattern loop */
            case 0xE6: {
                if (ch->param) {
                    if (!ch->loop_count) {
                        ch->loop_count = ch->param;
                        c.line = ch->loop_line;
                    } else if (--ch->loop_count) {
                        c.line = ch->loop_line;
                    }
                } else {
                    ch->loop_line = c.line - 1;
                }
            } break;

            /* E7x: Set tremolo waveform */
            case 0xE7: {
                ch->lfo_type[1] = ch->param;
            } break;

            /* E8x: Set stereo balance (nonstandard) */
            case 0xE8: {
                _pocketmod_balance_set(ch, ch->param << 4);
            } break;

            /* EEx: Pattern delay */
            case 0xEE: {
                c.pattern_delay = ch->param;
            } break;

            /* Fxx: Set speed */
            case 0xF: {
                if (ch->param != 0) {
                    if (ch->param < 0x20) {
                        c.ticks_per_line = ch->param;
                    } else {
                        c.ticks_per_second = (0.4f * ch->param);
                        c.samples_per_tick = c.samples_per_second / c.ticks_per_second;
                    }
                }
            } break;

            default: break;
        }
    }

    /* Pattern breaks are handled here, so that only one jump happens even  */
    /* when multiple Dxy commands appear on the same line. (You guessed it: */
    /* There are songs that rely on this behavior!)                         */
    if (pattern_break != -1) {
        c.line = (pattern_break < 64 ? pattern_break : 0) - 1;
        if (++c.pattern == c.length) {
            c.pattern = c.reset;
        }
    }
}

static void _pocketmod_next_tick()
{
    int32_t i;

    /* Move to the next line if this was the last tick */
    if (++c.tick == c.ticks_per_line) {
        if (c.pattern_delay > 0) {
            c.pattern_delay--;
        } else {
            _pocketmod_next_line(c);
        }
        c.tick = 0;
    }

    /* Make per-tick adjustments for all channels */
    for (i = 0; i < c.num_channels; i++) {
        _pocketmod_chan *ch = &c.channels[i];
        int32_t param = ch->param;

        /* Advance the LFO random number generator */
        c.lfo_rng = 0x0019660d * c.lfo_rng + 0x3c6ef35f;

        /* Handle effects that may happen on any tick of a line */
        switch (ch->effect) {

            /* 0xy: Arpeggio */
            case 0x0: {
                ch->dirty |= POCKETMOD_PITCH;
            } break;

            /* E9x: Retrigger note every x ticks */
            case 0xE9: {
                if (!(param && c.tick % param)) {
                    _pocketmod_position_set(ch, 0.0f);
                    ch->lfo_step = 0;
                }
            } break;

            /* ECx: Cut note after x ticks */
            case 0xEC: {
                if (c.tick == param) {
                    ch->volume = 0;
                    ch->dirty |= POCKETMOD_VOLUME;
                }
            } break;

            /* EDx: Delay note for x ticks */
            case 0xED: {
                if (c.tick == param && ch->sample) {
                    ch->dirty |= POCKETMOD_VOLUME | POCKETMOD_PITCH;
                    ch->period = ch->delayed;
                    _pocketmod_position_set(ch, 0.f);
                    ch->lfo_step = 0;
                }
            } break;

            default: break;
        }

        /* Handle effects that only happen on the first tick of a line */
        if (c.tick == 0) {
            switch (ch->effect) {
                case 0xE1: _pocketmod_pitch_slide (ch, -ch->paramE1);     break;
                case 0xE2: _pocketmod_pitch_slide (ch, +ch->paramE2);     break;
                case 0xEA: _pocketmod_volume_slide(ch, ch->paramEA << 4); break;
                case 0xEB: _pocketmod_volume_slide(ch, ch->paramEB & 15); break;
                default: break;
            }

        /* Handle effects that are not applied on the first tick of a line */
        } else {
            switch (ch->effect) {

                /* 1xx: Portamento up */
                case 0x1: {
                    _pocketmod_pitch_slide(ch, -param);
                } break;

                /* 2xx: Portamento down */
                case 0x2: {
                    _pocketmod_pitch_slide(ch, +param);
                } break;

                /* 5xy: Volume slide + tone portamento */
                case 0x5: {
                    _pocketmod_volume_slide(ch, param);
                } /* Fall through */

                /* 3xx: Tone portamento */
                case 0x3: {
                    int32_t rate = ch->param3;
                    int32_t order = ch->period < ch->target;
                    int32_t closer = ch->period + (order ? rate : -rate);
                    int32_t new_order = closer < ch->target;
                    ch->period = new_order == order ? closer : ch->target;
                    ch->dirty |= POCKETMOD_PITCH;
                } break;

                /* 6xy: Volume slide + vibrato */
                case 0x6: {
                    _pocketmod_volume_slide(ch, param);
                } /* Fall through */

                /* 4xy: Vibrato */
                case 0x4: {
                    ch->lfo_step++;
                    ch->dirty |= POCKETMOD_PITCH;
                } break;

                /* 7xy: Tremolo */
                case 0x7: {
                    ch->lfo_step++;
                    ch->dirty |= POCKETMOD_VOLUME;
                } break;

                /* Axy: Volume slide */
                case 0xA: {
                    _pocketmod_volume_slide(ch, param);
                } break;

                default: break;
            }
        }

        /* Update channel volume/pitch if either is out of date */
        if (ch->dirty & POCKETMOD_VOLUME) { _pocketmod_update_volume(ch); }
        if (ch->dirty & POCKETMOD_PITCH)  { _pocketmod_update_pitch (ch); }
    }
}

static void _pocketmod_render_channel(_pocketmod_chan *chan,
                                      int16_t *output,
                                      int32_t samples_to_write)
{
    /* Gather some loop data */
    _pocketmod_sample *sample = &c.samples[chan->sample - 1];
    uint8_t* data = sample->data;
    const float sample_end = (float)(1 + _pocketmod_min(sample->loop_end, sample->length));

    /* Calculate left/right levels */
    /* 64 * 256 = 0x4000 */
    const int32_t level_l = (chan->real_volume * (255-chan->balance));
    const int32_t level_r = (chan->real_volume *      chan->balance);

    /* Write samples */
    int32_t i, num;
    do {
        /* Calculate how many samples we can write in one go */
        num = (int32_t)( (sample_end - chan->position) / chan->increment );
        num = _pocketmod_min(num, samples_to_write);

        /* Resample and write 'num' samples */
        for (i = 0; i < num; i++) {
            const int32_t x0 = (int32_t)(chan->position);
#ifdef POCKETMOD_NO_INTERPOLATION
            const int32_t s = sample->data[x0];
#else
            int32_t x1 = x0 + 1 - sample->loop_length * (x0 + 1 >= sample->loop_end);
            float t = chan->position - x0;
            float s = (1.0f - t) * sample->data[x0] + t * sample->data[x1];
#endif
            chan->position += chan->increment;
            *output++ += (level_l * s) / (64 * 4);  // 4=numchannels
            *output++ += (level_r * s) / (64 * 4);
        }

        /* Rewind the sample when reaching the loop point */
        if (chan->position >= sample->loop_end) {
            chan->position -= sample->loop_length;

        /* Cut the sample if the end is reached */
        } else if (chan->position >= sample->length) {
            chan->position = -1.0f;
            break;
        }

        samples_to_write -= num;
    } while (num > 0);
}

static int32_t _pocketmod_ident(int32_t size)
{
    /* 31-instrument files are at least 1084 bytes long */
    if (size < 1084) {
      return 0;
    }

    uint8_t tag[4] = { 0 };
    file_seek_to(1080);
    fread(tag, 1, 4, file);

    static const struct { char name[5]; } tags[] = { {"M.K."}, {"M!K!"}, {"FLT4"}, {"4CHN"} };

    /* Check the format tag to determine if this is a 31-sample MOD */
    for (int32_t i = 0; i < (int32_t) (sizeof(tags) / sizeof(*tags)); i++) {
        if (tags[i].name[0] == tag[0] &&
            tags[i].name[1] == tag[1] &&
            tags[i].name[2] == tag[2] &&
            tags[i].name[3] == tag[3]) {

            file_seek_to(950); c.length = read_u8();
            file_seek_to(951); c.reset  = read_u8();

            file_seek_to(952);
            fread(c.order, 1, 128, file);
            c.pattern_addr = 1084;
            c.num_samples  = 31;
            c.num_channels = 4;
            return 1;
        }
    }

    return 0;
}

int32_t pocketmod_init(pocketmod_events* e, FILE *fd, int32_t size, int32_t rate)
{
    file = fd;
    int32_t i, remaining, header_bytes, pattern_bytes;
    uint32_t sample_offset = 0;

    /* Check that arguments look more or less sane */
    if (!file || rate <= 0 || size <= 0) {
        return 0;
    }

    /* Zero out the whole context and identify the MOD type */
    _pocketmod_zero(&c, sizeof(pocketmod_context));
    if (!_pocketmod_ident(size)) {
        return 0;
    }

    /* Check that we are compiled with support for enough channels */
    if (c.num_channels > POCKETMOD_MAX_CHANNELS) {
        return 0;
    }

    /* Check that we have enough sample slots for this file */
    if (POCKETMOD_MAX_SAMPLES < 31) {
        file_seek_to(20 + 22);
        for (i = 0; i < c.num_samples; i++) {
            const uint32_t length = read_u16() << 1;
            if (i >= POCKETMOD_MAX_SAMPLES && length > 2) {
                return 0; /* Can't fit this sample */
            }
            file_skip(30 - 2);
        }
    }

    /* Check that the song length is in valid range (1..128) */
    if (c.length == 0 || c.length > 128) {
        return 0;
    }

    /* Make sure that the reset pattern doesn't take us out of bounds */
    if (c.reset >= c.length) {
        c.reset = 0;
    }

    /* Count how many patterns there are in the file */
    c.num_patterns = 0;
    for (i = 0; i < 128 && c.order[i] < 128; i++) {
        c.num_patterns = _pocketmod_max(c.num_patterns, c.order[i]);
    }
    pattern_bytes = 256 * c.num_channels * ++c.num_patterns;
    header_bytes  = c.pattern_addr;

    c.patterns = malloc(pattern_bytes);
    file_seek_to(c.pattern_addr);
    fread(c.patterns, 1, pattern_bytes, file);

    /* Check that each pattern in the order is within file bounds */
    for (i = 0; i < c.length; i++) {
        if (header_bytes + 256 * c.num_channels * c.order[i] > size) {
            return 0; /* Reading this pattern would be a buffer over-read! */
        }
    }

    /* Check that the pattern data doesn't extend past the end of the file */
    if (header_bytes + pattern_bytes > size) {
        return 0;
    }

    /* setup event notifiers */
    c.events = e;

    //DEBUG: used to calcuate the total required sample space
    uint32_t total_size = 0;

    /* Load sample payload data, truncating ones that extend outside the file */
    remaining = size - header_bytes - pattern_bytes;
    sample_offset = header_bytes + pattern_bytes;
    for (i = 0; i < c.num_samples; i++) {

        file_seek_to(12 + 30 * (i + 1));
        uint32_t length = read_u16() << 1;

        _pocketmod_sample *sample = &c.samples[i];
        sample->index       = i;
        sample->length      = _pocketmod_min(length > 2 ? length : 0, remaining);
        sample->finetune    = read_u8() & 0xf;
        sample->volume      = _pocketmod_min(read_u8(), 0x40);
        sample->loop_start  = (read_u16()) << 1;
        sample->loop_length = (read_u16()) << 1;
        sample->loop_end    = sample->loop_length > 2 ? sample->loop_start + sample->loop_length : 0xffffff;

#if 0
        sample->data = (int8_t*)data + sample_offset;
#else
        sample->data = malloc(sample->length);
        if (sample->data) {
          file_seek_to(sample_offset);
          fread(sample->data, 1, sample->length, file);
        }
#endif
        _pocketmod_upload_sample(sample, sample_offset);

        //DEBUG: calculate the used sample size
        total_size += _pocketmod_min(sample->loop_end, sample->length);

        sample_offset += sample->length;
        remaining     -= sample->length;
    }

    //DEBUG: print to total required sample space
    printf("total sample size: %uKB (%u bytes)\n", total_size / 1024, total_size);

    /* Set up ProTracker default panning for all channels */
    for (i = 0; i < c.num_channels; i++) {
        c.channels[i].index = i;
        uint8_t balance = 0x80 + ((((i + 1) >> 1) & 1) ? 0x20 : -0x20);
        _pocketmod_balance_set(c.channels + i, balance);
    }

    /* Prepare to render from the start */
    c.ticks_per_line     = 6;
    c.samples_per_second = rate;
    c.ticks_per_second   = 50.f;
    c.samples_per_tick   = rate / c.ticks_per_second;
    c.lfo_rng            = 0xbadc0de;
    c.line               = -1;
    c.tick               = c.ticks_per_line - 1;
    _pocketmod_next_tick(c);
    return 1;
}

int32_t pocketmod_render(void *buffer, int32_t buffer_size)
{
    int32_t i, samples_rendered = 0;
    int32_t samples_remaining = buffer_size / POCKETMOD_SAMPLE_SIZE;
    if (buffer) {
        int16_t (*output)[2] = (int16_t(*)[2]) buffer;
        while (samples_remaining > 0) {

            /* Calculate the number of samples left in this tick */
            int32_t num = (int32_t) (c.samples_per_tick - c.sample);
            num = _pocketmod_min(num + !num, samples_remaining);

            /* Render and mix 'num' samples from each channel */
            _pocketmod_zero(output, num * POCKETMOD_SAMPLE_SIZE);
            for (i = 0; i < c.num_channels; i++) {
                _pocketmod_chan *chan = &c.channels[i];
                if (chan->sample != 0 && chan->position >= 0.0f) {
                    _pocketmod_render_channel(chan, *output, num);
                }
            }
            samples_remaining -= num;
            samples_rendered  += num;
            output            += num;

            /* Advance song position by 'num' samples */
            if ((c.sample += num) >= c.samples_per_tick) {
                c.sample -= c.samples_per_tick;
                _pocketmod_next_tick(c);

                /* Stop if a new pattern was reached */
                if (c.line == 0 && c.tick == 0) {

                    /* Increment loop counter as needed */
                    if (c.visited[c.pattern >> 3] & (1 << (c.pattern & 7))) {
                        _pocketmod_zero(c.visited, sizeof(c.visited));
                        c.loop_count++;
                    }
                    break;
                }
            }
        }
    }
    return samples_rendered * POCKETMOD_SAMPLE_SIZE;
}

int32_t pocketmod_loop_count()
{
    return c.loop_count;
}

int32_t pocketmod_tick()
{
  _pocketmod_next_tick(c);
  /* If a new pattern was reached... */
  if (c.line == 0 && c.tick == 0) {
    /* Increment loop counter as needed */
    if (c.visited[c.pattern >> 3] & (1 << (c.pattern & 7))) {
      _pocketmod_zero(c.visited, sizeof(c.visited));
      c.loop_count++;
    }
  }
  return 0;
}
