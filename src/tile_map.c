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

/*
 * Dependendencies: game.h
 */

typedef struct TileMapParseState {
	MapSegment *map_segment;
	i32 file_index;
	i32 map_segment_count;
} TileMapParseState;

static TileMapParseState tm__get_map_segment_count(TileMapParseState parse_state,
						char *file_location);
static TileMapParseState
tm__handle_map_segment_metadata(TileMapParseState parse_state, Memory *memory,
			     i32 state, i32 map_segment_index);
static TileMapParseState
tm__read_map_segment_metadata(TileMapParseState parse_state, Memory *memory,
			   char *file_location);
static TileMapParseState tm__read_map_segment_tiles(TileMapParseState parse_state,
						 char *file_location);
static void tm__set_map_segment_value(i32 x, i32 y, i32 layer, i32 tile_number,
				   MapSegment *map_segment);

bool tm_load_tile_map(const char file_path[], Memory *memory)
{
	void *temp_location = mem_load_file_to_temp_storage(
		memory, file_path, &debug_platform_load_asset, true);

	if (!temp_location)
		return false;

	TileMapParseState parse_state = {
		.map_segment       = &memory->map_segments[0],
		.file_index     = 0,
		.map_segment_count = 0,
	};

	parse_state = tm__get_map_segment_count(parse_state, temp_location);

	for (i32 i = 0; i < parse_state.map_segment_count; i++) {
		parse_state = tm__read_map_segment_metadata(parse_state, memory,
							 temp_location);
		parse_state =
			tm__read_map_segment_tiles(parse_state, temp_location);
	}

	return true;
}

static TileMapParseState tm__get_map_segment_count(TileMapParseState parse_state,
						char *file_location)
{
	i32 count = 0;

	for (i32 i = 0; i < 128; i++) {
		char c = file_location[parse_state.file_index];
		parse_state.file_index += 1;

		if (c == '\n' || c == 0) {
			break;
		} else if (c >= 48 && c <= 57) {
			count = count * 10 + (c - '0');
		} else {
			continue;
		}
	}

	if (count <= MAX_MAP_SEGMENTS) {
		parse_state.map_segment_count = count;
	}

	return parse_state;
}

static TileMapParseState
tm__handle_map_segment_metadata(TileMapParseState parse_state, Memory *memory,
			     i32 state, i32 map_segment_index)
{
	if (map_segment_index >= MAX_MAP_SEGMENTS) {
		return parse_state;
	}

	switch (state) {
	case 0:
		parse_state.map_segment = &memory->map_segments[map_segment_index];
		break;
	case 1:
		parse_state.map_segment->top_connection =
			&memory->map_segments[map_segment_index];
		break;
	case 2:
		parse_state.map_segment->right_connection =
			&memory->map_segments[map_segment_index];
		break;
	case 3:
		parse_state.map_segment->bottom_connection =
			&memory->map_segments[map_segment_index];
		break;
	case 4:
		parse_state.map_segment->left_connection =
			&memory->map_segments[map_segment_index];
		break;
	}

	return parse_state;
}

static TileMapParseState
tm__read_map_segment_metadata(TileMapParseState parse_state, Memory *memory,
			   char *file_location)
{
	i32 state          = 0;
	i32 map_segment_index = 0;
	for (i32 i = 0; i < 128; i++) {
		char c = file_location[parse_state.file_index];
		parse_state.file_index += 1;

		if (c == '\n' || c == 0) {
			if (map_segment_index >= 0) {
				parse_state = tm__handle_map_segment_metadata(
					parse_state, memory, state,
					map_segment_index);
			}
			break;
		} else if (c == '-') {
			if (map_segment_index >= 0) {
				parse_state = tm__handle_map_segment_metadata(
					parse_state, memory, state,
					map_segment_index);
			}
			map_segment_index = -1;
			state += 1;
		} else if (c >= 48 && c <= 57) {
			if (map_segment_index < 0)
				map_segment_index = 0;

			map_segment_index = map_segment_index * 10 + (c - '0');
		} else {
			map_segment_index = -1;
			continue;
		}
	}

	return parse_state;
}

static TileMapParseState tm__read_map_segment_tiles(TileMapParseState parse_state,
						 char *file_location)
{
	for (i32 y = 0; y < SCREEN_HEIGHT_TILES; y++) {
		for (i32 x = 0; x < SCREEN_WIDTH_TILES; x++) {
			i32 tile_number = 0;
			i32 layer       = 0;
			for (i32 j = 0; j < 128; j++) {
				char c = file_location[parse_state.file_index];
				parse_state.file_index += 1;

				if (c == '\n' || c == 0) {
					break;
				} else if (c == ',') {
					if (tile_number > 0) {
						tm__set_map_segment_value(
							x, y, layer,
							tile_number,
							parse_state.map_segment);
					}
					tile_number = 0;
					layer += 1;
				} else if (c >= 48 && c <= 57) {
					tile_number =
						tile_number * 10 + (c - '0');
				} else {
					continue;
				}
			}
		}
	}

	return parse_state;
}

static void tm__set_map_segment_value(i32 x, i32 y, i32 layer, i32 tile_number,
				   MapSegment *map_segment)
{
	switch (layer) {
	case 0:
		map_segment->tiles[y][x] = (tile_number & 0xFFFF) << 16;
		break;
	case 1: {

		map_segment->tiles[y][x] =
			map_segment->tiles[y][x] | tile_number;
		break;
	}
	case 2:
		map_segment->tile_props[y][x] = tile_number;
		break;
	default:
		break;
	}
}
