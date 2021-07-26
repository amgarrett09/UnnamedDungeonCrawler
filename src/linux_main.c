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

#include <assert.h>
#include <time.h>

#include <X11/Intrinsic.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include <alsa/asoundlib.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "tile_map.c"
#include "game.c"

typedef struct WindowState {
	Display *display;
	Visual *visual;
	Window window;
	GC gc;
	XEvent event;
	XImage *ximage;
	char *image_buffer;
	i32 err;
} WindowState;

typedef struct AlsaState {
	Sound game_sound;
	snd_pcm_t *sound_handle;
	snd_pcm_uframes_t frames;
	i32 err;
} AlsaState;

typedef struct StorageState {
	void *temp_storage;
	size_t temp_storage_size;
	i32 err;
} StorageState;

static void handle_key_press(XKeyEvent *xkey, Input *input);
static void handle_key_release(XKeyEvent *xkey, Input *input);
static WindowState init_window(void);
static AlsaState init_sound(void);
static StorageState allocate_temp_storage(void);

int main()
{
	static Memory game_memory       = {};
	static ScreenState screen_state = {};

	int exit_code = 0;

	WindowState window_state = init_window();
	if (window_state.err < 0) {
		exit_code = 1;
		goto cleanup;
	}

	screen_state.image_buffer = (i32 *)window_state.image_buffer;

	AlsaState alsa = init_sound();
	if (alsa.err < 0) {
		exit_code = 1;
		goto cleanup;
	}

	Sound game_sound = alsa.game_sound;

	StorageState storage = allocate_temp_storage();

	if (storage.err < 0) {
		exit_code = 1;
		goto cleanup;
	}

	game_memory.temp_storage          = storage.temp_storage;
	game_memory.temp_storage_size     = storage.temp_storage_size;
	game_memory.temp_next_load_offset = 0;
	game_memory.is_initialized        = false;

	Input input = {};

	/* Setup timespecs to enforce a set framerate in main loop */
	i64 frametime = 16666667;
	struct timespec start, end, sleeptime;
	i64 delta;

	/*
	 * Approx. target time in ms between frames, used for calculations.
	 * Will be actual time between frames when we have variable framerates.
	 */
	i32 dt = 16;

	game_initialize_memory(&game_memory, &screen_state, dt);

	game_sound.sound_playing = true;

	/*
	 * MAIN LOOP
	 */
	bool should_close_window = false;
	clock_gettime(CLOCK_REALTIME, &start);

	while (!should_close_window) {
		while (XPending(window_state.display)) {
			XNextEvent(window_state.display, &window_state.event);

			switch (window_state.event.type) {
			case ClientMessage:
				should_close_window = true;
				break;
			case KeyPress: {
				handle_key_press(&window_state.event.xkey,
						 &input);
				break;
			}
			case KeyRelease: {
				handle_key_release(&window_state.event.xkey,
						   &input);
				break;
			}
			case Expose:
				XPutImage(window_state.display,
					  window_state.window, window_state.gc,
					  window_state.ximage, 0, 0, 0, 0,
					  WIN_WIDTH, WIN_HEIGHT);
				break;
			}
		}

		game_update_and_render(&game_memory, &input, &game_sound,
				       &screen_state);

		if (game_sound.sound_playing && game_sound.sound_initialized) {
			i32 err = snd_pcm_writei(alsa.sound_handle,
						 game_sound.sound_buffer,
						 alsa.frames);
			if (err < 0) {
				printf("%s\n", snd_strerror(err));
				snd_pcm_prepare(alsa.sound_handle);
			}
		}

		XPutImage(window_state.display, window_state.window,
			  window_state.gc, window_state.ximage, 0, 0, 0, 0,
			  WIN_WIDTH, WIN_HEIGHT);

		clock_gettime(CLOCK_REALTIME, &end);

		/* Sleep until we reach frametime */
		delta = ((i64)end.tv_sec - (i64)start.tv_sec) *
				(i64)1000000000 +
			((i64)end.tv_nsec - (i64)start.tv_nsec);
		sleeptime.tv_sec  = 0;
		sleeptime.tv_nsec = frametime - delta;

		nanosleep(&sleeptime, NULL);
		clock_gettime(CLOCK_REALTIME, &start);
	}

cleanup:
	if (window_state.ximage) {
		XDestroyImage(window_state.ximage);
	}

	if (window_state.display) {
		XDestroyWindow(window_state.display, window_state.window);
		XCloseDisplay(window_state.display);
	}

	if (alsa.sound_handle) {
		snd_pcm_drain(alsa.sound_handle);
		snd_pcm_close(alsa.sound_handle);
	}

	if (game_sound.sound_buffer) {
		free(game_sound.sound_buffer);
	}
	if (game_sound.stream) {
		fclose(game_sound.stream);
	}

	if (game_memory.temp_storage) {
		free(game_memory.temp_storage);
	}

	return exit_code;
}

i32 debug_platform_stream_audio(const char file_path[], Sound *game_sound)
{
	if (!game_sound->stream) {
		game_sound->stream = fopen(file_path, "r");
		printf("Opening file\n");
		if (game_sound->stream == NULL) {
			return -1;
		}
	}

	size_t result =
		fread(game_sound->sound_buffer, 1,
		      game_sound->sound_buffer_size, game_sound->stream);

	if (result != game_sound->sound_buffer_size) {
		fclose(game_sound->stream);
		game_sound->stream = NULL;
		return 0;
	}

	return (i32)result;
}

