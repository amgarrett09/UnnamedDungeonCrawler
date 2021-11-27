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

#include "game.h"
#include "tile_map.c"
#include "game.c"

typedef struct StorageState {
	void *temp_storage;
	size_t temp_storage_size;
	i32 err;
} StorageState;

static const i32 image_buffer_size  = WIN_WIDTH * WIN_HEIGHT * 4;
static const i32 image_buffer_pitch = WIN_WIDTH * 4;

static void handle_key_press(SDL_Keycode code, Input *input);
static void handle_key_release(SDL_Keycode code, Input *input);
static void handle_window_event(SDL_Event *event);
static void sdl_audio_callback(void *user_data, unsigned char *audio_buffer,
			       i32 length);
static StorageState allocate_temp_storage();

int main()
{
	SDL_Window *window     = NULL;
	SDL_Renderer *renderer = NULL;
	SDL_Texture *texture   = NULL;
	void *image_buffer     = NULL;
	int ret                = 0;

	static Memory game_memory       = {};
	static ScreenState screen_state = {};
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

	image_buffer = malloc(image_buffer_size);

	if (!image_buffer) {
		SDL_Log("%s", "Failed to allocate image buffer");
		ret = 1;
		goto cleanup;
	}

	screen_state.image_buffer = (i32 *)image_buffer;

	AudioRingBuffer audio_ring_buffer = {.size         = RING_BUFFER_SIZE,
					     .write_cursor = 0,
					     .play_cursor  = 0,
					     .data = malloc(RING_BUFFER_SIZE)};
	memset(audio_ring_buffer.data, 0, RING_BUFFER_SIZE);

	SDL_AudioSpec audio_settings = {.freq     = SAMPLES_PER_SECOND,
					.format   = AUDIO_S16SYS,
					.channels = 2,
					.samples  = RING_BUFFER_SIZE / 2,
					.callback = &sdl_audio_callback,
					.userdata = (void *)&audio_ring_buffer};

	SDL_OpenAudio(&audio_settings, 0);

	if (audio_settings.format != AUDIO_S16SYS) {
		SDL_Log("%s", "Got wrong audio format");
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

	Input input = {};

	/* Setup timespecs to enforce a set framerate in main loop */
	i64 frametime = 16666667;
	struct timespec start, end, sleeptime;
	i64 delta;

	/* MAIN LOOP */
	bool should_quit = false;
	game_initialize_memory(&game_memory, &screen_state, dt);
	clock_gettime(CLOCK_REALTIME, &start);
	SDL_PauseAudio(0);

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

		game_update_and_render(&game_memory, &input, &screen_state);

		SDL_UpdateTexture(texture, 0, image_buffer, image_buffer_pitch);
		SDL_RenderCopy(renderer, texture, 0, 0);
		SDL_RenderPresent(renderer);

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
	if (texture) {
		SDL_DestroyTexture(texture);
	}
	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}

	if (window) {
		SDL_DestroyWindow(window);
	}

	if (image_buffer) {
		free(image_buffer);
	}
	if (audio_ring_buffer.data) {
		free(audio_ring_buffer.data);
	}

	SDL_CloseAudio();
	SDL_Quit();

	return ret;
}

void lock_audio() {
	SDL_LockAudio();
}
void unlock_audio() {
	SDL_UnlockAudio();
}

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

static void sdl_audio_callback(void *user_data, unsigned char *audio_data,
			       i32 length)
{
	AudioRingBuffer *ring_buffer = (AudioRingBuffer *)user_data;

	i32 region_1_size = length;
	i32 region_2_size = 0;

	if (ring_buffer->play_cursor + length > ring_buffer->size) {
		region_1_size = ring_buffer->size - ring_buffer->play_cursor;
		region_2_size = length - region_1_size;
	}

	memcpy(audio_data,
	       (unsigned char *)ring_buffer->data + ring_buffer->play_cursor,
	       region_1_size);
	memcpy(&audio_data[region_1_size], ring_buffer->data, region_2_size);

	ring_buffer->play_cursor =
		(ring_buffer->play_cursor + length) % ring_buffer->size;
	ring_buffer->write_cursor =
		(ring_buffer->play_cursor + 2048) % ring_buffer->size;
}

