/* @TODO@
	- Saturday 2022-5-21:
		- Fix heart beat sfx.

	- Sunday 2022-5-22:

	- Monday 2022-5-23:

	- Tuesday 2022-5-24:

	- Wednesday 2022-5-25:

	- Thursday 2022-5-26:

	- Friday 2022-5-27:

	- Saturday 2022-5-28:

	- Sunday 2022-5-29:

	- Monday 2022-5-30:

	- Tuesday 2022-5-31:

	- Wednesday 2022-6-1:

	- Thursday 2022-6-2:

	- Friday 2022-6-3:

	* Better input.
	* Handle disconnected initial and updated values.
	* Handle different resolutions.
*/

// @NOTE@ Credits
// "A Fast Voxel Traversal Algorithm for Ray Tracing" https://www.flipcode.com/archives/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf
// "How to check if two given line segments intersect?" https://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/, http://www.dcs.gla.ac.uk/~pat/52233/slides/Geometry1x1.pdf

#define STB_IMAGE_IMPLEMENTATION true
#include <stb_image.h>
#include "unified.h"
#include "platform.h"
#include "rng.cpp"
#include "utilities.cpp"

global constexpr i32 COMPUTER_TASKBAR_HEIGHT            = 50;
global constexpr i32 COMPUTER_ICON_DIM                  = 50;
global constexpr vi2 COMPUTER_CREDITS_POSITION          = { 55, static_cast<i32>(WIN_DIM.y - (COMPUTER_ICON_DIM + 50) * 0.75f) };
global constexpr vi2 COMPUTER_ANTIHOME_PROGRAM_POSITION = { 55, static_cast<i32>(WIN_DIM.y - (COMPUTER_ICON_DIM + 50) * 1.75f) };
global constexpr i32 COMPUTER_TITLE_BAR_HEIGHT          = 25;
global constexpr vi2 COMPUTER_BUTTON_DIMENSIONS         = { 45, 27 };

global constexpr vi2 SCREEN_RES        = WIN_DIM / 3;
global constexpr i32 STATUS_HUD_HEIGHT = SCREEN_RES.y / 4;
global constexpr vi2 VIEW_RES          = { SCREEN_RES.x, SCREEN_RES.y - STATUS_HUD_HEIGHT };

global constexpr f32 HORT_TO_VERT_K                 = 0.927295218f * VIEW_RES.x;
global constexpr f32 WALL_HEIGHT                    = 2.7432f;
global constexpr f32 WALL_THICKNESS                 = 0.4f;
global constexpr f32 LUCIA_HEIGHT                   = 1.4986f;
global constexpr i32 MAP_DIM                        = 32;
global constexpr f32 WALL_SPACING                   = 3.0f;
global constexpr i32 INVENTORY_DIM                  = 30;
global constexpr i32 INVENTORY_PADDING              = 5;
global constexpr f32 CREEPY_SOUND_MIN_TIME          = 15.0f;
global constexpr f32 CREEPY_SOUND_MAX_TIME          = 90.0f;

global constexpr vi2 CIRCUIT_BREAKER_SWITCH_GRID       = { 4, 2 };
global constexpr vf2 CIRCUIT_BREAKER_HUD_DIMENSIONS    = { 200.0f, 115.0f };
global constexpr vf2 CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS        = { CIRCUIT_BREAKER_HUD_DIMENSIONS.x * 0.1f, CIRCUIT_BREAKER_HUD_DIMENSIONS.y * 0.75f };
global constexpr vf2 CIRCUIT_BREAKER_VOLTAGE_DISPLAY_COORDINATES       = { VIEW_RES.x * 0.75f, VIEW_RES.y / 2.0f + CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS.y / 2.0f };
global constexpr vf2 CIRCUIT_BREAKER_SWITCH_DIMENSIONS = { 30.0f, CIRCUIT_BREAKER_HUD_DIMENSIONS.y / CIRCUIT_BREAKER_SWITCH_GRID.y - 20.0f };
global constexpr vf2 CIRCUIT_BREAKER_SWITCH_PADDINGS   = { (CIRCUIT_BREAKER_VOLTAGE_DISPLAY_COORDINATES.x - (VIEW_RES.x - CIRCUIT_BREAKER_HUD_DIMENSIONS.x) / 2.0f - CIRCUIT_BREAKER_SWITCH_DIMENSIONS.x * CIRCUIT_BREAKER_SWITCH_GRID.x) / (1.0f + CIRCUIT_BREAKER_SWITCH_GRID.x), 20.0f };
global constexpr vf2 CIRCUIT_BREAKER_HUD_POSITION      = (VIEW_RES - CIRCUIT_BREAKER_HUD_DIMENSIONS) / 2.0f;

enum_loose (AudioChannel, i8)
{
	unreserved = -1,

	enum_start_region(RESERVED)
		r0,
		r1,
		r2,
		r3,
	enum_end_region(RESERVED)
	enum_start_region(UNRESERVED)
		_4,
		_5,
		_6,
		_7,
	enum_end_region(UNRESERVED)
};

global constexpr strlit WALK_STEP_WAV_FILE_PATHS[] =
	{
		DATA_DIR "audio/step_0.wav",
		DATA_DIR "audio/step_1.wav",
		DATA_DIR "audio/step_2.wav",
		DATA_DIR "audio/step_3.wav",
		DATA_DIR "audio/step_4.wav",
		DATA_DIR "audio/step_5.wav",
		DATA_DIR "audio/step_6.wav",
		DATA_DIR "audio/step_7.wav"
	};

global constexpr strlit RUN_STEP_WAV_FILE_PATHS[] =
	{
		DATA_DIR "audio/run_0.wav",
		DATA_DIR "audio/run_1.wav",
		DATA_DIR "audio/run_2.wav",
		DATA_DIR "audio/run_3.wav",
		DATA_DIR "audio/run_4.wav",
		DATA_DIR "audio/run_5.wav",
		DATA_DIR "audio/run_6.wav",
		DATA_DIR "audio/run_7.wav"
	};

global constexpr strlit CREEPY_SOUND_WAV_FILE_PATHS[] =
	{
		DATA_DIR "audio/creepy_sound_0.wav",
		DATA_DIR "audio/creepy_sound_1.wav",
		DATA_DIR "audio/creepy_sound_2.wav",
		DATA_DIR "audio/creepy_sound_3.wav",
		DATA_DIR "audio/creepy_sound_4.wav",
		DATA_DIR "audio/creepy_sound_5.wav",
		DATA_DIR "audio/creepy_sound_6.wav",
		DATA_DIR "audio/creepy_sound_7.wav"
	};

global constexpr struct { f32 min_scalar; f32 max_scalar; strlit file_path; } PAPER_DATA[] =
	{
		{ 0.2f, 0.8f, DATA_DIR "papers/terry_entry_0.png" },
		{ 0.2f, 0.8f, DATA_DIR "papers/terry_entry_1.png" },
		{ 0.2f, 0.8f, DATA_DIR "papers/terry_entry_2.png" },
		{ 0.2f, 0.8f, DATA_DIR "papers/terry_entry_3.png" },
		{ 0.2f, 0.8f, DATA_DIR "papers/terry_entry_4.png" }
	};

enum_loose (ItemType, u8)
{
	null,

	enum_start_region(ITEM)
		cheap_batteries,
		paper,
		flashlight,
		cowbell,
		eyedrops,
		first_aid_kit,
		nightvision_goggles,
		pills,
		military_grade_batteries,
		radio,
	enum_end_region(ITEM)
};

global constexpr struct { strlit img_file_path; f32 spawn_weight; } ITEM_DATA[ItemType::ITEM_COUNT] =
	{
		{ DATA_DIR "items/cheap_batteries.png"          , 10.0f },
		{ DATA_DIR "items/paper.png"                    ,  5.0f },
		{ DATA_DIR "items/flashlight_off.png"           ,  4.0f },
		{ DATA_DIR "items/cowbell.png"                  ,  0.0f },
		{ DATA_DIR "items/eyedrops.png"                 ,  3.0f },
		{ DATA_DIR "items/first_aid_kit.png"            ,  4.0f },
		{ DATA_DIR "items/nightvision_goggles.png"      ,  2.0f },
		{ DATA_DIR "items/pills.png"                    ,  6.0f },
		{ DATA_DIR "items/military_grade_batteries.png" ,  4.0f },
		{ DATA_DIR "items/radio.png"                    ,  4.0f }
	};

struct Item
{
	ItemType  type;

	vf3       position;
	vf2       velocity;
	vf2       normal;

	union
	{
		struct
		{
			f32 power;
		} flashlight;

		struct
		{
			i32 index;
		} paper;
	};
};

flag_struct (WallVoxel, u8)
{
	left          = 1 << 0,
	bottom        = 1 << 1,
	back_slash    = 1 << 2,
	forward_slash = 1 << 3
};

global constexpr struct WallVoxelData { WallVoxel voxel; vf2 start; vf2 end; vf2 normal; } WALL_VOXEL_DATA[] =
	{
		{ WallVoxel::left         , { 0.0f, 0.0f }, { 0.0f, 1.0f }, {     1.0f,      0.0f } },
		{ WallVoxel::bottom       , { 0.0f, 0.0f }, { 1.0f, 0.0f }, {     0.0f,     -1.0f } },
		{ WallVoxel::back_slash   , { 1.0f, 0.0f }, { 0.0f, 1.0f }, { INVSQRT2,  INVSQRT2 } },
		{ WallVoxel::forward_slash, { 0.0f, 0.0f }, { 1.0f, 1.0f }, { INVSQRT2, -INVSQRT2 } }
	};

struct WallSide
{
	vi2       coordinates;
	WallVoxel voxel;
	vf2       normal;
};

struct PathCoordinatesNode
{
	vi2                  coordinates;
	PathCoordinatesNode* next_node;
};

enum struct StateContext : u8
{
	title_menu,
	game,
	end
};

enum_loose (ComputerWindowType, u8)
{
	null,

	enum_start_region(TYPE)
		credits,
		antihome_program,
		power,
	enum_end_region(TYPE)
};

enum struct HandOnState : u8
{
	null,
	door,
	circuit_breaker,
	item
};

enum struct HudType : u8
{
	null,
	inventory,
	paper,
	circuit_breaker
};

struct BreakerSwitch
{
	bool32 active;
	i32    voltage;
};

enum struct GameGoal : u8
{
	find_door,
	fix_power,
	escape
};

struct State
{
	MemoryArena  long_term_arena;
	MemoryArena  transient_arena;

	union
	{
		struct
		{
			FC_Font* major;
			FC_Font* minor;
		} font;

		FC_Font* fonts[sizeof(font) / sizeof(FC_Font*)];
	};

	u32          seed;
	f32          time;
	StateContext context;

	struct
	{
		union
		{
			struct
			{
				SDL_Texture* desktop;
				SDL_Texture* cursor;
				SDL_Texture* power_button;
				SDL_Texture* text_file;
				SDL_Texture* antihome_program;
				SDL_Texture* window_close;
			} texture;

			SDL_Texture* textures[sizeof(texture) / sizeof(SDL_Texture*)];
		};

		union
		{
			struct
			{
				Mix_Chunk* ambience;
			} audio;

			Mix_Chunk* audios[sizeof(audio) / sizeof(Mix_Chunk*)];
		};

		vf2                cursor_velocity;
		vf2                cursor;
		bool32             cursor_dragging_window;
		ComputerWindowType window_type;
		vf2                window_dimensions;
		vf2                window_velocity;
		vf2                window_position;
	} title_menu;

	struct
	{
		union
		{
			struct
			{
				Image door;
				Image circuit_breaker;
				Image wall_left_arrow;
				Image wall_right_arrow;
			} image;

			Image images[sizeof(image) / sizeof(Image)];
		};

		union
		{
			struct
			{
				TextureSprite hand;
				TextureSprite flashlight_on;
				TextureSprite default_items[ItemType::ITEM_COUNT];
				TextureSprite papers[ARRAY_CAPACITY(PAPER_DATA)];
			} texture_sprite;

			TextureSprite texture_sprites[sizeof(texture_sprite) / sizeof(TextureSprite)];
		};

		union
		{
			struct
			{
				AnimatedSprite monster;
				AnimatedSprite fire;
			} animated_sprite;

			AnimatedSprite animated_sprites[sizeof(animated_sprite) / sizeof(AnimatedSprite)];
		};

		union
		{
			struct
			{
				Mipmap wall;
				Mipmap floor;
				Mipmap ceiling;
			} mipmap;

			Mipmap mipmaps[sizeof(mipmap) / sizeof(Mipmap)];
		};

		union
		{
			struct
			{
				SDL_Texture* screen;
				SDL_Texture* view;
				SDL_Texture* lucia_haunted;
				SDL_Texture* lucia_healed;
				SDL_Texture* lucia_hit;
				SDL_Texture* lucia_normal;
				SDL_Texture* lucia_wounded;
				SDL_Texture* circuit_breaker_switches[2];
				SDL_Texture* circuit_breaker_panel;
			} texture;

			SDL_Texture* textures[sizeof(texture) / sizeof(SDL_Texture*)];
		};

		union
		{
			struct
			{
				Mix_Chunk* drone;
				Mix_Chunk* drone_low;
				Mix_Chunk* drone_off;
				Mix_Chunk* drone_on;
				Mix_Chunk* blackout;
				Mix_Chunk* eletronical;
				Mix_Chunk* pick_up_paper;
				Mix_Chunk* pick_up_heavy;
				Mix_Chunk* switch_toggle;
				Mix_Chunk* circuit_breaker_switch;
				Mix_Chunk* door_budge;
				Mix_Chunk* panel_open;
				Mix_Chunk* panel_close;
				Mix_Chunk* generator;
				Mix_Chunk* heartbeats   [2];
				Mix_Chunk* walk_steps   [ARRAY_CAPACITY(WALK_STEP_WAV_FILE_PATHS)];
				Mix_Chunk* run_steps    [ARRAY_CAPACITY(RUN_STEP_WAV_FILE_PATHS)];
				Mix_Chunk* creepy_sounds[ARRAY_CAPACITY(CREEPY_SOUND_WAV_FILE_PATHS)];
			} audio;

			Mix_Chunk* audios[sizeof(audio) / sizeof(Mix_Chunk*)];
		};

		GameGoal             goal;

		f32                  entering_keytime;
		f32                  exiting_keytime;

		PathCoordinatesNode* available_path_coordinates_node;
		strlit               notification_message;
		f32                  notification_keytime;
		f32                  heart_rate_values[32];
		i32                  heart_rate_index;
		f32                  heart_rate_update_keytime;
		f32                  heart_rate_velocity;
		f32                  heart_rate_current_value;
		f32                  heart_rate_beat_keytime;
		f32                  heart_rate_bpm;
		i32                  heartbeat_sfx_index;

		WallVoxel            wall_voxels[MAP_DIM][MAP_DIM];
		WallSide             door_wall_side;
		WallSide             circuit_breaker_wall_side;
		f32                  creepy_sound_countdown;
		f32                  ceiling_lights_keytime;

		vf2                  lucia_velocity;
		vf3                  lucia_position;
		f32                  lucia_angle_velocity;
		f32                  lucia_angle;
		f32                  lucia_fov;
		f32                  lucia_head_bob_keytime;
		f32                  lucia_stamina;
		f32                  lucia_sprint_keytime;
		bool32               lucia_out_of_breath;

		PathCoordinatesNode* monster_path;
		vi2                  monster_path_goal;
		vf3                  monster_position;
		vf2                  monster_velocity;
		vf2                  monster_normal;

		vf3                  hand_position;
		HandOnState          hand_on_state;
		Item*                hand_hovered_item;

		i32                  item_count;
		Item                 item_buffer[MAP_DIM * MAP_DIM / 6];

		struct
		{
			struct
			{
				f32 battery_display_keytime;
				f32 battery_level_keytime;
			} status;

			vf2     cursor_velocity;
			vf2     cursor;
			HudType type;

			struct
			{
				Item   array[2][4];
				vf2    click_position;
				Item*  selected_item;
				bool32 grabbing;
			} inventory;

			struct
			{
				i32 index;
				vf2 velocity;
				vf2 delta_position;
				f32 scalar_velocity;
				f32 scalar;
			} paper;

			struct
			{
				union
				{
					BreakerSwitch switches[CIRCUIT_BREAKER_SWITCH_GRID.y][CIRCUIT_BREAKER_SWITCH_GRID.x];
					BreakerSwitch flat_switches[sizeof(switches) / sizeof(BreakerSwitch)];
				};

				i32 max_voltage;
				i32 goal_voltage;
				i32 active_voltage;
				f32 interpolated_voltage_velocity;
				f32 interpolated_voltage;
			} circuit_breaker;
		} hud;

		union
		{
			struct
			{
				Item* flashlight;
			} holding;

			Item* holdings[sizeof(holding) / sizeof(Item*)];
		};

		f32 flashlight_activation;
		vf3 flashlight_ray;
		f32 flashlight_keytime;
	} game;

	struct
	{
		union
		{
			struct
			{
				Mix_Chunk* door_enter;
				Mix_Chunk* shooting;
			} audio;

			Mix_Chunk* audios[sizeof(audio) / sizeof(Mix_Chunk*)];
		};

		f32    entering_keytime;
		bool32 is_exiting;
		f32    exiting_keytime;
	} end;
};

internal const WallVoxelData* get_wall_voxel_data(WallVoxel voxel)
{
	switch (voxel)
	{
		case WallVoxel::left          : return &WALL_VOXEL_DATA[0];
		case WallVoxel::bottom        : return &WALL_VOXEL_DATA[1];
		case WallVoxel::back_slash    : return &WALL_VOXEL_DATA[2];
		case WallVoxel::forward_slash : return &WALL_VOXEL_DATA[3];
	}

	ASSERT(false);
	return 0;
}

