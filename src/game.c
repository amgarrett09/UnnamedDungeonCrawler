#include "game.h"

static TileMap tile_map1 = {
    {
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1 },
        { 1,0,0,0,0,1,1,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1 },
        { 1,0,0,0,0,1,1,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,1,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
    },
    NULL,
    NULL,
    NULL,
    NULL
};

static TileMap tile_map2 = {
    {
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
    },
    NULL,
    NULL,
    NULL,
    NULL
};

static TileMap tile_map3 = {
    {
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1 },
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
    },
    NULL,
    NULL,
    NULL,
    NULL
};

static TileMap tile_map4 = {
    {
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
    },
    NULL,
    NULL,
    NULL,
    NULL
};

static TileMap tile_map5 = {
    {
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
    },
    NULL,
    NULL,
    NULL,
    NULL
};


void game_initialize_memory(Memory *memory, i32 dt) {
    PlayerState *player_state = (PlayerState *)memory->perm_storage;
    char * temp = (char *) player_state + 64;
    WorldState *world_state = (WorldState *) temp;

    if (!memory->is_initialized) {
        player_state->tile_x = 15;
        player_state->tile_y = 0;
        player_state->pixel_x = 
            convert_tile_to_pixel_coordinate(player_state->tile_x, X_DIMENSION);
        player_state->pixel_y = 
            convert_tile_to_pixel_coordinate(player_state->tile_y, Y_DIMENSION);
        player_state->move_counter = 0;
        player_state->move_direction = NULLDIR;
        player_state->speed = (4 * dt) / 16;

        tile_map1.top_connection = &tile_map2;
        tile_map1.right_connection = &tile_map3;
        tile_map1.bottom_connection = &tile_map4;
        tile_map1.left_connection = &tile_map5;

        tile_map2.bottom_connection = &tile_map1;
        tile_map3.left_connection = &tile_map1;
        tile_map4.top_connection = &tile_map1;
        tile_map5.right_connection = &tile_map1;

        world_state->current_tile_map = &tile_map1;
        world_state->next_tile_map = &tile_map1;
        world_state->screen_transitioning = false;
        world_state->transition_direction = NULLDIR;
        world_state->transition_counter = 0;

        memory->is_initialized = true;
    }
}

