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

size_t mem_get_free_storage_bytes(Memory *memory)
{
	return memory->temp_storage_size - memory->temp_next_load_offset;
}

void *mem_get_storage_load_location(Memory *memory)
{
	char *load_location =
		(char *)memory->temp_storage + memory->temp_next_load_offset;

	return (void *)load_location;
}

void mem_update_next_load_offset(Memory *memory, size_t min_to_add,
				 size_t alignment)
{
	memory->temp_next_load_offset = mem__get_next_aligned_offset(
		memory->temp_next_load_offset, min_to_add, alignment);
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
