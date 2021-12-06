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

i32 util_bit_scan_forward_u(u32 number);
size_t util_get_free_storage_bytes(Memory *memory);
size_t get_next_aligned_offset(size_t start_offset, size_t min_to_add,
				      size_t alignment);

/*
 * Finds first set bit in an unsigned integer, starting from lowest bit.
 * Returns the index of the set bit.
 */
i32 util_bit_scan_forward_u(u32 number)
{
	/* TODO: Use intrinsic for this */
	bool found = false;
	i32 index  = 0;

	while (!found) {
		if (index > 31) {
			index = -1;
			break;
		} else if (number & (1 << index)) {
			found = true;
		} else {
			index++;
		}
	}

	return index;
}

size_t util_get_free_storage_bytes(Memory *memory)
{
	return memory->temp_storage_size - memory->temp_next_load_offset;
}

size_t util_get_next_aligned_offset(size_t start_offset, size_t min_to_add,
				      size_t alignment)
{
	size_t min_new_offset = start_offset + min_to_add;
	size_t remainder      = min_new_offset % alignment;

	if (remainder) {
		return min_new_offset + (alignment - remainder);
	} else {
		return min_new_offset;
	}
}

