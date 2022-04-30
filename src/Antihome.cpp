// @NOTE@ Credits
// "A Fast Voxel Traversal Algorithm for Ray Tracing" https://www.flipcode.com/archives/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf

#define STB_IMAGE_IMPLEMENTATION true
#include <stb_image.h>
#include "unified.h"
#include "platform.h"
#include "utilities.cpp"

global constexpr vi2 VIEW_DIM       = { 350, 175 };
global constexpr f32 HORT_TO_VERT_K = 0.927295218f * VIEW_DIM.x;
global constexpr f32 WALL_HEIGHT    = 2.7432f;
global constexpr f32 WALL_THICKNESS = 0.25f;
global constexpr f32 LUCIA_HEIGHT   = 1.4986f;
global constexpr i32 MAP_DIM        = 8;
global constexpr f32 WALL_SPACING   = 3.0f;

flag_struct (WallVoxel, u8)
{
	left          = 1 << 0,
	bottom        = 1 << 1,
	back_slash    = 1 << 2,
	forward_slash = 1 << 3
};

struct Pixel
{
	vf3 color;
	f32 inv_depth;
};

struct Sprite
{
	ImgRGBA* img;
	vf3      position;
	vf2      normal;
};

struct PathCoordinatesNode
{
	vi2                  coordinates;
	PathCoordinatesNode* next_node;
};

struct State
{
	MemoryArena          long_term_arena;
	MemoryArena          transient_arena;
	PathCoordinatesNode* available_path_coordinates_node;

	u32                  seed;
	SDL_Surface*         view;
	Pixel                frame_buffer[VIEW_DIM.x][VIEW_DIM.y];
	vf2                  lucia_velocity;
	vf3                  lucia_position;
	f32                  lucia_angle_velocity;
	f32                  lucia_angle;
	f32                  lucia_fov;
	f32                  lucia_head_bob_keytime;
	vf3                  flashlight_ray;
	f32                  flashlight_keytime;
	WallVoxel            wall_voxels[MAP_DIM][MAP_DIM];
	ImgRGB               wall;
	ImgRGB               floor;
	ImgRGB               ceiling;
	ImgRGBA              monster_img;
	Sprite               monster_sprite;
	ImgRGBA              item_img;
	Sprite               item_sprite;
	PathCoordinatesNode* monster_path;
};

global constexpr struct { WallVoxel voxel; vf2 start; vf2 end; vf2 normal; } WALL_VOXEL_DATA[] =
	{
		{ WallVoxel::left         , { 0.0f, 0.0f }, { 0.0f, 1.0f }, {     -1.0f,      0.0f } },
		{ WallVoxel::bottom       , { 0.0f, 0.0f }, { 1.0f, 0.0f }, {      0.0f,      1.0f } },
		{ WallVoxel::back_slash   , { 1.0f, 0.0f }, { 0.0f, 1.0f }, { -INVSQRT2, -INVSQRT2 } },
		{ WallVoxel::forward_slash, { 0.0f, 0.0f }, { 1.0f, 1.0f }, { -INVSQRT2,  INVSQRT2 } }
	};

internal WallVoxel* get_wall_voxel(State* state, vi2 v)
{
	return &state->wall_voxels[mod(v.y, MAP_DIM)][mod(v.x, MAP_DIM)];
}

