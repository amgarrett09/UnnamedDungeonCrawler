/* 
 * Copyright (C) 2021 Alex Garrett
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <time.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Intrinsic.h>

#include <alsa/asoundlib.h>

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "game.h"
#include "game.c"

void handle_key_press(XKeyEvent *restrict xkey, Input *restrict input);
void handle_key_release(XKeyEvent *restrict xkey, Input *restrict input);

int 
main(int argc, char *argv[]) 
{
        int exit_code = 0;

        /* Declarations for X11 */
        Display *display = NULL;
        Visual *visual = NULL;
        Window window = {0};
        XEvent event = {0};
        GC gc = {0};
        i32 screen = 0;
        i32 depth = 0;
        XImage *ximage = NULL;

        /* Declarations for ALSA */
        Sound game_sound = {0};
        game_sound.sound_initialized = false;
        game_sound.sound_playing = false;
        game_sound.sound_buffer = NULL;

        i32 err = 0;
        snd_pcm_hw_params_t *params = NULL;
        snd_pcm_t *handle = NULL;
        u32 sample_rate = 0;
        i32 dir = 0;
        snd_pcm_uframes_t frames = 0;

        /* X11 Initialization */
        display = XOpenDisplay(NULL);
        if (!display) {
                fprintf(stderr, "Cannot open display\n");
                exit_code = 1;
                goto cleanup;
        }

        screen = DefaultScreen(display);

        depth = DefaultDepth(display, screen);

        visual = DefaultVisual(display, screen);

        window = XCreateSimpleWindow(display, RootWindow(display, screen), 
                                     WIN_X, WIN_Y, WIN_WIDTH, WIN_HEIGHT, 
                                     WIN_BORDER, BlackPixel(display, screen),   
                                     BlackPixel(display, screen));

        gc = DefaultGC(display, screen);

        Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
        XSetWMProtocols(display, window, &delete_window, 1);

        XSelectInput(display, window, 
                     ExposureMask | KeyPressMask | KeyReleaseMask);

        XMapWindow(display, window);

        i32 video_buffer_size = WIN_WIDTH * WIN_HEIGHT * 4;
        assert(video_buffer_size % 64 == 0);
        /* This will get freed by XDestroyImage */
        char *image_buffer = (char *) aligned_alloc(64, video_buffer_size);
        if (!image_buffer) {
                fprintf(stderr, "Failed to allocate image buffer\n");
                exit_code = 1;
                goto cleanup;
        }
        memset(image_buffer, 0, video_buffer_size);

        /* TODO: Explore using Shm for this */
        ximage = XCreateImage(display, visual, depth, ZPixmap, 0, image_buffer, 
                              WIN_WIDTH, WIN_HEIGHT, 32, 0);

        XkbSetDetectableAutoRepeat(display, true, NULL);


        /* ALSA initialization */
        err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
        if (err < 0) {
                fprintf(stderr, "failed to open PCM device: %s\n", 
                        snd_strerror(err));
        } else {
                snd_pcm_hw_params_alloca(&params);
                snd_pcm_hw_params_any(handle, params);

                /* Interleaved mode */
                snd_pcm_hw_params_set_access(handle, params, 
                                             SND_PCM_ACCESS_RW_INTERLEAVED);

                /* Stereo PCM 16 bit little-endian */
                snd_pcm_hw_params_set_format(handle, params, 
                                             SND_PCM_FORMAT_S16_LE);

                snd_pcm_hw_params_set_channels(handle, params, 2);
                
                sample_rate = 44100;
                snd_pcm_hw_params_set_rate_near(handle, params, 
                                                &sample_rate, &dir);

                /* 
                 * Amount of frames in 16.67 ms, 
                 * plus a little extra for some leeway 
                 */
                frames = 742;

                snd_pcm_hw_params_set_period_size_near(handle, params, 
                                                       &frames, &dir);

                err = snd_pcm_hw_params(handle, params);
                if (err < 0) {
                        fprintf(stderr, "failed to set hw parameters: %s\n", 
                                snd_strerror(err));
                } else {
                        snd_pcm_hw_params_get_period_size(params, &frames, 
                                                          &dir);
                        /* 
                         * Size of buffer should be one period 
                         * with two channels and two bytes per sample.
                         */
                        game_sound.sound_buffer_size = frames * 2 * 2;
                        game_sound.sound_buffer = 
                            (char *) malloc(game_sound.sound_buffer_size);
                        if (!game_sound.sound_buffer) {
                                fprintf(stderr, 
                                        "Failed to allocate sound buffer\n");
                                exit_code = 1;
                                goto cleanup;
                        }
                        memset(game_sound.sound_buffer, 0, 
                               game_sound.sound_buffer_size);

                        game_sound.sound_initialized = true;
                }
        }

        /* Setup memory pools for game to use */
        Memory memory;

        i32 perm_storage_size_bytes = 4*1024;
        assert(perm_storage_size_bytes  % 64 == 0);
        memory.perm_storage = aligned_alloc(64, perm_storage_size_bytes);
        if (!memory.perm_storage) {
                fprintf(stderr, "Failed to allocate perm storage for game\n");
                exit_code = 1;
                goto cleanup;
        }
        memory.perm_storage_size = perm_storage_size_bytes;
        memset(memory.perm_storage, 0, memory.perm_storage_size);


        i32 temp_storage_size_bytes = 40*1024*1024;
        assert(temp_storage_size_bytes  % 64 == 0);
        memory.temp_storage = aligned_alloc(64, temp_storage_size_bytes);
        if (!memory.temp_storage) {
                fprintf(stderr, "Failed to temp storage for game\n");
                exit_code = 1;
                goto cleanup;
        }
        memory.temp_storage_size = temp_storage_size_bytes;
        memory.temp_next_load_offset = 0;
        memset(memory.temp_storage, 0, memory.temp_storage_size);

        memory.is_initialized = false;

        /* Setup input */
        Input input = { NULLKEY, NULLKEY };

        /* Setup timespecs to enforce a set framerate in main loop */
        i64 frametime = 16666667;
        struct timespec start, end, sleeptime;
        i64 delta;

        /* 
         * Approx. target time in ms between frames, used for calculations.
         * Will be actual time between frames when we have variable framerates.
         */
        i32 dt = 16;


        game_initialize_memory(&memory, dt);

        /* 
         * MAIN LOOP 
         */
        bool should_close_window = false;
        clock_gettime(CLOCK_REALTIME, &start);

        while (!should_close_window) {
                if (XPending(display)) {
                        XNextEvent(display, &event);

                        switch (event.type) {
                                case ClientMessage:
                                        should_close_window = true;
                                        break;
                                case KeyPress: {
                                        handle_key_press(&event.xkey, &input);
                                        break;
                                }
                                case KeyRelease: {
                                        handle_key_release(&event.xkey, &input);
                                        break;
                                }
                                case Expose:
                                        XPutImage(display, window, gc, ximage,
                                                  0, 0, 0, 0, WIN_WIDTH, 
                                                  WIN_HEIGHT);
                                        break;
                        }
                }

                game_update_and_render(&memory, &input, &game_sound, 
                                       (i32 *)image_buffer, dt);

                if (game_sound.sound_playing && game_sound.sound_initialized) {
                        err = snd_pcm_writei(handle, game_sound.sound_buffer, 
                                             frames);
                        if (err < 0) {
                                printf("%s\n", snd_strerror(err));
                                snd_pcm_prepare(handle);
                        }
                }

                XPutImage(display, window, gc, ximage, 0, 0, 0, 0, 
                          WIN_WIDTH, WIN_HEIGHT);

                clock_gettime(CLOCK_REALTIME, &end);

                /* Sleep until we reach frametime */
                delta = ((i64)end.tv_sec - (i64)start.tv_sec)
                    * (i64)1000000000 + ((i64)end.tv_nsec - (i64)start.tv_nsec);
                sleeptime.tv_sec = 0;
                sleeptime.tv_nsec = frametime - delta;

                nanosleep(&sleeptime, NULL);
                clock_gettime(CLOCK_REALTIME, &start);
        }

