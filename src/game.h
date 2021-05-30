#define WIN_X 10
#define WIN_Y 10
#define WIN_WIDTH 1280
#define WIN_HEIGHT 720
#define WIN_BORDER 1
#define TILE_WIDTH 40
#define TILE_HEIGHT 40
#define SCREEN_WIDTH_TILES 32
#define SCREEN_HEIGHT_TILES 18

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define UP_MASK 0x01
#define DOWN_MASK 0x02
#define LEFT_MASK 0x04
#define RIGHT_MASK 0x08

#pragma pack(push, 1)
typedef struct {
        u16 signature;
        u32 file_size;
        u16 reserved1;
        u16 reserved2;
        u32 image_offset;
        u32 info_header_size;
        i32 image_width;
        i32 image_height;
        u16 number_of_planes;
        u16 bits_per_pixel;
        u32 compression_type;
        u32 image_data_size;
        i32 horizontal_resolution;
        i32 vertical_resolution;
        u32 colors_used;
        u32 important_colors;
        u32 red_mask;
        u32 green_mask;
        u32 blue_mask;
        u32 alpha_mask;
} BMPHeader;
#pragma pack(pop)

typedef enum {
        X_DIMENSION,
        Y_DIMENSION
} CoordDimension;

typedef enum {
        NULLDIR,
        UPDIR,
        RIGHTDIR,
        DOWNDIR,
        LEFTDIR
} Direction;


typedef enum {
        NULLKEY,
        UPKEY,
        RIGHTKEY,
        DOWNKEY,
        LEFTKEY
} Key;

typedef struct {
        Key key_pressed;         
        Key key_released;
} Input;

typedef struct {
        i32 keys;
        i32 pixel_x;
        i32 pixel_y;
        i32 tile_x;
        i32 tile_y;
        i32 move_counter;
        Direction move_direction;
        i32 speed;
} PlayerState;

typedef struct TileMap {
        i32 data[SCREEN_HEIGHT_TILES][SCREEN_WIDTH_TILES];
        struct TileMap *top_connection;
        struct TileMap *right_connection;
        struct TileMap *bottom_connection;
        struct TileMap *left_connection;
} TileMap;

typedef struct {
        TileMap *current_tile_map;
        TileMap *next_tile_map;
        bool screen_transitioning;
        Direction transition_direction;
        i32 transition_counter;
} WorldState;

typedef struct {
        void *perm_storage;
        size_t perm_storage_size;
        void *temp_storage;
        size_t temp_storage_size;
        bool is_initialized;
} Memory;

typedef struct {
        PlayerState *player_state;    
        WorldState *world_state;
        TileMap *tile_maps;
} MemoryPartitions;

typedef struct {
        char *sound_buffer;
        FILE *stream;
        int sound_buffer_size;
        bool sound_initialized;
        bool sound_playing;
} Sound;


i32 bit_scan_forward_u(u32 number);
i32 convert_tile_to_pixel(i32 tile_value, CoordDimension dimension);
void display_bitmap(i32 *restrict image_buffer, BMPHeader *bmp);
void game_initialize_memory(Memory *memory, i32 dt);
void game_update_and_render(Memory *restrict memory, Input *restrict input, 
                       Sound *restrict game_sound, i32 *restrict image_buffer,
                       i32 dt);
i32 load_bitmap(const char file_path[], void *location);
void render_player(i32 *restrict image_buffer, 
                   PlayerState *restrict player_state);
void render_rectangle(i32 *image_buffer, i32 min_x, i32 min_y, i32 max_x,
                      i32 max_y, float red, float green, float blue);
void render_tile_map(i32 *restrict image_buffer, i32 *restrict tile_map,
                     i32 x_offset,i32 y_offset);
void transition_screens(i32 *restrict image_buffer,
                        PlayerState *restrict player_state,
                        WorldState *restrict world_state);


/* 
 * These are defined by platform layer.
 * We'll probably use a different solution later on. 
 */
i32 debug_platform_stream_audio(const char file_path[], Sound *game_sound);

i32 debug_platform_load_asset(const char file_path[], void *memory_location);