void game_update_and_render(
    Memory *restrict memory, 
    Input *restrict input,
    Sound *restrict game_sound,
    i32 *restrict image_buffer,
    i32 dt
) {
    PlayerState *player_state = (PlayerState *) memory->perm_storage;
    char *temp = (char *) player_state + 64;
    WorldState *world_state = (WorldState *) temp;
    
    if (world_state->screen_transitioning) {
        transition_screens(
            image_buffer,
            player_state,
            world_state
        );
        return;
    }

    // Read input
    switch (input->key_pressed) {
        case UPKEY:
            player_state->keys = player_state->keys | UP_MASK;
            break;
        case RIGHTKEY:
            player_state->keys = player_state->keys | RIGHT_MASK;
            break;
        case DOWNKEY:
            player_state->keys = player_state->keys | DOWN_MASK;
            break;
        case LEFTKEY:
            player_state->keys = player_state->keys | LEFT_MASK;
            break;
        default:
            break;
    }
    switch (input->key_released) {
        case UPKEY:
            player_state->keys = player_state->keys & (~UP_MASK);
            break;
        case RIGHTKEY:
            player_state->keys = player_state->keys & (~RIGHT_MASK);
            break;
        case DOWNKEY:
            player_state->keys = player_state->keys & (~DOWN_MASK);
            break;
        case LEFTKEY:
            player_state->keys = player_state->keys & (~LEFT_MASK);
            break;
        default:
            break;
    }


    // Movement Controls and Collision Checking
    i32 keys = player_state->keys;
    if (player_state->move_counter <= 0) {
        // Check for screen transition
        TileMap *old_tile_map = world_state->current_tile_map;
        i32 tile_x = player_state->tile_x;
        i32 tile_y = player_state->tile_y;
        if (tile_y == -1 && old_tile_map->top_connection) {
            world_state->screen_transitioning = true;
            world_state->transition_counter = WIN_HEIGHT;
            world_state->transition_direction = UPDIR;
            world_state->next_tile_map = old_tile_map->top_connection;
            return;
        } else if (
            tile_y == SCREEN_HEIGHT_TILES && 
            old_tile_map->bottom_connection
        ) {
            world_state->screen_transitioning = true;
            world_state->transition_counter = WIN_HEIGHT;
            world_state->transition_direction = DOWNDIR;
            world_state->next_tile_map = old_tile_map->bottom_connection;
            return;
        } else if(tile_x == -1 && old_tile_map->left_connection) {
            world_state->screen_transitioning = true;
            world_state->transition_counter = WIN_WIDTH;
            world_state->transition_direction = LEFTDIR;
            world_state->next_tile_map = old_tile_map->left_connection;
            return;
        } else if(
            tile_x == SCREEN_WIDTH_TILES && 
            old_tile_map->right_connection
        ) {
            world_state->screen_transitioning = true;
            world_state->transition_counter = WIN_WIDTH;
            world_state->transition_direction = RIGHTDIR;
            world_state->next_tile_map = old_tile_map->right_connection;
            return;
        }

        i32 *current_tile_map = (i32 *) world_state->current_tile_map->data;
        if (keys & UP_MASK || keys & DOWN_MASK) {
            bool is_up = !(keys & DOWN_MASK);
            i32 change = is_up ? -1 : 1;
            i32 new_tile_y = player_state->tile_y + change;
            i32 tile_x = player_state->tile_x;

            bool is_not_colliding = true;

            if (
                new_tile_y >= 0 &&
                new_tile_y < SCREEN_HEIGHT_TILES &&
                tile_x >= 0 &&
                tile_x < SCREEN_WIDTH_TILES
            ) {
                is_not_colliding = !current_tile_map[
                    new_tile_y*SCREEN_WIDTH_TILES + tile_x
                ];
            }

            if (is_not_colliding) {
                player_state->tile_y = new_tile_y;
                player_state->move_direction = is_up ? UPDIR : DOWNDIR;
                player_state->move_counter = TILE_HEIGHT;
            }
        } else if (keys & RIGHT_MASK || keys & LEFT_MASK) {
            bool is_right = !(keys & LEFT_MASK);
            i32 change = is_right ? 1 : -1;
            i32 new_tile_x = player_state->tile_x + change;
            i32 tile_y = player_state->tile_y;

            bool is_not_colliding = true;

            if (
                new_tile_x >= 0 &&
                new_tile_x < SCREEN_WIDTH_TILES &&
                tile_y >= 0 &&
                tile_y < SCREEN_HEIGHT_TILES
            ) {
                is_not_colliding = !current_tile_map[
                    tile_y*SCREEN_WIDTH_TILES + new_tile_x
                ];
            }

            if (is_not_colliding) {
                player_state->tile_x = new_tile_x;
                player_state->move_direction = is_right ? RIGHTDIR : LEFTDIR;
                player_state->move_counter = TILE_WIDTH;
            }
        }
    }

    if (player_state->move_counter > 0) {
        switch (player_state->move_direction) {
            case UPDIR:
                player_state->pixel_y -= player_state->speed;
                break;
            case RIGHTDIR:
                player_state->pixel_x += player_state->speed;
                break;
            case DOWNDIR:
                player_state->pixel_y += player_state->speed;
                break;
            case LEFTDIR:
                player_state->pixel_x -= player_state->speed;
                break;
            default:
                break;
        }

        player_state->move_counter -= player_state->speed;
    }


    render_tile_map(
        image_buffer, 
        (i32 *) world_state->current_tile_map->data,
        0,
        0
    );

    // Render player
    i32 player_min_x = player_state->pixel_x - 16;
    i32 player_max_x = player_state->pixel_x + 17;
    i32 player_min_y = player_state->pixel_y - 16;
    i32 player_max_y = player_state->pixel_y + 17;

    render_rectangle(
        image_buffer, 
        player_min_x, 
        player_max_x, 
        player_min_y, 
        player_max_y,
        0.0, 0.8, 0.25
    );
}