internal vf2 get_position_of_wall_side(WallSide wall_side, f32 normal_length = 1.0f)
{
	const WallVoxelData* data = get_wall_voxel_data(wall_side.voxel);
	return (wall_side.coordinates + (data->start + data->end) / 2.0f) * WALL_SPACING + data->normal * normal_length;
}

internal bool32 equal_wall_sides(WallSide a, WallSide b)
{
	return a.coordinates == b.coordinates && a.voxel == b.voxel && a.normal == b.normal;
}

internal TextureSprite* get_corresponding_texture_sprite_of_item(State* state, Item* item)
{
	switch (item->type)
	{
		case ItemType::null:
		{
			return 0;
		} break;

		case ItemType::flashlight:
		{
			if (state->game.holding.flashlight == item)
			{
				return &state->game.texture_sprite.flashlight_on;
			}
		} break;
	}

	return &state->game.texture_sprite.default_items[+item->type - +ItemType::ITEM_START];
}

internal bool32 check_combine(Item** out_a, ItemType a_type, Item** out_b, ItemType b_type, Item* fst, Item* snd)
{
	if (fst->type == a_type && snd->type == b_type)
	{
		*out_a = fst;
		*out_b = snd;
		return true;
	}
	else if (fst->type == b_type && snd->type == a_type)
	{
		*out_a = snd;
		*out_b = fst;
		return true;
	}
	else
	{
		return false;
	}
}

internal WallVoxel* get_wall_voxel(State* state, vi2 v)
{
	return &state->game.wall_voxels[mod(v.y, MAP_DIM)][mod(v.x, MAP_DIM)];
}

internal PathCoordinatesNode* allocate_path_coordinates_node(State* state)
{
	if (state->game.available_path_coordinates_node)
	{
		PathCoordinatesNode* node = state->game.available_path_coordinates_node;
		state->game.available_path_coordinates_node = state->game.available_path_coordinates_node->next_node;
		return node;
	}
	else
	{
		return memory_arena_allocate<PathCoordinatesNode>(&state->long_term_arena);
	}
}

internal PathCoordinatesNode* deallocate_path_coordinates_node(State* state, PathCoordinatesNode* node)
{
	PathCoordinatesNode* tail = node->next_node;
	node->next_node = state->game.available_path_coordinates_node;
	state->game.available_path_coordinates_node = node;
	return tail;
}

internal vf2 path_coordinates_to_position(vi2 coordinates)
{
	coordinates.y = mod(coordinates.y, MAP_DIM * 2);
	return
		vf2 {
			mod(coordinates.x, MAP_DIM) + (coordinates.y % 2 == 0 ? 0.5f : 0.0f),
			coordinates.y / 2.0f
		} * WALL_SPACING;
}

internal vi2 path_coordinates_to_map_coordinates(vi2 coordinates)
{
	return { mod(coordinates.x, MAP_DIM), mod(coordinates.y, MAP_DIM * 2) / 2 };
}

internal vi2 get_closest_open_path_coordinates(State* state, vf2 position)
{
	position.x = mod(position.x, MAP_DIM * WALL_SPACING);
	position.y = mod(position.y, MAP_DIM * WALL_SPACING);

	vi2 map_coordinates       = vxx(position / WALL_SPACING);
	vf2 remainder_coordinates = position / WALL_SPACING - map_coordinates;

	constexpr struct { vi2 delta_coordinates; WallVoxel side; vf2 offset; } SIDES[] =
		{
			{ { 0, 0 }, WallVoxel::bottom, { 0.5f, 0.0f } },
			{ { 0, 0 }, WallVoxel::left  , { 0.0f, 0.5f } },
			{ { 1, 0 }, WallVoxel::left  , { 1.0f, 0.5f } },
			{ { 0, 1 }, WallVoxel::bottom, { 0.5f, 1.0f } }
		};

	i32 best_index       = -1;
	f32 best_distance_sq = NAN;
	FOR_ELEMS(it, SIDES)
	{
		if (+(*get_wall_voxel(state, map_coordinates + it->delta_coordinates) & it->side))
		{
			continue;
		}
		if (+(*get_wall_voxel(state, map_coordinates) & WallVoxel::back_slash))
		{
			if (remainder_coordinates.x < 1.0f - remainder_coordinates.y)
			{
				if (!(it_index == 0 || it_index == 1))
				{
					continue;
				}
			}
			else if (!(it_index == 2 || it_index == 3))
			{
				continue;
			}
		}
		else if (+(*get_wall_voxel(state, map_coordinates) & WallVoxel::forward_slash))
		{
			if (remainder_coordinates.x < remainder_coordinates.y)
			{
				if (!(it_index == 1 || it_index == 3))
				{
					continue;
				}
			}
			else if (!(it_index == 0 || it_index == 2))
			{
				continue;
			}
		}

		f32 distance_sq = norm_sq(remainder_coordinates - it->offset);
		if (best_index == -1 || distance_sq < best_distance_sq)
		{
			best_index       = it_index;
			best_distance_sq = distance_sq;
		}
	}

	vi2 result = {};
	switch (best_index)
	{
		case 0: result = { map_coordinates.x    , map_coordinates.y * 2     }; break;
		case 1: result = { map_coordinates.x    , map_coordinates.y * 2 + 1 }; break;
		case 2: result = { map_coordinates.x + 1, map_coordinates.y * 2 + 1 }; break;
		case 3: result = { map_coordinates.x    , map_coordinates.y * 2 + 2 }; break;
		default: ASSERT(false);
	}

	result.x = mod(result.x, MAP_DIM    );
	result.y = mod(result.y, MAP_DIM * 2);
	return result;
}

internal f32 path_distance_function(vi2 a, vi2 b)
{
	a.x = mod(a.x, MAP_DIM    );
	a.y = mod(a.y, MAP_DIM * 2);
	b.x = mod(b.x, MAP_DIM    );
	b.y = mod(b.y, MAP_DIM * 2);

	f32 best = -1.0f;
	FOR_RANGE(i, 9)
	{
		vi2 deltas   = { abs(a.x - b.x + (i % 3 - 1) * MAP_DIM), abs(a.y - b.y + (i / 3 - 1) * MAP_DIM * 2) } ;
		f32 distance = abs(deltas.x - deltas.y) + min(deltas.x, deltas.y) * SQRT2;
		if (best == -1.0f || best > distance)
		{
			best = distance;
		}
	}

	return best;
}

internal vf2 ray_to_closest(vf2 position, vf2 target)
{
	position.x = mod(position.x, MAP_DIM * WALL_SPACING);
	position.y = mod(position.y, MAP_DIM * WALL_SPACING);
	target.x   = mod(target  .x, MAP_DIM * WALL_SPACING);
	target.y   = mod(target  .y, MAP_DIM * WALL_SPACING);
	return target - position + vf2 { roundf((position.x - target.x) / (MAP_DIM * WALL_SPACING)), roundf((position.y - target.y) / (MAP_DIM * WALL_SPACING)) } * MAP_DIM * WALL_SPACING;
}

internal Item* allocate_item(State* state)
{
	ASSERT(IN_RANGE(state->game.item_count, 0, ARRAY_CAPACITY(state->game.item_buffer)));
	return &state->game.item_buffer[state->game.item_count++];
}

internal void deallocate_item(State* state, Item* item)
{
	ASSERT(IN_RANGE(item, state->game.item_buffer, state->game.item_buffer + state->game.item_count));
	*item = state->game.item_buffer[state->game.item_count - 1];
	state->game.item_count -= 1;
}

internal vf2 rng_open_position(State* state, vi2 coordinates)
{
	// @TODO@ Clean up?
	constexpr f32 SPAWN_PADDING = 0.1f;

	if (+(*get_wall_voxel(state, coordinates) & WallVoxel::back_slash))
	{
		vf2 p = { rng(&state->seed), rng(&state->seed) };
		p = { lerp(SPAWN_PADDING, 1.0f - 2.0f * SPAWN_PADDING, p.x), SPAWN_PADDING + (lerp(SPAWN_PADDING, 1.0f - 2 * SPAWN_PADDING, 1.0f - p.x) - SPAWN_PADDING) * p.y };

		return (coordinates + (rng(&state->seed) < 0.5f ? p : vf2 { 1.0f, 1.0f } - p)) * WALL_SPACING;
	}
	else if (+(*get_wall_voxel(state, coordinates) & WallVoxel::forward_slash))
	{
		vf2 p = { rng(&state->seed), rng(&state->seed) };
		p = { lerp(SPAWN_PADDING, 1.0f - 2.0f * SPAWN_PADDING, p.x), SPAWN_PADDING + (lerp(SPAWN_PADDING, 1.0f - 2 * SPAWN_PADDING, 1.0f - p.x) - SPAWN_PADDING) * p.y };

		return (coordinates + (rng(&state->seed) < 0.5f ? vf2 { p.x, 1.0f - p.y } : vf2 { 1.0f - p.x, p.y })) * WALL_SPACING;
	}
	else
	{
		return (coordinates + vf2 { rng(&state->seed, SPAWN_PADDING, 1.0f - SPAWN_PADDING), rng(&state->seed, SPAWN_PADDING, 1.0f - SPAWN_PADDING) }) * WALL_SPACING;
	}
}

internal vf2 rng_open_position(State* state)
{
	return rng_open_position(state, { rng(&state->seed, 0, MAP_DIM), rng(&state->seed, 0, MAP_DIM) });
}

internal vf2 move(State* state, vf2 position, vf2 displacement)
{
	#if 1
	vf2 current_position     = position;
	vf2 current_displacement = displacement;
	FOR_RANGE(4)
	{
		CollisionData data;
		data.exists       = false;
		data.inside       = false;
		data.displacement = { NAN, NAN };
		data.normal       = { NAN, NAN };

		// @TODO@ Make this smarter.
		FOR_RANGE(y, static_cast<i32>(floorf(position.y / WALL_SPACING) + min(ceilf(current_displacement.y / WALL_SPACING), 0.0f) - 2.0f), static_cast<i32>(floorf(position.y / WALL_SPACING) + max(floor(current_displacement.y / WALL_SPACING), 0.0f) + 2.0f))
		{
			FOR_RANGE(x, static_cast<i32>(floorf(position.x / WALL_SPACING) + min(ceilf(current_displacement.x / WALL_SPACING), 0.0f) - 2.0f), static_cast<i32>(floorf(position.x / WALL_SPACING) + max(floor(current_displacement.x / WALL_SPACING), 0.0f) + 2.0f))
			{
				FOR_ELEMS(voxel_data, WALL_VOXEL_DATA)
				{
					if (+(*get_wall_voxel(state, { x, y }) & voxel_data->voxel))
					{
						data =
							prioritize_collision
							(
								data,
								collide_pill
								(
									current_position,
									current_displacement,
									(vi2 { x, y } + voxel_data->start) * WALL_SPACING,
									(vi2 { x, y } + voxel_data->end  ) * WALL_SPACING,
									WALL_THICKNESS
								)
							);
					}
				}

				if (data.exists)
				{
					goto COLLISION;
				}
			}
		}


		if (data.exists)
		{
			COLLISION:
			current_position     += data.displacement;
			current_position.x    = mod(current_position.x, MAP_DIM * WALL_SPACING);
			current_position.y    = mod(current_position.y, MAP_DIM * WALL_SPACING);
			current_displacement  = dot(displacement - data.displacement, rotate90(data.normal)) * rotate90(data.normal);
		}
		else
		{
			current_position   += current_displacement;
			current_position.x  = mod(current_position.x, MAP_DIM * WALL_SPACING);
			current_position.y  = mod(current_position.y, MAP_DIM * WALL_SPACING);
			return current_position;
		}
	}
	return current_position;
	#else
	return position + displacement;
	#endif
}

internal bool32 exists_clear_way(State* state, vf2 position, vf2 goal)
{
	position.x = mod(position.x, MAP_DIM * WALL_SPACING);
	position.y = mod(position.y, MAP_DIM * WALL_SPACING);
	goal.x     = mod(goal.x    , MAP_DIM * WALL_SPACING);
	goal.y     = mod(goal.y    , MAP_DIM * WALL_SPACING);

	vf2 ray = ray_to_closest(position, goal);

	vi2 step    = { sign(ray.x), sign(ray.y) };
	vf2 t_delta = vf2 { step.x / ray.x, step.y / ray.y } * WALL_SPACING;
	vf2 t_max   =
		{
			(floorf(position.x / WALL_SPACING + (ray.x >= 0.0f)) * WALL_SPACING - position.x) / ray.x,
			(floorf(position.y / WALL_SPACING + (ray.y >= 0.0f)) * WALL_SPACING - position.y) / ray.y
		};
	vi2 coordinates =
		{
			static_cast<i32>(floorf(position.x / WALL_SPACING)),
			static_cast<i32>(floorf(position.y / WALL_SPACING))
		};
	vi2 goal_coordinates =
		{
			static_cast<i32>(floorf(goal.x / WALL_SPACING)),
			static_cast<i32>(floorf(goal.y / WALL_SPACING))
		};

	FOR_RANGE(static_cast<i32>(ceilf(fabsf(ray.x) / WALL_SPACING) + ceilf(fabsf(ray.y) / WALL_SPACING)) + 1)
	{
		FOR_ELEMS(voxel_data, WALL_VOXEL_DATA)
		{
			if
			(
				+(*get_wall_voxel(state, coordinates) & voxel_data->voxel) &&
				is_line_segment_intersecting((coordinates + voxel_data->start) * WALL_SPACING, (coordinates + voxel_data->end) * WALL_SPACING, position, position + ray)
			)
			{
				return false;
			}
		}

		if (mod(coordinates.x, MAP_DIM) == goal_coordinates.x && mod(coordinates.y, MAP_DIM) == goal_coordinates.y)
		{
			return true;
		}

		if (t_max.x < t_max.y)
		{
			t_max.x       += t_delta.x;
			coordinates.x += step.x;
		}
		else
		{
			t_max.y       += t_delta.y;
			coordinates.y += step.y;
		}
	}

	return false;
}

internal void boot_up_state(SDL_Renderer* renderer, State* state)
{
	switch (state->context)
	{
		case StateContext::title_menu:
		{
			state->title_menu.texture.desktop          = IMG_LoadTexture(renderer, DATA_DIR "computer/desktop.png");
			state->title_menu.texture.cursor           = IMG_LoadTexture(renderer, DATA_DIR "computer/cursor.png");
			state->title_menu.texture.power_button     = IMG_LoadTexture(renderer, DATA_DIR "computer/terminal_power_button.png");
			state->title_menu.texture.text_file        = IMG_LoadTexture(renderer, DATA_DIR "computer/text_file.png");
			state->title_menu.texture.antihome_program = IMG_LoadTexture(renderer, DATA_DIR "computer/antihome_program.png");
			state->title_menu.texture.window_close     = IMG_LoadTexture(renderer, DATA_DIR "computer/window_close.png");

			state->title_menu.audio.ambience = Mix_LoadWAV(DATA_DIR "audio/computer.wav");

			#if DEBUG
			FOR_ELEMS(it, state->title_menu.textures) { ASSERT(*it); }
			FOR_ELEMS(it, state->title_menu.audios  ) { ASSERT(*it); }
			#endif

			Mix_PlayChannel(+AudioChannel::r0, state->title_menu.audio.ambience, -1);
		} break;

		case StateContext::game:
		{
			state->game.image.door             = init_image(DATA_DIR "overlays/door.png");
			state->game.image.circuit_breaker  = init_image(DATA_DIR "overlays/circuit_breaker.png");
			state->game.image.wall_left_arrow  = init_image(DATA_DIR "overlays/streak_left_0.png");
			state->game.image.wall_right_arrow = init_image(DATA_DIR "overlays/streak_right_0.png");

			state->game.texture_sprite.hand          = init_texture_sprite(renderer, DATA_DIR "hand.png");
			state->game.texture_sprite.flashlight_on = init_texture_sprite(renderer, DATA_DIR "items/flashlight_on.png");
			FOR_ELEMS(it, ITEM_DATA)
			{
				state->game.texture_sprite.default_items[it_index] = init_texture_sprite(renderer, it->img_file_path);
			}
			FOR_ELEMS(it, state->game.texture_sprite.papers)
			{
				*it = init_texture_sprite(renderer, PAPER_DATA[it_index].file_path);
			}

			state->game.animated_sprite.monster = init_animated_sprite(DATA_DIR "eye.png", { 1, 1 }, 0.0f);
			state->game.animated_sprite.fire    = init_animated_sprite(DATA_DIR "fire.png", { 10, 6 }, 60.0f);

			state->game.mipmap.wall    = init_mipmap(DATA_DIR "room/wall.png", 4);
			state->game.mipmap.floor   = init_mipmap(DATA_DIR "room/floor.png", 4);
			state->game.mipmap.ceiling = init_mipmap(DATA_DIR "room/ceiling.png", 4);

			state->game.texture.screen                          = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET   , SCREEN_RES.x, SCREEN_RES.y);
			state->game.texture.view                            = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, VIEW_RES.x  , VIEW_RES.y  );
			state->game.texture.lucia_haunted                   = IMG_LoadTexture(renderer, DATA_DIR "hud/lucia_haunted.png");
			state->game.texture.lucia_healed                    = IMG_LoadTexture(renderer, DATA_DIR "hud/lucia_healed.png");
			state->game.texture.lucia_hit                       = IMG_LoadTexture(renderer, DATA_DIR "hud/lucia_hit.png");
			state->game.texture.lucia_normal                    = IMG_LoadTexture(renderer, DATA_DIR "hud/lucia_normal.png");
			state->game.texture.lucia_wounded                   = IMG_LoadTexture(renderer, DATA_DIR "hud/lucia_wounded.png");
			state->game.texture.circuit_breaker_switches[false] = IMG_LoadTexture(renderer, DATA_DIR "hud/circuit_breaker_switch_off.png");
			state->game.texture.circuit_breaker_switches[true ] = IMG_LoadTexture(renderer, DATA_DIR "hud/circuit_breaker_switch_on.png");
			state->game.texture.circuit_breaker_panel           = IMG_LoadTexture(renderer, DATA_DIR "hud/circuit_breaker_panel.png");

			state->game.audio.drone                  = Mix_LoadWAV(DATA_DIR "audio/drone.wav");
			state->game.audio.drone_low              = Mix_LoadWAV(DATA_DIR "audio/drone_low.wav");
			state->game.audio.drone_off              = Mix_LoadWAV(DATA_DIR "audio/drone_off.wav");
			state->game.audio.drone_on               = Mix_LoadWAV(DATA_DIR "audio/drone_on.wav");
			state->game.audio.blackout               = Mix_LoadWAV(DATA_DIR "audio/blackout.wav");
			state->game.audio.eletronical            = Mix_LoadWAV(DATA_DIR "audio/eletronical.wav");
			state->game.audio.pick_up_paper          = Mix_LoadWAV(DATA_DIR "audio/pick_up_paper.wav");
			state->game.audio.pick_up_heavy          = Mix_LoadWAV(DATA_DIR "audio/pick_up_heavy.wav");
			state->game.audio.switch_toggle          = Mix_LoadWAV(DATA_DIR "audio/switch_toggle.wav");
			state->game.audio.circuit_breaker_switch = Mix_LoadWAV(DATA_DIR "audio/lever_flip.wav");
			state->game.audio.door_budge             = Mix_LoadWAV(DATA_DIR "audio/door_budge.wav");
			state->game.audio.panel_open             = Mix_LoadWAV(DATA_DIR "audio/panel_open.wav");
			state->game.audio.panel_close            = Mix_LoadWAV(DATA_DIR "audio/panel_close.wav");
			state->game.audio.generator              = Mix_LoadWAV(DATA_DIR "audio/generator.wav");
			state->game.audio.heartbeats[0]          = Mix_LoadWAV(DATA_DIR "audio/heartbeat_0.wav");
			state->game.audio.heartbeats[1]          = Mix_LoadWAV(DATA_DIR "audio/heartbeat_1.wav");
			FOR_ELEMS(it, state->game.audio.walk_steps)
			{
				*it = Mix_LoadWAV(WALK_STEP_WAV_FILE_PATHS[it_index]);
			}
			FOR_ELEMS(it, state->game.audio.run_steps)
			{
				*it = Mix_LoadWAV(RUN_STEP_WAV_FILE_PATHS[it_index]);
			}
			FOR_ELEMS(it, state->game.audio.creepy_sounds)
			{
				*it = Mix_LoadWAV(CREEPY_SOUND_WAV_FILE_PATHS[it_index]);
			}

			#if DEBUG
			FOR_ELEMS(it, state->game.textures) { ASSERT(*it); }
			FOR_ELEMS(it, state->game.audios  ) { ASSERT(*it); }
			#endif

			SDL_SetTextureBlendMode(state->game.texture.screen, SDL_BLENDMODE_BLEND);

			Mix_VolumeChunk(state->game.audio.drone, 0);
			Mix_PlayChannel(+AudioChannel::r0, state->game.audio.drone, -1);

			Mix_VolumeChunk(state->game.audio.drone_low, 0);
			Mix_PlayChannel(+AudioChannel::r1, state->game.audio.drone_low, -1);

			Mix_VolumeChunk(state->game.audio.generator, 0);
			Mix_PlayChannel(+AudioChannel::r2, state->game.audio.generator, -1);
		} break;

		case StateContext::end:
		{
			state->end.audio.door_enter = Mix_LoadWAV(DATA_DIR "audio/door_enter.wav");
			state->end.audio.shooting   = Mix_LoadWAV(DATA_DIR "audio/shooting.wav");

			Mix_PlayChannel(+AudioChannel::r2, state->end.audio.door_enter, 0);

			#if DEBUG
			FOR_ELEMS(it, state->game.audios) { ASSERT(*it); }
			#endif
		};
	}
}