internal PathCoordinatesNode* allocate_path_coordinates_node(State* state)
{
	if (state->available_path_coordinates_node)
	{
		PathCoordinatesNode* node = state->available_path_coordinates_node;
		state->available_path_coordinates_node = state->available_path_coordinates_node->next_node;
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
	node->next_node = state->available_path_coordinates_node;
	state->available_path_coordinates_node = node;
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
	return { coordinates.x, mod(coordinates.y, MAP_DIM * 2) / 2 };
}

internal vi2 get_closest_open_path_coordinates(State* state, vf2 position)
{
	// @TODO@ Handle torus topology?
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

internal f32 path_distance_function(vi2 coordinates_a, vi2 coordinates_b)
{
	coordinates_a.x = mod(coordinates_a.x, MAP_DIM    );
	coordinates_a.y = mod(coordinates_a.y, MAP_DIM * 2);
	coordinates_b.x = mod(coordinates_b.x, MAP_DIM    );
	coordinates_b.y = mod(coordinates_b.y, MAP_DIM * 2);

	vf2 position_a = { coordinates_a.x + (coordinates_a.y % 2 == 0 ? 0.5f : 0.0f), coordinates_a.y / 2.0f };
	vf2 position_b = { coordinates_b.x + (coordinates_b.y % 2 == 0 ? 0.5f : 0.0f), coordinates_b.y / 2.0f };
	return
		MINIMUM
		(
			MINIMUM
			(
				norm(position_a - position_b + vi2 {       0, 0 }),
				norm(position_a - position_b + vi2 { MAP_DIM, 0 })
			),
			MINIMUM
			(
				norm(position_a - position_b + vi2 {       0, MAP_DIM }),
				norm(position_a - position_b + vi2 { MAP_DIM, MAP_DIM })
			)
		);
}

internal void write_pixel(Pixel* pixel, Pixel new_pixel)
{
	if (pixel->inv_depth <= new_pixel.inv_depth)
	{
		*pixel = new_pixel;
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

	state->lucia_position  = { 3.75f * WALL_SPACING, 1.5f * WALL_SPACING, LUCIA_HEIGHT };
	state->lucia_fov       = TAU / 3.0f;

	state->monster_sprite.position = { 2.5f * WALL_SPACING, 2.5f * WALL_SPACING, LUCIA_HEIGHT };

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

	SDL_SetRelativeMouseMode(SDL_TRUE);
}

extern "C" PROTOTYPE_BOOT_UP(boot_up)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	// @TODO@ More robustiness needed here.
	state->view =
		SDL_CreateRGBSurface
		(
			0,
			VIEW_DIM.x,
			VIEW_DIM.y,
			32,
			0x000000FF,
			0x0000FF00,
			0x00FF0000,
			0xFF000000
		);

	state->wall        = init_img_rgb (DATA_DIR "wall.png");
	state->floor       = init_img_rgb (DATA_DIR "floor.png");
	state->ceiling     = init_img_rgb (DATA_DIR "ceiling.png");
	state->monster_img = init_img_rgba(DATA_DIR "sprite.png");
	state->item_img    = init_img_rgba(DATA_DIR "item.png");

	state->monster_sprite.img = &state->monster_img;
	state->item_sprite.img    = &state->item_img;
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	deinit_img_rgba(&state->item_img);
	deinit_img_rgba(&state->monster_img);
	deinit_img_rgb (&state->ceiling);
	deinit_img_rgb (&state->floor);
	deinit_img_rgb (&state->wall);
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(platform->memory);
	state->transient_arena.used = 0;

	state->lucia_angle_velocity -= platform->cursor_delta.x * 0.01f / SECONDS_PER_UPDATE;
	state->lucia_angle_velocity *= 0.4f;
	state->lucia_angle          += state->lucia_angle_velocity * SECONDS_PER_UPDATE;
	if (state->lucia_angle < 0.0f)
	{
		state->lucia_angle += TAU;
	}
	else if (state->lucia_angle >= TAU)
	{
		state->lucia_angle -= TAU;
	}

	vf2 lucia_move = { 0.0f, 0.0f };
	if (HOLDING(Input::s))
	{
		lucia_move.x -= 1.0f;
	}
	if (HOLDING(Input::w))
	{
		lucia_move.x += 1.0f;
	}
	if (HOLDING(Input::d))
	{
		lucia_move.y -= 1.0f;
	}
	if (HOLDING(Input::a))
	{
		lucia_move.y += 1.0f;
	}

	if (+lucia_move)
	{
		state->lucia_velocity += rotate(normalize(lucia_move), state->lucia_angle) * 2.0f;
	}

	state->lucia_velocity *= HOLDING(Input::shift) ? 0.75f : 0.6f;

	vf2 displacement = state->lucia_velocity * SECONDS_PER_UPDATE;

	FOR_RANGE(4)
	{
		Intersection closest_intersection;
		closest_intersection.status   = IntersectionStatus::none;
		closest_intersection.position = { NAN, NAN };
		closest_intersection.normal   = { NAN, NAN };
		closest_intersection.distance = NAN;

		vi2 step =
			{
				displacement.x < 0.0f ? -1 : 1,
				displacement.y < 0.0f ? -1 : 1
			};
		vf2 t_max =
			{
				((step.x == -1 ? floorf : ceilf)(state->lucia_position.x / WALL_SPACING) * WALL_SPACING - state->lucia_position.x) / displacement.x,
				((step.y == -1 ? floorf : ceilf)(state->lucia_position.y / WALL_SPACING) * WALL_SPACING - state->lucia_position.y) / displacement.y
			};
		vf2 t_delta =
			{
				step.x / displacement.x * WALL_SPACING,
				step.y / displacement.y * WALL_SPACING
			};
		vi2 coordinates =
			{
				static_cast<i32>(floorf(state->lucia_position.x / WALL_SPACING)),
				static_cast<i32>(floorf(state->lucia_position.y / WALL_SPACING))
			};
		FOR_RANGE(MAXIMUM(fabsf(displacement.x), fabsf(displacement.y)) + 1)
		{
			FOR_ELEMS(voxel_data, WALL_VOXEL_DATA)
			{
				if (+(*get_wall_voxel(state, coordinates) & voxel_data->voxel))
				{
					vf2 start = (coordinates + voxel_data->start) * WALL_SPACING;
					vf2 end   = (coordinates + voxel_data->end  ) * WALL_SPACING;

					Intersection intersection = intersect_thick_line_segment(state->lucia_position.xy, displacement, start, end, WALL_THICKNESS);

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
		else
		{
			state->lucia_position.xy = closest_intersection.position;
			displacement             = dot(state->lucia_position.xy + displacement - closest_intersection.position, { -closest_intersection.normal.y, closest_intersection.normal.x }) * vf2 { -closest_intersection.normal.y, closest_intersection.normal.x };
		}
	}
	state->lucia_position.xy += displacement;
	state->lucia_position.z   = LUCIA_HEIGHT + 0.1f * (cosf(state->lucia_head_bob_keytime * TAU) - 1.0f);

	state->lucia_position.x = mod(state->lucia_position.x, MAP_DIM * WALL_SPACING);
	state->lucia_position.y = mod(state->lucia_position.y, MAP_DIM * WALL_SPACING);

	state->lucia_head_bob_keytime += 0.001f + 0.35f * norm(state->lucia_velocity) * SECONDS_PER_UPDATE;
	if (state->lucia_head_bob_keytime > 1.0f)
	{
		state->lucia_head_bob_keytime -= 1.0f;
	}

	state->lucia_fov += platform->scroll * 0.1f;

	// @TEMP@
	persist f32 TEMP;
	TEMP += 1.5f * SECONDS_PER_UPDATE;

	#if 0
	if (state->monster_path)
	{
		state->monster_sprite.position.xy = dampen(state->monster_sprite.position.xy, path_coordinates_to_position(state->monster_path->coordinates), 1.0f, SECONDS_PER_UPDATE);
		if (norm_sq(state->monster_sprite.position.xy - path_coordinates_to_position(state->monster_path->coordinates)) < 1.0f)
		{
			PathCoordinatesNode* tail = state->monster_path->next_node;
			deallocate_path_coordinates_node(state, state->monster_path);
			state->monster_path = tail;
		}
	}
	#endif

	state->monster_sprite.position.z = (cosf(TEMP * 2.0f) * 0.15f) + WALL_HEIGHT / 2.0f;
	state->monster_sprite.normal = normalize(dampen(state->monster_sprite.normal, normalize(state->lucia_position.xy - state->monster_sprite.position.xy), 1.0f, SECONDS_PER_UPDATE));

	state->item_sprite.position.xy = { 1.0f, 5.0f };
	state->item_sprite.position.z = (sinf(TEMP * 2.0f) * 0.15f) + WALL_HEIGHT / 2.0f;
	state->item_sprite.normal = normalize(dampen(state->item_sprite.normal, normalize(state->lucia_position.xy - state->item_sprite.position.xy), 1.0f, SECONDS_PER_UPDATE));

	state->flashlight_keytime += 0.0025f + 0.05f * norm(state->lucia_velocity) * SECONDS_PER_UPDATE;
	if (state->flashlight_keytime > 1.0f)
	{
		state->flashlight_keytime -= 1.0f;
	}

	state->flashlight_ray.xy  = dampen(state->flashlight_ray.xy, polar(state->lucia_angle + sinf(state->flashlight_keytime * TAU * 2.0f) * 0.03f), 16.0f, SECONDS_PER_UPDATE);
	state->flashlight_ray.z   = sinf(state->flashlight_keytime * TAU) * 0.05f;
	state->flashlight_ray     = normalize(state->flashlight_ray);

	persist vi2 TEMP_OLD_CLOSEST = get_closest_open_path_coordinates(state, state->lucia_position.xy);

	if (get_closest_open_path_coordinates(state, state->lucia_position.xy) != TEMP_OLD_CLOSEST)
	{
		TEMP_OLD_CLOSEST = get_closest_open_path_coordinates(state, state->lucia_position.xy);

		for (PathCoordinatesNode* node = state->monster_path; node;)
		{
			node = deallocate_path_coordinates_node(state, node);
		}

		state->monster_path = 0;

		struct Vertex
		{
			bool32 is_set;
			f32    best_weight;
			vi2    prev_coordinates;
		};

		struct QueueNode
		{
			f32        heuristic;
			f32        total_weight;
			vi2        prev_coordinates;
			vi2        coordinates;
			QueueNode* next_node;
		};

		Vertex     graph_vertices[MAP_DIM * 2][MAP_DIM] = {};
		QueueNode* queue;

		vi2 starting_coordinates = get_closest_open_path_coordinates(state, state->monster_sprite.position.xy);
		vi2 ending_coordinates   = get_closest_open_path_coordinates(state, state->lucia_position.xy);

		graph_vertices[starting_coordinates.y][starting_coordinates.x].is_set           = true;
		graph_vertices[starting_coordinates.y][starting_coordinates.x].best_weight      = 0;
		graph_vertices[starting_coordinates.y][starting_coordinates.x].prev_coordinates = { -1, -1 };

		queue                   = memory_arena_push<QueueNode>(&state->transient_arena);
		queue->heuristic        = path_distance_function(starting_coordinates, ending_coordinates);
		queue->total_weight     = 0.0f;
		queue->prev_coordinates = { -1, -1 };
		queue->coordinates      = starting_coordinates;
		queue->next_node        = 0;

		while (queue && queue->coordinates != ending_coordinates)
		{
			QueueNode* head = queue;
			queue = queue->next_node; // @TODO@ Fix leak.

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

			FOR_ELEMS(it, (head->coordinates.y % 2 == 0 ? VERT : HORI))
			{
				vi2 next_coordinates = head->coordinates + it->delta_coordinates;
				next_coordinates.x = mod(next_coordinates.x, MAP_DIM    );
				next_coordinates.y = mod(next_coordinates.y, MAP_DIM * 2);

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
					it->delta_coordinates != vi2 { 0, -1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates)) & WallVoxel::back_slash) ||
					it->delta_coordinates != vi2 { 0,  1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates)) & WallVoxel::forward_slash)
				)
				{
					continue;
				}

				f32     next_heuristic = path_distance_function(next_coordinates, ending_coordinates);
				f32     next_weight    = head->total_weight + path_distance_function(head->coordinates, next_coordinates);
				Vertex* next_vertex    = &graph_vertices[next_coordinates.y][next_coordinates.x];

				if (!next_vertex->is_set || next_vertex->best_weight > next_weight)
				{
					next_vertex->is_set           = true;
					next_vertex->best_weight      = next_weight;
					next_vertex->prev_coordinates = head->coordinates;

					QueueNode** post_node = &queue;
					while (*post_node && (*post_node)->heuristic < next_heuristic)
					{
						post_node = &(*post_node)->next_node;
					}
					QueueNode* new_node = memory_arena_push<QueueNode>(&state->transient_arena);
					new_node->heuristic        = next_heuristic;
					new_node->total_weight     = next_weight;
					new_node->prev_coordinates = head->coordinates;
					new_node->coordinates      = next_coordinates;
					new_node->next_node        = *post_node;
					*post_node = new_node;
				}
			}

		}

		if (queue && queue->coordinates == ending_coordinates)
		{
			vi2 coordinates = ending_coordinates;
			while (true)
			{
				PathCoordinatesNode* path_coordinates_node = allocate_path_coordinates_node(state);
				path_coordinates_node->coordinates = coordinates;
				path_coordinates_node->next_node   = state->monster_path;
				state->monster_path = path_coordinates_node;

				if (coordinates == starting_coordinates)
				{
					break;
				}

				coordinates = graph_vertices[coordinates.y][coordinates.x].prev_coordinates;
			}
		}
		else
		{
			ASSERT(false);
		}
	}


	return UpdateCode::resume;
}

