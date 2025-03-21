#include <SDL.h>
#undef main

#include <stdio.h>
#include <string.h>

#include "../pocketmod.h"


static void audio_callback(void *userdata, Uint8 *buffer, int bytes)
{
    int i = 0;
    while (i < bytes) {
        i += pocketmod_render(buffer + i, bytes - i);
    }
}

int main(int argc, char **argv)
{
    SDL_SetMainReady();

    const Uint32 allowed_changes = SDL_AUDIO_ALLOW_FREQUENCY_CHANGE;
    Uint32 start_time;
    SDL_AudioSpec format;
    SDL_AudioDeviceID device;
    char *slash;
    size_t mod_size;
    pocketmod_events events = { 0 };

    /* Print usage if no file was given */
    if (argc != 2) {
        printf("usage: %s <modfile>\n", argv[0]);
        return -1;
    }

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("error: SDL_Init() failed: %s\n", SDL_GetError());
        return -1;
    }

    /* Initialize the audio subsystem */
    format.freq     = 44100;
    format.format   = AUDIO_S16;
    format.channels = 2;
    format.samples  = 4096;
    format.callback = audio_callback;
    format.userdata = 0;
    device = SDL_OpenAudioDevice(NULL, 0, &format, &format, allowed_changes);
    if (!device) {
        printf("error: SDL_OpenAudioDevice() failed: %s\n", SDL_GetError());
        return -1;
    }

    FILE* mod_file = fopen(argv[1], "rb");
    if (!mod_file) {
      printf("error: can't open '%s' for reading\n", argv[1]);
      return -1;
    }
    fseek(mod_file, 0, SEEK_END);
    mod_size = ftell(mod_file);
    fseek(mod_file, 0, SEEK_SET);

    /* Initialize the renderer */
    if (!pocketmod_init(NULL, mod_file, (int32_t)mod_size, format.freq)) {
        printf("error: '%s' is not a valid MOD file\n", argv[1]);
        return -1;
    }

    /* Strip the directory part from the source file's path */
    while ((slash = strpbrk(argv[1], "/\\"))) {
        argv[1] = slash + 1;
    }

    /* Start playback */
    SDL_PauseAudioDevice(device, 0);
    start_time = SDL_GetTicks();
    for (;;) {

        /* Print some information during playback */
        int seconds = (SDL_GetTicks() - start_time) / 1000;
        printf("\rPlaying '%s' ", argv[1]);
        printf("[%d:%02d] ", seconds / 60, seconds % 60);
        printf("Press Ctrl + C to stop");
        fflush(stdout);
        SDL_Delay(500);
    }
    return 0;
}