internal void boot_down_state(State* state)
{
	switch (state->context)
	{
		case StateContext::title_menu:
		{
			FOR_ELEMS(it, state->title_menu.textures) { SDL_DestroyTexture(*it); }
			FOR_ELEMS(it, state->title_menu.audios  ) { Mix_FreeChunk(*it);      }
		} break;

		case StateContext::game:
		{
			FOR_ELEMS(it, state->game.images          ) { deinit_image(it);           }
			FOR_ELEMS(it, state->game.texture_sprites ) { deinit_texture_sprite(it);  }
			FOR_ELEMS(it, state->game.animated_sprites) { deinit_animated_sprite(it); }
			FOR_ELEMS(it, state->game.mipmaps         ) { deinit_mipmap(it);          }
			FOR_ELEMS(it, state->game.textures        ) { SDL_DestroyTexture(*it);    }
			FOR_ELEMS(it, state->game.audios          ) { Mix_FreeChunk(*it);         }
		} break;

		case StateContext::end:
		{
			FOR_ELEMS(it, state->end.audios) { Mix_FreeChunk(*it); }
		} break;
	}
}

extern "C" PROTOTYPE_INITIALIZE(initialize)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	*state = {};

	state->long_term_arena.size = static_cast<memsize>((platform->memory_capacity - sizeof(State)) * 0.75f);
	state->long_term_arena.base = platform->memory + sizeof(State);
	state->long_term_arena.used = 0;

	state->transient_arena.size = platform->memory_capacity - sizeof(State) - state->long_term_arena.size;
	state->transient_arena.base = platform->memory          + sizeof(State) + state->long_term_arena.size;
	state->transient_arena.used = 0;

	state->title_menu.cursor = WIN_DIM / 2.0f;

	SDL_SetRelativeMouseMode(SDL_TRUE);

	static_assert(+AudioChannel::RESERVED_START == 0);
	Mix_ReserveChannels(+AudioChannel::RESERVED_COUNT);
}

