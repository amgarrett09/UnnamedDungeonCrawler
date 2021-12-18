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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <SDL2/SDL.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#include "game.h"
#include "util.c"
#include "hashmap.c"
#include "ai.c"
#include "memory.c"
#include "tile_map.c"
#include "game.c"

typedef struct StorageState {
	void *temp_storage;
	size_t temp_storage_size;
	i32 err;
} StorageState;

static const i32 image_buffer_size  = WIN_WIDTH * WIN_HEIGHT * 4;
static const i32 image_buffer_pitch = WIN_WIDTH * 4;
static const i32 target_sound_buffer_size =
	(SAMPLES_PER_SECOND / 60) * BYTES_PER_SAMPLE * 5;

static void handle_key_press(SDL_Keycode code, Input *input);
static void handle_key_release(SDL_Keycode code, Input *input);
static void handle_window_event(SDL_Event *event);
static StorageState allocate_temp_storage();

int main()
{
	SDL_Window *window     = NULL;
	SDL_Renderer *renderer = NULL;
	SDL_Texture *texture   = NULL;
	int ret                = 0;

	static Memory game_memory       = {0};
	static ScreenState screen_state = {{0}, NULL, 0};
	i32 dt                          = 16;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
		SDL_Log("Failed to init SDL: %s", SDL_GetError());
		ret = 1;
		goto cleanup;
	}

	window = SDL_CreateWindow("Unnamed Dungeon Crawler", 10, 10, WIN_WIDTH,
				  WIN_HEIGHT, SDL_WINDOW_RESIZABLE);

	if (!window) {
		SDL_Log("Failed to create window: %s", SDL_GetError());
		ret = 1;
		goto cleanup;
	}

	renderer = SDL_CreateRenderer(window, -1, 0);

	if (!renderer) {
		SDL_Log("Failed to create renderer: %s", SDL_GetError());
		ret = 1;
		goto cleanup;
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
				    SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH,
				    WIN_HEIGHT);

	if (!texture) {
		SDL_Log("Failed to create screen texture: %s", SDL_GetError());
		ret = 1;
		goto cleanup;
	}

	screen_state.image_buffer = (u32 *)malloc(image_buffer_size);

	if (!screen_state.image_buffer) {
		SDL_Log("%s", "Failed to allocate image buffer");
		ret = 1;
		goto cleanup;
	}

	StorageState storage = allocate_temp_storage();

	if (storage.err < 0) {
		ret = 1;
		goto cleanup;
	}

	game_memory.temp_storage          = storage.temp_storage;
	game_memory.temp_storage_size     = storage.temp_storage_size;
	game_memory.temp_next_load_offset = 0;
	game_memory.is_initialized        = false;

	Sound sound             = {0};
	sound.sound_buffer      = malloc(target_sound_buffer_size);
	sound.sound_buffer_size = target_sound_buffer_size;

	if (!sound.sound_buffer) {
		SDL_Log("%s", "Failed to allocate sound buffer");
		ret = 1;
		goto cleanup;
	}

	SDL_AudioSpec audio_settings = {.freq     = SAMPLES_PER_SECOND,
					.format   = AUDIO_S16SYS,
					.channels = 2,
					.samples  = 1024};

	SDL_OpenAudio(&audio_settings, 0);

	if (audio_settings.format != AUDIO_S16SYS) {
		SDL_Log("%s", "Got wrong audio format");
		ret = 1;
		goto cleanup;
	}

	Input input       = {0};
	FileStream stream = {0};

	/* Setup timespecs to enforce a set framerate in main loop */
	i64 frametime = 16666667;
	struct timespec start, end, sleeptime;
	i64 delta;

	/* MAIN LOOP */
	bool should_quit = false;
	game_initialize_memory(&game_memory, &screen_state, dt);
	clock_gettime(CLOCK_REALTIME, &start);
	SDL_PauseAudio(0);
	sound.playing = true;

	while (!should_quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {

			switch (event.type) {
			case SDL_QUIT: {
				should_quit = true;
				break;
			}
			case SDL_WINDOWEVENT:
				handle_window_event(&event);
				break;
			case SDL_KEYDOWN: {
				handle_key_press(event.key.keysym.sym, &input);
				break;
			}
			case SDL_KEYUP: {
				handle_key_release(event.key.keysym.sym,
						   &input);
				break;
			}
			default:
				break;
			}
		}

		/* Test sound */
		if (sound.playing) {
			i32 rc = debug_platform_stream_audio(
				"resources/test.wav", &stream,
				sound.sound_buffer, sound.sound_buffer_size);
			if (rc == 0)
				sound.playing = false;
		}

		game_update_and_render(&game_memory, &input, &screen_state);

		SDL_UpdateTexture(texture, 0, (void *)screen_state.image_buffer,
				  image_buffer_pitch);
		SDL_RenderCopy(renderer, texture, 0, 0);
		SDL_RenderPresent(renderer);

		clock_gettime(CLOCK_REALTIME, &end);

		/* Sleep until we reach frametime */
		delta = ((i64)end.tv_sec - (i64)start.tv_sec) *
				(i64)1000000000 +
			((i64)end.tv_nsec - (i64)start.tv_nsec);
		sleeptime.tv_sec  = 0;
		sleeptime.tv_nsec = frametime - delta;
		printf("delta: %li\n", delta);

		nanosleep(&sleeptime, NULL);
		clock_gettime(CLOCK_REALTIME, &start);
	}

