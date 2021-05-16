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

typedef enum {
    NULLKEY,
    UPKEY,
    RIGHTKEY,
    DOWNKEY,
    LEFTKEY
} Key;

typedef enum {
    NULLDIR,
    UPDIR,
    RIGHTDIR,
    DOWNDIR,
    LEFTDIR
} Direction;

typedef enum {
    X_DIMENSION,
    Y_DIMENSION
} CoordDimension;

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
    char *sound_buffer;
    FILE *stream;
    int sound_buffer_size;
    bool sound_initialized;
    bool sound_playing;
} Sound;

typedef struct {
    void *perm_storage;
    size_t perm_storage_size;
    bool is_initialized;
} Memory;

typedef struct {
    PlayerState *player_state;    
    WorldState *world_state;
    TileMap *tile_maps;
} MemoryPartitions;

typedef struct {
    Key key_pressed;         
    Key key_released;
} Input;


void clear_image_buffer(i32 *image_buffer);
void render_rectangle(
    i32 *image_buffer, 
    i32 min_x, 
    i32 min_y,
    i32 max_x,
    i32 max_y,
    float red,
    float green,
    float blue
);
void render_tile_map(
    i32 *restrict image_buffer, 
    i32 *restrict tile_map,
    i32 x_offset,
    i32 y_offset
);

i32 convert_tile_to_pixel_coordinate(i32 tile_value, CoordDimension dimension);

void transition_screens(
    i32 *restrict image_buffer,
    PlayerState *restrict player_state,
    WorldState *restrict world_state
);

// This is defined by platform layer.
// We'll probably use a different solution later on.
i32 debug_platform_stream_audio(
    const char file_path[], 
    Sound *game_sound
);