void transition_screens(
    i32 *restrict image_buffer,
    PlayerState *restrict player_state,
    WorldState *restrict world_state
) {
    Direction transition_direction = world_state->transition_direction;
    if (world_state->transition_counter <= 0) {
        world_state->screen_transitioning = false;
        world_state->current_tile_map = world_state->next_tile_map;

        player_state->keys = 0;

        switch (transition_direction) {
            case UPDIR:
                player_state->tile_y += SCREEN_HEIGHT_TILES;
                player_state->pixel_y = 
                convert_tile_to_pixel_coordinate(
                    player_state->tile_y, 
                    Y_DIMENSION
                );
                break;
            case DOWNDIR:
                player_state->tile_y -= SCREEN_HEIGHT_TILES;
                player_state->pixel_y = 
                convert_tile_to_pixel_coordinate(
                    player_state->tile_y, 
                    Y_DIMENSION
                );
                break;
            case RIGHTDIR:
                player_state->tile_x -= SCREEN_WIDTH_TILES;
                player_state->pixel_x = 
                convert_tile_to_pixel_coordinate(
                    player_state->tile_x, 
                    X_DIMENSION
                );
                break;
            case LEFTDIR:
                player_state->tile_x += SCREEN_WIDTH_TILES;
                player_state->pixel_x = 
                convert_tile_to_pixel_coordinate(
                    player_state->tile_x, 
                    X_DIMENSION
                );
                break;
            default:
                break;
            }
        return;
    }

    if (transition_direction == UPDIR || transition_direction == DOWNDIR) {
        world_state->transition_counter -= 16;
    } else {
        world_state->transition_counter -= 20;
    }

    i32 old_map_y_offset = 0;
    i32 old_map_x_offset = 0;
    i32 new_map_y_offset = 0;
    i32 new_map_x_offset = 0;
    switch (world_state->transition_direction) {
        case UPDIR:
            old_map_y_offset = WIN_HEIGHT - world_state->transition_counter;
            new_map_y_offset = old_map_y_offset - WIN_HEIGHT;
            break;
        case DOWNDIR:
            old_map_y_offset = world_state->transition_counter - WIN_HEIGHT;
            new_map_y_offset = WIN_HEIGHT + old_map_y_offset;
            break;
        case RIGHTDIR:
            old_map_x_offset = world_state->transition_counter - WIN_WIDTH;
            new_map_x_offset = WIN_WIDTH + old_map_x_offset;
            break;
        case LEFTDIR:
            old_map_x_offset = WIN_WIDTH - world_state->transition_counter;
            new_map_x_offset = old_map_x_offset - WIN_WIDTH;
            break;
        default:
            break;
    }

    render_tile_map(
        image_buffer, 
        (i32 *) world_state->next_tile_map->data,
        new_map_x_offset,
        new_map_y_offset
    );

    render_tile_map(
        image_buffer, 
        (i32 *) world_state->current_tile_map->data,
        old_map_x_offset,
        old_map_y_offset
    );
}

void clear_image_buffer(i32 *image_buffer) {
    memset(image_buffer, 0, WIN_WIDTH * WIN_HEIGHT * 4);
}

void render_rectangle(
    i32 *image_buffer, 
    i32 min_x, 
    i32 max_x,
    i32 min_y,
    i32 max_y,
    float red,
    float green,
    float blue
) {
    i32 clamped_min_x = min_x >= 0 ? min_x : 0;
    i32 clamped_min_y = min_y >= 0 ? min_y : 0;
    i32 clamped_max_x = max_x > WIN_WIDTH ? WIN_WIDTH : max_x;
    i32 clamped_max_y = max_y > WIN_HEIGHT ? WIN_HEIGHT : max_y;

    i32 int_red = (i32) (red * 255.0f);
    i32 int_green = (i32) (green * 255.0f);
    i32 int_blue = (i32) (blue * 255.0f);

    i32 color = (int_red << 16) | (int_green << 8) | int_blue;

    i32 row_start = WIN_WIDTH * clamped_min_y;
    for (int y = clamped_min_y; y < clamped_max_y; y++) {
        for (int x = clamped_min_x; x < clamped_max_x; x++) {
            image_buffer[row_start + x] = color;
        }

        row_start += WIN_WIDTH;
    }
};

void render_tile_map(
    i32 *restrict image_buffer, 
    i32 *restrict tile_map,
    i32 x_offset,
    i32 y_offset
) {
    for (int row = 0; row < SCREEN_HEIGHT_TILES; row++) {
        for (int column = 0; column < SCREEN_WIDTH_TILES; column++) {
            i32 tile_value = tile_map[row*SCREEN_WIDTH_TILES + column];

            float red = tile_value == 1 ? 1.0f : 0.25f;
            float green = tile_value == 1 ? 1.0f : 0.25f;
            float blue = tile_value == 1 ? 1.0f : 0.25f;

            render_rectangle(
                image_buffer, 
                column * TILE_WIDTH + x_offset,
                (column + 1) * TILE_WIDTH + x_offset,
                row * TILE_HEIGHT + y_offset,
                (row + 1) * TILE_HEIGHT + y_offset,
                red, green, blue
            );
        }
    }
}

i32 convert_tile_to_pixel_coordinate(i32 tile_value, CoordDimension dimension) {
    if (dimension == Y_DIMENSION) {
        return TILE_HEIGHT * tile_value + (TILE_HEIGHT / 2);
    } else {
        return TILE_WIDTH * tile_value + (TILE_WIDTH / 2);
    }
}
