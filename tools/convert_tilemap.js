const input = process.argv[2];
const output = process.argv[3];

if (!input || !output) {
	console.log(
		"USAGE: node convert_tilemap.js [input json] [output file]"
	);

	process.exit(1);
}

const data = require(input);
const fs = require('fs');

const screen_height = 20;
const screen_width = 40;

/* 
 * Tile maps in the game engine get split into 40 tile by 20 tile chunks
 * (the size of one screen), along with references to other chunks connected
 * to it. We need to encode the connections in the map file, with the format:
 * top-right-bottom-left.
 *
 * basis: the square root of the number of screens in the map
 */
function build_connections(basis) {
	if (basis === 1)
		return ["_-_-_-_"];

	let top = 0;
	let right = 1;
	let bottom = basis;
	let left = -1;

	let out = [];
	for (let i = 0; i < basis*basis; i++) {
		let connections = "";

		if (i < basis) {
			connections += "_-";
		} else {
			connections += `${top}-`
			top += 1;
		}

		if ((i + 1) % basis === 0) {
			connections += "_-";
			right += 1;
		} else {
			connections += `${right}-`;
			right += 1;
		}

		if (i >= (basis*basis - basis)) {
			connections += "_-";
		} else {
			connections += `${bottom}-`;
			bottom += 1;
		}

		if (i % basis === 0) {
			connections += "_";
			left += 1;
		} else {
			connections += `${left}`;
			left += 1;
		}

		out.push(connections);
	}

	return out;
}


function get_starting_rows_columns(map, screen_width, screen_height, basis) {
	const source_start_column = (map % basis) * screen_width;
	const source_start_row = Math.floor(map / basis) * screen_height;

	return {
		source_start_column,
		source_start_row,
	}
}

const layers = data.layers;

const layer_1 = layers[0] ? layers[0].data : [];
const layer_2 = layers[1] ? layers[1].data : [];
const objects = layers[2] ? layers[2].objects : [];

const map_height = layers[0].height;
const map_width = layers[0].width;

let num_tile_maps = 
	Math.floor(map_height / screen_height) 
		* Math.floor(map_width / screen_width);

let basis = Math.floor(Math.sqrt(num_tile_maps));


if (basis*basis !== num_tile_maps) {
	throw new Error("Unsupported tile map size");
}

const connections = build_connections(basis);

let obj_data = new Array(map_height*map_width).fill(0);

for (let i = 0; i < objects.length; i++) {		
	const obj = objects[i];

	const tile_x = Math.floor(obj.x / 32);
	const tile_y = Math.floor(obj.y / 32);
	const width = Math.ceil(obj.width / 32);
	const height = Math.ceil(obj.height / 32);

	for (let row = tile_y; row < tile_y + height; row++) {
		for (let col = tile_x; col < tile_x + width; col++) {
			const index = row * map_width + col;

			const collision_prop = obj.properties.find(x => {
				return x.name === 'collision';
			});
			const warp_x = obj.properties.find(x => {
				return x.name === 'warp_x';
			});
			const warp_y = obj.properties.find(x => {
				return x.name === 'warp_y';
			});

			if (obj_data[index] === 0) {
				let props = 0;
				if (collision_prop && collision_prop.value)
					props = props | 1;
				if (warp_x && warp_y) {
					const abs_x = Math.floor(
						warp_x.value / 32
					);
					const abs_y = Math.floor(
						warp_y.value / 32
					);
					const rel_x = abs_x
						% screen_width;
					const rel_y = abs_y
						% screen_height;
					const map_row = Math.floor(
						abs_y / screen_height
					);
					const map_col = Math.floor(
						abs_x / screen_width
					);

					const map_number =
						map_row * basis + map_col;


					props = props | (map_number << 24);
					props = props | (rel_x << 16);
					props = props | (rel_y << 8);
					props = props | 2;
				}

				obj_data[index] = props;
			}
		}
	}	
}

let out_string = [];

out_string.push(num_tile_maps.toString() + "\n");

for (let map_number = 0; map_number < num_tile_maps; map_number++) {
	out_string.push(map_number.toString() 
		+ "-" + connections[map_number] + "\n");

	const { source_start_row, source_start_column } =
		get_starting_rows_columns(map_number,
			                  screen_width,
			                  screen_height,
					  basis);

	for (let row = 0; row < screen_height; row++) {
		for (let col = 0; col < screen_width; col++) {
			const source_row = source_start_row + row;
			const source_col = source_start_column + col;
			const layer_1_val =  
				layer_1[source_row * map_width + source_col];
			const layer_2_val = 
				layer_2[source_row * map_width + source_col];
			const obj_val = 
				obj_data[source_row * map_width + source_col];
			out_string.push(
				layer_1_val ? layer_1_val.toString() : "0"
			);
			out_string.push(",");
			out_string.push(
				layer_2_val ? layer_2_val.toString() : "0"
			);
			out_string.push(",");
			out_string.push(
				obj_val ? obj_val.toString() : "0"
			);
			out_string.push(",\n");
		}
	}
}

fs.writeFile(output, out_string.join(""), function(err) {
	if (err)
		console.log(err);
});


