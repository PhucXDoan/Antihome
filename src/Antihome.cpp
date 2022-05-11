/* @TODO@
	- Varying Anomal types and behaviors
	- Implemented document reading
	- Map display
	- Sound effects
	- End screen
	- Fleshed out screens

	- Handle disconnected initial and updated values
	- Mouse controls
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
global constexpr i32 MAP_DIM           = 8;
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

enum_loose (TitleMenuOption, i32)
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

enum_loose (SettingOption, i32)
{
	master_volume,
	brightness,
	done,
	CAPACITY
};

global constexpr strlit SETTING_OPTIONS[SettingOption::CAPACITY]
{
	"Master Volume",
	"Brightness",
	"Done"
};

struct RGB
{
	u8 r;
	u8 g;
	u8 b;
	u8 PADDING_;
};

struct Mipmap
{
	vi2  base_dim;
	RGB* data;
};

enum_loose (ItemType, i32)
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

struct PathCoordinatesNode
{
	vi2                  coordinates;
	PathCoordinatesNode* next_node;
};

enum struct StateContext : u32
{
	title_menu,
	game
};

enum struct TitleMenuContext : u32
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

	u32          seed;
	f32          time;
	f32          master_volume;
	f32          brightness;
	StateContext context;

	struct
	{
		SDL_Texture*     background;

		TitleMenuContext context;

		i32 option_index;
		f32 option_index_repeated_movement_countdown;
		f32 option_cursor_interpolated_index;

		struct
		{
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
		Mipmap               door;
		ImgRGBA              monster_img;
		ImgRGBA              hand_img;
		ImgRGBA              flashlight_on_img;
		ImgRGBA              default_item_imgs[ItemType::ITEM_COUNT];
		SDL_Texture*         lucia_haunted;
		SDL_Texture*         lucia_healed;
		SDL_Texture*         lucia_hit;
		SDL_Texture*         lucia_normal;
		SDL_Texture*         lucia_wounded;
		SDL_Texture*         view_texture;

		f32                  entering_keytime;
		bool32               is_exiting;
		f32                  exiting_keytime;

		f32                  view_inv_depth_buffer[VIEW_RES.x][VIEW_RES.y];
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
		f32                  ceiling_lights_keytime;
		f32                  flashlight_activation;
		vf3                  flashlight_ray;
		f32                  flashlight_keytime;
		vi2                  door_coordinates;
		WallVoxel            door_wall_voxel;

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
		vf2                  hand_normal;
		bool32               hand_on_door;
		Item*                hand_hovered_item;

		i32                  item_count;
		Item                 item_buffer[32];

		bool32               inventory_visibility;
		vf2                  inventory_cursor;
		vf2                  inventory_click_position;
		Item*                inventory_selected;
		bool32               inventory_grabbing;
		union
		{
			Item             inventory[2][4];
			Item             flat_inventory[sizeof(inventory) / sizeof(Item)];
		};

		Item*                using_flashlight;
	} game;
};

global constexpr struct { WallVoxel voxel; vf2 start; vf2 end; vf2 normal; } WALL_VOXEL_DATA[] =
	{
		{ WallVoxel::left         , { 0.0f, 0.0f }, { 0.0f, 1.0f }, {     -1.0f,      0.0f } },
		{ WallVoxel::bottom       , { 0.0f, 0.0f }, { 1.0f, 0.0f }, {      0.0f,      1.0f } },
		{ WallVoxel::back_slash   , { 1.0f, 0.0f }, { 0.0f, 1.0f }, { -INVSQRT2, -INVSQRT2 } },
		{ WallVoxel::forward_slash, { 0.0f, 0.0f }, { 1.0f, 1.0f }, { -INVSQRT2,  INVSQRT2 } }
	};

internal constexpr u32 to_pixel(vf3 color)
{
	return
		(static_cast<u8>(color.x * 255.0f) << 24) |
		(static_cast<u8>(color.y * 255.0f) << 16) |
		(static_cast<u8>(color.z * 255.0f) <<  8);
}

internal constexpr vf3 to_color(u32 pixel)
{
	return
	{
		static_cast<f32>((pixel >> 24) & 0xFF) / 255.0f,
		static_cast<f32>((pixel >> 16) & 0xFF) / 255.0f,
		static_cast<f32>((pixel >>  8) & 0xFF) / 255.0f
	};
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
			if (state->game.using_flashlight == item)
			{
				return &state->game.flashlight_on_img;
			}
			else
			{
			return &state->game.default_item_imgs[+item->type - +ItemType::ITEM_START];
			}
		} break;

		default:
		{
			return &state->game.default_item_imgs[+item->type - +ItemType::ITEM_START];
		} break;
	}
}

internal Mipmap init_mipmap(strlit file_path)
{
	Mipmap mipmap;

	vi2  absolute_dim;
	u32* stbimg = reinterpret_cast<u32*>(stbi_load(file_path, &absolute_dim.x, &absolute_dim.y, 0, STBI_rgb_alpha));
	DEFER { stbi_image_free(stbimg); };
	ASSERT(stbimg);

	mipmap.base_dim = { absolute_dim.x * 2 / 3, absolute_dim.y };
	mipmap.data     = reinterpret_cast<RGB*>(malloc((mipmap.base_dim.x * mipmap.base_dim.y * 2 - mipmap.base_dim.x * mipmap.base_dim.y * 2 / (1 << MIPMAP_LEVELS)) * sizeof(RGB)));

	vi2  stbimg_coordinates = { 0, 0 };
	RGB* mipmap_pixel = mipmap.data;
	FOR_RANGE(i, MIPMAP_LEVELS)
	{
		FOR_RANGE(ix, mipmap.base_dim.x / (1 << i))
		{
			FOR_RANGE(iy, mipmap.base_dim.y / (1 << i))
			{
				u32 stbimg_pixel = *(stbimg + (stbimg_coordinates.y + iy) * absolute_dim.x + stbimg_coordinates.x + ix);
				*mipmap_pixel++ =
					{
						static_cast<u8>(stbimg_pixel >>  0 & 0xFF),
						static_cast<u8>(stbimg_pixel >>  8 & 0xFF),
						static_cast<u8>(stbimg_pixel >> 16 & 0xFF)
					};
			}
		}

		stbimg_coordinates = { mipmap.base_dim.x, mipmap.base_dim.y - mipmap.base_dim.y / (1 << i) };
	}

	return mipmap;
}

internal void deinit_mipmap(Mipmap* mipmap)
{
	free(mipmap->data);
}

internal vf3 mipmap_color_at(Mipmap* mipmap, f32 level, vf2 uv)
{
	ASSERT(0.0f <= uv.x && uv.x <= 1.0f);
	ASSERT(0.0f <= uv.y && uv.y <= 1.0f);

	i32 l = static_cast<i32>(clamp(level, 0.0f, MIPMAP_LEVELS - 1.0f));
	RGB p =
		*(
			mipmap->data
				+ mipmap->base_dim.x * mipmap->base_dim.y * 4 / 3 - mipmap->base_dim.x * mipmap->base_dim.y * 4 / 3 / (1 << (l * 2))
				+ static_cast<i32>(uv.x * (mipmap->base_dim.x / (1 << l) - 1.0f)) * (mipmap->base_dim.y / (1 << l))
				+ static_cast<i32>((1.0f - uv.y) * (mipmap->base_dim.y / (1 << l) - 1.0f))
		);

	if (IN_RANGE(level, 0.0f, MIPMAP_LEVELS - 1.0f))
	{
		RGB q =
			*(
				mipmap->data
					+ mipmap->base_dim.x * mipmap->base_dim.y * 4 / 3 - mipmap->base_dim.x * mipmap->base_dim.y * 4 / 3 / (1 << ((l + 1) * 2))
					+ static_cast<i32>(uv.x * (mipmap->base_dim.x / (1 << (l + 1)) - 1.0f)) * (mipmap->base_dim.y / (1 << (l + 1)))
					+ static_cast<i32>((1.0f - uv.y) * (mipmap->base_dim.y / (1 << (l + 1)) - 1.0f))
			);

		return vf3 { lerp(p.r, q.r, level - l), lerp(p.g, q.g, level - l), lerp(p.b, q.b, level - l) } / 255.0f;
	}
	else
	{
		return vf3 { p.r / 255.0f, p.g / 255.0f, p.b / 255.0f };
	}
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
			vf4* rgba =
				img->rgba
					+ static_cast<i32>(static_cast<f32>(x - position.x) / dimension * img->dim.x) * img->dim.y
					+ static_cast<i32>((1.0f - static_cast<f32>(y - position.y) / dimension) * (img->dim.y - 1.0f));

			view_pixels[y * VIEW_RES.x + x] = to_pixel(lerp(to_color(view_pixels[y * VIEW_RES.x + x]), rgba->xyz, rgba->w));
		}
	}
}

internal void draw_img_box(u32* view_pixels, ImgRGBA* img, vf3 border_color, vi2 position, i32 dimension)
{
	u32 border = to_pixel(border_color);
	constexpr u32 BG     = to_pixel({ 0.2f, 0.2f, 0.2f });

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
	Intersection closest_intersection;
	closest_intersection.status   = IntersectionStatus::none;
	closest_intersection.position = { NAN, NAN };
	closest_intersection.normal   = { NAN, NAN };
	closest_intersection.distance = NAN;

	vf2 current_displacement = displacement;
	vf2 current_position     = position;
	FOR_RANGE(4)
	{
		vi2 step =
			{
				current_displacement.x < 0.0f ? -1 : 1,
				current_displacement.y < 0.0f ? -1 : 1
			};
		vf2 t_max =
			{
				((step.x == -1 ? floorf : ceilf)(current_position.x / WALL_SPACING) * WALL_SPACING - current_position.x) / current_displacement.x,
				((step.y == -1 ? floorf : ceilf)(current_position.y / WALL_SPACING) * WALL_SPACING - current_position.y) / current_displacement.y
			};
		vf2 t_delta =
			{
				step.x / current_displacement.x * WALL_SPACING,
				step.y / current_displacement.y * WALL_SPACING
			};
		vi2 coordinates =
			{
				static_cast<i32>(floorf(current_position.x / WALL_SPACING)),
				static_cast<i32>(floorf(current_position.y / WALL_SPACING))
			};

		FOR_RANGE(MAXIMUM(fabsf(current_displacement.x), fabsf(current_displacement.y)) / WALL_SPACING + 1)
		{
			FOR_ELEMS(voxel_data, WALL_VOXEL_DATA)
			{
				if (+(*get_wall_voxel(state, coordinates) & voxel_data->voxel))
				{
					Intersection intersection =
						intersect_thick_line_segment
						(
							current_position,
							current_displacement,
							(coordinates + voxel_data->start) * WALL_SPACING,
							(coordinates + voxel_data->end  ) * WALL_SPACING,
							WALL_THICKNESS
						);

					if
					(
						closest_intersection.status == IntersectionStatus::none    ||
						intersection.status         == IntersectionStatus::outside &&  closest_intersection.status == IntersectionStatus::outside && intersection.distance < closest_intersection.distance ||
						intersection.status         == IntersectionStatus::inside  && (closest_intersection.status == IntersectionStatus::outside || intersection.distance > closest_intersection.distance)
					)
					{
						closest_intersection = intersection;
					}
				}
			}

			if (closest_intersection.status != IntersectionStatus::none)
			{
				break;
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

		if (closest_intersection.status == IntersectionStatus::none)
		{
			break;
		}

		current_position = closest_intersection.position;
		current_displacement = dot(current_position + current_displacement - closest_intersection.position, rotate90(closest_intersection.normal)) * rotate90(closest_intersection.normal);
	}

	return current_position + current_displacement;
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
			((step.x == -1 ? floorf : ceilf)(position.x / WALL_SPACING) * WALL_SPACING - position.x) / ray.x,
			((step.y == -1 ? floorf : ceilf)(position.y / WALL_SPACING) * WALL_SPACING - position.y) / ray.y
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
			state->title_menu.background = IMG_LoadTexture(renderer, DATA_DIR "title_menu.png");
		} break;

		case StateContext::game:
		{
			state->game.wall              = init_mipmap(DATA_DIR "wall.png");
			state->game.floor             = init_mipmap(DATA_DIR "floor.png");
			state->game.ceiling           = init_mipmap(DATA_DIR "ceiling.png");
			state->game.door              = init_mipmap(DATA_DIR "door.png");
			state->game.monster_img       = init_img_rgba(DATA_DIR "monster.png");
			state->game.hand_img          = init_img_rgba(DATA_DIR "hand.png");
			state->game.flashlight_on_img = init_img_rgba(DATA_DIR "flashlight_on.png");

			FOR_ELEMS(it, DEFAULT_ITEM_IMG_FILE_PATHS)
			{
				state->game.default_item_imgs[it_index] = init_img_rgba(*it);
			}

			state->game.lucia_haunted    = IMG_LoadTexture(renderer, DATA_DIR "lucia_haunted.png");
			state->game.lucia_healed     = IMG_LoadTexture(renderer, DATA_DIR "lucia_healed.png");
			state->game.lucia_hit        = IMG_LoadTexture(renderer, DATA_DIR "lucia_hit.png");
			state->game.lucia_normal     = IMG_LoadTexture(renderer, DATA_DIR "lucia_normal.png");
			state->game.lucia_wounded    = IMG_LoadTexture(renderer, DATA_DIR "lucia_wounded.png");
			state->game.view_texture     = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, VIEW_RES.x, VIEW_RES.y);
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
			deinit_mipmap(&state->game.door);
			deinit_img_rgba(&state->game.monster_img);
			deinit_img_rgba(&state->game.hand_img);
			deinit_img_rgba(&state->game.flashlight_on_img);

			FOR_ELEMS(it, state->game.default_item_imgs)
			{
				deinit_img_rgba(it);
			}

			SDL_DestroyTexture(state->game.lucia_haunted);
			SDL_DestroyTexture(state->game.lucia_healed);
			SDL_DestroyTexture(state->game.lucia_hit);
			SDL_DestroyTexture(state->game.lucia_normal);
			SDL_DestroyTexture(state->game.lucia_wounded);
			SDL_DestroyTexture(state->game.view_texture);
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
}

extern "C" PROTOTYPE_BOOT_UP(boot_up)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	state->major_font = FC_CreateFont();
	FC_LoadFont(state->major_font, platform->renderer, DATA_DIR "Consolas.ttf", 32, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);

	state->minor_font = FC_CreateFont();
	FC_LoadFont(state->minor_font, platform->renderer, DATA_DIR "Consolas.ttf", 10, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);

	boot_up_state(platform->renderer, state);
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	FC_FreeFont(state->major_font);
	FC_FreeFont(state->minor_font);

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
			switch (state->title_menu.context)
			{
				case TitleMenuContext::title_menu:
				{
					state->title_menu.option_index_repeated_movement_countdown -= SECONDS_PER_UPDATE;

					// @TODO@ Compatable with arrows.
					state->title_menu.option_index += iterate_repeated_movement(platform, Input::w, Input::s, &state->title_menu.option_index_repeated_movement_countdown);
					state->title_menu.option_index  = clamp(state->title_menu.option_index, 0, +TitleMenuOption::CAPACITY - 1);

					if (PRESSED(Input::space) || PRESSED(Input::enter))
					{
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
					state->title_menu.option_index_repeated_movement_countdown -= SECONDS_PER_UPDATE;

					// @TODO@ Compatable with arrows.
					state->title_menu.option_index += iterate_repeated_movement(platform, Input::w, Input::s, &state->title_menu.option_index_repeated_movement_countdown);
					state->title_menu.option_index  = clamp(state->title_menu.option_index, 0, +SettingOption::CAPACITY - 1);

					if ((PRESSED(Input::space) || PRESSED(Input::enter)) && state->title_menu.option_index == +SettingOption::done)
					{
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

							FOR_RANGE(start_walk_y, MAP_DIM)
							{
								FOR_RANGE(start_walk_x, MAP_DIM)
								{
									if (!+*get_wall_voxel(state, { start_walk_x, start_walk_y }) && rng(&state->seed) < 0.15f)
									{
										vi2 walk = { start_walk_x, start_walk_y };
										FOR_RANGE(64)
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

								state->game.door_coordinates = door_spot_node->coordinates;
								state->game.door_wall_voxel  = door_spot_node->wall_voxel;
							}

							state->game.lucia_position.xy = rng_open_position(state);
							state->game.lucia_position.z  = LUCIA_HEIGHT;
							state->game.lucia_fov         = TAU / 3.0f;
							state->game.lucia_stamina     = 1.0f;

							state->game.monster_position.xy = rng_open_position(state);

							FOR_RANGE(ARRAY_CAPACITY(state->game.item_buffer))
							{
								Item* item = allocate_item(state);
								*item             = {};
								item->type        = static_cast<ItemType>(rng(&state->seed, +ItemType::ITEM_START, +ItemType::ITEM_END));
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
								[&]()
								{
									if (INTRO_DATA[intro.current_text_index].text[intro.next_char_index] != '\0')
									{
										if (INTRO_DATA[intro.current_text_index].text[intro.next_char_index] != '`')
										{
											ASSERT(intro.current_text_length < ARRAY_CAPACITY(intro.text) - 1);
											intro.text[intro.current_text_length] = INTRO_DATA[intro.current_text_index].text[intro.next_char_index];
											intro.current_text_length += 1;
											intro.text[intro.current_text_length] = '\0';
										}

										intro.next_char_index += 1;

										return INTRO_DATA[intro.current_text_index].text[intro.next_char_index] != '\0';
									}

									return false;
								};

							for (intro.next_char_keytime += SECONDS_PER_UPDATE / 0.05f; intro.next_char_keytime >= 1.0f; intro.next_char_keytime -= 1.0f)
							{
								iterate_text();
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
									while (iterate_text());
								}
							}
						}
					}
				} break;
			}
		} break;

		case StateContext::game:
		{
			if (state->game.is_exiting)
			{
				state->game.exiting_keytime += SECONDS_PER_UPDATE / 0.5f;

				if (state->game.exiting_keytime >= 1.0f)
				{
					boot_down_state(state);
					state->context    = StateContext::title_menu;
					state->title_menu = {};
					boot_up_state(platform->renderer, state);

					return UpdateCode::resume;
				}
			}
			else
			{
				state->game.entering_keytime = clamp(state->game.entering_keytime + SECONDS_PER_UPDATE / 1.0f, 0.0f, 1.0f);

				if (!state->game.inventory_visibility)
				{
					state->game.lucia_angle_velocity -= platform->cursor_delta.x * 0.01f / SECONDS_PER_UPDATE;
					state->game.lucia_angle_velocity *= 0.4f;
					state->game.lucia_angle           = mod(state->game.lucia_angle + state->game.lucia_angle_velocity * SECONDS_PER_UPDATE, TAU);
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
					it->position.xy  = move(state, it->position.xy, it->velocity * SECONDS_PER_UPDATE);
					it->position.z   = lerp(0.15f, state->game.lucia_position.z, clamp(1.0f - norm_sq(ray_to_closest(state->game.lucia_position.xy, it->position.xy)) / 36.0f, 0.0f, 1.0f)) + sinf(state->time * 3.0f) * 0.025f;
					it->normal       = polar(state->time * 0.7f);
				}

				state->game.lucia_position.xy = move(state, state->game.lucia_position.xy, state->game.lucia_velocity * SECONDS_PER_UPDATE);
				state->game.lucia_position.x  = mod(state->game.lucia_position.x, MAP_DIM * WALL_SPACING);
				state->game.lucia_position.y  = mod(state->game.lucia_position.y, MAP_DIM * WALL_SPACING);
				state->game.lucia_position.z  = LUCIA_HEIGHT + 0.1f * (cosf(state->game.lucia_head_bob_keytime * TAU) - 1.0f);

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
				if (+state->game.door_wall_voxel)
				{
					FOR_ELEMS(it, WALL_VOXEL_DATA)
					{
						if (it->voxel == state->game.door_wall_voxel)
						{
							vf2 door_position = (state->game.door_coordinates + (it->start + it->end) / 2.0f) * WALL_SPACING;
							if (dot(ray_to_closest(state->game.lucia_position.xy, door_position), it->normal) < 0.0f && norm_sq(door_position - state->game.lucia_position.xy) < 4.0f)
							{
								state->game.hand_on_door = true;
								exists_hand_object       = true;
								hand_object_position     = vx3(door_position + it->normal * 0.25f, WALL_HEIGHT / 2.0f);
							}

							break;
						}
					}
				}

				state->game.hand_hovered_item = 0;
				if (!state->game.hand_on_door)
				{
					f32 closest_distance = NAN;
					FOR_ELEMS(item, state->game.item_buffer, state->game.item_count)
					{
						f32 distance = norm(ray_to_closest(state->game.lucia_position.xy, item->position.xy));
						if ((!state->game.hand_hovered_item && distance < 2.0f || closest_distance > distance) && exists_clear_way(state, state->game.lucia_position.xy, item->position.xy))
						{
							state->game.hand_hovered_item = item;
							closest_distance              = distance;
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
					state->game.hand_normal      = normalize(hand_object_ray) * 0.15f;
				}

				if (PRESSED(Input::space))
				{
					if (state->game.hand_on_door)
					{
						state->game.is_exiting = true;
					}
					else if (state->game.hand_hovered_item)
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
				}

				if (state->game.using_flashlight)
				{
					state->game.using_flashlight->flashlight.power = clamp(state->game.using_flashlight->flashlight.power - SECONDS_PER_UPDATE / 60.0f, 0.0f, 1.0f);
					state->game.flashlight_activation              = dampen(state->game.flashlight_activation, sinf(TAU / 4.0f * (1.0f - powf(1.0f - state->game.using_flashlight->flashlight.power, 16.0f))), 25.0f, SECONDS_PER_UPDATE);

					if (state->game.using_flashlight->flashlight.power == 0.0f)
					{
						state->game.using_flashlight = 0;

						if (state->game.using_flashlight == state->game.using_flashlight)
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

				if (PRESSED(Input::tab))
				{
					state->game.inventory_visibility = !state->game.inventory_visibility;
					state->game.inventory_cursor     = VIEW_RES / 2.0f;
					state->game.inventory_selected   = 0;
					state->game.inventory_grabbing   = false;
				}

				if (state->game.inventory_visibility)
				{
					state->game.inventory_cursor   += conjugate(platform->cursor_delta) * 0.25f;
					state->game.inventory_cursor.x  = clamp(state->game.inventory_cursor.x, 0.0f, static_cast<f32>(VIEW_RES.x));
					state->game.inventory_cursor.y  = clamp(state->game.inventory_cursor.y, 0.0f, static_cast<f32>(VIEW_RES.y));

					if (PRESSED(Input::left_mouse))
					{
						state->game.inventory_click_position = state->game.inventory_cursor;

						FOR_RANGE(y, ARRAY_CAPACITY(state->game.inventory))
						{
							FOR_RANGE(x, ARRAY_CAPACITY(state->game.inventory[y]))
							{
								if
								(
									fabsf(VIEW_RES.x / 2.0f + (x + (1.0f - ARRAY_CAPACITY(state->game.inventory[y])) / 2.0f) * (INVENTORY_DIM + INVENTORY_PADDING) - state->game.inventory_cursor.x) < INVENTORY_DIM / 2.0f &&
									fabsf(VIEW_RES.y / 2.0f + (y + (1.0f - ARRAY_CAPACITY(state->game.inventory   )) / 2.0f) * (INVENTORY_DIM + INVENTORY_PADDING) - state->game.inventory_cursor.y) < INVENTORY_DIM / 2.0f
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
							if (!state->game.inventory_grabbing && norm_sq(state->game.inventory_cursor - state->game.inventory_click_position) > 25.0f)
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
									fabs(state->game.inventory_cursor.x * 2.0f - VIEW_RES.x) < ARRAY_CAPACITY(state->game.inventory[0]) * (INVENTORY_DIM + INVENTORY_PADDING) &&
									fabs(state->game.inventory_cursor.y * 2.0f - VIEW_RES.y) < ARRAY_CAPACITY(state->game.inventory   ) * (INVENTORY_DIM + INVENTORY_PADDING)
								)
								{
									FOR_RANGE(y, ARRAY_CAPACITY(state->game.inventory))
									{
										FOR_RANGE(x, ARRAY_CAPACITY(state->game.inventory[y]))
										{
											if
											(
												fabsf(VIEW_RES.x / 2.0f + (x + (1.0f - ARRAY_CAPACITY(state->game.inventory[y])) / 2.0f) * (INVENTORY_DIM + INVENTORY_PADDING) - state->game.inventory_cursor.x) < INVENTORY_DIM / 2.0f &&
												fabsf(VIEW_RES.y / 2.0f + (y + (1.0f - ARRAY_CAPACITY(state->game.inventory   )) / 2.0f) * (INVENTORY_DIM + INVENTORY_PADDING) - state->game.inventory_cursor.y) < INVENTORY_DIM / 2.0f
											)
											{
												if (&state->game.inventory[y][x] != state->game.inventory_selected)
												{
													if (state->game.inventory[y][x].type == ItemType::null)
													{
														// @NOTE@ Item move.

														state->game.inventory[y][x]          = *state->game.inventory_selected;
														state->game.inventory_selected->type = ItemType::null;

														if (state->game.using_flashlight == state->game.inventory_selected)
														{
															state->game.using_flashlight = &state->game.inventory[y][x];
														}
													}
													else
													{
														// @NOTE@ Item combine.

														Item* battery;
														Item* flashlight;
														if (check_combine(&battery, ItemType::battery, &flashlight, ItemType::flashlight, &state->game.inventory[y][x], state->game.inventory_selected))
														{
															state->game.notification_message = "(You replaced the batteries in the flashlight.)";
															state->game.notification_keytime = 1.0f;

															flashlight->flashlight.power = 1.0f;

															flashlight->type                 = ItemType::null;
															battery->type                    = ItemType::null;
															state->game.inventory[y][x]      = *flashlight;
															state->game.inventory[y][x].type = ItemType::flashlight;
														}
														else
														{
															state->game.notification_message = "\"I'm not sure how these fit.\"";
															state->game.notification_keytime = 1.0f;
														}
													}
												}

												break;
											}
										}
									}
								}
								else
								{
									// @NOTE@ Item drop.

									Item* dropped = allocate_item(state);
									*dropped = *state->game.inventory_selected;
									dropped->position = state->game.lucia_position;
									dropped->velocity = polar(state->game.lucia_angle + rng(&state->seed, -0.5f, 0.5f) * 1.0f) * rng(&state->seed, 1.5f, 5.0f);

									state->game.inventory_selected->type = ItemType::null;

									if (state->game.using_flashlight == state->game.inventory_selected)
									{
										state->game.using_flashlight = 0;
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

									case ItemType::flashlight:
									{
										if (state->game.inventory_selected == state->game.using_flashlight)
										{
											state->game.using_flashlight = 0;
										}
										else
										{
											if (state->game.inventory_selected->flashlight.power)
											{
												state->game.using_flashlight = 0;
												state->game.using_flashlight = state->game.inventory_selected;
											}
											else
											{
												state->game.notification_message = "\"The flashlight is dead.\"";
												state->game.notification_keytime = 1.0f;
											}
										}
									};
								}
							}

							state->game.inventory_selected = 0;
							state->game.inventory_grabbing = false;
						}
					}
				}

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
					state->game.ceiling_lights_keytime = clamp(state->game.ceiling_lights_keytime + SECONDS_PER_UPDATE / 1.0f, 0.0f, 1.0f);
				}
				else
				{
					state->game.ceiling_lights_keytime = clamp(state->game.ceiling_lights_keytime - SECONDS_PER_UPDATE / 0.5f, 0.0f, 1.0f);
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
			set_color(platform->renderer, { 0.05f, 0.1f, 0.15f, 1.0f });
			SDL_RenderClear(platform->renderer);

			LARGE_INTEGER DEBUG_PERFORMANCE_FREQ;
			QueryPerformanceFrequency(&DEBUG_PERFORMANCE_FREQ);

			u32* view_pixels;
			i32  view_pitch_;
			SDL_LockTexture(state->game.view_texture, 0, reinterpret_cast<void**>(&view_pixels), &view_pitch_);

			constexpr f32 FLASHLIGHT_INNER_CUTOFF = 1.00f;
			constexpr f32 FLASHLIGHT_OUTER_CUTOFF = 0.91f;
			constexpr f32 FLASHLIGHT_STRENGTH     = 16.0f;
			constexpr f32 AMBIENT_LIGHT_POW       = 4.0f;
			constexpr f32 AMBIENT_LIGHT_RADIUS    = 8.0f;

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

				i32 starting_y       = 0;
				i32 ending_y         = 0;
				i32 pixel_starting_y = 0;
				i32 pixel_ending_y   = 0;
				if (wall_exists)
				{
					starting_y       = static_cast<i32>(VIEW_RES.y / 2.0f - HORT_TO_VERT_K / state->game.lucia_fov *                state->game.lucia_position.z  / (wall_distance + 0.1f));
					ending_y         = static_cast<i32>(VIEW_RES.y / 2.0f + HORT_TO_VERT_K / state->game.lucia_fov * (WALL_HEIGHT - state->game.lucia_position.z) / (wall_distance + 0.1f));
					pixel_starting_y = MAXIMUM(0, starting_y);
					pixel_ending_y   = MINIMUM(ending_y, VIEW_RES.y);
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

				FOR_RANGE(y, 0, VIEW_RES.y)
				{
					vf3 ray = normalize({ ray_horizontal.x, ray_horizontal.y, (y - VIEW_RES.y / 2.0f) * state->game.lucia_fov / HORT_TO_VERT_K });

					f32     d;
					vf2     uv;
					vf3     n;
					Mipmap* mm;

					if (IN_RANGE(y, pixel_starting_y, pixel_ending_y))
					{
						uv = { wall_portion, static_cast<f32>(y - starting_y) / (ending_y - starting_y) };
						d  = sqrtf(square(wall_distance) + square(uv.y * WALL_HEIGHT - state->game.lucia_position.z));
						n  = vxx(wall_normal, 0.0f);


						if (wall_coordinates == state->game.door_coordinates && wall_voxel == state->game.door_wall_voxel && dot(wall_normal, ray.xy) < 0)
						{
							constexpr f32 SLASH_SPAN   = 0.53f;
							constexpr f32 ALIGNED_SPAN = 0.75f;
							if (+(state->game.door_wall_voxel & (WallVoxel::back_slash | WallVoxel::forward_slash)))
							{
								if (fabs(wall_portion - 0.5f) < SLASH_SPAN / 2.0f)
								{
									uv.x = (uv.x - (0.5f - SLASH_SPAN / 2.0f)) / SLASH_SPAN;
									mm   = &state->game.door;
								}
								else
								{
									mm = &state->game.wall;
								}
							}
							else if (fabs(wall_portion - 0.5f) < ALIGNED_SPAN / 2.0f)
							{
								uv.x = (uv.x - (0.5f - ALIGNED_SPAN / 2.0f)) / ALIGNED_SPAN;
								mm   = &state->game.door;
							}
							else
							{
								mm = &state->game.wall;
							}
						}
						else
						{
							mm = &state->game.wall;
						}
					}
					else if (fabs(ray.z) > 0.0001f)
					{
						if (y < VIEW_RES.y / 2)
						{
							f32 zk = -state->game.lucia_position.z / ray.z;
							uv   = state->game.lucia_position.xy + zk * ray.xy;
							d    = sqrtf(square(zk) * 2.0f - square(state->game.lucia_position.z));
							n    = { 0.0f, 0.0f, 1.0f };
							mm   = &state->game.floor;
						}
						else
						{
							f32 zk = (WALL_HEIGHT - state->game.lucia_position.z) / ray.z;
							uv   = state->game.lucia_position.xy + zk * ray.xy;
							d    = sqrtf(square(zk) * 2.0f - square(state->game.lucia_position.z));
							n    = { 0.0f, 0.0f, -1.0f };
							mm   = &state->game.ceiling;
						}

						uv.x = mod(uv.x / 4.0f, 1.0f);
						uv.y = mod(uv.y / 4.0f, 1.0f);
					}
					else
					{
						continue;
					}

					f32 flashlight_k =
						clamp
						(
							clamp((dot(ray, state->game.flashlight_ray) - FLASHLIGHT_OUTER_CUTOFF) / (FLASHLIGHT_INNER_CUTOFF - FLASHLIGHT_OUTER_CUTOFF), 0.0f, 1.0f)
								/ (square(d) + 0.1f)
								* FLASHLIGHT_STRENGTH
								* state->game.flashlight_activation,
							0.0f,
							1.0f
						);

					vf3 color =
						mipmap_color_at(mm, d / 4.0f + MIPMAP_LEVELS * square(1.0f - fabsf(dot(ray, n))), uv)
							* clamp
								(
									0.125f
										- fabsf(dot(ray, n)) * 0.01f
										+ ((state->game.lucia_position.z + ray.z * d) / WALL_HEIGHT + 0.95f) * 0.7f * (0.5f - 4.0f * cube(state->game.ceiling_lights_keytime - 0.5f))
										+ flashlight_k
										+ powf(clamp(1.0f - d / AMBIENT_LIGHT_RADIUS, 0.0f, 1.0f), AMBIENT_LIGHT_POW),
									0.0f,
									1.0f
								)
							+ vxx(powf(square(dot(ray, n)), 64) * square(dot(ray, state->game.flashlight_ray)) * 0.4f * flashlight_k);

					view_pixels[(VIEW_RES.y - 1 - y) * VIEW_RES.x + x] = to_pixel({ clamp(color.x, 0.0f, 1.0f), clamp(color.y, 0.0f, 1.0f), clamp(color.z, 0.0f, 1.0f) });
					state->game.view_inv_depth_buffer[x][y] = 1.0f / d;
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
					RenderScanNode* next_node;
				};

				RenderScanNode* render_scan_node = 0;
				memory_arena_checkpoint(&state->transient_arena);

				lambda intersect =
					[&](ImgRGBA* img, vf3 position, vf2 normal)
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
								position.xy - rotate90(normal) / 2.0f,
								position.xy + rotate90(normal) / 2.0f
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
							new_node->img       = img;
							new_node->position  = position;
							new_node->normal    = normal;
							new_node->distance  = distance;
							new_node->portion   = portion;
							new_node->next_node = *post_node;
							*post_node = new_node;
						}
					};

				FOR_RANGE(i, 9) // @TODO@ Optimize
				{
					vf3 offset = vi3 { i % 3 - 1, i / 3 - 1, 0 } * MAP_DIM * WALL_SPACING;

					intersect(&state->game.monster_img, offset + state->game.monster_position, state->game.monster_normal);

					if (state->game.hand_hovered_item || state->game.hand_on_door)
					{
						intersect(&state->game.hand_img, offset + state->game.hand_position, state->game.hand_normal);
					}

					FOR_ELEMS(item, state->game.item_buffer, state->game.item_count)
					{
						intersect(&state->game.default_item_imgs[+item->type - +ItemType::ITEM_START], offset + item->position, item->normal / 2.0f);
					}
				}

				for (RenderScanNode* node = render_scan_node; node; node = node->next_node)
				{
					i32 scan_starting_y       = static_cast<i32>(VIEW_RES.y / 2.0f + HORT_TO_VERT_K / state->game.lucia_fov * (node->position.z - norm(node->normal) / 2.0f - state->game.lucia_position.z) / (node->distance + 0.1f));
					i32 scan_ending_y         = static_cast<i32>(VIEW_RES.y / 2.0f + HORT_TO_VERT_K / state->game.lucia_fov * (node->position.z + norm(node->normal) / 2.0f - state->game.lucia_position.z) / (node->distance + 0.1f));
					i32 scan_pixel_starting_y = MAXIMUM(0, scan_starting_y);
					i32 scan_pixel_ending_y   = MINIMUM(scan_ending_y, VIEW_RES.y);

					FOR_RANGE(y, scan_pixel_starting_y, scan_pixel_ending_y)
					{
						vf4* scan_pixel =
							node->img->rgba
								+ static_cast<i32>(node->portion * node->img->dim.x) * node->img->dim.y
								+ static_cast<i32>(static_cast<f32>(y - scan_starting_y) / (scan_ending_y - scan_starting_y) * node->img->dim.y);

						if (scan_pixel->w)
						{
							vf3 ray = normalize({ ray_horizontal.x, ray_horizontal.y, (y - VIEW_RES.y / 2.0f) * state->game.lucia_fov / HORT_TO_VERT_K });

							if (IN_RANGE(state->game.lucia_position.z + ray.z * node->distance, 0.0f, WALL_HEIGHT))
							{
								if (state->game.view_inv_depth_buffer[x][y] < 1.0f / node->distance)
								{
									state->game.view_inv_depth_buffer[x][y] = 1.0f / node->distance;
									view_pixels[(VIEW_RES.y - 1 - y) * VIEW_RES.x + x] =
										to_pixel
										(
											lerp
											(
												to_color(view_pixels[y * VIEW_RES.x + x]),
												scan_pixel->xyz
													* clamp
														(
															0.125f
																- fabsf(dot(ray.xy, node->normal)) * 0.01f
																+ ((state->game.lucia_position.z + ray.z * node->distance) / WALL_HEIGHT + 0.95f) * 0.7f * (0.5f - 4.0f * cube(state->game.ceiling_lights_keytime - 0.5f))
																+ clamp((dot(ray, state->game.flashlight_ray) - FLASHLIGHT_OUTER_CUTOFF) / (FLASHLIGHT_INNER_CUTOFF - FLASHLIGHT_OUTER_CUTOFF), 0.0f, 1.0f)
																	/ (square(node->distance) + 0.1f)
																	* FLASHLIGHT_STRENGTH
																	* state->game.flashlight_activation
																+ powf(clamp(1.0f - node->distance / AMBIENT_LIGHT_RADIUS, 0.0f, 1.0f), AMBIENT_LIGHT_POW),
															0.0f,
															1.0f
														),
												scan_pixel->w
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
					draw_img(view_pixels, get_corresponding_item_img(state, state->game.inventory_selected), vxx(state->game.inventory_cursor) - vi2 { INVENTORY_DIM, INVENTORY_DIM } / 4, INVENTORY_DIM / 2);
				}

				i32 cursor_dim = HOLDING(Input::left_mouse) ? 10 : 15;
				draw_img(view_pixels, &state->game.hand_img, vxx(vf2 { state->game.inventory_cursor.x - cursor_dim / 2.0f, state->game.inventory_cursor.y - cursor_dim / 2.0f }), cursor_dim);
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
						state->game.using_flashlight
							? clamp((state->game.using_flashlight->flashlight.power - static_cast<f32>(level_index) / ARRAY_CAPACITY(BATTERY_LEVEL_COLORS)) * ARRAY_CAPACITY(BATTERY_LEVEL_COLORS), 0.0f, 1.0f)
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
		} break;
	}

	set_color(platform->renderer, { 0.0f, 0.0f, 0.0f, blackout });
	draw_filled_rect(platform->renderer, { 0, 0 }, WIN_RES);

	SDL_RenderPresent(platform->renderer);
}