cleanup:
	if (texture) {
		SDL_DestroyTexture(texture);
	}
	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}

	if (window) {
		SDL_DestroyWindow(window);
	}

	if (screen_state.image_buffer) {
		free(screen_state.image_buffer);
	}

	if (sound.sound_buffer) {
		free(sound.sound_buffer);
	}

	if (storage.temp_storage) {
		free(storage.temp_storage);
	}

	if (stream.fd) {
		fclose(stream.fd);
	}

	SDL_CloseAudio();
	SDL_Quit();

	return ret;
}

i32 debug_platform_stream_audio(const char file_path[], FileStream *stream,
				void *sound_buffer, i32 sound_buffer_size)
{
	if (!stream->fd) {
		stream->fd = fopen(file_path, "r");
		printf("%s\n", "Opening file");
		if (stream->fd == NULL) {
			return -1;
		}
	}

	u32 queued_size    = SDL_GetQueuedAudioSize(1);
	i32 target_size    = sound_buffer_size;
	i32 bytes_to_write = target_size - (i32)queued_size;

	if (bytes_to_write > 0) {
		size_t result =
			fread(sound_buffer, 1, bytes_to_write, stream->fd);

		if ((i32)result != bytes_to_write) {
			fclose(stream->fd);
			stream->fd = NULL;
			return 0;
		}

		SDL_QueueAudio(1, sound_buffer, bytes_to_write);

		return (i32)result;
	}

	return -1;
}

size_t debug_platform_load_asset(const char file_path[], void *memory_location,
				 size_t max_size)
{
	FILE *file = fopen(file_path, "rb");

	if (file == NULL) {
		fprintf(stderr, "Failed to open asset.\n");
		return 0;
	}

	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);

	if (file_size > max_size) {
		return 0;
	}

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

static void handle_window_event(SDL_Event *event)
{
	switch (event->window.event) {
	case SDL_WINDOWEVENT_EXPOSED:
		break;
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		break;
	default:
		break;
	}
}

static StorageState allocate_temp_storage()
{
	StorageState storage        = {0};
	i32 temp_storage_size_bytes = 5 * 1024 * 1024;
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

static void handle_key_press(SDL_Keycode code, Input *input)
{
	switch (code) {
	case SDLK_UP:
		input->keys |= KEYMASK_UP;
		break;
	case SDLK_RIGHT:
		input->keys |= KEYMASK_RIGHT;
		break;
	case SDLK_DOWN:
		input->keys |= KEYMASK_DOWN;
		break;
	case SDLK_LEFT:
		input->keys |= KEYMASK_LEFT;
		break;
	default:
		break;
	}
}

static void handle_key_release(SDL_Keycode code, Input *input)
{
	switch (code) {
	case SDLK_UP:
		input->keys &= ~KEYMASK_UP;
		break;
	case SDLK_RIGHT:
		input->keys &= ~KEYMASK_RIGHT;
		break;
	case SDLK_DOWN:
		input->keys &= ~KEYMASK_DOWN;
		break;
	case SDLK_LEFT:
		input->keys &= ~KEYMASK_LEFT;
		break;
	default:
		break;
	}
}
