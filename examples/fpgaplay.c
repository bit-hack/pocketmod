
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>


#include "../pocketmod.h"


enum {
  PIN_SS = 8,
  PIN_MISO = 9,
  PIN_MOSI = 10,
  PIN_SCK = 11,
  PIN_A0 = 4,  // fpga - 19 (29b), PI-40p - 7 (bc4)
};

//static int fpgaSpi = PI_BAD_SPI_CHANNEL;

static bool fpgaInit() {
#if 0
  if (gpioInitialise() < 0) {
    return false;
  }

  // set pin modes
  gpioSetMode(PIN_MISO, PI_INPUT);
  gpioSetMode(PIN_MOSI, PI_OUTPUT);
  gpioSetMode(PIN_SS, PI_OUTPUT);
  gpioSetMode(PIN_SCK, PI_OUTPUT);
  gpioSetMode(PIN_A0, PI_OUTPUT);

  // mode=3
  // - POL=1
  // - PHA=1
  const unsigned flags = /*mode=*/3;
  fpgaSpi = spiOpen(
    /*channel=*/0,        // CS broadcom pin 8
    /*   baud=*/1500000,
    /*  flags=*/flags
  );

  if (fpgaSpi < 0) {
    printf("spiOpen failed!\n");
    return false;
  }
#endif
  return true;
}

static void fpgaWrite(uint8_t A0, uint8_t data) {
#if 0
  gpioWrite(PIN_A0, A0);
  char tx = data;
  char rx = 0;
  spiXfer(fpgaSpi, &tx, &rx, 1);
#endif
}

static void fpgaShutdown() {
#if 0
  gpioSetMode(PIN_MISO, PI_INPUT);
  gpioSetMode(PIN_MOSI, PI_INPUT);
  gpioSetMode(PIN_SS, PI_INPUT);
  gpioSetMode(PIN_SCK, PI_INPUT);
  gpioSetMode(PIN_A0, PI_INPUT);
#endif
}

enum {
  REG_POSITION = 0,
  REG_PERIOD = 1,
  REG_VOLUME = 2,
  REG_SAMPLE_START = 3,
  REG_SAMPLE_LOOP = 4,
  REG_SAMPLE_END = 5,
};

void pm_event_upload_sample(_pocketmod_sample* sample) {
  printf("%s\n", __func__);
}

void pm_event_sample_set(_pocketmod_chan* ch, _pocketmod_sample* s) {
  printf("%s\n", __func__);
}

void pm_event_position_set(_pocketmod_chan* ch, float position) {
  printf("%s\n", __func__);
  uint32_t pos = (uint32_t)position;
  fpgaWrite(1, (ch->index << 4) | REG_POSITION);
  fpgaWrite(0, 0xff & (pos >> 16));
  fpgaWrite(0, 0xff & (pos >> 8));
  fpgaWrite(0, 0xff & (pos >> 0));
}

void pm_event_period_set(_pocketmod_chan* ch, float period) {
  printf("%s\n", __func__);
  uint16_t p16 = (uint16_t)period;
  fpgaWrite(1, (ch->index << 4) | REG_PERIOD);
  fpgaWrite(0, 0xff & (p16 >> 8));
  fpgaWrite(0, 0xff & (p16 >> 0));
}

void pm_event_volume_set(_pocketmod_chan* ch, uint8_t value) {
  printf("%s\n", __func__);
  fpgaWrite(1, (ch->index << 4) | REG_VOLUME);
  fpgaWrite(0, value);
}

void pm_event_balance_set(_pocketmod_chan* ch, uint8_t balance) {
  printf("%s\n", __func__);
  // TODO
}

int main(int argc, char** args) {

  if (!fpgaInit()) {
    printf("fpgaInit!\n");
    return 0;
  }

  if (argc <= 1) {
    printf("usage: %s [track.mod]\n", args[0]);
    return 1;
  }

  FILE* fd = fopen(args[1], "rb");
  if (!fd) {
    printf("fopen('%s') failed\n", args[1]);
    return 1;
  }
  fseek(fd, 0, SEEK_END);
  long modSize = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  void* modData = malloc(modSize);
  if (!modData) {
    printf("malloc() failed!\n");
    return 1;
  }
  if (fread(modData, 1, modSize, fd) != modSize) {
    printf("fread() failed!\n");
    return 1;
  }
  fclose(fd);

  pocketmod_context context = { 0 };
  pocketmod_events events = {
    .on_upload_sample = pm_event_upload_sample,
    .on_sample_set = pm_event_sample_set,
    .on_position_set = pm_event_position_set,
    .on_period_set = pm_event_period_set,
    .on_volume_set = pm_event_volume_set,
    .on_balance_set = pm_event_balance_set,
  };
  if (!pocketmod_init(&context, &events, modData, (int32_t)modSize, 44100)) {
    printf("error: '%s' is not a valid MOD file\n", args[1]);
    return -1;
  }

  while (context.loop_count <= 1) {
    pocketmod_tick(&context);
    const int32_t us = 1000000 / context.ticks_per_second;
    Sleep(us / 1000);
  }

  fpgaShutdown();
  return 0;
}
