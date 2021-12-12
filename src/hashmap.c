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

#define HASHMAP_SIZE 4096

typedef struct IntPair {
	u32 key;
	u64 value;
} IntPair;

/* TODO: Resize hash map when needed */
typedef struct IntHashMap {
	IntPair data[HASHMAP_SIZE];
	i32 filled_cells;
} IntHashMap;

static u32 hash_function(u32 input);
static bool keys_match_int(u32 key1, u32 key2);
static bool key_has_lower_hash_int(u32 key1, u32 key2);

i32 hash_insert_int(IntHashMap *int_hash_map, u32 key, u64 value)
{
	i32 rc = -1;
	u32 x  = 0;

	u32 key_hash = hash_function(key);

	while (x < HASHMAP_SIZE) {
		u32 index           = (key_hash + x) & (HASHMAP_SIZE - 1);
		IntPair stored_data = int_hash_map->data[index];

		if (!stored_data.key || keys_match_int(stored_data.key, key)) {
			IntPair data_to_store = {.key = key, .value = value};

			int_hash_map->data[index] = data_to_store;
			int_hash_map->filled_cells++;
			rc = 0;
			break;
		}

		x++;
	}

	return rc;
}

u64 hash_get_int(IntHashMap *int_hash_map, u32 key)
{
	u64 result = 0;
	u32 x      = 0;

	u32 key_hash = hash_function(key);

	while (x < HASHMAP_SIZE) {
		u32 index           = (key_hash + x) & (HASHMAP_SIZE - 1);
		IntPair stored_data = int_hash_map->data[index];

		if (!stored_data.key) {
			break;
		} else if (stored_data.key &&
			   keys_match_int(stored_data.key, key)) {
			result = stored_data.value;
			break;
		}

		x++;
	}

	return result;
}

void hash_delete_int(IntHashMap *int_hash_map, u32 key)
{
	u32 x = 0;

	u32 key_hash         = hash_function(key);
	u32 deleted_index    = 0;
	IntPair deleted_data = {};
	bool deleted         = false;

	while (x < HASHMAP_SIZE) {
		u32 index           = (key_hash + x) & (HASHMAP_SIZE - 1);
		IntPair stored_data = int_hash_map->data[index];

		if (!stored_data.key) {
			int_hash_map->filled_cells--;
			break;
		} else if (stored_data.key &&
			   keys_match_int(stored_data.key, key)) {
			int_hash_map->data[index].key   = 0;
			int_hash_map->data[index].value = 0;
			deleted_index                   = index;
			deleted_data                    = stored_data;
			deleted                         = true;
		} else if (deleted && stored_data.key &&
			   key_has_lower_hash_int(stored_data.key,
						  deleted_data.key)) {
			int_hash_map->data[deleted_index] = stored_data;
			int_hash_map->data[index].key     = 0;
			int_hash_map->data[index].value   = 0;
			deleted_index                     = index;
			deleted_data                      = stored_data;
		}

		x++;
	}
}

/*
 * Adapted from the Hash Function Prospector project:
 * https://github.com/skeeto/hash-prospector
 */
static u32 hash_function(u32 input)
{
	u32 out = input;
	out *= 0x43021123;
	out ^= out >> 15 ^ out >> 30;
	out *= 0x1d69e2a5;
	out ^= out >> 16;
	return out;
}

static bool keys_match_int(u32 key1, u32 key2) { return key1 == key2; }

static bool key_has_lower_hash_int(u32 key1, u32 key2)
{
	u32 data_hash = hash_function(key1) & (HASHMAP_SIZE - 1);
	u32 test_hash = hash_function(key2) & (HASHMAP_SIZE - 1);

	return data_hash <= test_hash;
}
