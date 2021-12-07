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
