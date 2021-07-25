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

static i32 tm__get_tile_map_count(char *file_location, i32 *file_index);
static void tm__handle_tile_map_metadata(Memory *memory, TileMap **tile_map,
					 i32 state, i32 tile_map_index);
static void tm__read_tile_map_metadata(Memory *memory, TileMap **tile_map,
				       i32 *file_index, char *file_location);
static void tm__read_tile_map_tiles(TileMap *tile_map, i32 *file_index,
				    char *file_location);
static void tm__set_tile_map_value(i32 x, i32 y, i32 layer, i32 tile_number,
				   TileMap *tile_map);

size_t tm_load_tile_map(const char file_path[], Memory *memory)
{
	char *temp_location =
		(char *)memory->temp_storage + memory->temp_next_load_offset;

	size_t result =
		debug_platform_load_asset(file_path, (void *)temp_location);

	if (!result)
		return 0;

	i32 file_index    = 0;
	TileMap *tile_map = &memory->tile_maps[0];

	i32 tile_map_count = tm__get_tile_map_count(temp_location, &file_index);

	for (i32 i = 0; i < tile_map_count; i++) {
		tm__read_tile_map_metadata(memory, &tile_map, &file_index,
					   temp_location);
		tm__read_tile_map_tiles(tile_map, &file_index, temp_location);
	}

	return result;
}

static i32 tm__get_tile_map_count(char *file_location, i32 *file_index)
{
	i32 count = 0;

	for (i32 i = 0; i < 128; i++) {
		char c = file_location[*file_index];
		*file_index += 1;

		if (c == '\n' || c == 0) {
			break;
		} else if (c >= 48 && c <= 57) {
			count = count * 10 + (c - '0');
		} else {
			continue;
		}
	}

	if (count > MAX_TILE_MAPS)
		return 0;
	else
		return count;
}

static void tm__handle_tile_map_metadata(Memory *memory, TileMap **tile_map,
					 i32 state, i32 tile_map_index)
{
	switch (state) {
	case 0:
		*tile_map = &memory->tile_maps[tile_map_index];
		break;
	case 1: {
		TileMap *map        = *tile_map;
		map->top_connection = &memory->tile_maps[tile_map_index];
		break;
	}
	case 2: {
		TileMap *map          = *tile_map;
		map->right_connection = &memory->tile_maps[tile_map_index];
		break;
	}
	case 3: {
		TileMap *map           = *tile_map;
		map->bottom_connection = &memory->tile_maps[tile_map_index];
		break;
	}
	case 4: {
		TileMap *map         = *tile_map;
		map->left_connection = &memory->tile_maps[tile_map_index];
		break;
	}
	}
}

static void tm__read_tile_map_metadata(Memory *memory, TileMap **tile_map,
				       i32 *file_index, char *file_location)
{
	i32 state          = 0;
	i32 tile_map_index = 0;
	for (i32 i = 0; i < 128; i++) {
		char c = file_location[*file_index];
		*file_index += 1;

		if (c == '\n' || c == 0) {
			break;
		} else if (c == '-') {
			if (tile_map_index >= 0) {
				tm__handle_tile_map_metadata(memory, tile_map,
							     state,
							     tile_map_index);
			}
			state += 1;
		} else if (c >= 48 && c <= 57) {
			if (tile_map_index < 0)
				tile_map_index = 0;

			tile_map_index = tile_map_index * 10 + (c - '0');
		} else {
			tile_map_index = -1;
			continue;
		}
	}
}

static void tm__read_tile_map_tiles(TileMap *tile_map, i32 *file_index,
				    char *file_location)
{
	for (i32 y = 0; y < SCREEN_HEIGHT_TILES; y++) {
		for (i32 x = 0; x < SCREEN_WIDTH_TILES; x++) {
			i32 tile_number = 0;
			i32 layer       = 0;
			for (i32 j = 0; j < 128; j++) {
				char c = file_location[*file_index];
				*file_index += 1;

				if (c == '\n' || c == 0) {
					break;
				} else if (c == ',') {
					if (tile_number > 0) {
						tm__set_tile_map_value(
							x, y, layer,
							tile_number, tile_map);
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
}

static void tm__set_tile_map_value(i32 x, i32 y, i32 layer, i32 tile_number,
				   TileMap *tile_map)
{
	switch (layer) {
	case 0:
		tile_map->background_map[y][x] = tile_number;
		break;
	case 1: {

		i32 len = tile_map->foreground_tiles_length;
		tile_map->foreground_tiles[len] = ((x & 0xFF) << 24) |
			((y & 0xFF) << 16) | (tile_number & 0xFFFF);
		tile_map->foreground_tiles_length += 1;
		break;
	}
	case 2:
		tile_map->collision_map[y][x] = !!tile_number;
		break;
	default:
		break;
	}
}