extern "C" PROTOTYPE_RENDER(render)
{
	State* state = reinterpret_cast<State*>(platform->memory);
	state->transient_arena.used = 0;

	constexpr i32 VIEW_PADDING = 10;

	fill(platform->surface, { 0.05f, 0.10f, 0.15f });
	memset(state->frame_buffer, 0, sizeof(state->frame_buffer));

	#if 0
	constexpr f32 AMBIENT_LIGHT_RADIUS = 4.0f;
	constexpr f32 FLASHLIGHT_POW       = 8.0f;

	#if 0
	persist u64 DEBUG_total;
	persist u64 DEBUG_counter;
	LARGE_INTEGER DEBUG_li0;
	QueryPerformanceCounter(&DEBUG_li0);

	LARGE_INTEGER DEBUG_li1;
	QueryPerformanceCounter(&DEBUG_li1);
	DEBUG_total   += DEBUG_li1.QuadPart - DEBUG_li0.QuadPart;
	DEBUG_counter += 1;
	if (DEBUG_counter > 8'000)
	{
		DEBUG_printf("%f\n", static_cast<f64>(DEBUG_total) / DEBUG_counter);
		DEBUG_total   = 0;
		DEBUG_counter = 0;
	}
	#endif

	FOR_RANGE(x, VIEW_DIM.x)
	{
		vf2 ray_horizontal = polar(state->lucia_angle + (0.5f - static_cast<f32>(x) / VIEW_DIM.x) * state->lucia_fov);

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
				((step.x == -1 ? floorf : ceilf)(state->lucia_position.x / WALL_SPACING) * WALL_SPACING - state->lucia_position.x) / ray_horizontal.x,
				((step.y == -1 ? floorf : ceilf)(state->lucia_position.y / WALL_SPACING) * WALL_SPACING - state->lucia_position.y) / ray_horizontal.y
			};
		vf2 t_delta =
			{
				step.x / ray_horizontal.x * WALL_SPACING,
				step.y / ray_horizontal.y * WALL_SPACING
			};
		vi2 coordinates =
			{
				static_cast<i32>(floorf(state->lucia_position.x / WALL_SPACING)),
				static_cast<i32>(floorf(state->lucia_position.y / WALL_SPACING))
			};
		FOR_RANGE(MAP_DIM * MAP_DIM)
		{
			FOR_ELEMS(voxel_data, WALL_VOXEL_DATA)
			{
				if (+(*get_wall_voxel(state, coordinates) & voxel_data->voxel))
				{
					f32 distance;
					f32 portion;
					if
					(
						ray_cast_line
						(
							&distance,
							&portion,
							state->lucia_position.xy,
							ray_horizontal,
							(coordinates + voxel_data->start) * WALL_SPACING,
							(coordinates + voxel_data->end  ) * WALL_SPACING
						)
						&& IN_RANGE(portion, 0.0f, 1.0f)
						&& (!wall_exists || distance < wall_distance)
					)
					{
						wall_exists   = true;
						wall_normal   = voxel_data->normal;
						wall_distance = distance;
						wall_portion  = portion;
					}
				}
			}

			if (wall_exists)
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

		i32 starting_y       = 0;
		i32 ending_y         = 0;
		i32 pixel_starting_y = 0;
		i32 pixel_ending_y   = 0;
		if (wall_exists)
		{
			starting_y       = static_cast<i32>(VIEW_DIM.y / 2.0f - HORT_TO_VERT_K / state->lucia_fov *                state->lucia_position.z  / (wall_distance + 0.1f));
			ending_y         = static_cast<i32>(VIEW_DIM.y / 2.0f + HORT_TO_VERT_K / state->lucia_fov * (WALL_HEIGHT - state->lucia_position.z) / (wall_distance + 0.1f));
			pixel_starting_y = MAXIMUM(0, starting_y);
			pixel_ending_y   = MINIMUM(ending_y, VIEW_DIM.y);
		}

		FOR_RANGE(y, 0, VIEW_DIM.y)
		{
			vf3 ray          = normalize({ ray_horizontal.x, ray_horizontal.y, (y - VIEW_DIM.y / 2.0f) * state->lucia_fov / HORT_TO_VERT_K });
			f32 flashlight_k = powf(CLAMP(dot(ray, state->flashlight_ray), 0.0f, 1.0f), FLASHLIGHT_POW);

			if (IN_RANGE(y, pixel_starting_y, pixel_ending_y))
			{
				f32 k =
					flashlight_k
						* square(CLAMP(1.0f - wall_distance / 32.0f, 0.0f, 1.0f))
					+ fabsf(dot(ray.xy, wall_normal))
						* square(CLAMP(1.0f - wall_distance / AMBIENT_LIGHT_RADIUS, 0.0f, 1.0f));

				write_pixel
				(
					&state->frame_buffer[x][y],
					{
						*(state->wall.rgb + static_cast<i32>(wall_portion * (state->wall.dim.x - 1.0f)) * state->wall.dim.y + static_cast<i32>(static_cast<f32>(y - starting_y) / (ending_y - starting_y) * state->wall.dim.y))
							* CLAMP(k, 0.0f, 1.0f),
						1.0f / wall_distance
					}
				);
			}
			else
			{
				f32 zk       = ((y < VIEW_DIM.y / 2 ? 0 : WALL_HEIGHT) - state->lucia_position.z) / ray.z;
				vf2 portions = (state->lucia_position.xy + zk * ray.xy) / 4.0f;
				portions.x   = mod(portions.x, 1.0f);
				portions.y   = mod(portions.y, 1.0f);

				f32 distance = sqrtf(square(zk) * 2.0f - square(state->lucia_position.z));
				f32 k        =
					flashlight_k
						* square(CLAMP(1.0f - distance / 32.0f, 0.0f, 1.0f))
					+ fabsf(ray.z)
						* square(CLAMP(1.0f - distance / AMBIENT_LIGHT_RADIUS, 0.0f, 1.0f));

				ImgRGB* img = y < VIEW_DIM.y / 2 ? &state->floor : &state->ceiling;
				write_pixel
				(
					&state->frame_buffer[x][y],
					{
						*(img->rgb + static_cast<i32>(portions.x * (img->dim.x - 1.0f)) * img->dim.y + static_cast<i32>(portions.y * img->dim.y))
							* CLAMP(k, 0.0f, 1.0f),
						1.0f / distance
					}
				);
			}
		}

		struct SpriteNode
		{
			Sprite*     sprite;
			f32         distance;
			f32         portion;
			SpriteNode* next_node;
		};

		SpriteNode* intersected_sprite_node = 0;

		Sprite* sprites[] = { &state->monster_sprite, &state->item_sprite };
		FOR_ELEMS(it, sprites)
		{
			Sprite* sprite = *it;

			FOR_RANGE(i, 0, 9)
			{
				f32 distance;
				f32 portion;
				if
				(
					ray_cast_line
					(
						&distance,
						&portion,
						state->lucia_position.xy,
						ray_horizontal,
						vi2 { i % 3 - 1, i / 3 - 1 } * MAP_DIM * WALL_SPACING + sprite->position.xy - rotate90(sprite->normal) / 2.0f,
						vi2 { i % 3 - 1, i / 3 - 1 } * MAP_DIM * WALL_SPACING + sprite->position.xy + rotate90(sprite->normal) / 2.0f
					)
					&& IN_RANGE(portion, 0.0f, 1.0f)
				)
				{
					SpriteNode** post_node = &intersected_sprite_node;
					while (*post_node && (*post_node)->distance > distance)
					{
						post_node = &(*post_node)->next_node;
					}

					SpriteNode* new_node = memory_arena_push<SpriteNode>(&state->transient_arena);
					new_node->sprite    = *it;
					new_node->distance  = distance;
					new_node->portion   = portion;
					new_node->next_node = *post_node;
					*post_node = new_node;
				}
			}
		}

		for (SpriteNode* node = intersected_sprite_node; node; node = node->next_node)
		{
			i32 sprite_starting_y       = static_cast<i32>(VIEW_DIM.y / 2.0f + HORT_TO_VERT_K / state->lucia_fov * (node->sprite->position.z - 0.5f - state->lucia_position.z) / (node->distance + 0.1f));
			i32 sprite_ending_y         = static_cast<i32>(VIEW_DIM.y / 2.0f + HORT_TO_VERT_K / state->lucia_fov * (node->sprite->position.z + 0.5f - state->lucia_position.z) / (node->distance + 0.1f));
			i32 sprite_pixel_starting_y = MAXIMUM(0, sprite_starting_y);
			i32 sprite_pixel_ending_y   = MINIMUM(sprite_ending_y, VIEW_DIM.y);

			FOR_RANGE(y, sprite_pixel_starting_y, sprite_pixel_ending_y)
			{
				vf4* sprite_pixel =
					node->sprite->img->rgba
						+ static_cast<i32>(node->portion * node->sprite->img->dim.x) * node->sprite->img->dim.y
						+ static_cast<i32>(static_cast<f32>(y - sprite_starting_y) / (sprite_ending_y - sprite_starting_y) * node->sprite->img->dim.y);

				if (sprite_pixel->w)
				{
					vf3 ray          = normalize({ ray_horizontal.x, ray_horizontal.y, (y - VIEW_DIM.y / 2.0f) * state->lucia_fov / HORT_TO_VERT_K });
					f32 flashlight_k = powf(CLAMP(dot(ray, state->flashlight_ray), 0.0f, 1.0f), FLASHLIGHT_POW);
					f32 k =
						flashlight_k
							* square(CLAMP(1.0f - node->distance / 32.0f, 0.0f, 1.0f))
						+ fabsf(dot(ray, { node->sprite->normal.x, node->sprite->normal.y, 0.0f }))
							* square(CLAMP(1.0f - node->distance / AMBIENT_LIGHT_RADIUS, 0.0f, 1.0f));

					write_pixel
					(
						&state->frame_buffer[x][y],
						{
							lerp
							(
								state->frame_buffer[x][y].color,
								sprite_pixel->xyz * CLAMP(k, 0.0f, 1.0f),
								sprite_pixel->w
							),
							1.0f / node->distance
						}
					);
				}
			}
		}
	}

	FOR_RANGE(y, VIEW_DIM.y)
	{
		FOR_RANGE(x, VIEW_DIM.x)
		{
			*(reinterpret_cast<u32*>(state->view->pixels) + (VIEW_DIM.y - 1 - y) * state->view->w + x) =
				SDL_MapRGB
				(
					state->view->format,
					static_cast<u8>(state->frame_buffer[x][y].color.x * 255.0f),
					static_cast<u8>(state->frame_buffer[x][y].color.y * 255.0f),
					static_cast<u8>(state->frame_buffer[x][y].color.z * 255.0f)
				);
		}
	}
	#else
	fill(state->view, { 1.0f, 1.0f, 1.0f });

	persist vf2 cam_pos = vf2 { MAP_DIM * WALL_SPACING, MAP_DIM * WALL_SPACING } / 2.0f;

	constexpr f32 SPEED = 32.0f;
	if (HOLDING(Input::left))
	{
		cam_pos.x -= SPEED * SECONDS_PER_UPDATE;
	}
	if (HOLDING(Input::right))
	{
		cam_pos.x += SPEED * SECONDS_PER_UPDATE;
	}
	if (HOLDING(Input::down))
	{
		cam_pos.y -= SPEED * SECONDS_PER_UPDATE;
	}
	if (HOLDING(Input::up))
	{
		cam_pos.y += SPEED * SECONDS_PER_UPDATE;
	}

	constexpr f32 PIXELS_PER_METER = 10.0f;

	vf2 ray  = polar(state->lucia_angle);
	vi2 step =
		{
			ray.x < 0.0f ? -1 : 1,
			ray.y < 0.0f ? -1 : 1
		};

	vf2 t_max =
		{
			step.x == -1 ? (floorf(state->lucia_position.x / WALL_SPACING) - state->lucia_position.x / WALL_SPACING) / (ray.x / WALL_SPACING) : (ceilf(state->lucia_position.x / WALL_SPACING) - state->lucia_position.x / WALL_SPACING) / (ray.x / WALL_SPACING),
			step.y == -1 ? (floorf(state->lucia_position.y / WALL_SPACING) - state->lucia_position.y / WALL_SPACING) / (ray.y / WALL_SPACING) : (ceilf(state->lucia_position.y / WALL_SPACING) - state->lucia_position.y / WALL_SPACING) / (ray.y / WALL_SPACING)
		};

	vf2 t_delta =
		{
			step.x / (ray.x / WALL_SPACING),
			step.y / (ray.y / WALL_SPACING)
		};

	vi2 pos = { static_cast<i32>(floorf(state->lucia_position.x / WALL_SPACING)), static_cast<i32>(floorf(state->lucia_position.y / WALL_SPACING)) };
	FOR_RANGE(4)
	{
		fill
		(
			state->view,
			VIEW_DIM / 2.0f + conjugate(pos * WALL_SPACING - cam_pos) * PIXELS_PER_METER - vf2 { 0.0f, WALL_SPACING * PIXELS_PER_METER },
			vf2 { WALL_SPACING, WALL_SPACING } * PIXELS_PER_METER,
			{ 0.5f, 0.7f, 0.9f }
		);

		if (t_max.x < t_max.y)
		{
			t_max.x += t_delta.x;
			pos.x   += step.x;
		}
		else
		{
			t_max.y += t_delta.y;
			pos.y   += step.y;
		}
	}

	FOR_RANGE(i, MAP_DIM)
	{
		draw_line
		(
			state->view,
			VIEW_DIM / 2.0f + conjugate(vi2 { i,       0 } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
			VIEW_DIM / 2.0f + conjugate(vi2 { i, MAP_DIM } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
			{ 0.8f, 0.8f, 0.8f }
		);
		draw_line
		(
			state->view,
			VIEW_DIM / 2.0f + conjugate(vi2 {       0, i } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
			VIEW_DIM / 2.0f + conjugate(vi2 { MAP_DIM, i } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
			{ 0.8f, 0.8f, 0.8f }
		);
	}

	FOR_RANGE(y, MAP_DIM)
	{
		FOR_RANGE(x, MAP_DIM)
		{
			if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::left))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 1.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}

			if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::bottom))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 1.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}

			if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::back_slash))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 1.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 1.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}

			if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::forward_slash))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 1.0f, y + 1.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}

			constexpr f32 DIM = 5.0f;
			fill
			(
				state->view,
				VIEW_DIM / 2.0f + conjugate((vf2 { x + 0.5f, y + 0.0f }) * WALL_SPACING - cam_pos) * PIXELS_PER_METER - vf2 { DIM, DIM } / 2.0f,
				vf2 { DIM, DIM },
				{ 0.0f, 0.25f, 0.0f }
			);
			fill
			(
				state->view,
				VIEW_DIM / 2.0f + conjugate((vf2 { x + 0.0f, y + 0.5f }) * WALL_SPACING - cam_pos) * PIXELS_PER_METER - vf2 { DIM, DIM } / 2.0f,
				vf2 { DIM, DIM },
				{ 0.0f, 0.25f, 0.0f }
			);
		}
	}

	for (PathCoordinatesNode* node = state->monster_path; node && node->next_node; node = node->next_node)
	{
		draw_line
		(
			state->view,
			VIEW_DIM / 2.0f + conjugate(path_coordinates_to_position(node           ->coordinates) - cam_pos) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f } / 4.0f,
			VIEW_DIM / 2.0f + conjugate(path_coordinates_to_position(node->next_node->coordinates) - cam_pos) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f } / 4.0f,
			abs(node->coordinates.x - node->next_node->coordinates.x) < 3 && abs(node->coordinates.y - node->next_node->coordinates.y) < 3
				? vf3 { 0.6f, 0.1f, 0.9f }
				: vf3 { 0.8f, 0.8f, 1.0f }
		);
	}

	fill(state->view, VIEW_DIM / 2.0f + conjugate(state->monster_sprite.position.xy - cam_pos) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f }, { 5.0f, 5.0f }, { 0.0f, 0.0f, 1.0f });
	fill(state->view, VIEW_DIM / 2.0f + conjugate(state->lucia_position.xy - cam_pos) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f } / 2.0f, { 2.5f, 2.5f }, { 1.0f, 0.0f, 0.0f });
	draw_line
	(
		state->view,
		VIEW_DIM / 2.0f + conjugate(state->lucia_position.xy - cam_pos                                    ) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f } / 4.0f,
		VIEW_DIM / 2.0f + conjugate(state->lucia_position.xy - cam_pos + polar(state->lucia_angle) * 6.0f) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f } / 4.0f,
		{ 0.6f, 0.1f, 0.1f }
	);
	#endif

	SDL_Rect dst = { VIEW_PADDING, VIEW_PADDING, WIN_DIM.x - VIEW_PADDING * 2, (WIN_DIM.x - VIEW_PADDING * 2) * VIEW_DIM.y / VIEW_DIM.x };
	SDL_BlitScaled(state->view, 0, platform->surface, &dst);
}
