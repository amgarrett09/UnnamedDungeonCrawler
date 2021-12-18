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
 * Dependencies: game.h
 */

#define HASHMAP_INIT_SIZE 4096
#define ASTAR_MAP_LENGTH 256

typedef struct AStarPair {
	u32 key;
	AStarNode node;
} AStarPair;

typedef struct AStarHashMap {
	struct AStarPair data[ASTAR_MAP_LENGTH];
} AStarHashMap;

static u32 hash__hash_function(u32 input);
static bool hash__keys_match_int(u32 key1, u32 key2);
static bool hash__key_has_lower_hash_int(u32 key1, u32 key2, size_t length);
static void hash__realloc_int();

IntHashMap hash_create_hash_int(Memory *memory,
				void *(*alloc_func)(Memory *, size_t))
{
	IntHashMap map = {0};

	map.data       = (IntPair *)alloc_func(memory,
                                         HASHMAP_INIT_SIZE * sizeof(IntPair));
	map.memory     = memory;
	map.alloc_func = alloc_func;
	map.length     = HASHMAP_INIT_SIZE;

	return map;
}

i32 hash_insert_int(IntHashMap *int_hash_map, u32 key, u64 value)
{
	if (!int_hash_map->data)
		return -1;

	i32 rc        = -1;
	u32 x         = 0;
	size_t length = int_hash_map->length;

	u32 key_hash = hash__hash_function(key);

	while (x < length) {
		u32 index           = (key_hash + x) & (length - 1);
		IntPair stored_data = int_hash_map->data[index];

		if (!stored_data.key ||
		    hash__keys_match_int(stored_data.key, key)) {
			IntPair data_to_store = {.key = key, .value = value};

			int_hash_map->data[index] = data_to_store;
			int_hash_map->filled_cells++;
			rc = 0;
			break;
		}

		x++;
	}

	if (int_hash_map->filled_cells > int_hash_map->length / 2) {
		hash__realloc_int();
	}

	return rc;
}

u64 hash_get_int(IntHashMap *int_hash_map, u32 key)
{
	if (!int_hash_map->data)
		return 0;

	u64 result    = 0;
	u32 x         = 0;
	size_t length = int_hash_map->length;

	u32 key_hash = hash__hash_function(key);

	while (x < length) {
		u32 index           = (key_hash + x) & (length - 1);
		IntPair stored_data = int_hash_map->data[index];

		if (!stored_data.key) {
			break;
		} else if (stored_data.key &&
			   hash__keys_match_int(stored_data.key, key)) {
			result = stored_data.value;
			break;
		}

		x++;
	}

	return result;
}

void hash_delete_int(IntHashMap *int_hash_map, u32 key)
{
	if (!int_hash_map->data)
		return;

	u32 x         = 0;
	size_t length = int_hash_map->length;

	u32 key_hash         = hash__hash_function(key);
	u32 deleted_index    = 0;
	IntPair deleted_data = {0};
	bool deleted         = false;

	while (x < length) {
		u32 index           = (key_hash + x) & (length - 1);
		IntPair stored_data = int_hash_map->data[index];

		if (!stored_data.key) {
			if (x != 0) {
				int_hash_map->filled_cells--;
			}
			break;
		} else if (stored_data.key &&
			   hash__keys_match_int(stored_data.key, key)) {
			int_hash_map->data[index].key   = 0;
			int_hash_map->data[index].value = 0;
			deleted_index                   = index;
			deleted_data                    = stored_data;
			deleted                         = true;
		} else if (deleted && stored_data.key &&
			   hash__key_has_lower_hash_int(
				   stored_data.key, deleted_data.key, length)) {
			int_hash_map->data[deleted_index] = stored_data;
			int_hash_map->data[index].key     = 0;
			int_hash_map->data[index].value   = 0;
			deleted_index                     = index;
			deleted_data                      = stored_data;
		}

		x++;
	}
}

i32 hash_insert_astar(AStarHashMap *map, u32 key, AStarNode node)
{
	i32 rc     = -1;
	u32 x      = 0;
	u32 length = ASTAR_MAP_LENGTH;

	u32 key_hash = hash__hash_function(key);

	while (x < length) {
		u32 index             = (key_hash + x) & (length - 1);
		AStarPair stored_data = map->data[index];

		if (!stored_data.key ||
		    hash__keys_match_int(stored_data.key, key)) {
			AStarPair data_to_store = {.key = key, .node = node};

			map->data[index] = data_to_store;
			rc               = 0;
			break;
		}

		x++;
	}

	return rc;
}

AStarNode hash_get_astar(AStarHashMap *map, u32 key)
{
	AStarNode result = {.position = {-1, -1}};
	u32 x            = 0;
	u32 length       = ASTAR_MAP_LENGTH;

	u32 key_hash = hash__hash_function(key);

	while (x < length) {
		u32 index             = (key_hash + x) & (length - 1);
		AStarPair stored_data = map->data[index];

		if (!stored_data.key) {
			break;
		} else if (stored_data.key &&
			   hash__keys_match_int(stored_data.key, key)) {
			result = stored_data.node;
			break;
		}

		x++;
	}

	return result;
}

void hash_delete_astar(AStarHashMap *map, u32 key)
{
	u32 x         = 0;
	size_t length = ASTAR_MAP_LENGTH;

	u32 key_hash           = hash__hash_function(key);
	u32 deleted_index      = 0;
	AStarPair deleted_data = {0};
	bool deleted           = false;

	while (x < length) {
		u32 index             = (key_hash + x) & (length - 1);
		AStarPair stored_data = map->data[index];

		if (!stored_data.key) {
			break;
		} else if (stored_data.key &&
			   hash__keys_match_int(stored_data.key, key)) {
			map->data[index].key = 0;
			deleted_index        = index;
			deleted_data         = stored_data;
			deleted              = true;
		} else if (deleted && stored_data.key &&
			   hash__key_has_lower_hash_int(
				   stored_data.key, deleted_data.key, length)) {
			map->data[deleted_index] = stored_data;
			map->data[index].key     = 0;
			deleted_index            = index;
			deleted_data             = stored_data;
		}

		x++;
	}
}

/*
 * Adapted from the Hash Function Prospector project:
 * https://github.com/skeeto/hash-prospector
 */
static u32 hash__hash_function(u32 input)
{
	u32 out = input;
	out *= 0x43021123;
	out ^= out >> 15 ^ out >> 30;
	out *= 0x1d69e2a5;
	out ^= out >> 16;
	return out;
}

static bool hash__keys_match_int(u32 key1, u32 key2) { return key1 == key2; }

static bool hash__key_has_lower_hash_int(u32 key1, u32 key2, size_t length)
{
	u32 data_hash = hash__hash_function(key1) & (length - 1);
	u32 test_hash = hash__hash_function(key2) & (length - 1);

	return data_hash <= test_hash;
}

/* TODO: implement reallocation and rehashing of data */
static void hash__realloc_int()
{
	printf("Hash map realloc not implemented yet!\n");
	return;
}