// TODO: maybe pass an expected size parameter and fail if file is larger
size_t debug_platform_load_asset(const char file_path[], void *memory_location)
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

static StorageState allocate_temp_storage()
{
	StorageState storage        = {};
	i32 temp_storage_size_bytes = 40 * 1024 * 1024;
	assert(temp_storage_size_bytes % 64 == 0);
	storage.temp_storage = aligned_alloc(64, temp_storage_size_bytes);
	if (!storage.temp_storage) {
		fprintf(stderr, "Failed to temp storage for game\n");
		storage.err = -1;

		return storage;
	}

	storage.temp_storage_size = temp_storage_size_bytes;
	memset(storage.temp_storage, 0, storage.temp_storage_size);

	return storage;
}

static void handle_key_press(XKeyEvent *xkey, Input *input)
{
	switch (xkey->keycode) {
	case 111:
		input->keys |= KEYMASK_UP;
		break;
	case 114:
		input->keys |= KEYMASK_RIGHT;
		break;
	case 116:
		input->keys |= KEYMASK_DOWN;
		break;
	case 113:
		input->keys |= KEYMASK_LEFT;
		break;
	default:
		break;
	}
}

static void handle_key_release(XKeyEvent *xkey, Input *input)
{
	switch (xkey->keycode) {
	case 111:
		input->keys &= ~KEYMASK_UP;
		break;
	case 114:
		input->keys &= ~KEYMASK_RIGHT;
		break;
	case 116:
		input->keys &= ~KEYMASK_DOWN;
		break;
	case 113:
		input->keys &= ~KEYMASK_LEFT;
		break;
	default:
		break;
	}
}

static AlsaState init_sound(void)
{
	AlsaState alsa              = {};
	Sound game_sound            = {};
	snd_pcm_hw_params_t *params = NULL;
	snd_pcm_t *handle           = NULL;
	snd_pcm_uframes_t frames    = 0;

	i32 rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		fprintf(stderr, "failed to open PCM device: %s\n",
			snd_strerror(rc));
		alsa.err = -1;

		return alsa;
	}

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(handle, params);

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params,
				     SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Stereo PCM 16 bit little-endian */
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

	snd_pcm_hw_params_set_channels(handle, params, 2);

	i32 dir         = 0;
	u32 sample_rate = 44100;
	snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, &dir);

	/*
	 * Amount of frames in 16.67 ms,
	 * plus a little extra for some leeway
	 */
	frames = 742;

	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr, "failed to set hw parameters: %s\n",
			snd_strerror(rc));
		alsa.err = -1;

		return alsa;
	}

	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	/*
	 * Size of buffer should be one period
	 * with two channels and two bytes per sample.
	 */
	game_sound.sound_buffer_size = frames * 2 * 2;
	game_sound.sound_buffer      = malloc(game_sound.sound_buffer_size);
	if (!game_sound.sound_buffer) {
		fprintf(stderr, "Failed to allocate sound buffer\n");
		alsa.err = -1;

		return alsa;
	}
	memset(game_sound.sound_buffer, 0, game_sound.sound_buffer_size);

	for (i32 i = 0; i < 10; i++) {
		rc = snd_pcm_writei(handle, game_sound.sound_buffer, frames);
		if (rc < 0) {
			printf("%s\n", snd_strerror(rc));
			snd_pcm_prepare(handle);
		}
	}

	game_sound.sound_initialized = true;
	alsa.game_sound              = game_sound;
	alsa.sound_handle            = handle;
	alsa.frames                  = frames;

	return alsa;
}

static WindowState init_window(void)
{
	WindowState window_state = {};

	window_state.display = XOpenDisplay(NULL);
	if (!window_state.display) {
		fprintf(stderr, "Cannot open display\n");
		window_state.err = -1;

		return window_state;
	}

	Display *display = window_state.display;

	u32 screen = DefaultScreen(display);

	u32 depth = DefaultDepth(display, screen);

	window_state.visual = DefaultVisual(display, screen);

	window_state.window = XCreateSimpleWindow(
		display, RootWindow(display, screen), WIN_X, WIN_Y, WIN_WIDTH,
		WIN_HEIGHT, WIN_BORDER, BlackPixel(display, screen),
		BlackPixel(display, screen));

	window_state.gc = DefaultGC(display, screen);

	Window window = window_state.window;

	Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(display, window, &delete_window, 1);

	XSelectInput(display, window,
		     ExposureMask | KeyPressMask | KeyReleaseMask);

	XMapWindow(display, window);

	i32 video_buffer_size = WIN_WIDTH * WIN_HEIGHT * 4;
	assert(video_buffer_size % 64 == 0);
	/* This will get freed by XDestroyImage */
	char *image_buffer_tmp = aligned_alloc(64, video_buffer_size);
	if (!image_buffer_tmp) {
		fprintf(stderr, "Failed to allocate image buffer\n");
		window_state.err = -1;

		return window_state;
	}

	memset(image_buffer_tmp, 0, video_buffer_size);
	window_state.image_buffer = image_buffer_tmp;

	/* TODO: Explore using Shm for this */
	window_state.ximage = XCreateImage(
		display, window_state.visual, depth, ZPixmap, 0,
		window_state.image_buffer, WIN_WIDTH, WIN_HEIGHT, 32, 0);

	XkbSetDetectableAutoRepeat(display, true, NULL);

	return window_state;
}