extern "C" PROTOTYPE_BOOT_UP(boot_up)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	state->font.major = FC_CreateFont();
	FC_LoadFont(state->font.major, platform->renderer, DATA_DIR "Consolas.ttf", 32, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);

	state->font.minor = FC_CreateFont();
	FC_LoadFont(state->font.minor, platform->renderer, DATA_DIR "Consolas.ttf", 16, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);

	boot_up_state(platform->renderer, state);

	{
		SDL_Surface* icon = SDL_LoadBMP(DATA_DIR "antihome_icon.bmp");
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	FOR_ELEMS(it, state->fonts)
	{
		FC_FreeFont(*it);
	}

	boot_down_state(state);
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	state->time                 += SECONDS_PER_UPDATE;
	state->transient_arena.used  = 0;

	switch (state->context)
	{
		case StateContext::title_menu:
		{
			aliasing tm = state->title_menu;

			// @TODO@ Make window and cursor stop together.
			tm.cursor_velocity += platform->cursor_delta * 50.0f;
			tm.cursor_velocity *= 0.25f;
			tm.cursor          += tm.cursor_velocity * SECONDS_PER_UPDATE;

			constexpr f32 CURSOR_PADDING = 2.0f;
			if (tm.cursor.x < CURSOR_PADDING || tm.cursor.x > WIN_DIM.x - CURSOR_PADDING)
			{
				tm.cursor_velocity.x = 0.0f;
				tm.cursor.x          = clamp(tm.cursor.x, CURSOR_PADDING, static_cast<f32>(WIN_DIM.x - CURSOR_PADDING));
			}

			if (tm.cursor.y < CURSOR_PADDING || tm.cursor.y > WIN_DIM.y - CURSOR_PADDING)
			{
				tm.cursor_velocity.y = 0.0f;
				tm.cursor.y          = clamp(tm.cursor.y, CURSOR_PADDING, static_cast<f32>(WIN_DIM.y - CURSOR_PADDING));
			}

			if (PRESSED(Input::left_mouse))
			{
				if (tm.window_type != ComputerWindowType::null && in_rect(tm.cursor, tm.window_position + vf2 { 0.0f, tm.window_dimensions.y }, { tm.window_dimensions.x - COMPUTER_TITLE_BAR_HEIGHT, COMPUTER_TITLE_BAR_HEIGHT }))
				{
					tm.cursor_dragging_window = true;
				}
				else if (tm.window_type != ComputerWindowType::null && in_rect(tm.cursor, tm.window_position + tm.window_dimensions + vf2 { -COMPUTER_TITLE_BAR_HEIGHT, 0.0f }, { COMPUTER_TITLE_BAR_HEIGHT, COMPUTER_TITLE_BAR_HEIGHT }))
				{
					tm.window_type = ComputerWindowType::null;
				}
				else if (tm.window_type != ComputerWindowType::null && in_rect(tm.cursor, tm.window_position, tm.window_dimensions))
				{
					switch (tm.window_type)
					{
						case ComputerWindowType::antihome_program:
						{
							if (in_rect(tm.cursor, tm.window_position + vf2 { tm.window_dimensions.x * 0.5f, tm.window_dimensions.y * 0.33f } - COMPUTER_BUTTON_DIMENSIONS / 2.0f, vxx(COMPUTER_BUTTON_DIMENSIONS)))
							{
								boot_down_state(state);
								state->context = StateContext::game;
								state->game    = {};
								boot_up_state(platform->renderer, state);

								FOR_RANGE(MAP_DIM * MAP_DIM / 3)
								{
									vi2 start_walk = { rng(&state->seed, 0, MAP_DIM), rng(&state->seed, 0, MAP_DIM) };

									if
									(
										!+*get_wall_voxel(state, { start_walk.x, start_walk.y })
										&& !+(*get_wall_voxel(state, { start_walk.x    , start_walk.y - 1 }) & WallVoxel::left  )
										&& !+(*get_wall_voxel(state, { start_walk.x - 1, start_walk.y     }) & WallVoxel::bottom)
									)
									{
										vi2 walk = { start_walk.x, start_walk.y };
										FOR_RANGE(MAP_DIM)
										{
											switch (static_cast<i32>(rng(&state->seed) * 4.0f))
											{
												case 0:
												{
													if (!+*get_wall_voxel(state, walk + vi2 { -1, 0 }) && !+(*get_wall_voxel(state, walk + vi2 { -1, -1 }) & WallVoxel::left) && !+(*get_wall_voxel(state, walk + vi2 { -2, 0 }) & WallVoxel::bottom))
													{
														walk.x = mod(walk.x - 1, MAP_DIM);
														*get_wall_voxel(state, walk) |= WallVoxel::bottom;
													}
												} break;

												case 1:
												{
													if (!+*get_wall_voxel(state, walk + vi2 { 1, 0 }) && !+(*get_wall_voxel(state, walk) & WallVoxel::bottom) && !+(*get_wall_voxel(state, walk + vi2 { 1, -1 }) & WallVoxel::left))
													{
														*get_wall_voxel(state, walk) |= WallVoxel::bottom;
														walk.x = mod(walk.x + 1, MAP_DIM);
													}
												} break;

												case 2:
												{
													if (!+*get_wall_voxel(state, walk + vi2 { 0, -1 }) && !+(*get_wall_voxel(state, walk + vi2 { -1, -1 }) & WallVoxel::bottom) && !+(*get_wall_voxel(state, walk + vi2 { 0, -2 }) & WallVoxel::left))
													{
														walk.y = mod(walk.y - 1, MAP_DIM);
														*get_wall_voxel(state, walk) |= WallVoxel::left;
													}
												} break;

												case 3:
												{
													if (!+*get_wall_voxel(state, walk + vi2 { 0, 1 }) && !+(*get_wall_voxel(state, walk) & WallVoxel::left) && !+(*get_wall_voxel(state, walk + vi2 { -1, 1 }) & WallVoxel::bottom))
													{
														*get_wall_voxel(state, walk) |= WallVoxel::left;
														walk.y = mod(walk.y + 1, MAP_DIM);
													}
												} break;
											}

											if
											(
												!(!+*get_wall_voxel(state, walk + vi2 { -1, 0 }) && !+(*get_wall_voxel(state, walk + vi2 { -1, -1 }) & WallVoxel::left) && !+(*get_wall_voxel(state, walk + vi2 { -2, 0 }) & WallVoxel::bottom)) &&
												!(!+*get_wall_voxel(state, walk + vi2 { 1, 0 }) && !+(*get_wall_voxel(state, walk) & WallVoxel::bottom) && !+(*get_wall_voxel(state, walk + vi2 { 1, -1 }) & WallVoxel::left)) &&
												!(!+*get_wall_voxel(state, walk + vi2 { 0, -1 }) && !+(*get_wall_voxel(state, walk + vi2 { -1, -1 }) & WallVoxel::bottom) && !+(*get_wall_voxel(state, walk + vi2 { 0, -2 }) & WallVoxel::left)) &&
												!(!+*get_wall_voxel(state, walk + vi2 { 0, 1 }) && !+(*get_wall_voxel(state, walk) & WallVoxel::left) && !+(*get_wall_voxel(state, walk + vi2 { -1, 1 }) & WallVoxel::bottom))
											)
											{
												break;
											}
										}
									}
								}

								FOR_RANGE(y, MAP_DIM)
								{
									FOR_RANGE(x, MAP_DIM)
									{
										if (*get_wall_voxel(state, { x, y }) == (WallVoxel::left | WallVoxel::bottom) && rng(&state->seed) < 0.5f)
										{
											*get_wall_voxel(state, { x, y }) = WallVoxel::back_slash;
										}
										else if (+(*get_wall_voxel(state, { x + 1, y }) & WallVoxel::left) && +(*get_wall_voxel(state, { x, y + 1 }) & WallVoxel::bottom) && rng(&state->seed) < 0.5f)
										{
											*get_wall_voxel(state, { x + 1, y     }) &= ~WallVoxel::left;
											*get_wall_voxel(state, { x    , y + 1 }) &= ~WallVoxel::bottom;
											*get_wall_voxel(state, { x    , y     }) |= WallVoxel::back_slash;
										}
										else if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::bottom) && *get_wall_voxel(state, { x + 1, y }) == WallVoxel::left && rng(&state->seed) < 0.5f)
										{
											*get_wall_voxel(state, { x    , y }) &= ~WallVoxel::bottom;
											*get_wall_voxel(state, { x + 1, y }) &= ~WallVoxel::left;
											*get_wall_voxel(state, { x    , y }) |=  WallVoxel::forward_slash;
										}
										else if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::left) && *get_wall_voxel(state, { x, y + 1 }) == WallVoxel::bottom && rng(&state->seed) < 0.5f)
										{
											*get_wall_voxel(state, { x, y     }) &= ~WallVoxel::left;
											*get_wall_voxel(state, { x, y + 1 }) &= ~WallVoxel::bottom;
											*get_wall_voxel(state, { x, y     }) |=  WallVoxel::forward_slash;
										}
									}
								}

								// @TODO@ More robustness.
								{
									memory_arena_checkpoint(&state->transient_arena);

									struct WallSideNode
									{
										WallSide      wall_side;
										WallSideNode* next_node;
									};

									WallSideNode* wall_side_node  = 0;
									i32           wall_side_count = 0;

									FOR_RANGE(y, MAP_DIM)
									{
										FOR_RANGE(x, MAP_DIM)
										{
											FOR_ELEMS(it, WALL_VOXEL_DATA)
											{
												if (+(state->game.wall_voxels[y][x] & it->voxel))
												{
													{
														WallSideNode* node = memory_arena_allocate<WallSideNode>(&state->transient_arena);
														node->wall_side.coordinates = { x, y };
														node->wall_side.voxel       = it->voxel;
														node->wall_side.normal      = it->normal;
														node->next_node             = wall_side_node;
														wall_side_node   = node;
														wall_side_count += 1;
													}

													{
														WallSideNode* node = memory_arena_allocate<WallSideNode>(&state->transient_arena);
														node->wall_side.coordinates = { x, y };
														node->wall_side.voxel       = it->voxel;
														node->wall_side.normal      = -it->normal;
														node->next_node             = wall_side_node;
														wall_side_node   = node;
														wall_side_count += 1;
													}
												}
											}
										}
									}

									{
										WallSideNode* door_wall_side_node = wall_side_node;
										for (i32 i = rng(&state->seed, 0, wall_side_count); i; i -= 1)
										{
											door_wall_side_node = door_wall_side_node->next_node;
										}

										state->game.door_wall_side = door_wall_side_node->wall_side;
									}

									{
										WallSideNode* farthest_node     = 0;
										f32           farthest_distance = NAN;
										for (WallSideNode* node = wall_side_node; node; node = node->next_node)
										{
											f32 distance = norm(ray_to_closest(node->wall_side.coordinates * WALL_SPACING, state->game.door_wall_side.coordinates * WALL_SPACING));

											if (farthest_node == 0 || farthest_distance < distance)
											{
												farthest_node     = node;
												farthest_distance = distance;
											}
										}

										state->game.circuit_breaker_wall_side = farthest_node->wall_side;
									}

									{
										WallSideNode* farthest_node     = 0;
										f32           farthest_distance = NAN;
										for (WallSideNode* node = wall_side_node; node; node = node->next_node)
										{
											f32 distance =
												min
												(
													norm(ray_to_closest(node->wall_side.coordinates * WALL_SPACING, state->game.door_wall_side.coordinates * WALL_SPACING)),
													norm(ray_to_closest(node->wall_side.coordinates * WALL_SPACING, state->game.circuit_breaker_wall_side.coordinates * WALL_SPACING))
												);

											if (farthest_node == 0 || farthest_distance < distance)
											{
												farthest_node     = node;
												farthest_distance = distance;
											}
										}

										state->game.lucia_position.xy = rng_open_position(state, farthest_node->wall_side.coordinates);
									}
								}

								state->game.lucia_position.z  = LUCIA_HEIGHT;
								state->game.lucia_fov         = TAU / 3.0f;
								state->game.lucia_stamina     = 1.0f;

								state->game.creepy_sound_countdown = rng(&state->seed, CREEPY_SOUND_MIN_TIME, CREEPY_SOUND_MAX_TIME);

								state->game.monster_position.xy = rng_open_position(state);

								lambda create_item =
									[&](ItemType type)
									{
										Item* item = allocate_item(state);
										*item             = {};
										item->type        = type;
										item->position.xy = rng_open_position(state);
										item->normal      = polar(state->time * 1.5f);

										switch (type)
										{
											case ItemType::paper:
											{
												item->paper.index = rng(&state->seed, 0, ARRAY_CAPACITY(PAPER_DATA));
											};
										}
									};

								for (ItemType type = ItemType::ITEM_START; type != ItemType::ITEM_END; type = static_cast<ItemType>(+type + 1))
								{
									create_item(type);
								}

								FOR_RANGE(ARRAY_CAPACITY(state->game.item_buffer) - state->game.item_count)
								{
									f32 total_weights = 0.0f;
									FOR_ELEMS(it, ITEM_DATA)
									{
										total_weights += it->spawn_weight;
									}

									f32 n = rng(&state->seed) * total_weights;
									i32 i = -1;
									FOR_ELEMS(it, ITEM_DATA)
									{
										n -= it->spawn_weight;

										if (n <= 0.0f)
										{
											i = it_index;
											break;
										}
									}

									ASSERT(i != -1);

									create_item(static_cast<ItemType>(+ItemType::ITEM_START + i));
								}

								FOR_ELEMS(it, state->game.hud.circuit_breaker.flat_switches)
								{
									it->voltage                                  = 1 + rng(&state->seed, 0, 6);
									state->game.hud.circuit_breaker.max_voltage += it->voltage;
								}

								for (i32 i = 0; i < ARRAY_CAPACITY(state->game.hud.circuit_breaker.flat_switches) / 2;)
								{
									i32 index = rng(&state->seed, 0, ARRAY_CAPACITY(state->game.hud.circuit_breaker.flat_switches));
									if (!state->game.hud.circuit_breaker.flat_switches[index].active)
									{
										state->game.hud.circuit_breaker.flat_switches[index].active  = true;
										state->game.hud.circuit_breaker.goal_voltage                += state->game.hud.circuit_breaker.flat_switches[index].voltage;
										i += 1;
									}
								}

								state->game.hud.circuit_breaker.active_voltage = state->game.hud.circuit_breaker.goal_voltage;

								return UpdateCode::resume;
							}
						} break;

						case ComputerWindowType::power:
						{
							if (in_rect(tm.cursor, tm.window_position + vf2 { tm.window_dimensions.x * 0.25f, tm.window_dimensions.y * 0.25f } - COMPUTER_BUTTON_DIMENSIONS / 2.0f, vxx(COMPUTER_BUTTON_DIMENSIONS)))
							{
								return UpdateCode::terminate;
							}
							else if (in_rect(tm.cursor, tm.window_position + vf2 { tm.window_dimensions.x * 0.75f, tm.window_dimensions.y * 0.25f } - COMPUTER_BUTTON_DIMENSIONS / 2.0f, vxx(COMPUTER_BUTTON_DIMENSIONS)))
							{
								tm.window_type = ComputerWindowType::null;
							}
						} break;
					}
				}
				else
				{
					ComputerWindowType clicked_window_type = ComputerWindowType::null;

					if (in_rect(tm.cursor, vxx(COMPUTER_CREDITS_POSITION), vxx(vx2(COMPUTER_ICON_DIM))))
					{
						clicked_window_type  = ComputerWindowType::credits;
						tm.window_dimensions = { 250.0f, 300.0f };
					}
					else if (in_rect(tm.cursor, vxx(COMPUTER_ANTIHOME_PROGRAM_POSITION), vxx(vx2(COMPUTER_ICON_DIM))))
					{
						clicked_window_type  = ComputerWindowType::antihome_program;
						tm.window_dimensions = { 275.0f, 250.0f };
					}
					else if (in_rect(tm.cursor, { 0.0f, 0.0f }, vxx(vx2(COMPUTER_TASKBAR_HEIGHT))))
					{
						clicked_window_type  = ComputerWindowType::power;
						tm.window_dimensions = { 200.0f, 100.0f };
					}

					if (+clicked_window_type)
					{
						if (clicked_window_type == tm.window_type)
						{
							tm.window_type = ComputerWindowType::null;
						}
						else
						{
							tm.window_type       = clicked_window_type;
							tm.window_velocity   = { 0.0f, 0.0f };
							tm.window_position   = (WIN_DIM - tm.window_dimensions) / 2.0f;
						}
					}
				}
			}
			else if (RELEASED(Input::left_mouse))
			{
				tm.cursor_dragging_window = false;
			}

			if (tm.window_type != ComputerWindowType::null)
			{
				if (tm.cursor_dragging_window)
				{
					tm.window_velocity = tm.cursor_velocity;
				}
				else
				{
					tm.window_velocity *= 0.25f;
				}

				tm.window_position += tm.window_velocity * SECONDS_PER_UPDATE;

				if (tm.window_position.x < 2.0f * COMPUTER_TITLE_BAR_HEIGHT - tm.window_dimensions.x || tm.window_position.x > WIN_DIM.x - COMPUTER_TITLE_BAR_HEIGHT)
				{
					tm.window_position.x = clamp(tm.window_position.x, 2.0f * COMPUTER_TITLE_BAR_HEIGHT - tm.window_dimensions.x, static_cast<f32>(WIN_DIM.x - COMPUTER_TITLE_BAR_HEIGHT));
					tm.window_velocity.x = 0.0f;
				}

				if (tm.window_position.y < COMPUTER_TASKBAR_HEIGHT - tm.window_dimensions.y || tm.window_position.y > WIN_DIM.y - COMPUTER_TITLE_BAR_HEIGHT - tm.window_dimensions.y)
				{
					tm.window_position.y = clamp(tm.window_position.y, COMPUTER_TASKBAR_HEIGHT - tm.window_dimensions.y, WIN_DIM.y - COMPUTER_TITLE_BAR_HEIGHT - tm.window_dimensions.y);
					tm.window_velocity.y = 0.0f;
				}
			}
		} break;

		case StateContext::game:
		{
			Mix_VolumeChunk(state->game.audio.drone    , static_cast<i32>(MIX_MAX_VOLUME *         state->game.ceiling_lights_keytime  * (1.0f - state->game.exiting_keytime)));
			Mix_VolumeChunk(state->game.audio.drone_low, static_cast<i32>(MIX_MAX_VOLUME * (1.0f - state->game.ceiling_lights_keytime) * (1.0f - state->game.exiting_keytime)));

			f32 circuit_breaker_volume = 2.0f / (norm(ray_to_closest(state->game.lucia_position.xy, get_position_of_wall_side(state->game.circuit_breaker_wall_side, 0.25f))) + 0.25f);
			Mix_VolumeChunk(state->game.audio.generator, static_cast<i32>(MIX_MAX_VOLUME * clamp(circuit_breaker_volume, 0.0f, 1.0f)));

			state->game.entering_keytime = clamp(state->game.entering_keytime + SECONDS_PER_UPDATE / 1.0f, 0.0f, 1.0f);

			if (PRESSED(Input::tab))
			{
				if (state->game.hud.type == HudType::inventory)
				{
					state->game.hud.type = HudType::null;
				}
				else
				{
					if (state->game.hud.type == HudType::circuit_breaker)
					{
						Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.panel_close, 0);
					}

					state->game.hud.type                    = HudType::inventory;
					state->game.hud.inventory.selected_item = 0;
					state->game.hud.inventory.grabbing      = false;
				}
			}

			if (state->game.hud.type != HudType::null)
			{
				state->game.hud.cursor_velocity += platform->cursor_delta * 16.0f;
				state->game.hud.cursor_velocity *= 0.3f;
				state->game.hud.cursor          += state->game.hud.cursor_velocity * SECONDS_PER_UPDATE;
				if (state->game.hud.cursor.x < 0.0f || state->game.hud.cursor.x > VIEW_RES.x)
				{
					state->game.hud.cursor.x          = clamp(state->game.hud.cursor.x, 0.0f, static_cast<f32>(VIEW_RES.x));
					state->game.hud.cursor_velocity.x = 0.0f;
				}
				if (state->game.hud.cursor.y < 0.0f || state->game.hud.cursor.y > VIEW_RES.y)
				{
					state->game.hud.cursor.y          = clamp(state->game.hud.cursor.y, 0.0f, static_cast<f32>(VIEW_RES.y));
					state->game.hud.cursor_velocity.y = 0.0f;
				}
			}

			switch (state->game.hud.type)
			{
				case HudType::inventory:
				{
					if (PRESSED(Input::left_mouse))
					{
						constexpr vf2 INVENTORY_HUD_DIMENSIONS = vf2 { static_cast<f32>(ARRAY_CAPACITY(state->game.hud.inventory.array[0])), static_cast<f32>(ARRAY_CAPACITY(state->game.hud.inventory.array)) } * (INVENTORY_DIM + INVENTORY_PADDING);
						if (in_rect(state->game.hud.cursor, VIEW_RES / 2.0f - INVENTORY_HUD_DIMENSIONS / 2.0f, INVENTORY_HUD_DIMENSIONS))
						{
							state->game.hud.inventory.click_position = state->game.hud.cursor;

							FOR_RANGE(y, ARRAY_CAPACITY(state->game.hud.inventory.array)) // @TODO@ Optimize this
							{
								FOR_RANGE(x, ARRAY_CAPACITY(state->game.hud.inventory.array[y]))
								{
									if
									(
										in_rect
										(
											state->game.hud.cursor,
											VIEW_RES / 2.0f + vf2 { x - ARRAY_CAPACITY(state->game.hud.inventory.array[y]) / 2.0f, ARRAY_CAPACITY(state->game.hud.inventory.array) / 2.0f - y - 1.0f } * (INVENTORY_DIM + INVENTORY_PADDING),
											{ INVENTORY_DIM, INVENTORY_DIM }
										)
									)
									{
										if (state->game.hud.inventory.array[y][x].type != ItemType::null)
										{
											state->game.hud.inventory.selected_item = &state->game.hud.inventory.array[y][x];
										}

										break;
									}
								}
							}
						}
						else
						{
							state->game.hud.type = HudType::null;
						}
					}

					if (state->game.hud.inventory.selected_item)
					{
						if (HOLDING(Input::left_mouse))
						{
							if (!state->game.hud.inventory.grabbing && norm_sq(state->game.hud.cursor - state->game.hud.inventory.click_position) > 25.0f)
							{
								state->game.hud.inventory.grabbing = true;
							}
						}
						else if (RELEASED(Input::left_mouse))
						{
							if (state->game.hud.inventory.grabbing)
							{
								if
								(
									fabs(state->game.hud.cursor.x * 2.0f - VIEW_RES.x) < ARRAY_CAPACITY(state->game.hud.inventory.array[0]) * (INVENTORY_DIM + INVENTORY_PADDING) &&
									fabs(state->game.hud.cursor.y * 2.0f - VIEW_RES.y) < ARRAY_CAPACITY(state->game.hud.inventory.array   ) * (INVENTORY_DIM + INVENTORY_PADDING)
								)
								{
									FOR_RANGE(y, ARRAY_CAPACITY(state->game.hud.inventory.array))
									{
										FOR_RANGE(x, ARRAY_CAPACITY(state->game.hud.inventory.array[y]))
										{
											// @TODO@ Simplify
											if
											(
												fabsf(VIEW_RES.x / 2.0f + (x + (1.0f - ARRAY_CAPACITY(state->game.hud.inventory.array[y])) / 2.0f) * (INVENTORY_DIM + INVENTORY_PADDING) - state->game.hud.cursor.x) < INVENTORY_DIM / 2.0f &&
												fabsf(VIEW_RES.y / 2.0f - (y + (1.0f - ARRAY_CAPACITY(state->game.hud.inventory.array   )) / 2.0f) * (INVENTORY_DIM + INVENTORY_PADDING) - state->game.hud.cursor.y) < INVENTORY_DIM / 2.0f
											)
											{
												if (&state->game.hud.inventory.array[y][x] != state->game.hud.inventory.selected_item)
												{
													if (state->game.hud.inventory.array[y][x].type == ItemType::null)
													{
														// @NOTE@ Item move.

														state->game.hud.inventory.array[y][x]        = *state->game.hud.inventory.selected_item;
														state->game.hud.inventory.selected_item->type = ItemType::null;

														FOR_ELEMS(it, state->game.holdings)
														{
															if (*it == state->game.hud.inventory.selected_item)
															{
																*it = &state->game.hud.inventory.array[y][x];
																break;
															}
														}
													}
													else
													{
														// @NOTE@ Item combine.

														bool32 combined        = false;
														Item   combined_result = {};

														{
															Item* cheap_batteries;
															Item* flashlight;
															if (check_combine(&cheap_batteries, ItemType::cheap_batteries, &flashlight, ItemType::flashlight, &state->game.hud.inventory.array[y][x], state->game.hud.inventory.selected_item))
															{
																combined = true;

																Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.eletronical, 0);
																state->game.notification_message = "(You replaced the batteries in the flashlight.)";
																state->game.notification_keytime = 1.0f;

																combined_result = *flashlight;
																combined_result.flashlight.power = 1.0f;

																if (state->game.holding.flashlight == state->game.hud.inventory.selected_item)
																{
																	state->game.holding.flashlight = &state->game.hud.inventory.array[y][x];
																}
															}
															else
															{
																state->game.notification_message = "\"I'm not sure how these fit.\"";
																state->game.notification_keytime = 1.0f;
															}
														}

														if (combined)
														{
															state->game.hud.inventory.selected_item->type = ItemType::null;
															state->game.hud.inventory.array[y][x]          = combined_result;
														}
													}
												}

												goto DONE_SEARCH;
											}
										}
									}

									DONE_SEARCH:;
								}
								else
								{
									// @NOTE@ Item drop.

									Item* dropped = allocate_item(state);
									*dropped = *state->game.hud.inventory.selected_item;
									dropped->position = state->game.lucia_position;
									dropped->velocity = polar(state->game.lucia_angle + rng(&state->seed, -0.5f, 0.5f) * 1.0f) * rng(&state->seed, 2.5f, 6.0f);

									state->game.hud.inventory.selected_item->type = ItemType::null;

									FOR_ELEMS(it, state->game.holdings)
									{
										if (*it == state->game.hud.inventory.selected_item)
										{
											*it = 0;
											break;
										}
									}
								}
							}
							else
							{
								// @NOTE@ Item use.

								switch (state->game.hud.inventory.selected_item->type)
								{
									case ItemType::cheap_batteries:
									{
										state->game.notification_message = "\"Some batteries. They feel cheap.\"";
										state->game.notification_keytime = 1.0f;
									} break;

									case ItemType::paper:
									{
										Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.pick_up_paper, 0);
										state->game.hud.type         = HudType::paper;
										state->game.hud.paper        = {};
										state->game.hud.paper.index  = state->game.hud.inventory.selected_item->paper.index;
										state->game.hud.paper.scalar = PAPER_DATA[state->game.hud.paper.index].min_scalar;
									} break;

									case ItemType::flashlight:
									{
										Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.switch_toggle, 0);
										if (state->game.hud.inventory.selected_item == state->game.holding.flashlight)
										{
											state->game.holding.flashlight = 0;
										}
										else if (state->game.hud.inventory.selected_item->flashlight.power)
										{
											state->game.holding.flashlight = state->game.hud.inventory.selected_item;
										}
										else
										{
											state->game.notification_message = "\"The flashlight is dead.\"";
											state->game.notification_keytime = 1.0f;
										}
									};
								}

								if (state->game.hud.type == HudType::inventory)
								{
									state->game.hud.type = HudType::null;
								}
							}

							state->game.hud.inventory.selected_item = 0;
							state->game.hud.inventory.grabbing      = false;
						}
					}
				} break;

				case HudType::paper:
				{
					// @TODO@ Clean up
					if
					(
						PRESSED(Input::left_mouse) &&
						!in_rect
						(
							state->game.hud.cursor,
							VIEW_RES / 2.0f + (state->game.hud.paper.delta_position - state->game.texture_sprite.papers[state->game.hud.paper.index].image.dim / 2.0f) * state->game.hud.paper.scalar,
							state->game.texture_sprite.papers[state->game.hud.paper.index].image.dim * state->game.hud.paper.scalar
						)
					)
					{
						state->game.hud.type = HudType::null;
					}
					else
					{
						state->game.hud.paper.scalar_velocity += platform->scroll * 2.0f;
						state->game.hud.paper.scalar_velocity *= 0.5f;

						if (HOLDING(Input::left_mouse))
						{
							state->game.hud.paper.velocity        = state->game.hud.cursor_velocity;
							state->game.hud.paper.scalar_velocity = 0.0f;
						}
						else
						{
							state->game.hud.paper.velocity *= 0.5f;
						}

						state->game.hud.paper.scalar = state->game.hud.paper.scalar + state->game.hud.paper.scalar_velocity * state->game.hud.paper.scalar * SECONDS_PER_UPDATE;

						if (PAPER_DATA[state->game.hud.paper.index].min_scalar > state->game.hud.paper.scalar || state->game.hud.paper.scalar > PAPER_DATA[state->game.hud.paper.index].max_scalar)
						{
							state->game.hud.paper.scalar          = clamp(state->game.hud.paper.scalar, PAPER_DATA[state->game.hud.paper.index].min_scalar, PAPER_DATA[state->game.hud.paper.index].max_scalar);
							state->game.hud.paper.scalar_velocity = 0.0f;
						}

						state->game.hud.paper.delta_position += state->game.hud.paper.velocity / state->game.hud.paper.scalar * SECONDS_PER_UPDATE;

						constexpr f32 PAPER_MARGIN = 25.0f;
						vf2 region =
							vf2 {
								(VIEW_RES.x + state->game.texture_sprite.papers[state->game.hud.paper.index].image.dim.x * state->game.hud.paper.scalar) / 2.0f - PAPER_MARGIN,
								(VIEW_RES.y + state->game.texture_sprite.papers[state->game.hud.paper.index].image.dim.y * state->game.hud.paper.scalar) / 2.0f - PAPER_MARGIN
							} / state->game.hud.paper.scalar;

						if (fabsf(state->game.hud.paper.delta_position.x) > region.x)
						{
							state->game.hud.paper.delta_position.x = clamp(state->game.hud.paper.delta_position.x, -region.x, region.x);
							state->game.hud.paper.velocity.x       = 0.0f;
						}
						if (fabsf(state->game.hud.paper.delta_position.y) > region.y)
						{
							state->game.hud.paper.delta_position.y = clamp(state->game.hud.paper.delta_position.y, -region.y, region.y);
							state->game.hud.paper.velocity.y       = 0.0f;
						}
					}
				} break;

				case HudType::circuit_breaker:
				{
					if (PRESSED(Input::left_mouse))
					{
						if (in_rect(state->game.hud.cursor, (VIEW_RES - CIRCUIT_BREAKER_HUD_DIMENSIONS) / 2.0f, CIRCUIT_BREAKER_HUD_DIMENSIONS))
						{
							if (state->game.hud.circuit_breaker.active_voltage != state->game.hud.circuit_breaker.goal_voltage)
							{
								vi2 breaker_switch_position =
									vxx(vf2 {
										roundf((state->game.hud.cursor.x - (VIEW_RES.x - CIRCUIT_BREAKER_HUD_DIMENSIONS.x + CIRCUIT_BREAKER_SWITCH_DIMENSIONS.x) / 2.0f - CIRCUIT_BREAKER_SWITCH_PADDINGS.x       ) / (CIRCUIT_BREAKER_SWITCH_DIMENSIONS.x + CIRCUIT_BREAKER_SWITCH_PADDINGS.x)),
										roundf((state->game.hud.cursor.y - (VIEW_RES.y - CIRCUIT_BREAKER_HUD_DIMENSIONS.y + CIRCUIT_BREAKER_SWITCH_DIMENSIONS.y) / 2.0f - CIRCUIT_BREAKER_SWITCH_PADDINGS.y / 2.0f) / (CIRCUIT_BREAKER_SWITCH_DIMENSIONS.y + CIRCUIT_BREAKER_SWITCH_PADDINGS.y))
									});

								if
								(
									IN_RANGE(breaker_switch_position.x, 0, ARRAY_CAPACITY(state->game.hud.circuit_breaker.switches[0])) &&
									IN_RANGE(breaker_switch_position.y, 0, ARRAY_CAPACITY(state->game.hud.circuit_breaker.switches   )) &&
									in_rect
									(
										state->game.hud.cursor,
										(VIEW_RES - CIRCUIT_BREAKER_HUD_DIMENSIONS) / 2.0f
											+ vf2
											{
												(CIRCUIT_BREAKER_SWITCH_DIMENSIONS.x + CIRCUIT_BREAKER_SWITCH_PADDINGS.x) * breaker_switch_position.x + CIRCUIT_BREAKER_SWITCH_PADDINGS.x,
												(CIRCUIT_BREAKER_SWITCH_DIMENSIONS.y + CIRCUIT_BREAKER_SWITCH_PADDINGS.y) * breaker_switch_position.y + CIRCUIT_BREAKER_SWITCH_PADDINGS.y / 2.0f
											},
										CIRCUIT_BREAKER_SWITCH_DIMENSIONS
									)
								)
								{
									Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.circuit_breaker_switch, 0);

									aliasing breaker_switch = state->game.hud.circuit_breaker.switches[breaker_switch_position.y][breaker_switch_position.x];
									if (breaker_switch.active)
									{
										breaker_switch.active                           = false;
										state->game.hud.circuit_breaker.active_voltage -= breaker_switch.voltage;
									}
									else
									{
										breaker_switch.active                           = true;
										state->game.hud.circuit_breaker.active_voltage += breaker_switch.voltage;
									}

									if (state->game.hud.circuit_breaker.active_voltage == state->game.hud.circuit_breaker.goal_voltage)
									{
										Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.drone_on, 0);
										state->game.goal = GameGoal::escape;
									}
								}
							}
						}
						else
						{
							Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.panel_close, 0);
							state->game.hud.type = HudType::null;
						}
					}
				} break;

				default:
				{
					state->game.hud.cursor_velocity = { 0.0f, 0.0f };
					state->game.hud.cursor          = VIEW_RES / 2.0f;

					state->game.lucia_angle_velocity -= platform->cursor_delta.x * 0.01f / SECONDS_PER_UPDATE;

					if (PRESSED(Input::space) || PRESSED(Input::left_mouse))
					{
						switch (state->game.hand_on_state)
						{
							case HandOnState::door:
							{
								if (state->game.goal == GameGoal::escape)
								{
									boot_down_state(state);
									state->context = StateContext::end;
									state->end     = {};
									boot_up_state(platform->renderer, state);
								}
								else
								{
									Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.door_budge, 0);

									state->game.notification_message =
										rng(&state->seed) < 0.1f
											? "\"Door's jammed, damn it! Wait, is it electrically powered? Weird gameplay.\""
											: "\"I need to find a way to turn the power back on.\"";
									state->game.notification_keytime = 1.0f;
								}
							} break;

							case HandOnState::circuit_breaker:
							{
								Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.panel_open, 0);
								state->game.hud.type = HudType::circuit_breaker;
							} break;

							case HandOnState::item:
							{
								Item* open_space = 0;
								FOR_ELEMS(it, *state->game.hud.inventory.array, sizeof(state->game.hud.inventory.array) / sizeof(Item))
								{
									if (it->type == ItemType::null)
									{
										open_space = it;
										break;
									}
								}

								if (open_space)
								{
									switch (state->game.hand_hovered_item->type)
									{
										case ItemType::paper:
										{
											Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.pick_up_paper, 0);
										} break;

										case ItemType::cheap_batteries:
										case ItemType::military_grade_batteries:
										{
											Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.eletronical, 0);
										} break;

										default:
										{
											Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.pick_up_heavy, 0);
										} break;
									}

									*open_space = *state->game.hand_hovered_item;
									deallocate_item(state, state->game.hand_hovered_item);

									state->game.hand_on_state     = HandOnState::null;
									state->game.hand_hovered_item = 0;
								}
								else
								{
									state->game.notification_message = "\"I can't carry any more.\"";
									state->game.notification_keytime = 1.0f;
								}
							} break;
						}
					}
				} break;
			}

			state->game.lucia_angle_velocity *= 0.4f;
			state->game.lucia_angle           = mod(state->game.lucia_angle + state->game.lucia_angle_velocity * SECONDS_PER_UPDATE, TAU);

			vf2 wasd = { 0.0f, 0.0f };

			if (state->game.hud.type != HudType::circuit_breaker)
			{
				if (HOLDING(Input::s)) { wasd.x -= 1.0f; }
				if (HOLDING(Input::w)) { wasd.x += 1.0f; }
				if (HOLDING(Input::d)) { wasd.y -= 1.0f; }
				if (HOLDING(Input::a)) { wasd.y += 1.0f; }
				if (+wasd)
				{
					constexpr f32 MIN_PORTION = 0.7f;
					state->game.lucia_velocity += rotate(normalize(wasd), state->game.lucia_angle) * 2.0f * ((1.0f - powf(1.0f - state->game.lucia_stamina, 8) + MIN_PORTION) / (1.0f + MIN_PORTION));
				}
			}

			if (+wasd && HOLDING(Input::shift) && !state->game.lucia_out_of_breath)
			{
				state->game.lucia_velocity       *= 0.75f;
				state->game.lucia_stamina         = clamp(state->game.lucia_stamina - SECONDS_PER_UPDATE / 60.0f * (1.0f + (1.0f - square(1.0f - 2.0f * state->game.lucia_sprint_keytime)) * 4.0f), 0.0f, 1.0f);
				state->game.lucia_sprint_keytime  = clamp(state->game.lucia_sprint_keytime + SECONDS_PER_UPDATE / 1.5f, 0.0f, 1.0f);
				if (state->game.lucia_stamina == 0.0f)
				{
					state->game.lucia_out_of_breath = true;
				}
				else
				{
					state->game.lucia_fov = dampen(state->game.lucia_fov, TAU * 0.35f, 2.0f, SECONDS_PER_UPDATE);
				}
			}
			else
			{
				state->game.lucia_velocity       *= 0.6f;
				state->game.lucia_stamina         = clamp(state->game.lucia_stamina + SECONDS_PER_UPDATE / 10.0f * square(1.0f - state->game.lucia_sprint_keytime) * (state->game.lucia_out_of_breath ? 0.5f : 1.0f), 0.0f, 1.0f);
				state->game.lucia_sprint_keytime  = clamp(state->game.lucia_sprint_keytime - SECONDS_PER_UPDATE / 1.0f, 0.0f, 1.0f);

				if (state->game.lucia_out_of_breath)
				{
					if (state->game.lucia_stamina > 0.5f)
					{
						state->game.lucia_out_of_breath = false;
					}
					else
					{
						state->game.lucia_fov = dampen(state->game.lucia_fov, TAU * 0.2f, 1.0f, SECONDS_PER_UPDATE);
					}
				}
				else
				{
					state->game.lucia_fov = dampen(state->game.lucia_fov, TAU * 0.25f, 4.0f, SECONDS_PER_UPDATE);
				}
			}

			FOR_ELEMS(it, state->game.item_buffer, state->game.item_count)
			{
				it->velocity *= 0.9f;
				if (+it->velocity)
				{
					it->position.xy = move(state, it->position.xy, it->velocity * SECONDS_PER_UPDATE);
				}
				it->position.z = lerp(0.15f, state->game.lucia_position.z, clamp(1.0f - norm_sq(ray_to_closest(state->game.lucia_position.xy, it->position.xy)) / 36.0f, 0.0f, 1.0f)) + sinf(state->time * 3.0f) * 0.025f;
				it->normal     = polar(state->time * 0.7f);
			}

			state->game.lucia_position.xy = move(state, state->game.lucia_position.xy, state->game.lucia_velocity * SECONDS_PER_UPDATE);
			state->game.lucia_position.x  = mod(state->game.lucia_position.x, MAP_DIM * WALL_SPACING);
			state->game.lucia_position.y  = mod(state->game.lucia_position.y, MAP_DIM * WALL_SPACING);

			f32 old_z = state->game.lucia_position.z;
			state->game.lucia_position.z = LUCIA_HEIGHT + 0.1f * (cosf(state->game.lucia_head_bob_keytime * TAU) - 1.0f);

			if (+wasd && old_z > LUCIA_HEIGHT - 0.175f && state->game.lucia_position.z < LUCIA_HEIGHT - 0.175f)
			{
				Mix_PlayChannel
				(
					+AudioChannel::unreserved,
					+wasd && HOLDING(Input::shift) && !state->game.lucia_out_of_breath
						? state->game.audio.run_steps [rng(&state->seed, 0, ARRAY_CAPACITY(state->game.audio.run_steps ))]
						: state->game.audio.walk_steps[rng(&state->seed, 0, ARRAY_CAPACITY(state->game.audio.walk_steps))],
					0
				);
			}

			state->game.lucia_head_bob_keytime = mod(state->game.lucia_head_bob_keytime + 0.001f + 0.35f * norm(state->game.lucia_velocity) * SECONDS_PER_UPDATE, 1.0f);

			switch (state->game.goal)
			{
				case GameGoal::find_door:
				{
					state->game.ceiling_lights_keytime = clamp(state->game.ceiling_lights_keytime + SECONDS_PER_UPDATE / 2.0f, 0.0f, 1.0f);

					vf2 door_position = get_position_of_wall_side(state->game.door_wall_side);
					if (norm(ray_to_closest(state->game.lucia_position.xy, door_position)) < 4.0f && exists_clear_way(state, state->game.lucia_position.xy, door_position))
					{
						Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.drone_off, 0);
						Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.blackout, 0);
						state->game.goal = GameGoal::fix_power;

						FOR_ELEMS(it, state->game.hud.circuit_breaker.flat_switches)
						{
							it->active = false;
						}
						state->game.hud.circuit_breaker.active_voltage = 0;
					}
				} break;

				case GameGoal::fix_power:
				{
					state->game.creepy_sound_countdown -= SECONDS_PER_UPDATE;
					if (state->game.creepy_sound_countdown <= 0.0f)
					{
						Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.creepy_sounds[rng(&state->seed, 0, ARRAY_CAPACITY(state->game.audio.creepy_sounds))], 0);
						state->game.creepy_sound_countdown = rng(&state->seed, CREEPY_SOUND_MIN_TIME, CREEPY_SOUND_MAX_TIME);
					}

					state->game.ceiling_lights_keytime = clamp(state->game.ceiling_lights_keytime - SECONDS_PER_UPDATE / 1.0f, 0.0f, 1.0f);
				} break;

				case GameGoal::escape:
				{
					state->game.ceiling_lights_keytime = clamp(state->game.ceiling_lights_keytime + SECONDS_PER_UPDATE / 2.0f, 0.0f, 1.0f);
				} break;
			}

			state->game.hud.circuit_breaker.interpolated_voltage_velocity += (state->game.hud.circuit_breaker.active_voltage - state->game.hud.circuit_breaker.interpolated_voltage) * 16.0f;
			state->game.hud.circuit_breaker.interpolated_voltage_velocity *= 0.5f;
			state->game.hud.circuit_breaker.interpolated_voltage          += state->game.hud.circuit_breaker.interpolated_voltage_velocity * SECONDS_PER_UPDATE;

			if (state->game.hud.circuit_breaker.interpolated_voltage < 0.0f)
			{
				state->game.hud.circuit_breaker.interpolated_voltage          = 0.0f;
				state->game.hud.circuit_breaker.interpolated_voltage_velocity = 0.0f;
			}

			vi2 current_lucia_coordinates = get_closest_open_path_coordinates(state, state->game.lucia_position.xy);
			if
			(
				!state->game.monster_path && norm(ray_to_closest(state->game.monster_position.xy, state->game.lucia_position.xy)) > 2.0f
				|| state->game.monster_path && current_lucia_coordinates != state->game.monster_path_goal
			)
			{
				vi2 starting_coordinates = get_closest_open_path_coordinates(state, state->game.monster_position.xy);
				state->game.monster_path_goal = current_lucia_coordinates;

				struct PathVertex
				{
					bool32 is_set;
					f32    best_weight;
					vi2    prev_coordinates;
				};

				struct PathQueueNode
				{
					f32            estimated_length;
					vi2            prev_coordinates;
					vi2            coordinates;
					PathQueueNode* next_node;
				};

				PathVertex path_vertices[MAP_DIM * 2][MAP_DIM] = {};
				path_vertices[starting_coordinates.y][starting_coordinates.x].is_set = true;

				PathQueueNode* path_queue = memory_arena_allocate<PathQueueNode>(&state->transient_arena);
				path_queue->estimated_length = path_distance_function(starting_coordinates, state->game.monster_path_goal);
				path_queue->prev_coordinates = { -1, -1 };
				path_queue->coordinates      = starting_coordinates;
				path_queue->next_node        = 0;

				PathQueueNode* available_path_queue_node = 0;

				while (path_queue && path_queue->coordinates != state->game.monster_path_goal)
				{
					PathQueueNode* head = path_queue;
					path_queue = path_queue->next_node;

					struct ADJACENT { WallVoxel side; vi2 delta_coordinates; };
					constexpr ADJACENT HORI[] =
						{
							{ WallVoxel::bottom, { -1, -1 } },
							{ WallVoxel::bottom, {  0, -1 } },
							{ WallVoxel::left  , { -1,  0 } },
							{ WallVoxel::left  , {  1,  0 } },
							{ WallVoxel::bottom, { -1,  1 } },
							{ WallVoxel::bottom, {  0,  1 } }
						};

					constexpr ADJACENT VERT[] =
						{
							{ WallVoxel::bottom, { 0, -2 } },
							{ WallVoxel::left  , { 0, -1 } },
							{ WallVoxel::left  , { 1, -1 } },
							{ WallVoxel::left  , { 0,  1 } },
							{ WallVoxel::left  , { 1,  1 } },
							{ WallVoxel::bottom, { 0,  2 } }
						};

					FOR_ELEMS(it, head->coordinates.y % 2 == 0 ? VERT : HORI)
					{
						vi2 next_coordinates =
							{
								mod(head->coordinates.x + it->delta_coordinates.x, MAP_DIM    ),
								mod(head->coordinates.y + it->delta_coordinates.y, MAP_DIM * 2)
							};

						if (next_coordinates == head->prev_coordinates || +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(next_coordinates)) & it->side))
						{
							continue;
						}

						if (head->coordinates.y % 2 == 0)
						{
							if (it->delta_coordinates.y < 0)
							{
								if
								(
									it->delta_coordinates != vi2 { 1, -1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates) + vi2 { 0, -1 }) & WallVoxel::back_slash   ) ||
									it->delta_coordinates != vi2 { 0, -1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates) + vi2 { 0, -1 }) & WallVoxel::forward_slash)
								)
								{
									continue;
								}
							}
							else if
							(
								it->delta_coordinates != vi2 { 0, 1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates)) & WallVoxel::back_slash   ) ||
								it->delta_coordinates != vi2 { 1, 1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates)) & WallVoxel::forward_slash)
							)
							{
								continue;
							}
						}
						else if (it->delta_coordinates.x < 0)
						{
							if
							(
								it->delta_coordinates != vi2 { -1,  1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates) + vi2 { -1, 0 }) & WallVoxel::back_slash   ) ||
								it->delta_coordinates != vi2 { -1, -1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates) + vi2 { -1, 0 }) & WallVoxel::forward_slash)
							)
							{
								continue;
							}
						}
						else if
						(
							it->delta_coordinates != vi2 { 0, -1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates)) & WallVoxel::back_slash   ) ||
							it->delta_coordinates != vi2 { 0,  1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates)) & WallVoxel::forward_slash)
						)
						{
							continue;
						}

						f32         next_weight = path_vertices[head->coordinates.y][head->coordinates.x].best_weight + path_distance_function(head->coordinates, next_coordinates);
						PathVertex* next_vertex = &path_vertices[next_coordinates.y][next_coordinates.x];

						if (!next_vertex->is_set || next_vertex->best_weight > next_weight)
						{
							next_vertex->is_set           = true;
							next_vertex->best_weight      = next_weight;
							next_vertex->prev_coordinates = head->coordinates;

							PathQueueNode** repeated_node = &path_queue;

							while (*repeated_node && (*repeated_node)->coordinates != next_coordinates)
							{
								repeated_node = &(*repeated_node)->next_node;
							}

							if (*repeated_node)
							{
								PathQueueNode* tail = (*repeated_node)->next_node;
								(*repeated_node)->next_node = available_path_queue_node;
								available_path_queue_node = *repeated_node;
								*repeated_node = tail;
							}

							f32             next_estimated_length = next_weight + path_distance_function(next_coordinates, state->game.monster_path_goal);
							PathQueueNode** post_node             = &path_queue;
							while (*post_node && (*post_node)->estimated_length < next_estimated_length)
							{
								post_node = &(*post_node)->next_node;
							}

							PathQueueNode* new_node;
							if (available_path_queue_node)
							{
								new_node                  = available_path_queue_node;
								available_path_queue_node = available_path_queue_node->next_node;
							}
							else
							{
								new_node = memory_arena_allocate<PathQueueNode>(&state->transient_arena);
							}

							new_node->estimated_length = next_estimated_length;
							new_node->prev_coordinates = head->coordinates;
							new_node->coordinates      = next_coordinates;
							new_node->next_node        = *post_node;
							*post_node = new_node;
						}
					}

					head->next_node = available_path_queue_node;
					available_path_queue_node = head;
				}

				if (path_queue && path_queue->coordinates == state->game.monster_path_goal)
				{
					while (state->game.monster_path)
					{
						state->game.monster_path = deallocate_path_coordinates_node(state, state->game.monster_path);
					}

					vi2 coordinates = state->game.monster_path_goal;
					while (true)
					{
						PathCoordinatesNode* path_coordinates_node = allocate_path_coordinates_node(state);
						path_coordinates_node->coordinates = coordinates;
						path_coordinates_node->next_node   = state->game.monster_path;
						state->game.monster_path = path_coordinates_node;

						if (coordinates == starting_coordinates)
						{
							break;
						}

						coordinates = path_vertices[coordinates.y][coordinates.x].prev_coordinates;
					}
				}
				else
				{
					ASSERT(false);
				}

				ASSERT(get_closest_open_path_coordinates(state, state->game.lucia_position.xy) == state->game.monster_path_goal);
			}

			if (state->game.monster_path)
			{
				vf2 ray = ray_to_closest(state->game.monster_position.xy, path_coordinates_to_position(state->game.monster_path->coordinates));
				if (norm(ray) < WALL_SPACING / 2.0f)
				{
					state->game.monster_path = deallocate_path_coordinates_node(state, state->game.monster_path);
				}
				else
				{
					state->game.monster_velocity = dampen(state->game.monster_velocity, normalize(ray) * 3.0f, 4.0f, SECONDS_PER_UPDATE);
				}
			}
			else
			{
				vf2 ray = ray_to_closest(state->game.monster_position.xy, state->game.lucia_position.xy);
				if (norm(ray) > 4.0f)
				{
					state->game.monster_velocity = dampen(state->game.monster_velocity, normalize(ray) * 3.0f, 4.0f, SECONDS_PER_UPDATE);
				}
				else
				{
					state->game.monster_velocity = dampen(state->game.monster_velocity, { 0.0f, 0.0f }, 4.0f, SECONDS_PER_UPDATE);
				}
			}

			#if 0
			if (HOLDING(Input::space))
			{
				state->game.monster_position.xy = move(state, state->game.monster_position.xy, state->game.monster_velocity * SECONDS_PER_UPDATE);
			}
			#else
			state->game.monster_position.xy = move(state, state->game.monster_position.xy, state->game.monster_velocity * SECONDS_PER_UPDATE);
			#endif

			state->game.monster_position.x  = mod(state->game.monster_position.x, MAP_DIM * WALL_SPACING);
			state->game.monster_position.y  = mod(state->game.monster_position.y, MAP_DIM * WALL_SPACING);
			state->game.monster_position.z  = cosf(state->time * 3.0f) * 0.15f + WALL_HEIGHT / 2.0f;
			state->game.monster_normal      = normalize(dampen(state->game.monster_normal, normalize(ray_to_closest(state->game.monster_position.xy, state->game.lucia_position.xy)), 8.0f, SECONDS_PER_UPDATE));

			state->game.hand_on_state     = HandOnState::null;
			state->game.hand_hovered_item = 0;

			vf3 hand_reach_position = { NAN, NAN };

			if (state->game.hud.type == HudType::null)
			{
				lambda hand_on_heuristic =
					[&](vf2 position)
					{
						if (!exists_clear_way(state, state->game.lucia_position.xy, position))
						{
							return 0.0f;
						}

						vf2 ray      = ray_to_closest(state->game.lucia_position.xy, position);
						f32 distance = norm(ray);

						if (distance > 1.25f)
						{
							return 0.0f;
						}

						f32 dot_prod = dot(ray / distance, polar(state->game.lucia_angle));
						if (dot_prod < 0.9f)
						{
							return 0.0f;
						}

						return 1.0f / (distance + 0.5f) + square(dot_prod) * 2.0f;
					};

				f32 best_heuristic = 0.0f;

				{
					vf2 door_position = get_position_of_wall_side(state->game.door_wall_side, 0.25f);
					f32 heuristic     = hand_on_heuristic(door_position);
					if (best_heuristic < heuristic)
					{
						best_heuristic            = heuristic;
						state->game.hand_on_state = HandOnState::door;
						hand_reach_position       = vx3(door_position, WALL_HEIGHT / 2.0f);
					}
				}

				{
					vf2 circuit_breaker_position = get_position_of_wall_side(state->game.circuit_breaker_wall_side, 0.25f);
					f32 heuristic     = hand_on_heuristic(circuit_breaker_position);
					if (best_heuristic < heuristic)
					{
						best_heuristic            = heuristic;
						state->game.hand_on_state = HandOnState::circuit_breaker;
						hand_reach_position       = vx3(circuit_breaker_position, WALL_HEIGHT / 2.0f);
					}
				}

				FOR_ELEMS(it, state->game.item_buffer, state->game.item_count)
				{
					f32 heuristic = hand_on_heuristic(it->position.xy);
					if (best_heuristic < heuristic)
					{
						best_heuristic                = heuristic;
						state->game.hand_on_state     = HandOnState::item;
						state->game.hand_hovered_item = it;
						hand_reach_position           = it->position;
					}
				}
			}

			if (state->game.hand_on_state != HandOnState::null)
			{
				state->game.hand_position.xy = hand_reach_position.xy + ray_to_closest(hand_reach_position.xy, state->game.lucia_position.xy) / 2.0f;
				state->game.hand_position.z  = lerp(hand_reach_position.z, state->game.lucia_position.z, 0.75f);
			}

			if (state->game.holding.flashlight)
			{
				state->game.holding.flashlight->flashlight.power = clamp(state->game.holding.flashlight->flashlight.power - SECONDS_PER_UPDATE / 150.0f, 0.0f, 1.0f);
				state->game.flashlight_activation                = dampen(state->game.flashlight_activation, sinf(TAU / 4.0f * (1.0f - powf(1.0f - state->game.holding.flashlight->flashlight.power, 16.0f))), 25.0f, SECONDS_PER_UPDATE);

				if (state->game.holding.flashlight->flashlight.power == 0.0f)
				{
					state->game.holding.flashlight = 0;

					if (state->game.holding.flashlight == state->game.holding.flashlight)
					{
						Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.switch_toggle, 0);
						state->game.notification_message = "\"The flashlight died.\"";
						state->game.notification_keytime = 1.0f;
					}
				}
			}
			else
			{
				state->game.flashlight_activation = dampen(state->game.flashlight_activation, 0.0f, 25.0f, SECONDS_PER_UPDATE);
			}

			state->game.flashlight_keytime += 0.00005f + 0.005f * norm(state->game.lucia_velocity) * SECONDS_PER_UPDATE;
			if (state->game.flashlight_keytime > 1.0f)
			{
				state->game.flashlight_keytime -= 1.0f;
			}

			state->game.flashlight_ray.xy  = dampen(state->game.flashlight_ray.xy, polar(state->game.lucia_angle + sinf(state->game.flashlight_keytime * TAU * 15.0f) * 0.1f), 16.0f, SECONDS_PER_UPDATE);
			state->game.flashlight_ray.z   = sinf(state->game.flashlight_keytime * TAU * 36.0f) * 0.05f;
			state->game.flashlight_ray     = normalize(state->game.flashlight_ray);

			state->game.notification_keytime = clamp(state->game.notification_keytime - SECONDS_PER_UPDATE / 8.0f, 0.0f, 1.0f);

			state->game.heart_rate_bpm = 63.5f + 80.0f * (1.0f - state->game.lucia_stamina);
			if (state->game.heart_rate_bpm > 0.001f)
			{
				state->game.heart_rate_beat_keytime += SECONDS_PER_UPDATE * state->game.heart_rate_bpm / 60.0f;
			}

			Mix_VolumeChunk(state->game.audio.heartbeats[state->game.heartbeat_sfx_index], static_cast<i32>(MIX_MAX_VOLUME * clamp((state->game.heart_rate_bpm - 50.0f) / 80.0f, 0.0f, 1.0f)));

			if (state->game.heart_rate_beat_keytime >= 1.0f)
			{
				Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.heartbeats[state->game.heartbeat_sfx_index], 0);

				state->game.heart_rate_beat_keytime  = 0.0f;
				state->game.heart_rate_velocity      = 64.0f;
				state->game.heartbeat_sfx_index      = (state->game.heartbeat_sfx_index + 1) % ARRAY_CAPACITY(state->game.audio.heartbeats);
			}

			state->game.heart_rate_velocity      -= state->game.heart_rate_current_value * 640.0f * SECONDS_PER_UPDATE;
			state->game.heart_rate_velocity      *= 0.5f;
			state->game.heart_rate_current_value += state->game.heart_rate_velocity * SECONDS_PER_UPDATE;

			state->game.heart_rate_update_keytime += SECONDS_PER_UPDATE / 0.05f;
			if (state->game.heart_rate_update_keytime >= 1.0f)
			{
				state->game.heart_rate_update_keytime                        = 0.0f;
				state->game.heart_rate_values[state->game.heart_rate_index]  = state->game.heart_rate_current_value;
				state->game.heart_rate_index                                 = (state->game.heart_rate_index + 1) % ARRAY_CAPACITY(state->game.heart_rate_values);
			}

			if (state->game.holding.flashlight)
			{
				state->game.hud.status.battery_display_keytime = min(1.0f, state->game.hud.status.battery_display_keytime + SECONDS_PER_UPDATE / 0.25f);
				state->game.hud.status.battery_level_keytime   = dampen(state->game.hud.status.battery_level_keytime, state->game.holding.flashlight->flashlight.power, 8.0f, SECONDS_PER_UPDATE);
			}
			else
			{
				state->game.hud.status.battery_display_keytime = max(0.0f, state->game.hud.status.battery_display_keytime - SECONDS_PER_UPDATE / 0.25f);
				state->game.hud.status.battery_level_keytime   = dampen(state->game.hud.status.battery_level_keytime, 0.0f, 8.0f, SECONDS_PER_UPDATE);
			}

			age_animated_sprite(&state->game.animated_sprite.fire, SECONDS_PER_UPDATE);
		} break;

		case StateContext::end:
		{
			if (state->end.is_exiting)
			{
				state->end.exiting_keytime += SECONDS_PER_UPDATE / 1.0f;
				if (state->end.exiting_keytime >= 1.0f)
				{
					return UpdateCode::terminate;
				}
			}
			else
			{
				state->end.entering_keytime = clamp(state->end.entering_keytime + SECONDS_PER_UPDATE / 2.0f, 0.0f, 1.0f);

				if (state->end.entering_keytime == 1.0f)
				{
					if (PRESSED(Input::space))
					{
						Mix_PlayChannel(+AudioChannel::r2, state->end.audio.shooting, 0);
						state->end.is_exiting = true;
					}
				}
			}
		} break;
	}

	return UpdateCode::resume;
}

