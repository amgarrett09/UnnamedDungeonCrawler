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

static size_t mem__get_next_aligned_offset(size_t start_offset,
					   size_t min_to_add, size_t alignment);

void *mem_load_file_to_temp_storage(Memory *memory, const char file_path[],
				    size_t (*func)(const char[], void *,
						   size_t),
				    bool discard)
{
	char *load_location =
		(char *)memory->temp_storage + memory->temp_next_load_offset;
	ssize_t max_size = (ssize_t)memory->temp_storage_size -
		(ssize_t)memory->temp_next_load_offset;

	if (max_size <= 0)
		return NULL;

	size_t result = func(file_path, (void *)load_location, max_size);

	if (!result)
		return NULL;

	if (!discard) {
		memory->temp_next_load_offset = mem__get_next_aligned_offset(
			memory->temp_next_load_offset, result, 32);
	}

	return (void *)load_location;
}

static size_t mem__get_next_aligned_offset(size_t start_offset,
					   size_t min_to_add, size_t alignment)
{
	size_t min_new_offset = start_offset + min_to_add;
	size_t remainder      = min_new_offset % alignment;

	if (remainder) {
		return min_new_offset + (alignment - remainder);
	} else {
		return min_new_offset;
	}
}