cleanup: 
        if (ximage) {
                XDestroyImage(ximage);
        }

        if (display) {
                XDestroyWindow(display, window); 
                XCloseDisplay(display); 
        }

        if (handle) {
                snd_pcm_drain(handle);
                snd_pcm_close(handle);
        }

        if (game_sound.sound_buffer) {
                free(game_sound.sound_buffer);
        }
        if (game_sound.stream) {
                fclose(game_sound.stream);
        }

        if (memory.perm_storage) {
            free(memory.perm_storage);
        }

        if (memory.temp_storage) {
            free(memory.temp_storage);
        }

        return exit_code;
}

i32 
debug_platform_stream_audio(const char file_path[], Sound *game_sound) 
{
        if (!game_sound->stream) {
                game_sound->stream = fopen(file_path, "r");
                printf("Opening file\n");
                if (game_sound->stream == NULL) {
                        return -1;
                }
        }

        size_t result = fread(game_sound->sound_buffer, 1, 
                              game_sound->sound_buffer_size, 
                              game_sound->stream);

        if (result != game_sound->sound_buffer_size) {
                fclose(game_sound->stream);
                game_sound->stream = NULL;
                return 0;
        }
        
        return (i32) result;
}

size_t 
debug_platform_load_asset(const char file_path[], void *memory_location) 
{
        FILE *file = fopen(file_path, "rb");

        if (file == NULL) {
                fprintf(stderr, "Failed to open asset.\n");
                return 0;
        }

        fseek(file, 0, SEEK_END);
        size_t file_size = ftell(file);
        rewind(file);

        size_t result = fread(memory_location, 1, file_size, file);

        if (result != file_size) {
                fclose(file);
                fprintf(stderr, "Error reading asset\n");
                return 0;
        }

        fclose(file);

        return result;
}

void
handle_key_press(XKeyEvent *restrict xkey, Input *restrict input)
{
        input->key_released = NULLKEY;
        switch(xkey->keycode) {
                case 111:
                        input->key_pressed = UPKEY;
                        break;
                case 114:
                        input->key_pressed = RIGHTKEY;
                        break;
                case 116:
                        input->key_pressed = DOWNKEY;
                        break;
                case 113:
                        input->key_pressed = LEFTKEY;
                        break;
                default:
                    break;
        }
}

void
handle_key_release(XKeyEvent *restrict xkey, Input *restrict input) 
{
        input->key_pressed = NULLKEY;
        switch(xkey->keycode) {
                case 111:
                        input->key_released = UPKEY;
                        break;
                case 114:
                        input->key_released = RIGHTKEY;
                        break;
                case 116:
                        input->key_released = DOWNKEY;
                        break;
                case 113:
                        input->key_released = LEFTKEY;
                        break;
                default:
                        break;
        }
}