extern "C" PROTOTYPE_RENDER(render)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	state->transient_arena.used = 0;

	f32 blackout = 0.0f;

	switch (state->context)
	{
		case StateContext::title_menu:
		{
			aliasing tm = state->title_menu;

			SDL_RenderClear(platform->renderer);

			render_texture(platform->renderer, tm.texture.desktop, { 0.0f, 0.0f }, vxx(WIN_DIM));

			lambda draw_icon =
				[&](SDL_Texture* texture, vf2 position, strlit name)
				{
					render_texture(platform->renderer, texture, { position.x, WIN_DIM.y - COMPUTER_ICON_DIM - position.y }, { COMPUTER_ICON_DIM, COMPUTER_ICON_DIM });
					render_text
					(
						platform->renderer,
						state->font.major,
						{ position.x + COMPUTER_ICON_DIM / 2.0f, WIN_DIM.y - position.y },
						-0.75f,
						FC_ALIGN_CENTER,
						0.5f,
						{ 1.0f, 1.0f, 1.0f, 1.0f },
						"%s",
						name
					);
				};

			draw_icon(tm.texture.text_file       , vxx(COMPUTER_CREDITS_POSITION         ), "credits.txt");
			draw_icon(tm.texture.antihome_program, vxx(COMPUTER_ANTIHOME_PROGRAM_POSITION), "Antihome");

			if (+tm.window_type)
			{
				set_color(platform->renderer, monochrome(0.5f));
				render_filled_rect(platform->renderer, vf2 { tm.window_position.x, WIN_DIM.y - COMPUTER_TITLE_BAR_HEIGHT - tm.window_position.y - tm.window_dimensions.y }, { tm.window_dimensions.x, COMPUTER_TITLE_BAR_HEIGHT });

				render_texture
				(
					platform->renderer,
					tm.texture.window_close,
					{
						tm.window_position.x + tm.window_dimensions.x - COMPUTER_TITLE_BAR_HEIGHT,
						WIN_DIM.y - COMPUTER_TITLE_BAR_HEIGHT - tm.window_position.y - tm.window_dimensions.y,
					},
					{ COMPUTER_TITLE_BAR_HEIGHT, COMPUTER_TITLE_BAR_HEIGHT }
				);

				switch (tm.window_type)
				{
					case ComputerWindowType::credits:
					{
						set_color(platform->renderer, { 0.75f, 0.65f, 0.2f, 1.0f });
						render_filled_rect(platform->renderer, { tm.window_position.x, WIN_DIM.y - tm.window_dimensions.y - tm.window_position.y }, tm.window_dimensions);

						render_boxed_text
						(
							platform->renderer,
							state->font.minor,
							{ tm.window_position.x + 5.0f, WIN_DIM.y - tm.window_dimensions.y - tm.window_position.y + 5.0f },
							tm.window_dimensions - vf2 { 5.0f, 5.0f } * 2.0f,
							FC_ALIGN_LEFT,
							1.0f,
							{ 1.0f, 1.0f, 1.0f, 1.0f },
							"Programming done by Phuc Doan.\n\n"
							"Voice acting performed by Ren Stolebarger.\n\n"
							"Art drawn by Mila Matthews.\n\n"
							"Majority of sounds possibly stolen from SCP:CB."
						);
					} break;

					case ComputerWindowType::antihome_program:
					{
						set_color(platform->renderer, monochrome(0.25f));
						render_filled_rect(platform->renderer, { tm.window_position.x, WIN_DIM.y - tm.window_dimensions.y - tm.window_position.y }, tm.window_dimensions);

						render_text
						(
							platform->renderer,
							state->font.major,
							{ tm.window_position.x + tm.window_dimensions.x * 0.5f, WIN_DIM.y - tm.window_position.y - tm.window_dimensions.y * 0.7f },
							0.5f,
							FC_ALIGN_CENTER,
							1.25f,
							{ 1.0f, 1.0f, 1.0f, 1.0f },
							"ANTIHOME"
						);

						set_color(platform->renderer, { 1.0f, 0.0f, 0.0f, 1.0f });
						render_filled_rect(platform->renderer, vf2 { tm.window_position.x + tm.window_dimensions.x * 0.5f, WIN_DIM.y - tm.window_position.y - tm.window_dimensions.y * 0.33f } - COMPUTER_BUTTON_DIMENSIONS / 2.0f, vxx(COMPUTER_BUTTON_DIMENSIONS));
						render_text
						(
							platform->renderer,
							state->font.major,
							{ tm.window_position.x + tm.window_dimensions.x * 0.5f, WIN_DIM.y - tm.window_position.y - tm.window_dimensions.y * 0.33f },
							0.5f,
							FC_ALIGN_CENTER,
							0.55f,
							{ 1.0f, 1.0f, 1.0f, 1.0f },
							"Lure"
						);
					} break;

					case ComputerWindowType::power:
					{
						set_color(platform->renderer, monochrome(0.8f));
						render_filled_rect(platform->renderer, { tm.window_position.x, WIN_DIM.y - tm.window_dimensions.y - tm.window_position.y }, tm.window_dimensions);

						render_text
						(
							platform->renderer,
							state->font.major,
							{ tm.window_position.x + tm.window_dimensions.x * 0.5f, WIN_DIM.y - tm.window_position.y - tm.window_dimensions.y * 0.75f },
							0.5f,
							FC_ALIGN_CENTER,
							0.8f,
							{ 1.0f, 1.0f, 1.0f, 1.0f },
							"Quit?"
						);

						set_color(platform->renderer, monochrome(0.6f));
						render_filled_rect(platform->renderer, vf2 { tm.window_position.x + tm.window_dimensions.x * 0.25f, WIN_DIM.y - tm.window_position.y - tm.window_dimensions.y * 0.3f } - COMPUTER_BUTTON_DIMENSIONS / 2.0f, vxx(COMPUTER_BUTTON_DIMENSIONS));

						render_text
						(
							platform->renderer,
							state->font.major,
							{ tm.window_position.x + tm.window_dimensions.x * 0.25f, WIN_DIM.y - tm.window_position.y - tm.window_dimensions.y * 0.3f },
							0.5f,
							FC_ALIGN_CENTER,
							0.6f,
							{ 1.0f, 1.0f, 1.0f, 1.0f },
							"Yes"
						);

						set_color(platform->renderer, monochrome(0.6f));
						render_filled_rect(platform->renderer, vf2 { tm.window_position.x + tm.window_dimensions.x * 0.75f, WIN_DIM.y - tm.window_position.y - tm.window_dimensions.y * 0.3f } - COMPUTER_BUTTON_DIMENSIONS / 2.0f, vxx(COMPUTER_BUTTON_DIMENSIONS));

						render_text
						(
							platform->renderer,
							state->font.major,
							{ tm.window_position.x + tm.window_dimensions.x * 0.75f, WIN_DIM.y - tm.window_position.y - tm.window_dimensions.y * 0.3f },
							0.5f,
							FC_ALIGN_CENTER,
							0.6f,
							{ 1.0f, 1.0f, 1.0f, 1.0f },
							"No"
						);
					} break;
				}
			}

			set_color(platform->renderer, monochrome(0.3f));
			render_filled_rect(platform->renderer, { 0.0f, static_cast<f32>(WIN_DIM.y - COMPUTER_TASKBAR_HEIGHT) }, { static_cast<f32>(WIN_DIM.x), static_cast<f32>(COMPUTER_TASKBAR_HEIGHT) });

			render_texture
			(
				platform->renderer,
				state->title_menu.texture.power_button,
				{ 0.0f, static_cast<f32>(WIN_DIM.y - COMPUTER_TASKBAR_HEIGHT) },
				{ COMPUTER_TASKBAR_HEIGHT, COMPUTER_TASKBAR_HEIGHT }
			);

			render_texture
			(
				platform->renderer,
				state->title_menu.texture.cursor,
				{ state->title_menu.cursor.x, WIN_DIM.y - state->title_menu.cursor.y },
				{ 12.0f, 24.0f }
			);
		} break;

		case StateContext::game:
		{
			set_color(platform->renderer, monochrome(0.0f));
			SDL_RenderClear(platform->renderer);

			u32* view_pixels;
			i32  view_pitch_;
			SDL_LockTexture(state->game.texture.view, 0, reinterpret_cast<void**>(&view_pixels), &view_pitch_);

			DEBUG_begin_profiling(FRAME);
			FOR_RANGE(x, VIEW_RES.x)
			{
				vf2 ray_horizontal = polar(state->game.lucia_angle + (0.5f - static_cast<f32>(x) / VIEW_RES.x) * state->game.lucia_fov);

				WallSide  ray_casted_wall_side       = {};
				f32       wall_distance              = NAN;
				f32       wall_portion               = NAN;
				i32       wall_starting_y            = 0;
				i32       wall_ending_y              = 0;
				Image*    wall_overlay               = 0;
				vf2       wall_overlay_uv_position   = { NAN, NAN };
				vf2       wall_overlay_uv_dimensions = { NAN, NAN };
				bool32    wall_in_light              = true;

				{
					vi2 step    = { sign(ray_horizontal.x), sign(ray_horizontal.y) };
					vf2 t_delta = vf2 { step.x / ray_horizontal.x, step.y / ray_horizontal.y } * WALL_SPACING;
					vf2 t_max   =
						{
							(floorf(state->game.lucia_position.x / WALL_SPACING + (ray_horizontal.x >= 0.0f)) * WALL_SPACING - state->game.lucia_position.x) / ray_horizontal.x,
							(floorf(state->game.lucia_position.y / WALL_SPACING + (ray_horizontal.y >= 0.0f)) * WALL_SPACING - state->game.lucia_position.y) / ray_horizontal.y
						};

					ray_casted_wall_side.coordinates =
						{
							static_cast<i32>(floorf(state->game.lucia_position.x / WALL_SPACING)),
							static_cast<i32>(floorf(state->game.lucia_position.y / WALL_SPACING))
						};

					FOR_RANGE(MAP_DIM * MAP_DIM)
					{
						FOR_ELEMS(voxel_data, WALL_VOXEL_DATA)
						{
							if (+(*get_wall_voxel(state, ray_casted_wall_side.coordinates) & voxel_data->voxel))
							{
								f32 distance;
								f32 portion;
								if
								(
									ray_cast_line
									(
										&distance,
										&portion,
										state->game.lucia_position.xy,
										ray_horizontal,
										(ray_casted_wall_side.coordinates + voxel_data->start) * WALL_SPACING,
										(ray_casted_wall_side.coordinates + voxel_data->end  ) * WALL_SPACING
									)
									&& IN_RANGE(portion, 0.0f, 1.0f)
									&& (!+ray_casted_wall_side.voxel || distance < wall_distance)
								)
								{
									ray_casted_wall_side.normal = voxel_data->normal * (dot(ray_horizontal, voxel_data->normal) < 0.0f ? 1.0f : -1.0f);
									ray_casted_wall_side.voxel  = voxel_data->voxel;
									wall_distance               = distance;
									wall_portion                = portion;
								}
							}
						}

						if (+ray_casted_wall_side.voxel)
						{
							ray_casted_wall_side.coordinates.x = mod(ray_casted_wall_side.coordinates.x, MAP_DIM);
							ray_casted_wall_side.coordinates.y = mod(ray_casted_wall_side.coordinates.y, MAP_DIM);

							wall_starting_y = static_cast<i32>(VIEW_RES.y / 2.0f - HORT_TO_VERT_K / state->game.lucia_fov *                state->game.lucia_position.z  / (wall_distance + 0.01f));
							wall_ending_y   = static_cast<i32>(VIEW_RES.y / 2.0f + HORT_TO_VERT_K / state->game.lucia_fov * (WALL_HEIGHT - state->game.lucia_position.z) / (wall_distance + 0.01f));

							if (equal_wall_sides(ray_casted_wall_side, state->game.door_wall_side))
							{
								if (+(state->game.door_wall_side.voxel & (WallVoxel::back_slash | WallVoxel::forward_slash)))
								{
									wall_overlay               = &state->game.image.door;
									wall_overlay_uv_position   = { 0.5f - 0.25f / SQRT2, 0.0f };
									wall_overlay_uv_dimensions = { 0.25f / SQRT2, 0.85f };
								}
								else
								{
									wall_overlay               = &state->game.image.door;
									wall_overlay_uv_position   = { 0.25f, 0.0f };
									wall_overlay_uv_dimensions = { 0.5f, 0.85f };
								}
							}
							else if (equal_wall_sides(ray_casted_wall_side, state->game.circuit_breaker_wall_side))
							{
								// @TODO@ Prevent diagonal stretching.
								wall_overlay               = &state->game.image.circuit_breaker;
								wall_overlay_uv_position   = { 0.35f, 0.25f };
								wall_overlay_uv_dimensions = { 0.30f, 0.50f };
							}
							else
							{
								f32 direction =
									dot
									(
										rotate90(ray_casted_wall_side.normal),
										normalize(ray_to_closest(get_position_of_wall_side(ray_casted_wall_side, 0.0f), get_position_of_wall_side(state->game.door_wall_side, 0.0f)))
									);

								if (dot(ray_horizontal, get_wall_voxel_data(ray_casted_wall_side.voxel)->normal) > 0.0f)
								{
									direction = -direction;
								}

								constexpr f32 THRESHOLD = 0.7f;
								if (rng(static_cast<i32>((ray_casted_wall_side.coordinates.x + ray_casted_wall_side.coordinates.y) * 317 + ray_casted_wall_side.coordinates.y * 171)) < 0.2f)
								{
									if (direction < -THRESHOLD)
									{
										wall_overlay = &state->game.image.wall_left_arrow;
									}
									else if (direction > THRESHOLD)
									{
										wall_overlay = &state->game.image.wall_right_arrow;
									}
								}

								wall_overlay_uv_position   = { 0.0f, 0.0f };
								wall_overlay_uv_dimensions = { 1.0f, 1.0f };
							}

							if (wall_overlay && !IN_RANGE(wall_portion, wall_overlay_uv_position.x, wall_overlay_uv_position.x + wall_overlay_uv_dimensions.x))
							{
								wall_overlay = 0;
							}

							wall_in_light =
								dot(ray_to_closest(state->game.lucia_position.xy + ray_horizontal * wall_distance, state->game.monster_position.xy), ray_casted_wall_side.normal) > 0.0f
									&& exists_clear_way(state, state->game.monster_position.xy, state->game.lucia_position.xy + ray_horizontal * wall_distance * 0.99f);

							break;
						}

						if (t_max.x < t_max.y)
						{
							t_max.x                            += t_delta.x;
							ray_casted_wall_side.coordinates.x += step.x;
						}
						else
						{
							t_max.y                            += t_delta.y;
							ray_casted_wall_side.coordinates.y += step.y;
						}
					}
				}

				enum struct Material : u8
				{
					null,
					wall,
					floor,
					ceiling,
					item,
					monster,
					hand,
					fire
				};

				struct RenderScanNode
				{
					Material        material;
					bool32          in_light;
					Image           image;
					vf2             normal;
					f32             distance;
					f32             portion;
					i32             starting_y;
					i32             ending_y;
					RenderScanNode* next_node;
				};

				memory_arena_checkpoint(&state->transient_arena);
				RenderScanNode* render_scan_node = 0;

				vf2 delta_checks[4];
				{
					vf2 lucia_position_uv = state->game.lucia_position.xy / (MAP_DIM * WALL_SPACING);

					delta_checks[0]  = { 0.0f, 0.0f };
					delta_checks[3]  = vf2 { 2.0f * roundf(lucia_position_uv.x) - 1.0f, 2.0f * roundf(lucia_position_uv.y) - 1.0f } * MAP_DIM * WALL_SPACING;
					delta_checks[1]  = { delta_checks[3].x, 0.0f              };
					delta_checks[2]  = { 0.0f             , delta_checks[3].y };

					if (fabs(lucia_position_uv.x - roundf(lucia_position_uv.x)) > fabs(lucia_position_uv.y - roundf(lucia_position_uv.y)))
					{
						SWAP(&delta_checks[1], &delta_checks[2]);
					}
				}

				constexpr f32 SHADER_INV_EPSILON = 0.9f;

				lambda scan =
					[&](Material material, Image image, vf3 position, vf2 normal, vf2 dimensions)
					{
						FOR_ELEMS(it, delta_checks)
						{
							f32 distance;
							f32 portion;
							if
							(
								ray_cast_line
								(
									&distance,
									&portion,
									state->game.lucia_position.xy,
									ray_horizontal,
									position.xy + *it - rotate90(normal) * dimensions.x / 2.0f,
									position.xy + *it + rotate90(normal) * dimensions.x / 2.0f
								)
								&& (!+ray_casted_wall_side.voxel || wall_distance > distance)
								&& IN_RANGE(portion, 0.0f, 1.0f)
							)
							{
								RenderScanNode** post_node = &render_scan_node;
								while (*post_node && (*post_node)->distance < distance)
								{
									post_node = &(*post_node)->next_node;
								}

								RenderScanNode* new_node = memory_arena_allocate<RenderScanNode>(&state->transient_arena);
								new_node->material   = material;
								new_node->in_light   = exists_clear_way(state, state->game.lucia_position.xy + ray_horizontal * distance * SHADER_INV_EPSILON, state->game.monster_position.xy);
								new_node->image      = image;
								new_node->normal     = normal;
								new_node->distance   = distance;
								new_node->portion    = portion;
								new_node->next_node  = *post_node;
								new_node->starting_y = static_cast<i32>(VIEW_RES.y / 2.0f + HORT_TO_VERT_K / state->game.lucia_fov * (position.z - 0.5f * dimensions.y - state->game.lucia_position.z) / (distance + 0.1f));
								new_node->ending_y   = static_cast<i32>(VIEW_RES.y / 2.0f + HORT_TO_VERT_K / state->game.lucia_fov * (position.z + 0.5f * dimensions.y - state->game.lucia_position.z) / (distance + 0.1f));
								*post_node = new_node;
								break;
							}
						}
					};

				DEBUG_once // @TEMP@
				{
					state->game.lucia_position.xy = get_position_of_wall_side(state->game.door_wall_side, 1.0f);
					//state->game.lucia_position.xy = get_position_of_wall_side(state->game.circuit_breaker_wall_side, 1.0f);
				}

				scan(Material::fire, get_image_of_frame(&state->game.animated_sprite.fire), { 70.0f, 60.0f, WALL_HEIGHT / 2.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f });

				scan(Material::monster, get_image_of_frame(&state->game.animated_sprite.monster), state->game.monster_position, state->game.monster_normal, { 1.0f, 1.0f });

				if (state->game.hand_on_state != HandOnState::null)
				{
					scan(Material::hand, state->game.texture_sprite.hand.image, state->game.hand_position, normalize(ray_to_closest(state->game.hand_position.xy, state->game.lucia_position.xy)), { 0.05f, 0.05f });
				}

				FOR_ELEMS(it, state->game.item_buffer, state->game.item_count)
				{
					scan(Material::item, state->game.texture_sprite.default_items[+it->type - +ItemType::ITEM_START].image, it->position, it->normal, { 0.5f, 0.5f });
				}

				lambda shader =
					[&](Material material, bool32 in_light, vf3 color, vf3 ray, vf3 normal, f32 distance)
					{
						if (material == Material::hand)
						{
							return color;
						}
						else
						{
							f32 ceiling_lights_t = 0.5f - 4.0f * cube(0.5f - state->game.ceiling_lights_keytime);

							vf3 new_color =
								color
									* clamp
										(
											(
												material == Material::item ? 0.8f : 0.3f
													- fabsf(dot(ray, normal)) * 0.01f
													+ ((state->game.lucia_position.z + ray.z * distance) / WALL_HEIGHT + 0.95f) * 0.7f * ceiling_lights_t
											) * clamp(1.0f - distance / lerp(8.0f, 48.0f, ceiling_lights_t), 0.0f, 1.0f),
											0.0f,
											1.0f
										);

							if (state->game.flashlight_activation)
							{
								constexpr f32 FLASHLIGHT_INNER_CUTOFF = 1.00f;
								constexpr f32 FLASHLIGHT_OUTER_CUTOFF = 0.91f;

								new_color +=
									color
										* (1.0f + powf(square(dot(ray, normal)), 64) * square(dot(ray, state->game.flashlight_ray)) * 0.8f)
										* clamp
											(
												clamp((dot(ray, state->game.flashlight_ray) - FLASHLIGHT_OUTER_CUTOFF) / (FLASHLIGHT_INNER_CUTOFF - FLASHLIGHT_OUTER_CUTOFF), 0.0f, 1.0f)
													/ (square(distance) + 0.1f)
													* 32.0f
													* state->game.flashlight_activation,
												0.0f,
												1.0f
											);
							}

							if (in_light)
							{
								switch (material)
								{
									case Material::monster:
									{
										new_color += color;
									} break;

									default:
									{
										vf3 frag_position = state->game.lucia_position + ray * distance;
										vf3 frag_ray      = vx3(ray_to_closest(frag_position.xy, state->game.monster_position.xy), state->game.monster_position.z - frag_position.z);

										new_color +=
											(color * 3.0f + vf3 { 0.8863f, 0.3451f, 0.1333f } * 4.0f)
												* 2.0f
												* square(clamp(1.0f / (norm(frag_ray) + 0.1f), 0.0f, 1.0f))
												* fabsf(dot(normalize(frag_ray), normal));
									};
								}
							}

							return vf3 { clamp(new_color.x, 0.0f, 1.0f), clamp(new_color.y, 0.0f, 1.0f), clamp(new_color.z, 0.0f, 1.0f) };
						}
					};

				FOR_RANGE(y, VIEW_RES.y)
				{
					vf3 ray        = normalize({ ray_horizontal.x, ray_horizontal.y, (y - VIEW_RES.y / 2.0f) * state->game.lucia_fov / HORT_TO_VERT_K });
					vf4 scan_pixel = { NAN, NAN, NAN, NAN };

					for (RenderScanNode* node = render_scan_node; node; node = node->next_node)
					{
						if (IN_RANGE(y, node->starting_y, node->ending_y) && IN_RANGE(state->game.lucia_position.z + ray.z * node->distance, 0.0f, WALL_HEIGHT))
						{
							scan_pixel = sample_at(&node->image, { node->portion, (static_cast<f32>(y) - node->starting_y) / (node->ending_y - node->starting_y) });
							if (scan_pixel.w)
							{
								view_pixels[(VIEW_RES.y - 1 - y) * VIEW_RES.x + x] = pack_color(shader(node->material, node->in_light, scan_pixel.xyz, ray, vx3(node->normal, 0.0f), node->distance));
								goto NEXT_Y;
							}
						}
					}

					if (IN_RANGE(y, wall_starting_y, wall_ending_y))
					{
						f32 y_portion          = static_cast<f32>(y - wall_starting_y) / (wall_ending_y - wall_starting_y);
						f32 distance           = sqrtf(square(wall_distance) + square(y_portion * WALL_HEIGHT - state->game.lucia_position.z));

						vf4 wall_overlay_color = { NAN, NAN, NAN, 0.0f };
						if (wall_overlay && IN_RANGE(y_portion, wall_overlay_uv_position.y, wall_overlay_uv_position.y + wall_overlay_uv_dimensions.y))
						{
							wall_overlay_color = sample_at(wall_overlay, { (wall_portion - wall_overlay_uv_position.x) / wall_overlay_uv_dimensions.x, (y_portion - wall_overlay_uv_position.y) / wall_overlay_uv_dimensions.y });
						}

						vf3 wall_color = { NAN, NAN, NAN };
						if (wall_overlay_color.w < 1.0f)
						{
							wall_color = sample_at(&state->game.mipmap.wall, distance / 4.0f + state->game.mipmap.wall.level_count * square(1.0f - fabsf(dot(ray, vx3(ray_casted_wall_side.normal, 0.0f)))), { wall_portion, y_portion });
						}

						// @TODO@ Lerp is slow!
						view_pixels[(VIEW_RES.y - 1 - y) * VIEW_RES.x + x] =
							pack_color
							(
								shader
								(
									Material::wall,
									wall_in_light,
									wall_overlay_color.w == 0.0f
										? wall_color
										: wall_overlay_color.w == 1.0f
											? wall_overlay_color.xyz
											: lerp(wall_color, wall_overlay_color.xyz, wall_overlay_color.w),
									ray,
									vx3(ray_casted_wall_side.normal, 0.0f),
									distance
								)
							);
					}
					else if (fabs(ray.z) > 0.0001f)
					{
						vf2      uv;
						f32      distance;
						vf3      normal;
						Mipmap*  mipmap;
						Material material;

						if (y < VIEW_RES.y / 2)
						{
							f32 zk   = -state->game.lucia_position.z / ray.z;
							uv       = state->game.lucia_position.xy + zk * ray.xy;
							distance = sqrtf(norm_sq(uv - state->game.lucia_position.xy) + square(state->game.lucia_position.z));
							normal   = { 0.0f, 0.0f, 1.0f };
							mipmap   = &state->game.mipmap.floor;
							material = Material::floor;
						}
						else
						{
							f32 zk   = (WALL_HEIGHT - state->game.lucia_position.z) / ray.z;
							uv       = state->game.lucia_position.xy + zk * ray.xy;
							distance = sqrtf(norm_sq(uv - state->game.lucia_position.xy) + square(WALL_HEIGHT - state->game.lucia_position.z));
							normal   = { 0.0f, 0.0f, -1.0f };
							mipmap   = &state->game.mipmap.ceiling;
							material = Material::ceiling;
						}

						uv.x = mod(uv.x / 4.0f, 1.0f);
						uv.y = mod(uv.y / 4.0f, 1.0f);

						vf3 floor_ceiling_color = sample_at(mipmap, distance / 16.0f + mipmap->level_count * square(1.0f - fabsf(dot(ray, normal))), uv);

						// @TODO@ Lerp is slow!
						view_pixels[(VIEW_RES.y - 1 - y) * VIEW_RES.x + x] =
							pack_color
							(
								shader
								(
									material,
									exists_clear_way(state, state->game.lucia_position.xy + ray.xy * distance * SHADER_INV_EPSILON, state->game.monster_position.xy),
									floor_ceiling_color,
									ray,
									normal,
									distance
								)
							);
					}

					NEXT_Y:;
				}
			}
			DEBUG_end_profiling_averaged(FRAME, 32);

			SDL_UnlockTexture(state->game.texture.view);
			render_texture(platform->renderer, state->game.texture.view, { 0.0f, 0.0f }, { static_cast<f32>(VIEW_RES.x) / SCREEN_RES.x * WIN_DIM.x, static_cast<f32>(VIEW_RES.y) / SCREEN_RES.y * WIN_DIM.y });

			SDL_SetRenderTarget(platform->renderer, state->game.texture.screen);
			set_color(platform->renderer, { 0.0f, 0.0f, 0.0f, 0.0f });
			SDL_RenderClear(platform->renderer);

			switch (state->game.hud.type)
			{
				case HudType::inventory:
				{
					FOR_RANGE(y, ARRAY_CAPACITY(state->game.hud.inventory.array))
					{
						FOR_RANGE(x, ARRAY_CAPACITY(state->game.hud.inventory.array[y]))
						{
							vf2 position =
								{
									(VIEW_RES.x - (ARRAY_CAPACITY(state->game.hud.inventory.array[y]) * (INVENTORY_DIM + INVENTORY_PADDING) - INVENTORY_PADDING)) / 2.0f + x * (INVENTORY_DIM + INVENTORY_PADDING),
									(VIEW_RES.y - (ARRAY_CAPACITY(state->game.hud.inventory.array   ) * (INVENTORY_DIM + INVENTORY_PADDING) - INVENTORY_PADDING)) / 2.0f + y * (INVENTORY_DIM + INVENTORY_PADDING)
								};

							set_color(platform->renderer, &state->game.hud.inventory.array[y][x] == state->game.hud.inventory.selected_item ? vf3 { 0.7f, 0.7f, 0.25f } : monochrome(0.5f));
							render_rect(platform->renderer, position, vxx(vx2(INVENTORY_DIM)));

							set_color(platform->renderer, monochrome(0.3f));
							render_filled_rect(platform->renderer, position + vf2 { 1.0f, 1.0f }, vx2(INVENTORY_DIM) - vf2 { 2.0f, 2.0f });

							TextureSprite* sprite = get_corresponding_texture_sprite_of_item(state, &state->game.hud.inventory.array[y][x]);
							if (sprite && !(state->game.hud.inventory.grabbing && &state->game.hud.inventory.array[y][x] == state->game.hud.inventory.selected_item))
							{
								render_texture(platform->renderer, sprite->texture, position, vx2(INVENTORY_DIM) - vf2 { 2.0f, 2.0f });
							}
						}
					}

					if (state->game.hud.inventory.grabbing)
					{
						constexpr f32 ITEM_SCALAR = 0.75f;
						render_texture
						(
							platform->renderer,
							get_corresponding_texture_sprite_of_item(state, state->game.hud.inventory.selected_item)->texture,
							vf2 { state->game.hud.cursor.x, VIEW_RES.y - state->game.hud.cursor.y } - vx2(INVENTORY_DIM) / 2.0f * ITEM_SCALAR,
							vx2(INVENTORY_DIM) * ITEM_SCALAR
						);
					}
				} break;

				case HudType::paper:
				{
					render_texture
					(
						platform->renderer,
						state->game.texture_sprite.papers[state->game.hud.paper.index].texture,
						VIEW_RES / 2.0f + conjugate(state->game.hud.paper.delta_position) * state->game.hud.paper.scalar - state->game.texture_sprite.papers[state->game.hud.paper.index].image.dim * state->game.hud.paper.scalar / 2.0f,
						state->game.texture_sprite.papers[state->game.hud.paper.index].image.dim * state->game.hud.paper.scalar
					);
				} break;

				case HudType::circuit_breaker:
				{
					render_texture
					(
						platform->renderer,
						state->game.texture.circuit_breaker_panel,
						CIRCUIT_BREAKER_HUD_POSITION,
						CIRCUIT_BREAKER_HUD_DIMENSIONS
					);

					FOR_RANGE(y, ARRAY_CAPACITY(state->game.hud.circuit_breaker.switches))
					{
						FOR_RANGE(x, ARRAY_CAPACITY(state->game.hud.circuit_breaker.switches[0]))
						{
							render_texture
							(
								platform->renderer,
								state->game.texture.circuit_breaker_switches[state->game.hud.circuit_breaker.switches[y][x].active],
								CIRCUIT_BREAKER_HUD_POSITION
								+ vf2
									{
										x * (CIRCUIT_BREAKER_SWITCH_DIMENSIONS.x + CIRCUIT_BREAKER_SWITCH_PADDINGS.x) + CIRCUIT_BREAKER_SWITCH_PADDINGS.x,
										CIRCUIT_BREAKER_HUD_DIMENSIONS.y - (y + 1) * (CIRCUIT_BREAKER_SWITCH_DIMENSIONS.y + CIRCUIT_BREAKER_SWITCH_PADDINGS.y) + CIRCUIT_BREAKER_SWITCH_PADDINGS.y / 2.0f
									},
								CIRCUIT_BREAKER_SWITCH_DIMENSIONS
							);
						}
					}

					FOR_RANGE(i, static_cast<i32>(CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS.y))
					{
						if (static_cast<f32>(i) / CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS.y < state->game.hud.circuit_breaker.interpolated_voltage / state->game.hud.circuit_breaker.max_voltage)
						{
							f32 portion = (state->game.hud.circuit_breaker.interpolated_voltage + static_cast<f32>(i) / CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS.y * state->game.hud.circuit_breaker.max_voltage) / 2.0f;
							set_color
							(
								platform->renderer,
								{
									clamp(fabsf((state->game.hud.circuit_breaker.goal_voltage - portion) / state->game.hud.circuit_breaker.goal_voltage), 0.0f, 1.0f),
									1.0f - clamp(fabsf((state->game.hud.circuit_breaker.goal_voltage - portion) / state->game.hud.circuit_breaker.goal_voltage), 0.0f, 1.0f),
									0.0f,
									1.0f
								}
							);
						}
						else
						{
							set_color(platform->renderer, monochrome(0.2f));
						}
						render_line(platform->renderer, CIRCUIT_BREAKER_VOLTAGE_DISPLAY_COORDINATES + vf2 { 0.0f, -static_cast<f32>(i) }, CIRCUIT_BREAKER_VOLTAGE_DISPLAY_COORDINATES + vf2 { CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS.x, -static_cast<f32>(i) });
					}

					set_color(platform->renderer, monochrome(0.9f));
					render_filled_rect
					(
						platform->renderer,
						CIRCUIT_BREAKER_VOLTAGE_DISPLAY_COORDINATES + vf2 { 0.0f, -CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS.y * state->game.hud.circuit_breaker.goal_voltage / state->game.hud.circuit_breaker.max_voltage } - vf2 { 4.0f, 1.0f },
						{ 8.0f, 2.0f }
					);
				} break;
			}

			if (state->game.hud.type != HudType::null)
			{
				f32 cursor_dim = HOLDING(Input::left_mouse) ? 10.0f : 15.0f;
				render_texture(platform->renderer, state->game.texture_sprite.hand.texture, vf2 { state->game.hud.cursor.x, VIEW_RES.y - state->game.hud.cursor.y } - vx2(cursor_dim) / 2.0f, vx2(cursor_dim));
			}

			set_color(platform->renderer, monochrome(0.1f));
			render_filled_rect(platform->renderer, { 0.0f, static_cast<f32>(SCREEN_RES.y - STATUS_HUD_HEIGHT) }, vxx(vi2 { SCREEN_RES.x, STATUS_HUD_HEIGHT }));

			render_texture
			(
				platform->renderer,
				state->game.texture.lucia_normal,
				{ SCREEN_RES.x / 2.0f - STATUS_HUD_HEIGHT / 2.0f, static_cast<f32>(SCREEN_RES.y - STATUS_HUD_HEIGHT) },
				{ STATUS_HUD_HEIGHT, STATUS_HUD_HEIGHT }
			);

			if (state->game.hud.status.battery_display_keytime > 0.0f)
			{
				constexpr vf2 BATTERY_DIMENSIONS   = { 20.0f, STATUS_HUD_HEIGHT * 0.65f };
				constexpr vf2 NODE_DIMENSIONS      = { BATTERY_DIMENSIONS.x * 0.25f, BATTERY_DIMENSIONS.y * 0.1f };
				constexpr f32 BATTERY_OUTLINE      = 2.0f;

				f32 battery_x = lerp(-BATTERY_DIMENSIONS.x - BATTERY_OUTLINE, 15.0f, 1.0f - square(1.0f - state->game.hud.status.battery_display_keytime));

				set_color(platform->renderer, monochrome(0.25f));

				render_filled_rect
				(
					platform->renderer,
					{ battery_x - BATTERY_OUTLINE, SCREEN_RES.y - (STATUS_HUD_HEIGHT + BATTERY_DIMENSIONS.y - NODE_DIMENSIONS.y) / 2.0f - BATTERY_OUTLINE },
					BATTERY_DIMENSIONS + vx2(BATTERY_OUTLINE) * 2.0f
				);

				render_filled_rect
				(
					platform->renderer,
					{ battery_x + (BATTERY_DIMENSIONS.x - NODE_DIMENSIONS.x) / 2.0f, SCREEN_RES.y - (STATUS_HUD_HEIGHT + BATTERY_DIMENSIONS.y - NODE_DIMENSIONS.y) / 2.0f - BATTERY_OUTLINE - NODE_DIMENSIONS.y },
					NODE_DIMENSIONS
				);

				constexpr vf3 BATTERY_LEVEL_COLORS[] = { { 0.7f, 0.05f, 0.04f }, { 0.7f, 0.4f, 0.03f }, { 0.4f, 0.7f, 0.04f }, { 0.04f, 0.85f, 0.04f } };
				FOR_ELEMS(level, BATTERY_LEVEL_COLORS)
				{
					set_color
					(
						platform->renderer,
						lerp
						(
							monochrome((level->x + level->y + level->z) / (3.0f + level_index)),
							*level,
							clamp((state->game.hud.status.battery_level_keytime - static_cast<f32>(level_index) / ARRAY_CAPACITY(BATTERY_LEVEL_COLORS)) * ARRAY_CAPACITY(BATTERY_LEVEL_COLORS), 0.0f, 1.0f)
						)
					);

					render_filled_rect
					(
						platform->renderer,
						{ battery_x, SCREEN_RES.y - (STATUS_HUD_HEIGHT + BATTERY_DIMENSIONS.y - NODE_DIMENSIONS.y) / 2.0f + BATTERY_DIMENSIONS.y * (1.0f - (1.0f + level_index) / ARRAY_CAPACITY(BATTERY_LEVEL_COLORS)) },
						{ BATTERY_DIMENSIONS.x, BATTERY_DIMENSIONS.y / ARRAY_CAPACITY(BATTERY_LEVEL_COLORS) }
					);
				}
			}


			constexpr vf2 HEART_RATE_MONITOR_DIMENSIONS  = { 50.0f, STATUS_HUD_HEIGHT * 0.6f };
			constexpr vf2 HEART_RATE_MONITOR_COORDINATES = vf2 { SCREEN_RES.x - 15.0f - HEART_RATE_MONITOR_DIMENSIONS.x, SCREEN_RES.y - (STATUS_HUD_HEIGHT + HEART_RATE_MONITOR_DIMENSIONS.y) / 2.0f };

			set_color(platform->renderer, monochrome(0.15f));
			render_filled_rect(platform->renderer, HEART_RATE_MONITOR_COORDINATES, HEART_RATE_MONITOR_DIMENSIONS);

			FOR_ELEMS(it, state->game.heart_rate_values, ARRAY_CAPACITY(state->game.heart_rate_values) - 1)
			{
				if (it_index + 1 != state->game.heart_rate_index)
				{
					set_color(platform->renderer, { mod(it_index - state->game.heart_rate_index, ARRAY_CAPACITY(state->game.heart_rate_values)) / static_cast<f32>(ARRAY_CAPACITY(state->game.heart_rate_values)), 0.0f, 0.0f, 1.0f });
					render_line
					(
						platform->renderer,
						HEART_RATE_MONITOR_COORDINATES + vxx(vf2 {  it_index         / static_cast<f32>(ARRAY_CAPACITY(state->game.heart_rate_values)) * HEART_RATE_MONITOR_DIMENSIONS.x, clamp(0.75f - * it      / 2.0f, 0.0f, 1.0f) * HEART_RATE_MONITOR_DIMENSIONS.y }),
						HEART_RATE_MONITOR_COORDINATES + vxx(vf2 { (it_index + 1.0f) / static_cast<f32>(ARRAY_CAPACITY(state->game.heart_rate_values)) * HEART_RATE_MONITOR_DIMENSIONS.x, clamp(0.75f - *(it + 1) / 2.0f, 0.0f, 1.0f) * HEART_RATE_MONITOR_DIMENSIONS.y })
					);
				}
			}

			blackout = 1.0f - state->game.entering_keytime;

			SDL_SetRenderTarget(platform->renderer, 0);
			render_texture(platform->renderer, state->game.texture.screen, { 0.0f, 0.0f }, vxx(WIN_DIM));

			if (state->game.notification_keytime)
			{
				render_text
				(
					platform->renderer,
					state->font.minor,
					{ WIN_DIM.x * 0.5f, WIN_DIM.y * 0.6f },
					0.5f,
					FC_ALIGN_CENTER,
					1.0f,
					{ 1.0f, 1.0f, 1.0f, sinf(TAU / 4.0f * square(state->game.notification_keytime)) },
					"%s",
					state->game.notification_message
				);
			}

			// @TEMP@
			render_text
			(
				platform->renderer,
				state->font.minor,
				{ 0.0f, 0.0f },
				0.0f,
				FC_ALIGN_LEFT,
				1.0f,
				{ 1.0f, 1.0f, 1.0f, 1.0f },
				"%.2f %.2f\n%.2f %.2f\n%.2f %.2f\n%.2f %.2f",
				state->game.lucia_position.x, state->game.lucia_position.y,
				state->game.door_wall_side.coordinates.x * WALL_SPACING, state->game.door_wall_side.coordinates.y * WALL_SPACING,
				state->game.monster_position.x, state->game.monster_position.y,
				polar(state->game.lucia_angle).x, polar(state->game.lucia_angle).y
			);
		} break;

		case StateContext::end:
		{
			set_color(platform->renderer, { 0.2f, 0.05f, 0.05f, 1.0f });
			SDL_RenderClear(platform->renderer);

			if (state->end.is_exiting)
			{
				blackout = 1.0f;
			}
			else
			{
				blackout = 1.0f - state->end.entering_keytime;
			}
		};
	}

	set_color(platform->renderer, { 0.0f, 0.0f, 0.0f, blackout });
	render_filled_rect(platform->renderer, { 0.0f, 0.0f }, vxx(WIN_DIM));

	SDL_RenderPresent(platform->renderer);
}
