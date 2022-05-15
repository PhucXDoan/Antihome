/* @TODO@
	- Sunday 2022-5-15:
		- Add the new sounds in.
		- Add journal entries to be picked up in.

	- Monday 2022-5-16:
		- Revised title menu.
			- Vintage look.
			- Settings window.
			- Terminal prompt of debrief.

	- Tuesday 2022-5-17:
		- Breaker mechanic.
		- Better randomization of items, doors, breakers, and RNG system.
		- Text for inventory.

	- Wednesday 2022-5-18:
		- Anomaly: Serpent.
			- Multiple sprites in a row chasing player.
			- Constant hissing sound.
		- Anomaly: Guardian Angel.
			- Glows.
			- Moves rigidly.
			- Flapping noises when moving.

	- Thursday 2022-5-19:

	- Friday 2022-5-20:

	- Saturday 2022-5-21:

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

	* Items:
		* Satellite dish piece
		* SNAV
		* Drugs of varying effects
		* First aid kit
		* Radio
		* 18V battery
		* Eyedrops
		* Night Vision Goggles
		* Clipboard
		* Joint
		* Cowbell

	* Better input.
	* Handle disconnected initial and updated values.
	* Handle different resolutions.
*/

// @NOTE@ Credits
// "A Fast Voxel Traversal Algorithm for Ray Tracing" https://www.flipcode.com/archives/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf
// "How to check if two given line segments intersect?" https://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/, http://www.dcs.gla.ac.uk/~pat/52233/slides/Geometry1x1.pdf

// @TODO@ Consider half-precision floats for depth buffer.

#define STB_IMAGE_IMPLEMENTATION true
#include <stb_image.h>
#include "unified.h"
#include "platform.h"
#include "utilities.cpp"

global constexpr vi2 WIN_RES           = WIN_DIM / 3; // @TODO@ This affects text.
global constexpr i32 PADDING           = 5;
global constexpr vi2 VIEW_RES          = vxx(vf2 { 1.0f, 0.5f } * (WIN_RES.x - PADDING * 2.0f));
global constexpr vi2 HUD_RES           = { WIN_RES.x - PADDING * 2, WIN_RES.y - VIEW_RES.y - PADDING * 3 };
global constexpr f32 HORT_TO_VERT_K    = 0.927295218f * VIEW_RES.x;
global constexpr f32 WALL_HEIGHT       = 2.7432f;
global constexpr f32 WALL_THICKNESS    = 0.25f;
global constexpr f32 LUCIA_HEIGHT      = 1.4986f;
global constexpr i32 MAP_DIM           = 32;
global constexpr f32 WALL_SPACING      = 3.0f;
global constexpr i32 INVENTORY_DIM     = 30;
global constexpr i32 INVENTORY_PADDING = 5;

global constexpr struct { i32 slide_index; strlit text; } INTRO_DATA[] =
	{
		{ 0, "\"The subject of our debrief goes by the name of Lucia.```````````````` As always,```````` exposition will be the appetizer of the night.\"" },
		{ 1, "Several chuckles were dispensed." },
		{ 2, "\"It was a dark and stormy night.````.````.```` and Lucia here is stumbling around drunk,```````` although that's probably just how she walked.\"" },
		{ 2, "\"Anyways,```` she must've been getting some bad hunger pangs```` 'cause she managed to stumble behind our tagged Thai restaurant.\"" },
		{ 3, "\"MAN I LOVE PAD THAI!!\"" },
		{ 4, "\"...\"" },
		{ 5, "\".``.``.``.``.``.``\"" },
		{ 6, "\"Ahu -`-`-`\"" }
	};

enum_loose (AudioChannel, i8)
{
	unreserved = -1,

	enum_start_region(RESERVED)
		bg_0,
		bg_1,
		_2,
		_3,
	enum_end_region(RESERVED)
	enum_start_region(UNRESERVED)
		_4,
		_5,
		_6,
		_7,
	enum_end_region(UNRESERVED)
};

enum_loose (TitleMenuOption, u8)
{
	start,
	settings,
	credits,
	exit,
	CAPACITY
};

global constexpr strlit TITLE_MENU_OPTIONS[TitleMenuOption::CAPACITY] =
	{
		"Start",
		"Settings",
		"Credits",
		"Exit"
	};

enum_loose (SettingOption, u8)
{
	master_volume,
	brightness,
	done,
	CAPACITY
};

global constexpr strlit SETTING_OPTIONS[SettingOption::CAPACITY] =
	{
		"Master Volume",
		"Brightness",
		"Done"
	};

enum_loose (ItemType, u8)
{
	null,

	enum_start_region(ITEM)
		battery,
		paper,
		flashlight,
	enum_end_region(ITEM)
};

global constexpr strlit DEFAULT_ITEM_IMG_FILE_PATHS[ItemType::ITEM_COUNT] =
	{
		DATA_DIR "battery.png",
		DATA_DIR "paper.png",
		DATA_DIR "flashlight_off.png"
	};

global constexpr f32 ITEM_SPAWN_WEIGHTS[ItemType::ITEM_COUNT] =
	{
		10.0f,
		5.0f,
		1.0f
	};

global constexpr struct { f32 min_scalar; f32 max_scalar; strlit file_path; } PAPER_DATA[] =
	{
		{ 0.2f, 0.8f, DATA_DIR "terry_entry_4.png" }
	};

global constexpr strlit FOOTSTEP_WAV_FILE_PATHS[] =
	{
		DATA_DIR "debug_0.wav",
		DATA_DIR "debug_1.wav",
		DATA_DIR "debug_2.wav",
		DATA_DIR "debug_3.wav"
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
	};
};

flag_struct (WallVoxel, u8)
{
	left          = 1 << 0,
	bottom        = 1 << 1,
	back_slash    = 1 << 2,
	forward_slash = 1 << 3
};

struct WallSide
{
	vi2       coordinates;
	WallVoxel voxel;
	bool32    is_antinormal;
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

enum struct TitleMenuContext : u8
{
	title_menu,
	intro,
	settings,
	credits
};

struct State
{
	MemoryArena  long_term_arena;
	MemoryArena  transient_arena;

	FC_Font*     major_font;
	FC_Font*     minor_font;
	Mix_Chunk*   tiny_click;
	Mix_Chunk*   text_type_sfx;

	f32          reserved_channel_volumes[AudioChannel::RESERVED_COUNT];

	u32          seed;
	f32          time;
	f32          master_volume;
	f32          brightness;
	StateContext context;

	struct
	{
		SDL_Texture*     background;

		TitleMenuContext context;

		i32              option_index;
		f32              option_index_repeated_movement_countdown;
		f32              option_cursor_interpolated_index;

		struct
		{
			f32 repeat_sfx_keytime;
			i32 repeat_sfx_count;
		} settings;

		struct
		{
		} credits;

		struct
		{
			f32    entering_keytime;
			bool32 is_exiting;
			f32    exiting_keytime;
			char   text[256];
			i32    current_text_index;
			i32    current_text_length;
			i32    next_char_index;
			f32    next_char_keytime;
		} intro;
	} title_menu;

	struct
	{
		Mipmap               wall;
		Mipmap               floor;
		Mipmap               ceiling;
		ImgRGBA              monster_img;
		ImgRGBA              hand_img;
		ImgRGBA              flashlight_on_img;
		ImgRGBA              default_item_imgs[ItemType::ITEM_COUNT];
		ImgRGBA              paper_imgs[ARRAY_CAPACITY(PAPER_DATA)];
		ImgRGBA              door;
		ImgRGBA              wall_left_arrow;
		ImgRGBA              wall_right_arrow;
		SDL_Texture*         lucia_haunted;
		SDL_Texture*         lucia_healed;
		SDL_Texture*         lucia_hit;
		SDL_Texture*         lucia_normal;
		SDL_Texture*         lucia_wounded;
		SDL_Texture*         view_texture;
		Mix_Chunk*           room_ambience;
		Mix_Chunk*           room_ambience_quiet;
		Mix_Chunk*           footstep_sfxs[ARRAY_CAPACITY(FOOTSTEP_WAV_FILE_PATHS)];

		f32                  entering_keytime;
		bool32               is_exiting;
		f32                  exiting_keytime;

		PathCoordinatesNode* available_path_coordinates_node;
		strlit               notification_message;
		f32                  notification_keytime;
		f32                  heart_rate_values[50];
		i32                  heart_rate_index;
		f32                  heart_rate_update_keytime;
		f32                  heart_rate_velocity;
		f32                  heart_rate_beat_keytime;
		f32                  heart_rate_bpm;

		WallVoxel            wall_voxels[MAP_DIM][MAP_DIM];
		f32                  ceiling_lights_deactivation_keytime;
		WallSide             door_wall_side;

		vf2                  lucia_velocity;
		vf3                  lucia_position;
		f32                  lucia_angle_velocity;
		f32                  lucia_angle;
		f32                  lucia_fov;
		f32                  lucia_head_bob_keytime;
		f32                  lucia_stamina;
		f32                  lucia_sprint_keytime;
		bool32               lucia_out_of_breath;
		f32                  lucia_step_keytime;

		PathCoordinatesNode* monster_path;
		vi2                  monster_path_goal;
		vf3                  monster_position;
		vf2                  monster_velocity;
		vf2                  monster_normal;

		vf3                  hand_position;
		vf2                  hand_normal;
		bool32               hand_on_door;
		Item*                hand_hovered_item;

		i32                  item_count;
		Item                 item_buffer[32];

		vf2                  interacting_cursor_velocity;
		vf2                  interacting_cursor;
		bool32               inventory_visibility;
		vf2                  inventory_click_position;
		Item*                inventory_selected;
		bool32               inventory_grabbing;
		union
		{
			Item             inventory[2][4];
			Item             flat_inventory[sizeof(inventory) / sizeof(Item)];
		};

		union
		{
			struct
			{
				Item* paper;
				Item* flashlight;
			} holding;

			Item* holdings[sizeof(holding) / sizeof(Item*)];
		};

		f32                  flashlight_activation;
		vf3                  flashlight_ray;
		f32                  flashlight_keytime;
		vf2                  paper_velocity;
		vf2                  paper_delta_position;
		i32                  paper_index;
		f32                  paper_scalar_velocity;
		f32                  paper_scalar;
	} game;

	struct
	{
		f32 entering_keytime;
	} end;
};

struct WallVoxelData { WallVoxel voxel; vf2 start; vf2 end; vf2 normal; };

global constexpr WallVoxelData WALL_VOXEL_DATA[] =
	{
		{ WallVoxel::left         , { 0.0f, 0.0f }, { 0.0f, 1.0f }, {     1.0f,      0.0f } },
		{ WallVoxel::bottom       , { 0.0f, 0.0f }, { 1.0f, 0.0f }, {     0.0f,     -1.0f } },
		{ WallVoxel::back_slash   , { 1.0f, 0.0f }, { 0.0f, 1.0f }, { INVSQRT2,  INVSQRT2 } },
		{ WallVoxel::forward_slash, { 0.0f, 0.0f }, { 1.0f, 1.0f }, { INVSQRT2, -INVSQRT2 } }
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

	return 0;
}

internal ImgRGBA* get_corresponding_item_img(State* state, Item* item)
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
				return &state->game.flashlight_on_img;
			}
		} break;
	}

	return &state->game.default_item_imgs[+item->type - +ItemType::ITEM_START];
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

internal void draw_img(u32* view_pixels, ImgRGBA* img, vi2 position, i32 dimension)
{
	FOR_RANGE(x, clamp(position.x, 0, VIEW_RES.x), clamp(position.x + dimension, 0, VIEW_RES.x))
	{
		FOR_RANGE(y, clamp(position.y, 0, VIEW_RES.y), clamp(position.y + dimension, 0, VIEW_RES.y))
		{
			vf4 color = img_color_at(img, { static_cast<f32>(x - position.x) / dimension, (1.0f - static_cast<f32>(y - position.y) / dimension) });
			view_pixels[y * VIEW_RES.x + x] = pack_color(lerp(unpack_color(view_pixels[y * VIEW_RES.x + x]), color.xyz, color.w));
		}
	}
}

