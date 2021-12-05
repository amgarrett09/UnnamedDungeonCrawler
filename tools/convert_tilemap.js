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

const connections = {
	4: [
		"_-1-2-_",
		"_-_-3-0",
		"0-3-_-_",
		"1-_-_-2",
	],
	9: [
		"_-1-3-_",
		"_-2-4-0",
		"_-_-5-1",
		"0-4-6-_",
		"1-5-7-3",
		"2-_-8-4",
		"3-7-_-_",
		"4-8-_-6",
		"5-_-_-7",
	],
};

function get_starting_rows_columns(map, screen_width, screen_height, basis) {
	const source_start_column = (map % basis) * screen_width;
	const source_start_row = Math.floor(map / basis) * screen_height;

	return {
		source_start_column,
		source_start_row,
	}
}

const layers = data.layers;

const objects = layers[2].objects;

const map_height = layers[0].height;
const map_width = layers[0].width;

let num_tile_maps = 
	Math.floor(map_height / screen_height) 
		* Math.floor(map_width / screen_width);

let basis = Math.floor(Math.sqrt(num_tile_maps));

if (!connections[basis*basis]) {
	throw new Error("Unsupported tile map size");
}

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

			if (obj_data[index] === 0
			    && collision_prop
			    && collision_prop.value) {
				obj_data[index] = 1;
			}
		}
	}	
}

let out_string = [];

out_string.push(num_tile_maps.toString() + "\n");

for (let map_number = 0; map_number < num_tile_maps; map_number++) {
	out_string.push(map_number.toString() 
		+ "-" + connections[num_tile_maps][map_number] + "\n");

	const layer_1 = layers[0].data;
	const layer_2 = layers[1].data;
	const { source_start_row, source_start_column } =
		get_starting_rows_columns(map_number,
			                  screen_width,
			                  screen_height,
					  basis);

	for (let row = 0; row < screen_height; row++) {
		for (let col = 0; col < screen_width; col++) {
			const source_row = source_start_row + row;
			const source_col = source_start_column + col;

			out_string.push(
				layer_1[source_row * map_width + source_col]
					.toString()
			);
			out_string.push(",");
			out_string.push(
				layer_2[source_row * map_width + source_col]
					.toString()
			);
			out_string.push(",");
			out_string.push(
				obj_data[source_row * map_width + source_col]
					.toString()
			);
			out_string.push(",\n");
		}
	}
}

fs.writeFile(output, out_string.join(""), function(err) {
	if (err)
		console.log(err);
});