internal void draw_img_box(u32* view_pixels, ImgRGBA* img, vf3 border_color, vi2 position, i32 dimension)
{
	constexpr u32 BG = pack_color({ 0.2f, 0.2f, 0.2f });

	u32 border = pack_color(border_color);

	FOR_RANGE(i, dimension)
	{
		view_pixels[position.y * VIEW_RES.x + position.x + i]                   = border;
		view_pixels[(position.y + dimension - 1) * VIEW_RES.x + position.x + i] = border;
		view_pixels[(position.y + i) * VIEW_RES.x + position.x]                 = border;
		view_pixels[(position.y + i) * VIEW_RES.x + position.x + dimension - 1] = border;
	}

	FOR_RANGE(iy, dimension - 2)
	{
		FOR_RANGE(ix, dimension - 2)
		{
			view_pixels[(position.y + 1 + iy) * VIEW_RES.x + position.x + 1 + ix] = BG;
		}
	}

	if (img)
	{
		draw_img(view_pixels, img, position + vi2 { 1, 1 }, dimension - 2);
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
		return memory_arena_push<PathCoordinatesNode>(&state->long_term_arena);
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

	i32 closest_index = -1;
	f32 best_distance = NAN;
	vf2 best_ray      = { NAN, NAN };
	FOR_RANGE(i, 9)
	{
		vf2 ray      = target - position + vi2 { i % 3 - 1, i / 3 - 1 } * MAP_DIM * WALL_SPACING;
		f32 distance = norm(ray);

		if (closest_index == -1 || best_distance > distance)
		{
			closest_index = i;
			best_distance = distance;
			best_ray      = ray;
		}
	}

	return best_ray;
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

internal vf2 rng_open_position(State* state)
{
	// @TODO@ Clean up?
	vi2 coordinates = { rng(&state->seed, 0, MAP_DIM), rng(&state->seed, 0, MAP_DIM) };

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

internal vf2 move(State* state, vf2 position, vf2 displacement)
{
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
		FOR_RANGE(y, static_cast<i32>(floorf(position.y / WALL_SPACING) + min(ceilf(current_displacement.y / WALL_SPACING), 0.0f) - 2.0f), floorf(position.y / WALL_SPACING) + max(floor(current_displacement.y / WALL_SPACING), 0.0f) + 2.0f)
		{
			FOR_RANGE(x, static_cast<i32>(floorf(position.x / WALL_SPACING) + min(ceilf(current_displacement.x / WALL_SPACING), 0.0f) - 2.0f), floorf(position.x / WALL_SPACING) + max(floor(current_displacement.x / WALL_SPACING), 0.0f) + 2.0f)
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
}

internal bool32 exists_clear_way(State* state, vf2 position, vf2 goal)
{
	vf2 ray = ray_to_closest(position, goal);

	vi2 step =
		{
			ray.x < 0.0f ? -1 : 1,
			ray.y < 0.0f ? -1 : 1
		};
	vf2 t_max =
		{
			((static_cast<i32>(position.x / WALL_SPACING) + (step.x == 1)) * WALL_SPACING - position.x) / ray.x,
			((static_cast<i32>(position.y / WALL_SPACING) + (step.y == 1)) * WALL_SPACING - position.y) / ray.y
		};
	vf2 t_delta =
		{
			step.x / ray.x * WALL_SPACING,
			step.y / ray.y * WALL_SPACING
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

	FOR_RANGE(MAXIMUM(fabsf(ray.x), fabsf(ray.y)) / WALL_SPACING + 1)
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

		if (coordinates == goal_coordinates)
		{
			return true;
		}

		if (t_max.x < t_max.y)
		{
			t_max.x       += t_delta.x;
			coordinates.x  = mod(coordinates.x + step.x, MAP_DIM);
		}
		else
		{
			t_max.y       += t_delta.y;
			coordinates.y  = mod(coordinates.y + step.y, MAP_DIM);
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
			state->title_menu.background = IMG_LoadTexture(renderer, DATA_DIR "title_menu.png");
		} break;

		case StateContext::game:
		{
			state->game.wall              = init_mipmap(DATA_DIR "wall.png");
			state->game.floor             = init_mipmap(DATA_DIR "floor.png");
			state->game.ceiling           = init_mipmap(DATA_DIR "ceiling.png");
			state->game.monster_img       = init_img_rgba(DATA_DIR "monster.png");
			state->game.hand_img          = init_img_rgba(DATA_DIR "hand.png");
			state->game.flashlight_on_img = init_img_rgba(DATA_DIR "flashlight_on.png");

			FOR_ELEMS(it, state->game.paper_imgs)
			{
				*it = init_img_rgba(PAPER_DATA[it_index].file_path);
			}

			state->game.door             = init_img_rgba(DATA_DIR "door.png");
			state->game.wall_left_arrow  = init_img_rgba(DATA_DIR "streak_left_0.png");
			state->game.wall_right_arrow = init_img_rgba(DATA_DIR "streak_right_0.png");

			FOR_ELEMS(it, DEFAULT_ITEM_IMG_FILE_PATHS)
			{
				state->game.default_item_imgs[it_index] = init_img_rgba(*it);
			}

			state->game.lucia_haunted = IMG_LoadTexture(renderer, DATA_DIR "lucia_haunted.png");
			state->game.lucia_healed  = IMG_LoadTexture(renderer, DATA_DIR "lucia_healed.png");
			state->game.lucia_hit     = IMG_LoadTexture(renderer, DATA_DIR "lucia_hit.png");
			state->game.lucia_normal  = IMG_LoadTexture(renderer, DATA_DIR "lucia_normal.png");
			state->game.lucia_wounded = IMG_LoadTexture(renderer, DATA_DIR "lucia_wounded.png");
			state->game.view_texture  = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, VIEW_RES.x, VIEW_RES.y);

			state->game.room_ambience       = Mix_LoadWAV(DATA_DIR "room_ambience.wav");
			state->game.room_ambience_quiet = Mix_LoadWAV(DATA_DIR "room_ambience_quiet.wav");

			FOR_ELEMS(it, state->game.footstep_sfxs)
			{
				*it = Mix_LoadWAV(FOOTSTEP_WAV_FILE_PATHS[it_index]);
				// ASSERT(*it); // @TEMP@
			}

			Mix_PlayChannel(+AudioChannel::bg_0 - +AudioChannel::RESERVED_START, state->game.room_ambience      , -1);
			Mix_PlayChannel(+AudioChannel::bg_1 - +AudioChannel::RESERVED_START, state->game.room_ambience_quiet, -1);
		} break;
	}
}

internal void boot_down_state(State* state)
{
	switch (state->context)
	{
		case StateContext::title_menu:
		{
			SDL_DestroyTexture(state->title_menu.background);
		} break;

		case StateContext::game:
		{
			deinit_mipmap(&state->game.wall);
			deinit_mipmap(&state->game.floor);
			deinit_mipmap(&state->game.ceiling);
			deinit_img_rgba(&state->game.monster_img);
			deinit_img_rgba(&state->game.hand_img);
			deinit_img_rgba(&state->game.flashlight_on_img);

			deinit_img_rgba(&state->game.paper_imgs[0]);

			FOR_ELEMS(it, state->game.default_item_imgs)
			{
				deinit_img_rgba(it);
			}

			deinit_img_rgba(&state->game.door);
			deinit_img_rgba(&state->game.wall_left_arrow);
			deinit_img_rgba(&state->game.wall_right_arrow);

			SDL_DestroyTexture(state->game.lucia_haunted);
			SDL_DestroyTexture(state->game.lucia_healed);
			SDL_DestroyTexture(state->game.lucia_hit);
			SDL_DestroyTexture(state->game.lucia_normal);
			SDL_DestroyTexture(state->game.lucia_wounded);
			SDL_DestroyTexture(state->game.view_texture);

			Mix_FreeChunk(state->game.room_ambience);
			Mix_FreeChunk(state->game.room_ambience_quiet);
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

	state->master_volume = 0.5f;
	state->brightness    = 0.75f;

	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_RenderSetLogicalSize(platform->renderer, WIN_RES.x, WIN_RES.y);

	Mix_ReserveChannels(4);
}

extern "C" PROTOTYPE_BOOT_UP(boot_up)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	state->major_font = FC_CreateFont();
	FC_LoadFont(state->major_font, platform->renderer, DATA_DIR "Consolas.ttf", 32, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);

	state->minor_font = FC_CreateFont();
	FC_LoadFont(state->minor_font, platform->renderer, DATA_DIR "Consolas.ttf", 10, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);

	state->tiny_click    = Mix_LoadWAV(DATA_DIR "tiny_click.wav");
	state->text_type_sfx = Mix_LoadWAV(DATA_DIR "text_type_sfx_0.wav");

	boot_up_state(platform->renderer, state);
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	FC_FreeFont(state->major_font);
	FC_FreeFont(state->minor_font);

	Mix_FreeChunk(state->tiny_click);
	Mix_FreeChunk(state->text_type_sfx);

	boot_down_state(state);
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	state->time                 += SECONDS_PER_UPDATE;
	state->transient_arena.used  = 0;

	Mix_Volume(-1, static_cast<i32>(MIX_MAX_VOLUME * state->master_volume)); // @TODO@ Remove frame delay.
	FOR_ELEMS(it, state->reserved_channel_volumes)
	{
		Mix_Volume(+AudioChannel::RESERVED_START + it_index, static_cast<i32>(MIX_MAX_VOLUME * state->master_volume * *it));
	}

	switch (state->context)
	{
		case StateContext::title_menu:
		{
			switch (state->title_menu.context)
			{
				case TitleMenuContext::title_menu:
				{
					state->title_menu.option_index_repeated_movement_countdown -= SECONDS_PER_UPDATE;

					// @TODO@ Compatable with arrows.
					i32 option_index_delta = iterate_repeated_movement(platform, Input::w, Input::s, &state->title_menu.option_index_repeated_movement_countdown);
					if (option_index_delta && IN_RANGE(state->title_menu.option_index + option_index_delta, 0, +TitleMenuOption::CAPACITY))
					{
						Mix_PlayChannel(-1, state->tiny_click, 0);
						state->title_menu.option_index += option_index_delta;
					}

					if (PRESSED(Input::space) || PRESSED(Input::enter))
					{
						Mix_PlayChannel(-1, state->tiny_click, 0);
						switch (state->title_menu.option_index)
						{
							case TitleMenuOption::start:
							{
								state->title_menu.context = TitleMenuContext::intro;
								state->title_menu.intro   = {};
							} break;

							case TitleMenuOption::settings:
							{
								state->title_menu.context                                  = TitleMenuContext::settings;
								state->title_menu.settings                                 = {};
								state->title_menu.option_index                             = 0;
								state->title_menu.option_index_repeated_movement_countdown = 0.0f;
								state->title_menu.option_cursor_interpolated_index         = 0.0f;

								return UpdateCode::resume;
							} break;

							case TitleMenuOption::credits:
							{
								state->title_menu.context = TitleMenuContext::credits;
								state->title_menu.credits = {};

								return UpdateCode::resume;
							} break;

							case TitleMenuOption::exit:
							{
								return UpdateCode::terminate;
							} break;
						}
					}

					state->title_menu.option_cursor_interpolated_index = dampen(state->title_menu.option_cursor_interpolated_index, static_cast<f32>(state->title_menu.option_index), 8.0f, SECONDS_PER_UPDATE);
				} break;

				case TitleMenuContext::settings:
				{
					aliasing settings = state->title_menu.settings;

					state->title_menu.option_index_repeated_movement_countdown -= SECONDS_PER_UPDATE;

					// @TODO@ Compatable with arrows.
					i32 option_index_delta = iterate_repeated_movement(platform, Input::w, Input::s, &state->title_menu.option_index_repeated_movement_countdown);
					if (option_index_delta && IN_RANGE(state->title_menu.option_index + option_index_delta, 0, +SettingOption::CAPACITY))
					{
						Mix_PlayChannel(-1, state->tiny_click, 0);
						state->title_menu.option_index += option_index_delta;

						settings.repeat_sfx_count   = 0;
						settings.repeat_sfx_keytime = 0.0f;
					}

					if ((PRESSED(Input::space) || PRESSED(Input::enter)) && state->title_menu.option_index == +SettingOption::done)
					{
						Mix_PlayChannel(-1, state->tiny_click, 0);

						state->title_menu.context                          = TitleMenuContext::title_menu;
						state->title_menu.option_index                     = +TitleMenuOption::settings;
						state->title_menu.option_cursor_interpolated_index = static_cast<f32>(TitleMenuOption::settings);

						return UpdateCode::resume;
					}

					switch (state->title_menu.option_index)
					{
						case SettingOption::master_volume:
						{
							f32 delta = 0.0f;
							if (HOLDING(Input::a)) { delta -= 1.0f; }
							if (HOLDING(Input::d)) { delta += 1.0f; }

							if (delta)
							{
								settings.repeat_sfx_count = 2;
							}

							if (settings.repeat_sfx_count || settings.repeat_sfx_keytime)
							{
								settings.repeat_sfx_keytime = clamp(settings.repeat_sfx_keytime - SECONDS_PER_UPDATE / 0.5f, 0.0f, 1.0f);

								if (settings.repeat_sfx_count && settings.repeat_sfx_keytime == 0.0f)
								{
									Mix_PlayChannel(-1, state->tiny_click, 0);
									settings.repeat_sfx_count   -= 1;
									settings.repeat_sfx_keytime  = 1.0f;
								}
							}

							state->master_volume += delta * 0.5f * SECONDS_PER_UPDATE;
							state->master_volume  = clamp(state->master_volume, 0.0f, 1.0f);
						} break;

						case SettingOption::brightness:
						{
							f32 delta = 0.0f;
							if (HOLDING(Input::a)) { delta -= 1.0f; }
							if (HOLDING(Input::d)) { delta += 1.0f; }
							state->brightness += delta * 0.5f * SECONDS_PER_UPDATE;
							state->brightness  = clamp(state->brightness, 0.0f, 1.0f);
						} break;
					}

					state->title_menu.option_cursor_interpolated_index = dampen(state->title_menu.option_cursor_interpolated_index, static_cast<f32>(state->title_menu.option_index), 8.0f, SECONDS_PER_UPDATE);
				} break;

				case TitleMenuContext::credits:
				{
					if (PRESSED(Input::space) || PRESSED(Input::enter))
					{
						Mix_PlayChannel(-1, state->tiny_click, 0);

						state->title_menu.context                          = TitleMenuContext::title_menu;
						state->title_menu.option_index                     = +TitleMenuOption::credits;
						state->title_menu.option_cursor_interpolated_index = static_cast<f32>(TitleMenuOption::credits);

						return UpdateCode::resume;
					}
				} break;

				case TitleMenuContext::intro:
				{
					aliasing intro = state->title_menu.intro;

					if (intro.is_exiting)
					{
						intro.exiting_keytime += SECONDS_PER_UPDATE / 0.75f;

						if (intro.exiting_keytime >= 1.0f)
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

							{
								memory_arena_checkpoint(&state->transient_arena);

								struct DoorSpotNode
								{
									vi2           coordinates;
									WallVoxel     wall_voxel;
									DoorSpotNode* next_node;
								};

								DoorSpotNode* door_spot_node  = 0;
								i32           door_spot_count = 0;

								FOR_RANGE(y, MAP_DIM)
								{
									FOR_RANGE(x, MAP_DIM)
									{
										FOR_ELEMS(it, WALL_VOXEL_DATA)
										{
											if (+(state->game.wall_voxels[y][x] & it->voxel))
											{
												DoorSpotNode* node = memory_arena_push<DoorSpotNode>(&state->transient_arena);
												node->coordinates  = { x, y };
												node->wall_voxel   = it->voxel;
												node->next_node    = door_spot_node;
												door_spot_node     = node;
												door_spot_count   += 1;
											}
										}
									}
								}

								ASSERT(door_spot_node);

								for (i32 i = rng(&state->seed, 0, door_spot_count); i; i -= 1)
								{
									door_spot_node = door_spot_node->next_node;
								}

								state->game.door_wall_side.coordinates   = door_spot_node->coordinates;
								state->game.door_wall_side.voxel         = door_spot_node->wall_voxel;
								state->game.door_wall_side.is_antinormal = rng(&state->seed) < 0.5f;
							}

							state->game.lucia_position.xy = rng_open_position(state);
							state->game.lucia_position.z  = LUCIA_HEIGHT;
							state->game.lucia_fov         = TAU / 3.0f;
							state->game.lucia_stamina     = 1.0f;

							state->game.monster_position.xy = rng_open_position(state);

							for (ItemType type = ItemType::ITEM_START; type != ItemType::ITEM_END; type = static_cast<ItemType>(+type + 1))
							{
								Item* item = allocate_item(state);
								*item             = {};
								item->type        = type;
								item->position.xy = rng_open_position(state);
								item->normal      = polar(state->time * 1.5f);
							}

							FOR_RANGE(ARRAY_CAPACITY(state->game.item_buffer) - state->game.item_count)
							{
								f32 total_weights = 0.0f;
								FOR_ELEMS(w, ITEM_SPAWN_WEIGHTS)
								{
									total_weights += *w;
								}

								f32 n = rng(&state->seed, 0.0f, total_weights);
								i32 i = -1;
								total_weights = 0.0f;
								FOR_ELEMS(w, ITEM_SPAWN_WEIGHTS)
								{
									if (IN_RANGE(n - total_weights, 0.0f, *w))
									{
										i = w_index;
										break;
									}
									else
									{
										total_weights += *w;
									}
								}

								Item* item = allocate_item(state);
								*item             = {};
								item->type        = static_cast<ItemType>(+ItemType::ITEM_START + i);
								item->position.xy = rng_open_position(state);
								item->normal      = polar(state->time * 1.5f);
							}

							return UpdateCode::resume;
						}
					}
					else
					{
						if (intro.entering_keytime < 1.0f)
						{
							intro.entering_keytime += SECONDS_PER_UPDATE / 1.0f;

							if (intro.entering_keytime >= 1.0f)
							{
								intro.entering_keytime = 1.0f;
							}

							state->title_menu.option_cursor_interpolated_index = dampen(state->title_menu.option_cursor_interpolated_index, static_cast<f32>(state->title_menu.option_index), 8.0f, SECONDS_PER_UPDATE);
						}

						if (intro.entering_keytime == 1.0f)
						{
							lambda iterate_text =
								[&](bool32 play_sound)
								{
									if (INTRO_DATA[intro.current_text_index].text[intro.next_char_index] != '\0')
									{
										if (INTRO_DATA[intro.current_text_index].text[intro.next_char_index] != '`')
										{
											ASSERT(intro.current_text_length < ARRAY_CAPACITY(intro.text) - 1);
											intro.text[intro.current_text_length] = INTRO_DATA[intro.current_text_index].text[intro.next_char_index];
											intro.current_text_length += 1;
											intro.text[intro.current_text_length] = '\0';
											Mix_PlayChannel(-1, state->text_type_sfx, 0);
										}

										intro.next_char_index += 1;

										return INTRO_DATA[intro.current_text_index].text[intro.next_char_index] != '\0';
									}

									return false;
								};

							for (intro.next_char_keytime += SECONDS_PER_UPDATE / 0.05f; intro.next_char_keytime >= 1.0f; intro.next_char_keytime -= 1.0f)
							{
								iterate_text(true);
							}

							if (PRESSED(Input::escape))
							{
								intro.is_exiting = true;
							}
							else if (PRESSED(Input::space) || PRESSED(Input::enter))
							{
								if (INTRO_DATA[intro.current_text_index].text[intro.next_char_index] == '\0')
								{
									if (intro.current_text_index == ARRAY_CAPACITY(INTRO_DATA) - 1)
									{
										intro.is_exiting = true;
									}
									else
									{
										intro.next_char_keytime    = 0.0f;
										intro.current_text_index  += 1;
										intro.current_text_length  = 0;
										intro.next_char_index      = 0;
										intro.text[0]              = '\0';
									}
								}
								else
								{
									while (iterate_text(false));
								}
							}
						}
					}
				} break;
			}
		} break;

		case StateContext::game:
		{
			state->reserved_channel_volumes[+AudioChannel::bg_0 - +AudioChannel::RESERVED_START] = 1.0f - state->game.ceiling_lights_deactivation_keytime;
			state->reserved_channel_volumes[+AudioChannel::bg_1 - +AudioChannel::RESERVED_START] =        state->game.ceiling_lights_deactivation_keytime;

			if (state->game.is_exiting)
			{
				state->game.exiting_keytime += SECONDS_PER_UPDATE / 1.5f;

				if (state->game.exiting_keytime >= 1.0f)
				{
					boot_down_state(state);
					state->context = StateContext::end;
					state->end     = {};
					boot_up_state(platform->renderer, state);

					return UpdateCode::resume;
				}
			}
			else
			{
				state->game.entering_keytime = clamp(state->game.entering_keytime + SECONDS_PER_UPDATE / 1.0f, 0.0f, 1.0f);

				if (PRESSED(Input::tab))
				{
					state->game.inventory_visibility = !state->game.inventory_visibility;
					state->game.inventory_selected   = 0;
					state->game.inventory_grabbing   = false;

					if (!state->game.holding.paper)
					{
						state->game.interacting_cursor_velocity = { 0.0f, 0.0f };
						state->game.interacting_cursor          = VIEW_RES / 2.0f;
					}

					state->game.holding.paper = 0;
				}

				if (state->game.inventory_visibility || state->game.holding.paper)
				{
					state->game.interacting_cursor_velocity += platform->cursor_delta * 16.0f;
					state->game.interacting_cursor_velocity *= 0.3f;

					state->game.interacting_cursor   += state->game.interacting_cursor_velocity * SECONDS_PER_UPDATE;
					state->game.interacting_cursor.x  = clamp(state->game.interacting_cursor.x, 0.0f, static_cast<f32>(VIEW_RES.x));
					state->game.interacting_cursor.y  = clamp(state->game.interacting_cursor.y, 0.0f, static_cast<f32>(VIEW_RES.y));
				}


				if (state->game.inventory_visibility)
				{
					if (PRESSED(Input::left_mouse))
					{
						state->game.inventory_click_position = state->game.interacting_cursor;

						FOR_RANGE(y, ARRAY_CAPACITY(state->game.inventory))
						{
							FOR_RANGE(x, ARRAY_CAPACITY(state->game.inventory[y]))
							{
								if
								(
									fabsf(VIEW_RES.x / 2.0f + (x + (1.0f - ARRAY_CAPACITY(state->game.inventory[y])) / 2.0f) * (INVENTORY_DIM + INVENTORY_PADDING) - state->game.interacting_cursor.x) < INVENTORY_DIM / 2.0f &&
									fabsf(VIEW_RES.y / 2.0f - (y + (1.0f - ARRAY_CAPACITY(state->game.inventory   )) / 2.0f) * (INVENTORY_DIM + INVENTORY_PADDING) - state->game.interacting_cursor.y) < INVENTORY_DIM / 2.0f
								)
								{
									if (state->game.inventory[y][x].type != ItemType::null)
									{
										state->game.inventory_selected = &state->game.inventory[y][x];
									}

									break;
								}
							}
						}
					}
					else if (state->game.inventory_selected)
					{
						if (HOLDING(Input::left_mouse))
						{
							if (!state->game.inventory_grabbing && norm_sq(state->game.interacting_cursor - state->game.inventory_click_position) > 25.0f)
							{
								state->game.inventory_grabbing = true;
							}
						}
						else if (RELEASED(Input::left_mouse))
						{
							if (state->game.inventory_grabbing)
							{
								if
								(
									fabs(state->game.interacting_cursor.x * 2.0f - VIEW_RES.x) < ARRAY_CAPACITY(state->game.inventory[0]) * (INVENTORY_DIM + INVENTORY_PADDING) &&
									fabs(state->game.interacting_cursor.y * 2.0f - VIEW_RES.y) < ARRAY_CAPACITY(state->game.inventory   ) * (INVENTORY_DIM + INVENTORY_PADDING)
								)
								{
									FOR_RANGE(y, ARRAY_CAPACITY(state->game.inventory))
									{
										FOR_RANGE(x, ARRAY_CAPACITY(state->game.inventory[y]))
										{
											if
											(
												fabsf(VIEW_RES.x / 2.0f + (x + (1.0f - ARRAY_CAPACITY(state->game.inventory[y])) / 2.0f) * (INVENTORY_DIM + INVENTORY_PADDING) - state->game.interacting_cursor.x) < INVENTORY_DIM / 2.0f &&
												fabsf(VIEW_RES.y / 2.0f - (y + (1.0f - ARRAY_CAPACITY(state->game.inventory   )) / 2.0f) * (INVENTORY_DIM + INVENTORY_PADDING) - state->game.interacting_cursor.y) < INVENTORY_DIM / 2.0f
											)
											{
												if (&state->game.inventory[y][x] != state->game.inventory_selected)
												{
													if (state->game.inventory[y][x].type == ItemType::null)
													{
														// @NOTE@ Item move.

														state->game.inventory[y][x]          = *state->game.inventory_selected;
														state->game.inventory_selected->type = ItemType::null;

														FOR_ELEMS(it, state->game.holdings)
														{
															if (*it == state->game.inventory_selected)
															{
																*it = &state->game.inventory[y][x];
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
															Item* battery;
															Item* flashlight;
															if (check_combine(&battery, ItemType::battery, &flashlight, ItemType::flashlight, &state->game.inventory[y][x], state->game.inventory_selected))
															{
																combined = true;
																state->game.notification_message = "(You replaced the batteries in the flashlight.)";
																state->game.notification_keytime = 1.0f;

																combined_result = *flashlight;
																combined_result.flashlight.power = 1.0f;

																if (state->game.holding.flashlight == state->game.inventory_selected)
																{
																	state->game.holding.flashlight = &state->game.inventory[y][x];
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
															state->game.inventory_selected->type = ItemType::null;
															state->game.inventory[y][x]          = combined_result;
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
									*dropped = *state->game.inventory_selected;
									dropped->position = state->game.lucia_position;
									dropped->velocity = polar(state->game.lucia_angle + rng(&state->seed, -0.5f, 0.5f) * 1.0f) * rng(&state->seed, 2.5f, 6.0f);

									state->game.inventory_selected->type = ItemType::null;

									FOR_ELEMS(it, state->game.holdings)
									{
										if (*it == state->game.inventory_selected)
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

								switch (state->game.inventory_selected->type)
								{
									case ItemType::battery:
									{
										state->game.notification_message = "\"Some batteries. They feel cheap.\"";
										state->game.notification_keytime = 1.0f;
									} break;

									case ItemType::paper:
									{
										state->game.holding.paper         = state->game.inventory_selected;
										state->game.paper_velocity        = { 0.0f, 0.0f };
										state->game.paper_delta_position  = { 0.0f, 0.0f };
										state->game.paper_index           = 0; // @TEMP@
										state->game.paper_scalar_velocity = 0.0f;
										state->game.paper_scalar          = PAPER_DATA[state->game.paper_index].min_scalar;
									} break;

									case ItemType::flashlight:
									{
										if (state->game.inventory_selected == state->game.holding.flashlight)
										{
											state->game.holding.flashlight = 0;
										}
										else if (state->game.inventory_selected->flashlight.power)
										{
											state->game.holding.flashlight = state->game.inventory_selected;
										}
										else
										{
											state->game.notification_message = "\"The flashlight is dead.\"";
											state->game.notification_keytime = 1.0f;
										}
									};
								}

								state->game.inventory_visibility = false;
							}

							state->game.inventory_selected   = 0;
							state->game.inventory_grabbing   = false;
						}
					}
				}
				else if (state->game.holding.paper)
				{
					if
					(
						PRESSED(Input::left_mouse) &&
						(
							state->game.interacting_cursor.x < VIEW_RES.x / 2.0f + (state->game.paper_delta_position.x - state->game.paper_imgs[state->game.paper_index].dim.x / 2.0f) * state->game.paper_scalar ||
							state->game.interacting_cursor.x > VIEW_RES.x / 2.0f + (state->game.paper_delta_position.x + state->game.paper_imgs[state->game.paper_index].dim.x / 2.0f) * state->game.paper_scalar ||
							state->game.interacting_cursor.y < VIEW_RES.y / 2.0f + (state->game.paper_delta_position.y - state->game.paper_imgs[state->game.paper_index].dim.y / 2.0f) * state->game.paper_scalar ||
							state->game.interacting_cursor.y > VIEW_RES.y / 2.0f + (state->game.paper_delta_position.y + state->game.paper_imgs[state->game.paper_index].dim.y / 2.0f) * state->game.paper_scalar
						)
					)
					{
						state->game.holding.paper = 0;
					}
					else
					{
						state->game.paper_scalar_velocity += platform->scroll * 2.0f;
						state->game.paper_scalar_velocity *= 0.5f;

						if (HOLDING(Input::left_mouse))
						{
							state->game.paper_velocity        = state->game.interacting_cursor_velocity;
							state->game.paper_scalar_velocity = 0.0f;
						}
						else
						{
							state->game.paper_velocity *= 0.5f;
						}

						state->game.paper_scalar = state->game.paper_scalar + state->game.paper_scalar_velocity * state->game.paper_scalar * SECONDS_PER_UPDATE;

						if (PAPER_DATA[state->game.paper_index].min_scalar > state->game.paper_scalar || state->game.paper_scalar > PAPER_DATA[state->game.paper_index].max_scalar)
						{
							state->game.paper_scalar          = clamp(state->game.paper_scalar, PAPER_DATA[state->game.paper_index].min_scalar, PAPER_DATA[state->game.paper_index].max_scalar);
							state->game.paper_scalar_velocity = 0.0f;
						}

						state->game.paper_delta_position += state->game.paper_velocity / state->game.paper_scalar * SECONDS_PER_UPDATE;

						constexpr f32 PAPER_MARGIN = 25.0f;
						vf2 region =
							vf2 {
								(VIEW_RES.x + state->game.paper_imgs[state->game.paper_index].dim.x * state->game.paper_scalar) / 2.0f - PAPER_MARGIN,
								(VIEW_RES.y + state->game.paper_imgs[state->game.paper_index].dim.y * state->game.paper_scalar) / 2.0f - PAPER_MARGIN
							} / state->game.paper_scalar;

						if (fabsf(state->game.paper_delta_position.x) > region.x)
						{
							state->game.paper_delta_position.x = clamp(state->game.paper_delta_position.x, -region.x, region.x);
							state->game.paper_velocity.x       = 0.0f;
						}
						if (fabsf(state->game.paper_delta_position.y) > region.y)
						{
							state->game.paper_delta_position.y = clamp(state->game.paper_delta_position.y, -region.y, region.y);
							state->game.paper_velocity.y       = 0.0f;
						}
					}
				}
				else
				{
					state->game.lucia_angle_velocity -= platform->cursor_delta.x * 0.01f / SECONDS_PER_UPDATE;
				}

				state->game.lucia_angle_velocity *= 0.4f;
				state->game.lucia_angle           = mod(state->game.lucia_angle + state->game.lucia_angle_velocity * SECONDS_PER_UPDATE, TAU);

				if (PRESSED(Input::space) || PRESSED(Input::left_mouse))
				{
					if (state->game.hand_hovered_item)
					{
						Item* open_space = 0;
						FOR_ELEMS(it, state->game.flat_inventory)
						{
							if (it->type == ItemType::null)
							{
								open_space = it;
								break;
							}
						}

						if (open_space)
						{
							*open_space = *state->game.hand_hovered_item;
							deallocate_item(state, state->game.hand_hovered_item);
							state->game.hand_hovered_item = 0;
						}
						else
						{
							state->game.notification_message = "\"I can't carry anymore.\"";
							state->game.notification_keytime = 1.0f;
						}
					}
					else if (state->game.hand_on_door)
					{
						state->game.is_exiting = true;
					}
				}

				vf2 wasd = { 0.0f, 0.0f };
				if (HOLDING(Input::s)) { wasd.x -= 1.0f; }
				if (HOLDING(Input::w)) { wasd.x += 1.0f; }
				if (HOLDING(Input::d)) { wasd.y -= 1.0f; }
				if (HOLDING(Input::a)) { wasd.y += 1.0f; }
				if (+wasd)
				{
					constexpr f32 MIN_PORTION = 0.7f;
					state->game.lucia_velocity += rotate(normalize(wasd), state->game.lucia_angle) * 2.0f * ((1.0f - powf(1.0f - state->game.lucia_stamina, 8) + MIN_PORTION) / (1.0f + MIN_PORTION));

				}

				if (+wasd && HOLDING(Input::shift) && !state->game.lucia_out_of_breath)
				{
					state->game.lucia_velocity       *= 0.75f;
					state->game.lucia_stamina         = clamp(state->game.lucia_stamina - SECONDS_PER_UPDATE / 35.0f * (1.0f + (1.0f - square(1.0f - 2.0f * state->game.lucia_sprint_keytime)) * 4.0f), 0.0f, 1.0f);
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
					it->velocity    *= 0.9f;
					it->position.xy  = move(state, it->position.xy, it->velocity * SECONDS_PER_UPDATE);// @TEMP@
					it->position.z   = lerp(0.15f, state->game.lucia_position.z, clamp(1.0f - norm_sq(ray_to_closest(state->game.lucia_position.xy, it->position.xy)) / 36.0f, 0.0f, 1.0f)) + sinf(state->time * 3.0f) * 0.025f;
					it->normal       = polar(state->time * 0.7f);
				}

				state->game.lucia_position.xy = move(state, state->game.lucia_position.xy, state->game.lucia_velocity * SECONDS_PER_UPDATE);
				state->game.lucia_position.x  = mod(state->game.lucia_position.x, MAP_DIM * WALL_SPACING);
				state->game.lucia_position.y  = mod(state->game.lucia_position.y, MAP_DIM * WALL_SPACING);
				state->game.lucia_position.z  = LUCIA_HEIGHT + 0.1f * (cosf(state->game.lucia_head_bob_keytime * TAU) - 1.0f);

				state->game.lucia_step_keytime += SECONDS_PER_UPDATE * norm(state->game.lucia_velocity) / 2.75f; // @TODO@ Make this sync with head bob?
				if (state->game.lucia_step_keytime >= 1.0f)
				{
					state->game.lucia_step_keytime = 0.0f;
					Mix_PlayChannel(+AudioChannel::unreserved, state->game.footstep_sfxs[rng(&state->seed, 0, ARRAY_CAPACITY(state->game.footstep_sfxs))], 0);
				}

				state->game.lucia_head_bob_keytime = mod(state->game.lucia_head_bob_keytime + 0.001f + 0.35f * norm(state->game.lucia_velocity) * SECONDS_PER_UPDATE, 1.0f);

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

					PathQueueNode* path_queue = memory_arena_push<PathQueueNode>(&state->transient_arena);
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
									new_node = memory_arena_push<PathQueueNode>(&state->transient_arena);
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

				state->game.monster_position.xy += state->game.monster_velocity * SECONDS_PER_UPDATE;
				state->game.monster_position.x   = mod(state->game.monster_position.x, MAP_DIM * WALL_SPACING);
				state->game.monster_position.y   = mod(state->game.monster_position.y, MAP_DIM * WALL_SPACING);
				state->game.monster_position.z   = cosf(state->time * 3.0f) * 0.15f + WALL_HEIGHT / 2.0f;
				state->game.monster_normal       = normalize(dampen(state->game.monster_normal, normalize(ray_to_closest(state->game.lucia_position.xy, state->game.monster_position.xy)), 1.0f, SECONDS_PER_UPDATE));

				bool32 exists_hand_object   = false;
				vf3    hand_object_position = {};

				state->game.hand_on_door = false;
				if (!state->game.holding.paper)
				{
					const WallVoxelData* data = get_wall_voxel_data(state->game.door_wall_side.voxel);
					if (data)
					{
						vf2 door_position = (state->game.door_wall_side.coordinates + (data->start + data->end) / 2.0f) * WALL_SPACING;
						vf2 ray_to_door   = ray_to_closest(state->game.lucia_position.xy, door_position);

						// @TODO@ Unstupify this.
						vf2 normal = data->normal * (state->game.door_wall_side.is_antinormal ? -1.0f : 1.0f);

						if (dot(ray_to_door, normal) < 0.0f && norm_sq(ray_to_door) < 6.0f)
						{
							state->game.hand_on_door = true;
							exists_hand_object       = true;
							hand_object_position     = vx3(door_position + normal * 0.25f, WALL_HEIGHT / 2.0f);
						}
					}
				}

				state->game.hand_hovered_item = 0;
				if (!state->game.hand_on_door && !state->game.inventory_visibility && !state->game.holding.paper)
				{
					f32 best_heuristic = 0.0f;
					FOR_ELEMS(item, state->game.item_buffer, state->game.item_count)
					{
						vf2 ray      = ray_to_closest(state->game.lucia_position.xy, item->position.xy);
						f32 distance = norm(ray);

						if (distance < 1.5f)
						{
							f32 heuristic = 1.0f / (distance + 0.5f) + square(clamp(dot(ray / distance, polar(state->game.lucia_angle)), 0.0f, 1.0f));
							if (best_heuristic <= heuristic)
							{
								best_heuristic                = heuristic;
								state->game.hand_hovered_item = item;
							}
						}
					}

					if (state->game.hand_hovered_item)
					{
						exists_hand_object   = true;
						hand_object_position = state->game.hand_hovered_item->position;
					}
				}

				if (exists_hand_object)
				{
					vf2 hand_object_ray = ray_to_closest(hand_object_position.xy, state->game.lucia_position.xy);

					state->game.hand_position.xy = hand_object_position.xy + hand_object_ray / 2.0f;
					state->game.hand_position.z  = lerp(hand_object_position.z, state->game.lucia_position.z, 0.75f);
					state->game.hand_normal      = normalize(hand_object_ray);
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
				if (state->game.heart_rate_bpm)
				{
					state->game.heart_rate_beat_keytime += SECONDS_PER_UPDATE * state->game.heart_rate_bpm / 60.0f;
				}

				if (state->game.heart_rate_beat_keytime >= 1.0f)
				{
					state->game.heart_rate_beat_keytime -= 1.0f;
					state->game.heart_rate_velocity     += 64.0f;
				}

				state->game.heart_rate_update_keytime += SECONDS_PER_UPDATE / 0.02f;
				FOR_RANGE(8)
				{
					if (state->game.heart_rate_update_keytime >= 1.0f)
					{
						state->game.heart_rate_update_keytime                       -= 1.0f;
						state->game.heart_rate_velocity                             -= state->game.heart_rate_values[mod(state->game.heart_rate_index - 1, ARRAY_CAPACITY(state->game.heart_rate_values))] * 32.0f;
						state->game.heart_rate_velocity                             *= 0.45f;
						state->game.heart_rate_values[state->game.heart_rate_index]  = state->game.heart_rate_values[mod(state->game.heart_rate_index - 1, ARRAY_CAPACITY(state->game.heart_rate_values))] + state->game.heart_rate_velocity * SECONDS_PER_UPDATE;
						state->game.heart_rate_index                                 = (state->game.heart_rate_index + 1) % ARRAY_CAPACITY(state->game.heart_rate_values);
					}
					else
					{
						break;
					}
				}

				persist bool32 CEILING_ACTIVATION = false; // @TEMP@
				if (PRESSED(Input::left))
				{
					CEILING_ACTIVATION = !CEILING_ACTIVATION;
				}

				if (CEILING_ACTIVATION)
				{
					state->game.ceiling_lights_deactivation_keytime = clamp(state->game.ceiling_lights_deactivation_keytime + SECONDS_PER_UPDATE / 1.0f, 0.0f, 1.0f);
				}
				else
				{
					state->game.ceiling_lights_deactivation_keytime = clamp(state->game.ceiling_lights_deactivation_keytime - SECONDS_PER_UPDATE / 0.5f, 0.0f, 1.0f);
				}
			}
		} break;

		case StateContext::end:
		{
			aliasing end = state->end;

			end.entering_keytime = clamp(end.entering_keytime + SECONDS_PER_UPDATE / 1.0f, 0.0f, 1.0f);

			if (end.entering_keytime == 1.0f)
			{
				if (PRESSED(Input::space))
				{
					boot_down_state(state);
					state->context    = StateContext::title_menu;
					state->title_menu = {};
					boot_up_state(platform->renderer, state);
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
			if (state->title_menu.intro.entering_keytime < 0.5f)
			{
				set_color(platform->renderer, { 0.05f, 0.1f, 0.15f, 1.0f });
				SDL_RenderClear(platform->renderer);

				blit_texture(platform->renderer, state->title_menu.background, { 0, 0 }, WIN_RES);
				draw_text
				(
					platform->renderer,
					state->major_font,
					{ WIN_RES.x * 0.05f, WIN_RES.y * 0.05f },
					FC_ALIGN_LEFT,
					1.0f,
					{ 1.0f, 1.0f, 1.0f, 1.0f },
					"Antihome"
				);
			}
			else
			{
				set_color(platform->renderer, monochrome(0.0f));
				SDL_RenderClear(platform->renderer);
			}

			if (state->title_menu.context == TitleMenuContext::title_menu || state->title_menu.context == TitleMenuContext::intro && state->title_menu.intro.entering_keytime < 0.5f)
			{
				constexpr f32 OPTION_SCALAR  = 0.425f;
				constexpr f32 OPTION_SPACING = 0.1f;
				FOR_ELEMS(it, TITLE_MENU_OPTIONS)
				{
					f32 activation = 1.0f - clamp(fabsf(it_index - state->title_menu.option_cursor_interpolated_index), 0.0f, 1.0f);
					draw_text
					(
						platform->renderer,
						state->major_font,
						{ WIN_RES.x * 0.08f, WIN_RES.y * (0.3f + it_index * OPTION_SPACING) },
						FC_ALIGN_LEFT,
						OPTION_SCALAR,
						vxx
						(
							lerp(vf3 { 0.75f, 0.75f, 0.75f }, { 1.0f, 1.0f, 0.2f }, activation),
							lerp(0.95f, 1.0f, activation)
						),
						*it
					);
				}

				draw_text
				(
					platform->renderer,
					state->major_font,
					{ WIN_RES.x * 0.07f, WIN_RES.y * (0.3f + state->title_menu.option_cursor_interpolated_index * OPTION_SPACING) },
					FC_ALIGN_RIGHT,
					OPTION_SCALAR,
					{ 1.0f, 1.0f, 0.2f, 1.0f },
					">"
				);

				if (state->title_menu.context == TitleMenuContext::intro)
				{
					blackout = state->title_menu.intro.entering_keytime * 2.0f;
				}
			}
			else if (state->title_menu.context == TitleMenuContext::intro)
			{
				set_color(platform->renderer, monochrome(0.5f));
				draw_filled_rect(platform->renderer, { PADDING, PADDING }, VIEW_RES);

				set_color(platform->renderer, monochrome(0.2f));
				draw_filled_rect(platform->renderer, { PADDING, WIN_RES.y - PADDING - HUD_RES.y }, HUD_RES);

				draw_text
				(
					platform->renderer,
					state->major_font,
					{ WIN_RES.x * 0.5f, WIN_RES.y * 0.25f },
					FC_ALIGN_CENTER,
					1.0f,
					{ 0.9f, 0.9f, 0.9f, 1.0f },
					"%d",
					INTRO_DATA[state->title_menu.intro.current_text_index].slide_index
				);

				constexpr i32 TEXT_PADDING = 5;
				draw_boxed_text
				(
					platform->renderer,
					state->minor_font,
					{ PADDING + TEXT_PADDING, WIN_RES.y - PADDING - HUD_RES.y + TEXT_PADDING },
					HUD_RES - vi2 { TEXT_PADDING, TEXT_PADDING } * 2,
					FC_ALIGN_LEFT,
					0.8f,
					{ 0.9f, 0.9f, 0.9f, 1.0f },
					"%s",
					state->title_menu.intro.text
				);

				if (state->title_menu.intro.is_exiting)
				{
					blackout = state->title_menu.intro.exiting_keytime;
				}
				else
				{
					blackout = 1.0f - (state->title_menu.intro.entering_keytime - 0.5f) * 2.0f;
				}
			}
			else if (state->title_menu.context == TitleMenuContext::settings)
			{
				constexpr f32 OPTION_SCALAR  = 0.3f;
				constexpr f32 OPTION_SPACING = 0.12f;
				FOR_ELEMS(it, SETTING_OPTIONS)
				{
					f32 activation = 1.0f - clamp(fabsf(it_index - state->title_menu.option_cursor_interpolated_index), 0.0f, 1.0f);
					draw_text
					(
						platform->renderer,
						state->major_font,
						{ WIN_RES.x * 0.1f, WIN_RES.y * (0.37f + it_index * OPTION_SPACING) },
						FC_ALIGN_LEFT,
						OPTION_SCALAR,
						vxx
						(
							lerp(vf3 { 0.75f, 0.75f, 0.75f }, { 1.0f, 1.0f, 0.2f }, activation),
							lerp(0.95f, 1.0f, activation)
						),
						*it
					);

					f32* slider_value = 0;

					switch (it_index)
					{
						case SettingOption::master_volume : slider_value = &state->master_volume; break;
						case SettingOption::brightness    : slider_value = &state->brightness;    break;
					}

					if (slider_value)
					{
						f32 baseline = FC_GetBaseline(state->major_font) * OPTION_SCALAR;
						set_color(platform->renderer, { 1.0f, 1.0f, 1.0f, 1.0f });
						draw_line(platform->renderer, vxx(vf2 { WIN_RES.x * 0.5f, WIN_RES.y * (0.37f + it_index * OPTION_SPACING) + baseline / 2.0f }), vxx(vf2 { WIN_RES.x * 0.9f, WIN_RES.y * (0.37f + it_index * OPTION_SPACING) + baseline / 2.0f }));
						draw_filled_circle(platform->renderer, vxx(vf2 { WIN_RES.x * lerp(0.5f, 0.9f, *slider_value), WIN_RES.y * (0.37f + it_index * OPTION_SPACING) + baseline / 2.0f }), 5);
					}
				}

				draw_text
				(
					platform->renderer,
					state->major_font,
					{ WIN_RES.x * 0.09f, WIN_RES.y * (0.37f + state->title_menu.option_cursor_interpolated_index * OPTION_SPACING) },
					FC_ALIGN_RIGHT,
					OPTION_SCALAR,
					{ 1.0f, 1.0f, 0.2f, 1.0f },
					">"
				);

				// @TEMP@
				draw_text
				(
					platform->renderer,
					state->minor_font,
					{ 0.0f, 0.0f },
					FC_ALIGN_LEFT,
					1.0f,
					{ 1.0f, 1.0f, 1.0f, 1.0f },
					"%d %f\n",
					state->title_menu.settings.repeat_sfx_count,
					state->title_menu.settings.repeat_sfx_keytime
				);
			}
			else if (state->title_menu.context == TitleMenuContext::credits)
			{
				constexpr strlit CREDITS[] =
					{
						"Phuc Doan\n    Programmer",
						"Mila Matthews\n    Artist",
						"Ren Stolebarger\n    Voice Actor"
					};
				FOR_ELEMS(it, CREDITS)
				{
					draw_text
					(
						platform->renderer,
						state->major_font,
						{ WIN_RES.x * 0.1f, WIN_RES.y * (0.37f + it_index * 0.12f) },
						FC_ALIGN_LEFT,
						0.3f,
						{ 1.0f, 1.0f, 1.0f, 1.0f },
						*it
					);
				}
			}
		} break;

		case StateContext::game:
		{
#if 1
			f32* view_inv_depth_buffer = memory_arena_push<f32>(&state->transient_arena, VIEW_RES.x * VIEW_RES.y);

			set_color(platform->renderer, { 0.05f, 0.1f, 0.15f, 1.0f });
			SDL_RenderClear(platform->renderer);

			LARGE_INTEGER DEBUG_PERFORMANCE_FREQ;
			QueryPerformanceFrequency(&DEBUG_PERFORMANCE_FREQ);

			u32* view_pixels;
			i32  view_pitch_;
			SDL_LockTexture(state->game.view_texture, 0, reinterpret_cast<void**>(&view_pixels), &view_pitch_);

			FOR_RANGE(x, VIEW_RES.x)
			{
				#define PROFILE false
				#if PROFILE
				constexpr i32 DEBUG_SCANS = 10'000;
				persist u64 DEBUG_CAST_total;
				persist u64 DEBUG_CAST_counter;
				LARGE_INTEGER DEBUG_CAST_li0;
				QueryPerformanceCounter(&DEBUG_CAST_li0);
				#endif

				vf2 ray_horizontal = polar(state->game.lucia_angle + (0.5f - static_cast<f32>(x) / VIEW_RES.x) * state->game.lucia_fov);

				bool32 wall_exists   = false;
				vf2    wall_normal   = { NAN, NAN };
				f32    wall_distance = NAN;
				f32    wall_portion  = NAN;

				vi2 step =
					{
						ray_horizontal.x < 0.0f ? -1 : 1,
						ray_horizontal.y < 0.0f ? -1 : 1
					};
				vf2 t_max =
					{
						((static_cast<i32>(state->game.lucia_position.x / WALL_SPACING) + (step.x == 1)) * WALL_SPACING - state->game.lucia_position.x) / ray_horizontal.x,
						((static_cast<i32>(state->game.lucia_position.y / WALL_SPACING) + (step.y == 1)) * WALL_SPACING - state->game.lucia_position.y) / ray_horizontal.y
					};
				vf2 t_delta =
					{
						step.x / ray_horizontal.x * WALL_SPACING,
						step.y / ray_horizontal.y * WALL_SPACING
					};
				vi2 wall_coordinates =
					{
						static_cast<i32>(floorf(state->game.lucia_position.x / WALL_SPACING)),
						static_cast<i32>(floorf(state->game.lucia_position.y / WALL_SPACING))
					};
				WallVoxel wall_voxel = {};
				FOR_RANGE(MAP_DIM * MAP_DIM)
				{
					FOR_ELEMS(voxel_data, WALL_VOXEL_DATA)
					{
						if (+(*get_wall_voxel(state, wall_coordinates) & voxel_data->voxel))
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
									(wall_coordinates + voxel_data->start) * WALL_SPACING,
									(wall_coordinates + voxel_data->end  ) * WALL_SPACING
								)
								&& IN_RANGE(portion, 0.0f, 1.0f)
								&& (!wall_exists || distance < wall_distance)
							)
							{
								wall_exists   = true;
								wall_normal   = voxel_data->normal;
								wall_distance = distance;
								wall_portion  = portion;
								wall_voxel    = voxel_data->voxel;
							}
						}
					}

					if (wall_exists)
					{
						break;
					}

					if (t_max.x < t_max.y)
					{
						t_max.x            += t_delta.x;
						wall_coordinates.x += step.x;
					}
					else
					{
						t_max.y            += t_delta.y;
						wall_coordinates.y += step.y;
					}
				}

				wall_coordinates.x = mod(wall_coordinates.x, MAP_DIM);
				wall_coordinates.y = mod(wall_coordinates.y, MAP_DIM);

				i32      starting_y            = 0;
				i32      ending_y              = 0;
				i32      pixel_starting_y      = 0;
				i32      pixel_ending_y        = 0;
				ImgRGBA* overlay_img           = 0;
				vf2      overlay_uv_position   = { 0.0f, 0.0f };
				vf2      overlay_uv_dimensions = { 0.0f, 0.0f };

				if (wall_exists)
				{
					starting_y       = static_cast<i32>(VIEW_RES.y / 2.0f - HORT_TO_VERT_K / state->game.lucia_fov *                state->game.lucia_position.z  / (wall_distance + 0.1f));
					ending_y         = static_cast<i32>(VIEW_RES.y / 2.0f + HORT_TO_VERT_K / state->game.lucia_fov * (WALL_HEIGHT - state->game.lucia_position.z) / (wall_distance + 0.1f));
					pixel_starting_y = MAXIMUM(0, starting_y);
					pixel_ending_y   = MINIMUM(ending_y, VIEW_RES.y);

					constexpr f32 ALIGNED_SPAN = 0.5f;
					constexpr f32 SLASH_SPAN   = ALIGNED_SPAN / SQRT2;
					if (wall_coordinates == state->game.door_wall_side.coordinates && wall_voxel == state->game.door_wall_side.voxel && dot(ray_horizontal, wall_normal) * (state->game.door_wall_side.is_antinormal ? -1.0f : 1.0f) < 0.0f)
					{
						if (+(state->game.door_wall_side.voxel & (WallVoxel::back_slash | WallVoxel::forward_slash)))
						{
							overlay_img           = &state->game.door;
							overlay_uv_position   = { 0.5f - SLASH_SPAN / 2.0f, 0.0f };
							overlay_uv_dimensions = { SLASH_SPAN, 1.0f };
						}
						else
						{
							overlay_img           = &state->game.door;
							overlay_uv_position   = { 0.5f - ALIGNED_SPAN / 2.0f, 0.0f };
							overlay_uv_dimensions = { ALIGNED_SPAN, 1.0f };
						}
					}
					else
					{
						const WallVoxelData* intersected_data = get_wall_voxel_data(wall_voxel);
						const WallVoxelData* door_data        = get_wall_voxel_data(state->game.door_wall_side.voxel);

						f32 direction =
							dot
							(
								rotate90(dot(ray_horizontal, wall_normal) < 0.0f ? -intersected_data->normal : intersected_data->normal),
								normalize
								(
									ray_to_closest
									(
										(state->game.door_wall_side.coordinates + (door_data->start + door_data->end) / 2.0f) * WALL_SPACING + (state->game.door_wall_side.is_antinormal ? -door_data->normal : door_data->normal),
										(wall_coordinates + (intersected_data->start + intersected_data->end) / 2.0f) * WALL_SPACING + (dot(ray_horizontal, wall_normal) > 0.0f ? -intersected_data->normal : intersected_data->normal)
									)
								)
							);

						constexpr f32 THRESHOLD = 0.7f;
						if (dot(ray_horizontal, wall_normal) > 0.0f)
						{
							direction *= -1.0f;
						}

						if (rng_static((wall_coordinates.x + wall_coordinates.y) * 317 + wall_coordinates.y * 171 + (dot(ray_horizontal, wall_normal) < 0.0f ? 72 : 24)) < 0.2f)
						{
							if (direction < -THRESHOLD)
							{
								overlay_img = &state->game.wall_left_arrow;
							}
							else if (direction > THRESHOLD)
							{
								overlay_img = &state->game.wall_right_arrow;
							}
						}

						overlay_uv_position   = { 0.0f, 0.0f };
						overlay_uv_dimensions = { 1.0f, 1.0f };
					}

					if (!IN_RANGE(wall_portion, overlay_uv_position.x, overlay_uv_position.x + overlay_uv_dimensions.x))
					{
						overlay_img = 0;
					}
				}

				#if PROFILE
				LARGE_INTEGER DEBUG_CAST_li1;
				QueryPerformanceCounter(&DEBUG_CAST_li1);
				DEBUG_CAST_total   += DEBUG_CAST_li1.QuadPart - DEBUG_CAST_li0.QuadPart;
				DEBUG_CAST_counter += 1;
				if (DEBUG_CAST_counter > DEBUG_SCANS)
				{
					DEBUG_printf("cast %f\t", static_cast<f64>(DEBUG_CAST_total) / DEBUG_PERFORMANCE_FREQ.QuadPart);
					DEBUG_CAST_total   = 0;
					DEBUG_CAST_counter = 0;
				}

				persist u64 DEBUG_WALL_FLOOR_CEILING_total;
				persist u64 DEBUG_WALL_FLOOR_CEILING_counter;
				LARGE_INTEGER DEBUG_WALL_FLOOR_CEILING_li0;
				QueryPerformanceCounter(&DEBUG_WALL_FLOOR_CEILING_li0);
				#endif

				lambda shader =
					[&](vf3 color, vf3 ray, vf3 normal, f32 distance)
					{
						constexpr f32 FLASHLIGHT_INNER_CUTOFF = 1.00f;
						constexpr f32 FLASHLIGHT_OUTER_CUTOFF = 0.91f;

						f32 flashlight_k =
							clamp
							(
								clamp((dot(ray, state->game.flashlight_ray) - FLASHLIGHT_OUTER_CUTOFF) / (FLASHLIGHT_INNER_CUTOFF - FLASHLIGHT_OUTER_CUTOFF), 0.0f, 1.0f)
									/ (square(distance) + 0.1f)
									* 16.0f
									* state->game.flashlight_activation,
								0.0f,
								1.0f
							);

						vf3 new_color =
							color
								* clamp
									(
										(
											0.22f
												- fabsf(dot(ray, normal)) * 0.01f
												+ ((state->game.lucia_position.z + ray.z * distance) / WALL_HEIGHT + 0.95f) * 0.7f * (0.5f - 4.0f * cube(state->game.ceiling_lights_deactivation_keytime - 0.5f))
										)
											* clamp(1.0f - distance / lerp(48.0f, 10.0f, state->game.ceiling_lights_deactivation_keytime), 0.0f, 1.0f)
											+ flashlight_k,
										0.0f,
										1.0f
									)
								+ vxx(powf(square(dot(ray, normal)), 64) * square(dot(ray, state->game.flashlight_ray)) * 0.4f * flashlight_k);

						return vf3 { clamp(new_color.x, 0.0f, 1.0f), clamp(new_color.y, 0.0f, 1.0f), clamp(new_color.z, 0.0f, 1.0f) };
					};

				FOR_RANGE(y, 0, VIEW_RES.y)
				{
					vf3 ray = normalize({ ray_horizontal.x, ray_horizontal.y, (y - VIEW_RES.y / 2.0f) * state->game.lucia_fov / HORT_TO_VERT_K });

					if (IN_RANGE(y, pixel_starting_y, pixel_ending_y))
					{
						f32 y_portion = static_cast<f32>(y - starting_y) / (ending_y - starting_y);
						f32 distance  = sqrtf(square(wall_distance) + square(y_portion * WALL_HEIGHT - state->game.lucia_position.z));
						vf3 normal    = vxx(wall_normal, 0.0f);

						vf4 overlay_color =
							overlay_img && IN_RANGE(y_portion, overlay_uv_position.y, overlay_uv_position.y + overlay_uv_dimensions.y)
								? img_color_at(overlay_img, { (wall_portion - overlay_uv_position.x) / overlay_uv_dimensions.x, (y_portion - overlay_uv_position.y) / overlay_uv_dimensions.y })
								: vf4 { 0.0f, 0.0, 0.0f, 0.0f };

						view_pixels[(VIEW_RES.y - 1 - y) * VIEW_RES.x + x] =
							pack_color
							(
								shader
								(
									lerp
									(
										mipmap_color_at(&state->game.wall, distance / 4.0f + MIPMAP_LEVELS * square(1.0f - fabsf(dot(ray, normal))), { wall_portion, y_portion }),
										overlay_color.xyz,
										overlay_color.w
									),
									ray,
									normal,
									distance
								)
							);

						view_inv_depth_buffer[x * VIEW_RES.y + y] = 1.0f / distance;
					}
					else if (fabs(ray.z) > 0.0001f)
					{
						f32     distance;
						vf2     uv;
						vf3     normal;
						Mipmap* mipmap;

						if (y < VIEW_RES.y / 2)
						{
							f32 zk   = -state->game.lucia_position.z / ray.z;
							uv       = state->game.lucia_position.xy + zk * ray.xy;
							distance = sqrtf(norm_sq(uv - state->game.lucia_position.xy) + square(state->game.lucia_position.z));
							normal   = { 0.0f, 0.0f, 1.0f };
							mipmap   = &state->game.floor;
						}
						else
						{
							f32 zk   = (WALL_HEIGHT - state->game.lucia_position.z) / ray.z;
							uv       = state->game.lucia_position.xy + zk * ray.xy;
							distance = sqrtf(norm_sq(uv - state->game.lucia_position.xy) + square(WALL_HEIGHT - state->game.lucia_position.z));
							normal   = { 0.0f, 0.0f, -1.0f };
							mipmap   = &state->game.ceiling;
						}

						uv.x = mod(uv.x / 4.0f, 1.0f);
						uv.y = mod(uv.y / 4.0f, 1.0f);

						view_pixels[(VIEW_RES.y - 1 - y) * VIEW_RES.x + x] = pack_color(shader(mipmap_color_at(mipmap, distance / 16.0f + MIPMAP_LEVELS * square(1.0f - fabsf(dot(ray, normal))), uv), ray, normal, distance));
						view_inv_depth_buffer[x * VIEW_RES.y + y] = 1.0f / distance;
					}
				}

				#if PROFILE
				LARGE_INTEGER DEBUG_WALL_FLOOR_CEILING_li1;
				QueryPerformanceCounter(&DEBUG_WALL_FLOOR_CEILING_li1);
				DEBUG_WALL_FLOOR_CEILING_total   += DEBUG_WALL_FLOOR_CEILING_li1.QuadPart - DEBUG_WALL_FLOOR_CEILING_li0.QuadPart;
				DEBUG_WALL_FLOOR_CEILING_counter += 1;
				if (DEBUG_WALL_FLOOR_CEILING_counter > DEBUG_SCANS)
				{
					DEBUG_printf("wall floor ceiling %f\t", static_cast<f64>(DEBUG_WALL_FLOOR_CEILING_total) / DEBUG_PERFORMANCE_FREQ.QuadPart);
					DEBUG_WALL_FLOOR_CEILING_total   = 0;
					DEBUG_WALL_FLOOR_CEILING_counter = 0;
				}

				persist u64 DEBUG_THING_total;
				persist u64 DEBUG_THING_counter;
				LARGE_INTEGER DEBUG_THING_li0;
				QueryPerformanceCounter(&DEBUG_THING_li0);
				#endif

				struct RenderScanNode
				{
					ImgRGBA*        img;
					vf3             position;
					vf2             normal;
					f32             distance;
					f32             portion;
					i32             starting_y;
					i32             ending_y;
					RenderScanNode* next_node;
				};

				RenderScanNode* render_scan_node = 0;
				memory_arena_checkpoint(&state->transient_arena);

				lambda intersect =
					[&](ImgRGBA* img, vf3 position, vf2 normal, vf2 dimensions)
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
								position.xy - rotate90(normal) * dimensions.x / 2.0f,
								position.xy + rotate90(normal) * dimensions.x / 2.0f
							)
							&& IN_RANGE(portion, 0.0f, 1.0f)
						)
						{
							RenderScanNode** post_node = &render_scan_node;
							while (*post_node && (*post_node)->distance > distance)
							{
								post_node = &(*post_node)->next_node;
							}

							RenderScanNode* new_node = memory_arena_push<RenderScanNode>(&state->transient_arena);
							new_node->img        = img;
							new_node->position   = position;
							new_node->normal     = normal;
							new_node->distance   = distance;
							new_node->portion    = portion;
							new_node->next_node  = *post_node;
							new_node->starting_y = static_cast<i32>(VIEW_RES.y / 2.0f + HORT_TO_VERT_K / state->game.lucia_fov * (position.z - 0.5f * dimensions.y - state->game.lucia_position.z) / (distance + 0.1f));
							new_node->ending_y   = static_cast<i32>(VIEW_RES.y / 2.0f + HORT_TO_VERT_K / state->game.lucia_fov * (position.z + 0.5f * dimensions.y - state->game.lucia_position.z) / (distance + 0.1f));
							*post_node = new_node;
						}
					};

				FOR_RANGE(i, 9) // @TODO@ Optimize
				{
					vf3 offset = vi3 { i % 3 - 1, i / 3 - 1, 0 } * MAP_DIM * WALL_SPACING;

					intersect(&state->game.monster_img, offset + state->game.monster_position, state->game.monster_normal, { 1.0f, 1.0f });

					if (state->game.hand_hovered_item || state->game.hand_on_door)
					{
						intersect(&state->game.hand_img, offset + state->game.hand_position, state->game.hand_normal, { 0.05f, 0.05f });
					}

					FOR_ELEMS(item, state->game.item_buffer, state->game.item_count)
					{
						intersect(&state->game.default_item_imgs[+item->type - +ItemType::ITEM_START], offset + item->position, item->normal, { 0.25f, 0.25f });
					}
				}

				for (RenderScanNode* node = render_scan_node; node; node = node->next_node)
				{
					i32 scan_pixel_starting_y = clamp(node->starting_y, 0, VIEW_RES.y);
					i32 scan_pixel_ending_y   = clamp(node->ending_y  , 0, VIEW_RES.y);

					FOR_RANGE(y, scan_pixel_starting_y, scan_pixel_ending_y)
					{
						vf4 scan_pixel = img_color_at(node->img, { node->portion, (static_cast<f32>(y) - node->starting_y) / (node->ending_y - node->starting_y) });

						if (scan_pixel.w)
						{
							vf3 ray = normalize({ ray_horizontal.x, ray_horizontal.y, (y - VIEW_RES.y / 2.0f) * state->game.lucia_fov / HORT_TO_VERT_K });

							if (IN_RANGE(state->game.lucia_position.z + ray.z * node->distance, 0.0f, WALL_HEIGHT))
							{
								if (view_inv_depth_buffer[x * VIEW_RES.y + y] < 1.0f / node->distance)
								{
									view_inv_depth_buffer[x * VIEW_RES.y + y] = 1.0f / node->distance;
									view_pixels[(VIEW_RES.y - 1 - y) * VIEW_RES.x + x] =
										pack_color
										(
											lerp
											(
												unpack_color(view_pixels[y * VIEW_RES.x + x]),
												shader(scan_pixel.xyz, ray, vx3(node->normal, 0.0f), node->distance),
												scan_pixel.w
											)
										);
								}
							}
						}
					}
				}

				#if PROFILE
				LARGE_INTEGER DEBUG_THING_li1;
				QueryPerformanceCounter(&DEBUG_THING_li1);
				DEBUG_THING_total   += DEBUG_THING_li1.QuadPart - DEBUG_THING_li0.QuadPart;
				DEBUG_THING_counter += 1;
				if (DEBUG_THING_counter > DEBUG_SCANS)
				{
					DEBUG_printf("scan %f\n", static_cast<f64>(DEBUG_THING_total) / DEBUG_PERFORMANCE_FREQ.QuadPart);
					DEBUG_THING_total   = 0;
					DEBUG_THING_counter = 0;
				}
				#endif
			}

			if (state->game.holding.paper)
			{
				ImgRGBA* paper_img = &state->game.paper_imgs[0];

				vi2 paper_dimensions  = vxx(paper_img->dim * state->game.paper_scalar);
				vi2 paper_coordinates = vxx(VIEW_RES / 2.0f + (conjugate(state->game.paper_delta_position) - paper_img->dim / 2.0f) * state->game.paper_scalar);

				FOR_RANGE(x, clamp(paper_coordinates.x, 0, VIEW_RES.x), clamp(paper_coordinates.x + paper_dimensions.x, 0, VIEW_RES.x))
				{
					FOR_RANGE(y, clamp(paper_coordinates.y, 0, VIEW_RES.y), clamp(paper_coordinates.y + paper_dimensions.y, 0, VIEW_RES.y))
					{
						vf4 color = img_color_at(&state->game.paper_imgs[0], { static_cast<f32>(x - paper_coordinates.x) / paper_dimensions.x, (1.0f - static_cast<f32>(y - paper_coordinates.y) / paper_dimensions.y) });
						view_pixels[y * VIEW_RES.x + x] = pack_color(lerp(unpack_color(view_pixels[y * VIEW_RES.x + x]), color.xyz, color.w));
					}
				}
			}

			if (state->game.inventory_visibility)
			{
				FOR_RANGE(y, ARRAY_CAPACITY(state->game.inventory))
				{
					FOR_RANGE(x, ARRAY_CAPACITY(state->game.inventory[y]))
					{
						draw_img_box
						(
							view_pixels,
							state->game.inventory_grabbing && state->game.inventory_selected == &state->game.inventory[y][x]
								? 0
								: get_corresponding_item_img(state, &state->game.inventory[y][x]),
							&state->game.inventory[y][x] == state->game.inventory_selected
								? vf3 { 0.7f, 0.7f, 0.25f }
								: vf3 { 0.5f, 0.5f, 0.5f },
							{
								VIEW_RES.x / 2 + x * (INVENTORY_DIM + INVENTORY_PADDING) - static_cast<i32>(ARRAY_CAPACITY(state->game.inventory[y]) * (INVENTORY_DIM + INVENTORY_PADDING) - INVENTORY_PADDING) / 2,
								VIEW_RES.y / 2 + y * (INVENTORY_DIM + INVENTORY_PADDING) - static_cast<i32>(ARRAY_CAPACITY(state->game.inventory   ) * (INVENTORY_DIM + INVENTORY_PADDING) - INVENTORY_PADDING) / 2
							},
							INVENTORY_DIM
						);
					}
				}

				if (state->game.inventory_grabbing)
				{
					draw_img(view_pixels, get_corresponding_item_img(state, state->game.inventory_selected), vxx(vf2 { state->game.interacting_cursor.x, VIEW_RES.y - 1.0f - state->game.interacting_cursor.y }) - vi2 { INVENTORY_DIM, INVENTORY_DIM } / 4, INVENTORY_DIM / 2);
				}
			}

			if (state->game.inventory_visibility || state->game.holding.paper)
			{
				i32 cursor_dim = HOLDING(Input::left_mouse) ? 10 : 15;
				draw_img(view_pixels, &state->game.hand_img, vxx(vf2 { state->game.interacting_cursor.x - cursor_dim / 2.0f, VIEW_RES.y - 1.0f - state->game.interacting_cursor.y - cursor_dim / 2.0f }), cursor_dim);
			}

			SDL_UnlockTexture(state->game.view_texture);
			blit_texture(platform->renderer, state->game.view_texture, { PADDING, PADDING }, VIEW_RES);

			if (state->game.notification_keytime)
			{
				draw_text
				(
					platform->renderer,
					state->major_font,
					{ WIN_RES.x * 0.5f, PADDING + VIEW_RES.y * 0.8f },
					FC_ALIGN_CENTER,
					0.175f,
					{ 1.0f, 1.0f, 1.0f, sinf(TAU / 4.0f * square(state->game.notification_keytime)) },
					"%s",
					state->game.notification_message
				);
			}

			set_color(platform->renderer, monochrome(0.1f));
			draw_filled_rect(platform->renderer, { PADDING, VIEW_RES.y + PADDING * 2 }, HUD_RES);

			blit_texture
			(
				platform->renderer,
				state->game.lucia_normal,
				{ (WIN_RES.x - HUD_RES.y) / 2, WIN_RES.y - 1 - PADDING - HUD_RES.y },
				{ HUD_RES.y, HUD_RES.y }
			);

			constexpr i32 BATTERY_LEFT_PADDING       = 10;
			constexpr i32 BATTERY_TOP_BOTTOM_PADDING = 10;
			constexpr i32 BATTERY_WIDTH              = 15;
			constexpr i32 BATTERY_OUTLINE            = 2;

			set_color(platform->renderer, monochrome(0.25f));
			draw_filled_rect
			(
				platform->renderer,
				{ PADDING + BATTERY_LEFT_PADDING - BATTERY_OUTLINE, WIN_RES.y - PADDING - HUD_RES.y + BATTERY_TOP_BOTTOM_PADDING - BATTERY_OUTLINE },
				{ BATTERY_WIDTH + BATTERY_OUTLINE * 2, HUD_RES.y - BATTERY_TOP_BOTTOM_PADDING * 2 + BATTERY_OUTLINE * 2 }
			);
			draw_filled_rect
			(
				platform->renderer,
				{ PADDING + BATTERY_LEFT_PADDING + BATTERY_WIDTH / 2 - BATTERY_OUTLINE * 2, WIN_RES.y - PADDING - HUD_RES.y + BATTERY_TOP_BOTTOM_PADDING - BATTERY_OUTLINE * 2 },
				{ BATTERY_OUTLINE * 4, BATTERY_OUTLINE * 2 }
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
						state->game.holding.flashlight
							? clamp((state->game.holding.flashlight->flashlight.power - static_cast<f32>(level_index) / ARRAY_CAPACITY(BATTERY_LEVEL_COLORS)) * ARRAY_CAPACITY(BATTERY_LEVEL_COLORS), 0.0f, 1.0f)
							: 0.0f
					)
				);

				draw_filled_rect
				(
					platform->renderer,
					{ PADDING + BATTERY_LEFT_PADDING, static_cast<i32>(WIN_RES.y - PADDING - HUD_RES.y + BATTERY_TOP_BOTTOM_PADDING + (HUD_RES.y - BATTERY_TOP_BOTTOM_PADDING * 2.0f) * (1.0f - (1.0f + level_index) / static_cast<i32>(ARRAY_CAPACITY(BATTERY_LEVEL_COLORS)))) },
					{ BATTERY_WIDTH, (HUD_RES.y - BATTERY_TOP_BOTTOM_PADDING * 2) / static_cast<i32>(ARRAY_CAPACITY(BATTERY_LEVEL_COLORS)) }
				);
			}

			constexpr vi2 HEART_RATE_MONITOR_DIMENSIONS  = { 50, HUD_RES.y - 20 };
			constexpr vi2 HEART_RATE_MONITOR_COORDINATES = vi2 { WIN_RES.x - PADDING - 10, WIN_RES.y - PADDING - 10 } - HEART_RATE_MONITOR_DIMENSIONS;
			static_assert(ARRAY_CAPACITY(state->game.heart_rate_values) <= HEART_RATE_MONITOR_DIMENSIONS.x);

			set_color(platform->renderer, monochrome(0.15f));
			draw_filled_rect(platform->renderer, HEART_RATE_MONITOR_COORDINATES, HEART_RATE_MONITOR_DIMENSIONS);

			FOR_ELEMS(it, state->game.heart_rate_values, ARRAY_CAPACITY(state->game.heart_rate_values) - 1)
			{
				if (it_index + 1 != state->game.heart_rate_index)
				{
					set_color(platform->renderer, { mod(it_index - state->game.heart_rate_index, ARRAY_CAPACITY(state->game.heart_rate_values)) / static_cast<f32>(ARRAY_CAPACITY(state->game.heart_rate_values)), 0.0f, 0.0f, 1.0f });
					draw_line
					(
						platform->renderer,
						HEART_RATE_MONITOR_COORDINATES + vxx(vf2 {  it_index         / static_cast<f32>(ARRAY_CAPACITY(state->game.heart_rate_values)) * HEART_RATE_MONITOR_DIMENSIONS.x, clamp((0.75f - * it      / 2.0f), 0.0f, 1.0f) * HEART_RATE_MONITOR_DIMENSIONS.y }),
						HEART_RATE_MONITOR_COORDINATES + vxx(vf2 { (it_index + 1.0f) / static_cast<f32>(ARRAY_CAPACITY(state->game.heart_rate_values)) * HEART_RATE_MONITOR_DIMENSIONS.x, clamp((0.75f - *(it + 1) / 2.0f), 0.0f, 1.0f) * HEART_RATE_MONITOR_DIMENSIONS.y })
					);
				}
			}

			if (state->game.is_exiting)
			{
				blackout = state->game.exiting_keytime;
			}
			else
			{
				blackout = 1.0f - state->game.entering_keytime;
			}

			// @TEMP@
			draw_text
			(
				platform->renderer,
				state->minor_font,
				{ 0.0f, 0.0f },
				FC_ALIGN_LEFT,
				1.0f,
				{ 1.0f, 1.0f, 1.0f, 1.0f },
				"%f %f\n",
				state->game.lucia_position.x,
				state->game.lucia_position.y
			);
#elif 1
			set_color(platform->renderer, monochrome(0.0f));
			SDL_RenderClear(platform->renderer);

			vf2 camera_position  = state->game.lucia_position.xy;
			f32 pixels_per_meter = 64.0f;

			set_color(platform->renderer, monochrome(0.1f));
			draw_filled_rect
			(
				platform->renderer,
				vxx(WIN_RES / 2.0f + vf2 { -camera_position.x * pixels_per_meter, - 1.0f + (camera_position.y - MAP_DIM * WALL_SPACING) * pixels_per_meter }),
				vxx(vf2 { MAP_DIM * WALL_SPACING, MAP_DIM * WALL_SPACING } * pixels_per_meter)
			);

			FOR_RANGE(y, MAP_DIM)
			{
				FOR_RANGE(x, MAP_DIM)
				{
					FOR_ELEMS(it, WALL_VOXEL_DATA)
					{
						if (+(state->game.wall_voxels[y][x] & it->voxel))
						{
							vf2 start_pos = vf2 { (x + it->start.x) * WALL_SPACING, (y + it->start.y) * WALL_SPACING } - camera_position;
							vf2 end_pos   = vf2 { (x + it->end.x  ) * WALL_SPACING, (y + it->end.y  ) * WALL_SPACING } - camera_position;

							set_color(platform->renderer, monochrome(0.75f));
							SDL_RenderDrawLine
							(
								platform->renderer,
								static_cast<i32>(WIN_RES.x / 2.0f + start_pos.x * pixels_per_meter), static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - start_pos.y * pixels_per_meter),
								static_cast<i32>(WIN_RES.x / 2.0f + end_pos.x   * pixels_per_meter), static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - end_pos.y   * pixels_per_meter)
							);

							set_color(platform->renderer, { 0.5f, 0.5f, 0.25f, 1.0f });
							draw_circle(platform->renderer, { static_cast<i32>(WIN_RES.x / 2.0f + start_pos.x * pixels_per_meter), static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - start_pos.y * pixels_per_meter) }, static_cast<i32>(WALL_THICKNESS * pixels_per_meter));
							draw_circle(platform->renderer, { static_cast<i32>(WIN_RES.x / 2.0f + end_pos.x   * pixels_per_meter), static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - end_pos.y   * pixels_per_meter) }, static_cast<i32>(WALL_THICKNESS * pixels_per_meter));

							start_pos = vf2 { (x + it->start.x) * WALL_SPACING, (y + it->start.y) * WALL_SPACING } - it->normal * WALL_THICKNESS - camera_position;
							end_pos   = vf2 { (x + it->end.x  ) * WALL_SPACING, (y + it->end.y  ) * WALL_SPACING } - it->normal * WALL_THICKNESS - camera_position;
							SDL_RenderDrawLine
							(
								platform->renderer,
								static_cast<i32>(WIN_RES.x / 2.0f + start_pos.x * pixels_per_meter), static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - start_pos.y * pixels_per_meter),
								static_cast<i32>(WIN_RES.x / 2.0f + end_pos.x   * pixels_per_meter), static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - end_pos.y   * pixels_per_meter)
							);

							start_pos = vf2 { (x + it->start.x) * WALL_SPACING, (y + it->start.y) * WALL_SPACING } + it->normal * WALL_THICKNESS - camera_position;
							end_pos   = vf2 { (x + it->end.x  ) * WALL_SPACING, (y + it->end.y  ) * WALL_SPACING } + it->normal * WALL_THICKNESS - camera_position;
							SDL_RenderDrawLine
							(
								platform->renderer,
								static_cast<i32>(WIN_RES.x / 2.0f + start_pos.x * pixels_per_meter), static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - start_pos.y * pixels_per_meter),
								static_cast<i32>(WIN_RES.x / 2.0f + end_pos.x   * pixels_per_meter), static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - end_pos.y   * pixels_per_meter)
							);
						}
					}


					set_color(platform->renderer, monochrome(0.5f));
					SDL_RenderDrawPoint
					(
						platform->renderer,
						static_cast<i32>(WIN_RES.x / 2.0f + (x * WALL_SPACING - camera_position.x) * pixels_per_meter),
						static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - (y * WALL_SPACING - camera_position.y) * pixels_per_meter)
					);
				}
			}

			set_color(platform->renderer, { 0.5f, 0.25f, 0.25f, 1.0f });
			draw_circle
			(
				platform->renderer,
				vxx(WIN_RES / 2.0f + vf2 { (state->game.lucia_position.x - camera_position.x) * pixels_per_meter, -1.0f - (state->game.lucia_position.y - camera_position.y) * pixels_per_meter }),
				5
			);
			set_color(platform->renderer, { 0.5f, 0.25f, 0.25f, 1.0f });
			SDL_RenderDrawLine
			(
				platform->renderer,
				static_cast<i32>(WIN_RES.x / 2.0f + (state->game.lucia_position.x - camera_position.x) * pixels_per_meter),
				static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - (state->game.lucia_position.y - camera_position.y) * pixels_per_meter),
				static_cast<i32>(WIN_RES.x / 2.0f + (state->game.lucia_position.x + cosf(state->game.lucia_angle) * 3.0f - camera_position.x) * pixels_per_meter),
				static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - (state->game.lucia_position.y + sinf(state->game.lucia_angle) * 3.0f - camera_position.y) * pixels_per_meter)
			);

			const WallVoxelData* door_data = get_wall_voxel_data(state->game.door_wall_side.voxel);
			vf2                  door_pos  = (state->game.door_wall_side.coordinates + (door_data->start + door_data->end) / 2.0f) * WALL_SPACING + (state->game.door_wall_side.is_antinormal ? -door_data->normal : door_data->normal);
			set_color(platform->renderer, { 0.25f, 0.5f, 0.25f, 1.0f });
			draw_circle
			(
				platform->renderer,
				vxx(WIN_RES / 2.0f + vf2 { (door_pos.x - camera_position.x) * pixels_per_meter, -1.0f - (door_pos.y - camera_position.y) * pixels_per_meter }),
				5
			);
#else
			set_color(platform->renderer, monochrome(0.0f));
			SDL_RenderClear(platform->renderer);

			persist vf2 camera_position  = vf2 { MAP_DIM * WALL_SPACING / 2.0f, MAP_DIM * WALL_SPACING / 2.0f };
			f32 pixels_per_meter = 2.0f;

			constexpr f32 SPEED = 16.0f;
			if (HOLDING(Input::left))
			{
				camera_position.x -= SPEED * SECONDS_PER_UPDATE;
			}
			if (HOLDING(Input::right))
			{
				camera_position.x += SPEED * SECONDS_PER_UPDATE;
			}
			if (HOLDING(Input::down))
			{
				camera_position.y -= SPEED * SECONDS_PER_UPDATE;
			}
			if (HOLDING(Input::up))
			{
				camera_position.y += SPEED * SECONDS_PER_UPDATE;
			}

			set_color(platform->renderer, monochrome(0.1f));
			draw_filled_rect
			(
				platform->renderer,
				vxx(WIN_RES / 2.0f + vf2 { -camera_position.x * pixels_per_meter, - 1.0f + (camera_position.y - MAP_DIM * WALL_SPACING) * pixels_per_meter }),
				vxx(vf2 { MAP_DIM * WALL_SPACING, MAP_DIM * WALL_SPACING } * pixels_per_meter)
			);

			FOR_RANGE(y, MAP_DIM)
			{
				FOR_RANGE(x, MAP_DIM)
				{
					FOR_ELEMS(it, WALL_VOXEL_DATA)
					{
						if (+(state->game.wall_voxels[y][x] & it->voxel))
						{
							set_color(platform->renderer, monochrome(0.75f));
							SDL_RenderDrawLine
							(
								platform->renderer,
								static_cast<i32>(WIN_RES.x / 2.0f + ((x + it->start.x) * WALL_SPACING - camera_position.x) * pixels_per_meter),
								static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - ((y + it->start.y) * WALL_SPACING - camera_position.y) * pixels_per_meter),
								static_cast<i32>(WIN_RES.x / 2.0f + ((x + it->end.x) * WALL_SPACING - camera_position.x) * pixels_per_meter),
								static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - ((y + it->end.y) * WALL_SPACING - camera_position.y) * pixels_per_meter)
							);
						}
					}


					set_color(platform->renderer, monochrome(0.5f));
					SDL_RenderDrawPoint
					(
						platform->renderer,
						static_cast<i32>(WIN_RES.x / 2.0f + (x * WALL_SPACING - camera_position.x) * pixels_per_meter),
						static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - (y * WALL_SPACING - camera_position.y) * pixels_per_meter)
					);
				}
			}

			set_color(platform->renderer, { 0.5f, 0.25f, 0.25f, 1.0f });
			draw_circle
			(
				platform->renderer,
				vxx(WIN_RES / 2.0f + vf2 { (state->game.lucia_position.x - camera_position.x) * pixels_per_meter, -1.0f - (state->game.lucia_position.y - camera_position.y) * pixels_per_meter }),
				5
			);
			set_color(platform->renderer, { 0.5f, 0.25f, 0.25f, 1.0f });
			SDL_RenderDrawLine
			(
				platform->renderer,
				static_cast<i32>(WIN_RES.x / 2.0f + (state->game.lucia_position.x - camera_position.x) * pixels_per_meter),
				static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - (state->game.lucia_position.y - camera_position.y) * pixels_per_meter),
				static_cast<i32>(WIN_RES.x / 2.0f + (state->game.lucia_position.x + cosf(state->game.lucia_angle) * 3.0f - camera_position.x) * pixels_per_meter),
				static_cast<i32>(WIN_RES.y / 2.0f - 1.0f - (state->game.lucia_position.y + sinf(state->game.lucia_angle) * 3.0f - camera_position.y) * pixels_per_meter)
			);

			const WallVoxelData* door_data = get_wall_voxel_data(state->game.door_wall_side.voxel);
			vf2                  door_pos  = (state->game.door_wall_side.coordinates + (door_data->start + door_data->end) / 2.0f) * WALL_SPACING + (state->game.door_wall_side.is_antinormal ? -door_data->normal : door_data->normal);
			set_color(platform->renderer, { 0.25f, 0.5f, 0.25f, 1.0f });
			draw_circle
			(
				platform->renderer,
				vxx(WIN_RES / 2.0f + vf2 { (door_pos.x - camera_position.x) * pixels_per_meter, -1.0f - (door_pos.y - camera_position.y) * pixels_per_meter }),
				5
			);
#endif
		} break;

		case StateContext::end:
		{
			aliasing end = state->end;

			set_color(platform->renderer, { 0.1f, 0.05f, 0.05f, 1.0f });
			SDL_RenderClear(platform->renderer);

			draw_text
			(
				platform->renderer,
				state->major_font,
				{ WIN_RES.x * 0.5f, WIN_RES.y * 0.5f },
				FC_ALIGN_CENTER,
				1.0f,
				{ 1.0f, 1.0f, 1.0f, 1.0f },
				"END OF REPORT"
			);

			blackout = 1.0f - end.entering_keytime;
		};
	}

	set_color(platform->renderer, { 0.0f, 0.0f, 0.0f, blackout });
	draw_filled_rect(platform->renderer, { 0, 0 }, WIN_RES);

	SDL_RenderPresent(platform->renderer);
}
