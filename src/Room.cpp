// @NOTE@ Credits
// "A Fast Voxel Traversal Algorithm for Ray Tracing" https://www.flipcode.com/archives/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf
// "How to check if two given line segments intersect?" https://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/ (http://www.dcs.gla.ac.uk/~pat/52233/slides/Geometry1x1.pdf)

#define STB_IMAGE_IMPLEMENTATION true
#include <stb_image.h>
#include "unified.h"
#include "platform.h"
#include "rng.cpp"
#include "utilities.cpp"

global constexpr vi2 DISPLAY_RES = { 800, 600 };

global constexpr i32 COMPUTER_TASKBAR_HEIGHT     = 50;
global constexpr i32 COMPUTER_TITLE_BAR_HEIGHT   = 25;
global constexpr f32 COMPUTER_SLIDER_KNOB_RADIUS = 8.0f;

global constexpr vi2 SCREEN_RES        = DISPLAY_RES / 3;
global constexpr i32 STATUS_HUD_HEIGHT = SCREEN_RES.y / 4;
global constexpr vi2 VIEW_RES          = { SCREEN_RES.x, SCREEN_RES.y - STATUS_HUD_HEIGHT };

global constexpr f32 HORT_TO_VERT_K        = 0.927295218f * VIEW_RES.x;
global constexpr f32 WALL_HEIGHT           = 2.7432f;
global constexpr f32 WALL_THICKNESS        = 0.4f;
global constexpr f32 LUCIA_HEIGHT          = 1.4986f;
global constexpr i32 MAP_DIM               = 64;
global constexpr f32 WALL_SPACING          = 3.0f;
global constexpr i32 INVENTORY_DIM         = 30;
global constexpr i32 INVENTORY_PADDING     = 5;
global constexpr f32 CREEPY_SOUND_MIN_TIME = 15.0f;
global constexpr f32 CREEPY_SOUND_MAX_TIME = 60.0f;

global constexpr vf2 CIRCUIT_BREAKER_HUD_DIMENSIONS              = { 200.0f, 115.0f };
global constexpr vf2 CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS  = { CIRCUIT_BREAKER_HUD_DIMENSIONS.x * 0.1f, CIRCUIT_BREAKER_HUD_DIMENSIONS.y * 0.75f };
global constexpr f32 CIRCUIT_BREAKER_VOLTAGE_DISPLAY_X           = VIEW_RES.x * 0.75f;
global constexpr vf2 CIRCUIT_BREAKER_SWITCH_DIMENSIONS           = { 30.0f, 35.0f };
global constexpr vf2 CIRCUIT_BREAKER_MARGINS                     = { 10.0f, 10.0f };

global constexpr f32 FIRST_AID_KIT_THRESHOLD = 0.85f;

global constexpr f32 DEATH_DURATION = 8.0f;
global constexpr f32 FRICTION       = 8.0f;

enum_loose (AudioChannel, i8)
{
	unreserved = -1,

	enum_start_region(RESERVED)
		r0,
		r1,
		r2,
	enum_end_region(RESERVED)
	enum_start_region(UNRESERVED)
		_3,
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
		DATA_DIR "audio/creepy_sound_7.wav",
		DATA_DIR "audio/creepy_sound_8.wav",
		DATA_DIR "audio/creepy_sound_9.wav",
		DATA_DIR "audio/creepy_sound_10.wav",
		DATA_DIR "audio/creepy_sound_11.wav",
		DATA_DIR "audio/creepy_sound_12.wav",
		DATA_DIR "audio/creepy_sound_13.wav",
		DATA_DIR "audio/creepy_sound_14.wav",
		DATA_DIR "audio/creepy_sound_15.wav"
	};

global constexpr strlit RADIO_WAV_FILE_PATHS[] =
	{
		DATA_DIR "audio/radio_0.wav",
		DATA_DIR "audio/radio_1.wav",
		DATA_DIR "audio/radio_2.wav",
		DATA_DIR "audio/radio_3.wav",
		DATA_DIR "audio/radio_4.wav",
		DATA_DIR "audio/radio_5.wav",
		DATA_DIR "audio/radio_6.wav",
		DATA_DIR "audio/radio_7.wav",
		DATA_DIR "audio/radio_8.wav",
		DATA_DIR "audio/radio_9.wav"
	};

global constexpr struct { f32 min_scalar; f32 max_scalar; strlit file_path; } PAPER_DATA[] =
	{
		{ 0.20f, 0.8f, DATA_DIR "papers/terry_entry_0.png" },
		{ 0.20f, 0.8f, DATA_DIR "papers/terry_entry_1.png" },
		{ 0.20f, 0.8f, DATA_DIR "papers/terry_entry_2.png" },
		{ 0.20f, 0.8f, DATA_DIR "papers/terry_entry_3.png" },
		{ 0.20f, 0.8f, DATA_DIR "papers/terry_entry_4.png" },
		{ 0.15f, 0.7f, DATA_DIR "papers/he_misses_us.jpg"  }
	};

global constexpr strlit BLOOD_DROPS[] =
	{
		DATA_DIR "blood_drop_0.png",
		DATA_DIR "blood_drop_1.png"
	};

enum_loose (ItemType, u8)
{
	null,

	enum_start_region(ITEM)
		cheap_batteries,
		paper,
		flashlight,
		cowbell,
		eye_drops,
		first_aid_kit,
		night_vision_goggles,
		pills,
		military_grade_batteries,
		radio,
	enum_end_region(ITEM)
};

global constexpr struct { strlit img_file_path; f32 spawn_weight; } ITEM_DATA[ItemType::ITEM_COUNT] =
	{
		{ DATA_DIR "items/cheap_batteries.png"          , 15.0f },
		{ DATA_DIR "items/paper.png"                    ,  5.0f },
		{ DATA_DIR "items/flashlight_off.png"           ,  6.0f },
		{ DATA_DIR "items/cowbell.png"                  ,  0.0f },
		{ DATA_DIR "items/eye_drops.png"                ,  5.0f },
		{ DATA_DIR "items/first_aid_kit.png"            ,  4.0f },
		{ DATA_DIR "items/night_vision_goggles_off.png" ,  2.0f },
		{ DATA_DIR "items/pills.png"                    ,  8.0f },
		{ DATA_DIR "items/military_grade_batteries.png" ,  3.0f },
		{ DATA_DIR "items/radio.png"                    ,  2.0f }
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

		struct
		{
			f32 power;
		} night_vision_goggles;

		struct
		{
			f32 power;
		} radio;
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

enum struct HandOnState : u8
{
	null,
	door,
	circuit_breaker,
	item
};

enum_loose (WindowType, u8)
{
	null,

	enum_start_region(TYPE)
		credits,
		room_protocol,
		settings,
		power,
	enum_end_region(TYPE)
};

global struct { vf2 position; vf2 dimensions; strlit name; strlit img_file_path; } WINDOW_ICON_DATA[WindowType::TYPE_COUNT] =
	{
		{ { 32.0f, DISPLAY_RES.y - 100.0f }, vx2(64.0f)                       , "Credits" , DATA_DIR "computer/text_file.png"             },
		{ { 32.0f, DISPLAY_RES.y - 200.0f }, vx2(64.0f)                       , "Room"    , DATA_DIR "computer/room_protocol.png"         },
		{ { 32.0f, DISPLAY_RES.y - 300.0f }, vx2(64.0f)                       , "Settings", DATA_DIR "computer/gear.png"                  },
		{ { 0.0f, 0.0f                    }, vxx(vx2(COMPUTER_TASKBAR_HEIGHT)), 0         , DATA_DIR "computer/terminal_power_button.png" }
	};

enum_loose (WindowButtonFamily, i8)
{
	enum_start_region(CREDITS)
	enum_end_region(CREDITS)

	enum_start_region(ROOM_PROTOCOL)
		room_protocol_lure,
	enum_end_region(ROOM_PROTOCOL)

	enum_start_region(SETTINGS)
	enum_end_region(SETTINGS)

	enum_start_region(POWER)
		power_no,
		power_yes,
	enum_end_region(POWER)
};

enum_loose (WindowSliderFamily, i8)
{
	null,

	enum_start_region(CREDITS)
	enum_end_region(CREDITS)

	enum_start_region(ROOM_PROTOCOL)
	enum_end_region(ROOM_PROTOCOL)

	enum_start_region(SETTINGS)
		settings_master_volume,
		settings_brightness,
	enum_end_region(SETTINGS)

	enum_start_region(POWER)
	enum_end_region(POWER)
};

global constexpr struct
	{
		vf2 dimensions;
		i32 button_count;
		struct
		{
			strlit text;
			vf3    bg_color;
			vf2    rel_centered_uv_position;
			vf2    dimensions;
		} button_buffer[4];
		i32 slider_count;
		struct
		{
			strlit text;
			f32    min_value;
			f32    max_value;
			vf2    start_uv_position;
			f32    u_length;
		} slider_buffer[4];
	} WINDOW_DATA[WindowType::TYPE_COUNT] =
	{
		{
			{ 250.0f, 300.0f },
			+WindowButtonFamily::CREDITS_COUNT,
			{},
			+WindowSliderFamily::CREDITS_COUNT,
			{}
		},
		{
			{ 275.0f, 250.0f },
			+WindowButtonFamily::ROOM_PROTOCOL_COUNT,
			{
				{ "Lure", { 0.8f, 0.2f, 0.2f }, { 0.5f, 0.33f }, { 50.0f, 25.0f } }
			},
			+WindowSliderFamily::ROOM_PROTOCOL_COUNT,
			{}
		},
		{
			{ 500.0f, 350.0f },
			+WindowButtonFamily::SETTINGS_COUNT,
			{},
			+WindowSliderFamily::SETTINGS_COUNT,
			{
				{ "Master volume", 0.0f , 1.00f, { 0.4f, 0.75f }, 0.5f },
				{ "Brightness"   , 0.25f, 1.25f, { 0.4f, 0.60f }, 0.5f }
			}
		},
		{
			{ 200.0f, 100.0f },
			+WindowButtonFamily::POWER_COUNT,
			{
				{ "No" , { 0.75f, 0.25f }, { 50.0f, 25.0f } },
				{ "Yes", { 0.25f, 0.25f }, { 50.0f, 25.0f } }
			},
			+WindowSliderFamily::POWER_COUNT,
			{}
		}
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
	vf2    position;
	bool32 active;
	i32    voltage;
};

enum struct GameGoal : u8
{
	find_door,
	fix_power,
	escape
};

enum_loose (LuciaState, u8)
{
	normal,
	anxious,
	wounded,
	anxious_wounded,
	haunted,
	hit,
	healed,
	CAPACITY
};

global constexpr strlit LUCIA_STATE_IMG_FILE_PATHS[LuciaState::CAPACITY] =
	{
		DATA_DIR "hud/lucia_normal.png",
		DATA_DIR "hud/lucia_anxious.png",
		DATA_DIR "hud/lucia_wounded.png",
		DATA_DIR "hud/lucia_anxious_wounded.png",
		DATA_DIR "hud/lucia_haunted.png",
		DATA_DIR "hud/lucia_hit.png",
		DATA_DIR "hud/lucia_healed.png"
	};

struct State
{
	MemoryArena  context_arena;
	MemoryArena  transient_arena;

	union
	{
		struct
		{
			SDL_Texture* display;
		} texture;

		SDL_Texture* textures[sizeof(texture) / sizeof(SDL_Texture*)];
	};

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

	union
	{
		struct
		{
			f32 master_volume;
			f32 brightness;
		} settings;

		f32 settings_slider_values[WindowSliderFamily::SETTINGS_COUNT];

		static_assert(sizeof(settings_slider_values) == sizeof(settings));
	};

	struct
	{
		union
		{
			struct
			{
				SDL_Texture* desktop;
				SDL_Texture* cursor;
				SDL_Texture* window_close;
				SDL_Texture* icons[WindowType::TYPE_COUNT];
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

		vf2              cursor_velocity;
		vf2              cursor;
		bool32           cursor_dragging_window;
		vf2              cursor_rel_holding_position;
		WindowSliderFamily cursor_dragging_slider;
		WindowType       window_type;
		vf2              window_velocity;
		vf2              window_position;
	} title_menu;

	struct Game
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
				TextureSprite night_vision_goggles_on;
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
				SDL_Texture* circuit_breaker_switches[2];
				SDL_Texture* circuit_breaker_panel;
				SDL_Texture* lucia_states[LuciaState::CAPACITY];
			} texture;

			SDL_Texture* textures[sizeof(texture) / sizeof(SDL_Texture*)];
		};

		union
		{
			struct
			{
				Mix_Chunk* drone;
				Mix_Chunk* drone_low;
				Mix_Chunk* drone_loud;
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
				Mix_Chunk* shock;
				Mix_Chunk* night_vision_goggles_on;
				Mix_Chunk* first_aid_kit;
				Mix_Chunk* acid_burn;
				Mix_Chunk* squelch;
				Mix_Chunk* horror       [2];
				Mix_Chunk* heartbeats   [2];
				Mix_Chunk* radio_clips  [ARRAY_CAPACITY(RADIO_WAV_FILE_PATHS)];
				Mix_Chunk* walk_steps   [ARRAY_CAPACITY(WALK_STEP_WAV_FILE_PATHS)];
				Mix_Chunk* run_steps    [ARRAY_CAPACITY(RUN_STEP_WAV_FILE_PATHS)];
				Mix_Chunk* creepy_sounds[ARRAY_CAPACITY(CREEPY_SOUND_WAV_FILE_PATHS)];
			} audio;

			Mix_Chunk* audios[sizeof(audio) / sizeof(Mix_Chunk*)];
		};

		union
		{
			struct
			{
				Mix_Music* chases[2];
			} music;

			Mix_Music* musics[sizeof(music) / sizeof(Mix_Music*)];
		};

		GameGoal             goal;

		f32                  entering_keytime;
		f32                  exiting_keytime;
		f32                  interpolated_blur;
		f32                  blur_value;

		PathCoordinatesNode* available_path_coordinates_node;
		strlit               notification_message;
		f32                  notification_keytime;
		f32                  heart_rate_display_values[32];
		i32                  heart_rate_display_index;
		f32                  heart_rate_display_update_keytime;
		f32                  heart_pulse_keytime;
		f32                  heart_pulse_time_since;
		f32                  heart_bpm;
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
		f32                  lucia_health;
		f32                  interpolated_lucia_health;
		f32                  lucia_dying_keytime;
		LuciaState           lucia_state;
		f32                  lucia_state_keytime;

		f32                  monster_timeout;
		PathCoordinatesNode* monster_path;
		vi2                  monster_path_goal;
		vf3                  monster_position;
		f32                  monster_chase_keytime;
		f32                  monster_roam_update_keytime;
		vf2                  monster_velocity;
		vf2                  monster_normal;

		vf3                  hand_position;
		HandOnState          hand_on_state;
		Item*                hand_hovered_item;

		i32                  item_count;
		Item                 item_buffer[MAP_DIM * MAP_DIM / 16];

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
				union
				{
					Item array[2][4];
					Item flat_array[sizeof(array) / sizeof(Item)];
				};
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
					BreakerSwitch switches[2][4];
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
				Item* night_vision_goggles;
				Item* radio;
			} holding;

			Item* holdings[sizeof(holding) / sizeof(Item*)];
		};

		f32    flashlight_activation;
		vf3    flashlight_ray;
		f32    flashlight_keytime;
		f32    night_vision_goggles_activation;
		f32    night_vision_goggles_scan_line_keytime;
		vf2    night_vision_goggles_interpolated_ray_to_circuit_breaker;
		vf2    night_vision_goggles_interpolated_ray_to_door;
		f32    interpolated_eye_drops_activation;
		f32    eye_drops_activation;
		f32    flash_stun_activation;
		f32    radio_keytime;
		bool8  played_radio_clips[ARRAY_CAPACITY(RADIO_WAV_FILE_PATHS)];
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
	return (wall_side.coordinates + (data->start + data->end) / 2.0f) * WALL_SPACING + wall_side.normal * normal_length;
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

		case ItemType::night_vision_goggles:
		{
			if (state->game.holding.night_vision_goggles == item)
			{
				return &state->game.texture_sprite.night_vision_goggles_on;
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
		return memory_arena_allocate<PathCoordinatesNode>(&state->context_arena);
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

internal vf2 rng_open_position(State* state, vi2 coordinates)
{
	constexpr f32 SPAWN_PADDING = 0.1f;

	vf2 rng2 = { rng(&state->seed), rng(&state->seed) };
	vf2 offset;

	if (+(*get_wall_voxel(state, coordinates) & WallVoxel::back_slash))
	{
		if (rng(&state->seed) < 0.5f)
		{
			offset = { (1.0f - 3.0f * SPAWN_PADDING) * rng2.x, (1.0f - 3.0f * SPAWN_PADDING) * (1.0f - rng2.x) * rng2.y };
		}
		else
		{
			offset = { 2.0f * SPAWN_PADDING + (1.0f - 3.0f * SPAWN_PADDING) * rng2.x, 1.0f - SPAWN_PADDING - (1.0f - 3.0f * SPAWN_PADDING) * rng2.x * rng2.y };
		}
	}
	else if (+(*get_wall_voxel(state, coordinates) & WallVoxel::forward_slash))
	{
		if (rng(&state->seed) < 0.5f)
		{
			offset = { 2.0f * SPAWN_PADDING + (1.0f - 3.0f * SPAWN_PADDING) * rng2.x, SPAWN_PADDING + (1.0f - 3.0f * SPAWN_PADDING) * rng2.x * rng2.y };
		}
		else
		{
			offset = { SPAWN_PADDING + (1.0f - 3.0f * SPAWN_PADDING) * rng2.x, 1.0f - SPAWN_PADDING - (1.0f - 3.0f * SPAWN_PADDING) * (1.0f - rng2.x) * rng2.y };
		}
	}
	else
	{
		offset = vf2 { SPAWN_PADDING, SPAWN_PADDING } + rng2 * (1.0f - 3.0f * SPAWN_PADDING);
	}

	return (coordinates + offset) * WALL_SPACING;
}

internal vf2 rng_open_position(State* state)
{
	return rng_open_position(state, { rng(&state->seed, 0, MAP_DIM), rng(&state->seed, 0, MAP_DIM) });
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

internal Item* spawn_item(State* state, ItemType type = ItemType::null)
{
	Item* item = allocate_item(state);

	if (type == ItemType::null)
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

		item->type = static_cast<ItemType>(+ItemType::ITEM_START + i);
	}
	else
	{
		item->type = type;
	}

	item->position.xy = rng_open_position(state);

	f32 most_isolated_distance = -1.0f;

	FOR_RANGE(16)
	{
		vf2 position         = rng_open_position(state);
		f32 closest_distance = MAP_DIM * WALL_SPACING;

		FOR_ELEMS(it, state->game.item_buffer, state->game.item_count)
		{
			if (it != item)
			{
				f32 distance = norm(ray_to_closest(it->position.xy, position));
				if (closest_distance == -1.0f || distance < closest_distance)
				{
					closest_distance = distance;
				}
			}
		}

		if (most_isolated_distance < closest_distance)
		{
			most_isolated_distance = closest_distance;
			item->position.xy      = position;
		}
	}

	ASSERT(most_isolated_distance != -1.0f);

	item->position.z = 0.0f;
	item->velocity   = { 0.0f, 0.0f };
	item->normal     = polar(state->time * 1.5f);

	switch (item->type)
	{
		case ItemType::paper:
		{
			item->paper.index = rng(&state->seed, 0, ARRAY_CAPACITY(PAPER_DATA));
		} break;
	}

	return item;
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

internal PathCoordinatesNode* path_find(State* state, vi2 start, vi2 end)
{
	memory_arena_checkpoint(&state->transient_arena);

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
	path_vertices[start.y][start.x].is_set = true;

	PathQueueNode* path_queue = memory_arena_allocate<PathQueueNode>(&state->transient_arena);
	path_queue->estimated_length = path_distance_function(start, end);
	path_queue->prev_coordinates = { -1, -1 };
	path_queue->coordinates      = start;
	path_queue->next_node        = 0;

	PathQueueNode* available_path_queue_node = 0;

	while (path_queue && path_queue->coordinates != end)
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

				f32             next_estimated_length = next_weight + path_distance_function(next_coordinates, end);
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

	PathCoordinatesNode* path = 0;

	if (path_queue && path_queue->coordinates == end)
	{
		vi2 coordinates = end;
		while (true)
		{
			PathCoordinatesNode* path_coordinates_node = allocate_path_coordinates_node(state);
			path_coordinates_node->coordinates = coordinates;
			path_coordinates_node->next_node   = path;
			path = path_coordinates_node;

			if (coordinates == start)
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

	return path;
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

internal vf3 shader(State* state, vf3 color, Material material, bool32 in_light, vf3 ray, vf3 normal, f32 distance)
{
	constexpr f32 FLASHLIGHT_INNER_CUTOFF = 0.95f;
	constexpr f32 FLASHLIGHT_OUTER_CUTOFF = 0.93f;

	constexpr vf3 AMBIENT_COLOR = { 1.0f, 1.0f, 1.0f };
	f32 ambient_light =
		0.5f / (square(distance) / 9.0f / (square(state->game.night_vision_goggles_activation) * 16.0f + 1.0f + (material == Material::item ? 4.0f : 0.0f) + state->game.interpolated_eye_drops_activation * 3.0f) + 1.0f)
			+ lerp(0.6f, 1.3f, (state->game.lucia_position.z + ray.z * distance) / WALL_HEIGHT) * (0.5f - 4.0f * cube(0.5f - state->game.ceiling_lights_keytime));

	if (material == Material::item)
	{
		ambient_light += 0.1f;
	}

	constexpr vf3 FLASHLIGHT_COLOR = { 1.0f, 1.0f, 0.8f };
	f32 flashlight_light =
		(1.0f + powf(square(dot(ray, normal)), 64) * square(dot(ray, state->game.flashlight_ray)) * 0.8f)
			* clamp((dot(ray, state->game.flashlight_ray) - FLASHLIGHT_OUTER_CUTOFF) / (FLASHLIGHT_INNER_CUTOFF - FLASHLIGHT_OUTER_CUTOFF), 0.0f, 1.0f)
			/ (square(distance) * 0.1f + 7.0f)
			* 6.0f
			* state->game.flashlight_activation;

	constexpr vf3 FIRE_COLOR = { 0.8863f, 0.3451f, 0.1333f };
	f32 fire_light;
	if (in_light)
	{
		if (material == Material::monster)
		{
			fire_light = 2.0f;
		}
		else if (material == Material::fire)
		{
			fire_light = 3.0f;
		}
		else
		{
			vf3 frag_position = state->game.lucia_position + ray * distance;
			vf3 frag_ray      = vx3(ray_to_closest(frag_position.xy, state->game.monster_position.xy), state->game.monster_position.z - frag_position.z);

			fire_light =
				32.0f
				* square(clamp(1.0f / (norm(frag_ray) + 0.1f), 0.0f, 1.0f))
				* fabsf(dot(normalize(frag_ray), normal));
		}
	}
	else
	{
		fire_light = 0.0f;
	}

	return vf3
		{
			clamp((color.x * (AMBIENT_COLOR.x * ambient_light + FLASHLIGHT_COLOR.x * flashlight_light + FIRE_COLOR.x * fire_light)), 0.0f, square(1.0f - state->game.lucia_dying_keytime)),
			clamp((color.y * (AMBIENT_COLOR.y * ambient_light + FLASHLIGHT_COLOR.y * flashlight_light + FIRE_COLOR.y * fire_light)), 0.0f, square(1.0f - state->game.lucia_dying_keytime)),
			clamp((color.z * (AMBIENT_COLOR.z * ambient_light + FLASHLIGHT_COLOR.z * flashlight_light + FIRE_COLOR.z * fire_light)), 0.0f, square(1.0f - state->game.lucia_dying_keytime))
		};
}

internal void boot_up_state(SDL_Renderer* renderer, State* state)
{
	switch (state->context)
	{
		case StateContext::title_menu:
		{
			state->title_menu.texture.desktop      = IMG_LoadTexture(renderer, DATA_DIR "computer/desktop.png");
			state->title_menu.texture.cursor       = IMG_LoadTexture(renderer, DATA_DIR "computer/cursor.png");
			state->title_menu.texture.window_close = IMG_LoadTexture(renderer, DATA_DIR "computer/window_close.png");

			FOR_ELEMS(it, WINDOW_ICON_DATA)
			{
				state->title_menu.texture.icons[it_index] = IMG_LoadTexture(renderer, it->img_file_path);
			}

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

			state->game.texture_sprite.hand                    = init_texture_sprite(renderer, DATA_DIR "hand.png");
			state->game.texture_sprite.flashlight_on           = init_texture_sprite(renderer, DATA_DIR "items/flashlight_on.png");
			state->game.texture_sprite.night_vision_goggles_on = init_texture_sprite(renderer, DATA_DIR "items/night_vision_goggles_on.png");
			FOR_ELEMS(it, state->game.texture_sprite.default_items)
			{
				*it = init_texture_sprite(renderer, ITEM_DATA[it_index].img_file_path);
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
			state->game.texture.circuit_breaker_switches[false] = IMG_LoadTexture(renderer, DATA_DIR "hud/circuit_breaker_switch_off.png");
			state->game.texture.circuit_breaker_switches[true ] = IMG_LoadTexture(renderer, DATA_DIR "hud/circuit_breaker_switch_on.png");
			state->game.texture.circuit_breaker_panel           = IMG_LoadTexture(renderer, DATA_DIR "hud/circuit_breaker_panel.png");
			FOR_ELEMS(it, state->game.texture.lucia_states)
			{
				*it = IMG_LoadTexture(renderer, LUCIA_STATE_IMG_FILE_PATHS[it_index]);
			}

			state->game.audio.drone                   = Mix_LoadWAV(DATA_DIR "audio/drone.wav");
			state->game.audio.drone_low               = Mix_LoadWAV(DATA_DIR "audio/drone_low.wav");
			state->game.audio.drone_loud              = Mix_LoadWAV(DATA_DIR "audio/drone_loud.wav");
			state->game.audio.drone_off               = Mix_LoadWAV(DATA_DIR "audio/drone_off.wav");
			state->game.audio.drone_on                = Mix_LoadWAV(DATA_DIR "audio/drone_on.wav");
			state->game.audio.blackout                = Mix_LoadWAV(DATA_DIR "audio/blackout.wav");
			state->game.audio.eletronical             = Mix_LoadWAV(DATA_DIR "audio/eletronical.wav");
			state->game.audio.pick_up_paper           = Mix_LoadWAV(DATA_DIR "audio/pick_up_paper.wav");
			state->game.audio.pick_up_heavy           = Mix_LoadWAV(DATA_DIR "audio/pick_up_heavy.wav");
			state->game.audio.switch_toggle           = Mix_LoadWAV(DATA_DIR "audio/switch_toggle.wav");
			state->game.audio.circuit_breaker_switch  = Mix_LoadWAV(DATA_DIR "audio/lever_flip.wav");
			state->game.audio.door_budge              = Mix_LoadWAV(DATA_DIR "audio/door_budge.wav");
			state->game.audio.panel_open              = Mix_LoadWAV(DATA_DIR "audio/panel_open.wav");
			state->game.audio.panel_close             = Mix_LoadWAV(DATA_DIR "audio/panel_close.wav");
			state->game.audio.shock                   = Mix_LoadWAV(DATA_DIR "audio/shock.wav");
			state->game.audio.night_vision_goggles_on = Mix_LoadWAV(DATA_DIR "audio/night_vision_goggles_on.wav");
			state->game.audio.first_aid_kit           = Mix_LoadWAV(DATA_DIR "audio/first_aid_kit.wav");
			state->game.audio.acid_burn               = Mix_LoadWAV(DATA_DIR "audio/acid_burn.wav");
			state->game.audio.squelch                 = Mix_LoadWAV(DATA_DIR "audio/squelch.wav");
			state->game.audio.horror[0]               = Mix_LoadWAV(DATA_DIR "audio/horror_0.wav");
			state->game.audio.horror[1]               = Mix_LoadWAV(DATA_DIR "audio/horror_1.wav");
			state->game.audio.heartbeats[0]           = Mix_LoadWAV(DATA_DIR "audio/heartbeat_0.wav");
			state->game.audio.heartbeats[1]           = Mix_LoadWAV(DATA_DIR "audio/heartbeat_1.wav");
			FOR_ELEMS(it, state->game.audio.radio_clips)
			{
				*it = Mix_LoadWAV(RADIO_WAV_FILE_PATHS[it_index]);
			}
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

			state->game.music.chases[0] = Mix_LoadMUS(DATA_DIR "audio/chase_0.wav");
			state->game.music.chases[1] = Mix_LoadMUS(DATA_DIR "audio/chase_1.wav");

			#if DEBUG
			FOR_ELEMS(it, state->game.textures) { ASSERT(*it); }
			FOR_ELEMS(it, state->game.audios  ) { ASSERT(*it); }
			FOR_ELEMS(it, state->game.musics  ) { ASSERT(*it); }
			#endif

			SDL_SetTextureBlendMode(state->game.texture.screen, SDL_BLENDMODE_BLEND);

			Mix_VolumeChunk(state->game.audio.drone, 0);
			Mix_PlayChannel(+AudioChannel::r0, state->game.audio.drone, -1);

			Mix_VolumeChunk(state->game.audio.drone_low, 0);
			Mix_PlayChannel(+AudioChannel::r1, state->game.audio.drone_low, -1);

			Mix_VolumeChunk(state->game.audio.drone_loud, 0);
			Mix_PlayChannel(+AudioChannel::r2, state->game.audio.drone_loud, -1);
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
			FOR_ELEMS(it, state->game.musics          ) { Mix_FreeMusic(*it);         }
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

	state->seed = 3;

	state->context_arena.size = static_cast<memsize>((platform->memory_capacity - sizeof(State)) * 0.75f);
	state->context_arena.base = platform->memory + sizeof(State);
	state->context_arena.used = 0;

	state->transient_arena.size = platform->memory_capacity - sizeof(State) - state->context_arena.size;
	state->transient_arena.base = platform->memory          + sizeof(State) + state->context_arena.size;
	state->transient_arena.used = 0;

	FOR_ELEMS(it, state->settings_slider_values)
	{
		*it =
			lerp
			(
				WINDOW_DATA[+WindowType::settings - +WindowType::TYPE_START].slider_buffer[it_index].min_value,
				WINDOW_DATA[+WindowType::settings - +WindowType::TYPE_START].slider_buffer[it_index].max_value,
				0.75f
			);
	}

	state->title_menu.cursor = DISPLAY_RES / 2.0f;

	SDL_SetRelativeMouseMode(SDL_TRUE);

	static_assert(+AudioChannel::RESERVED_START == 0);
	Mix_ReserveChannels(+AudioChannel::RESERVED_COUNT);
}

extern "C" PROTOTYPE_BOOT_UP(boot_up)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	state->texture.display = SDL_CreateTexture(platform->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, DISPLAY_RES.x, DISPLAY_RES.y);

	state->font.major = FC_CreateFont();
	FC_LoadFont(state->font.major, platform->renderer, DATA_DIR "Consolas.ttf", 32, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);

	state->font.minor = FC_CreateFont();
	FC_LoadFont(state->font.minor, platform->renderer, DATA_DIR "Consolas.ttf", 16, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);

	boot_up_state(platform->renderer, state);

	{
		SDL_Surface* icon = SDL_LoadBMP(DATA_DIR "room_protocol.bmp");
		SDL_SetWindowIcon(platform->window, icon);
		SDL_FreeSurface(icon);
	}
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	FOR_ELEMS(it, state->textures) { SDL_DestroyTexture(*it); }

	FOR_ELEMS(it, state->fonts) { FC_FreeFont(*it); }
	boot_down_state(state);
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	state->time                 += SECONDS_PER_UPDATE;
	state->transient_arena.used  = 0;

	if (HOLDING(Input::alt) && PRESSED(Input::enter))
	{
		if (platform->window_state == WindowState::windowed)
		{
			platform->window_state = WindowState::fullscreen;
		}
		else
		{
			platform->window_state = WindowState::windowed;
		}
	}

	Mix_Volume(-1, static_cast<i32>(MIX_MAX_VOLUME * state->settings.master_volume));

	switch (state->context)
	{
		case StateContext::title_menu:
		{
			aliasing tm = state->title_menu;

			if (PRESSED(Input::left_mouse))
			{
				if (tm.window_type != WindowType::null)
				{
					aliasing window_data = WINDOW_DATA[+tm.window_type - +WindowType::TYPE_START];

					if (in_rect(tm.cursor, tm.window_position + vf2 { 0.0f, window_data.dimensions.y }, { window_data.dimensions.x - COMPUTER_TITLE_BAR_HEIGHT, COMPUTER_TITLE_BAR_HEIGHT }))
					{
						tm.cursor_dragging_window      = true;
						tm.cursor_rel_holding_position = tm.window_position - tm.cursor;
					}
					else if (in_rect(tm.cursor, tm.window_position + window_data.dimensions + vf2 { -COMPUTER_TITLE_BAR_HEIGHT, 0.0f }, { COMPUTER_TITLE_BAR_HEIGHT, COMPUTER_TITLE_BAR_HEIGHT }))
					{
						tm.window_type = WindowType::null;
					}
					else if (in_rect(tm.cursor, { tm.window_position.x, max(tm.window_position.y, COMPUTER_TASKBAR_HEIGHT) }, { window_data.dimensions.x, window_data.dimensions.y + tm.window_position.y - max(tm.window_position.y, COMPUTER_TASKBAR_HEIGHT) }))
					{
						FOR_ELEMS(button, window_data.button_buffer, window_data.button_count)
						{
							if (in_rect_centered(tm.cursor, tm.window_position + hadamard_multiply(button->rel_centered_uv_position, window_data.dimensions), button->dimensions))
							{
								switch (tm.window_type)
								{
									case WindowType::room_protocol:
									{
										switch (+WindowButtonFamily::ROOM_PROTOCOL_START + +button_index)
										{
											case WindowButtonFamily::room_protocol_lure:
											{
												boot_down_state(state);
												state->context_arena.used = 0;
												state->context            = StateContext::game;
												state->game               = {};
												boot_up_state(platform->renderer, state);

												FOR_RANGE(MAP_DIM * MAP_DIM)
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
														FOR_RANGE(MAP_DIM / 2)
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
																!(!+*get_wall_voxel(state, walk + vi2 { -1,  0 }) && !+(*get_wall_voxel(state, walk + vi2 { -1, -1 }) & WallVoxel::left) && !+(*get_wall_voxel(state, walk + vi2 { -2, 0 }) & WallVoxel::bottom)) &&
																!(!+*get_wall_voxel(state, walk + vi2 {  1,  0 }) && !+(*get_wall_voxel(state, walk) & WallVoxel::bottom) && !+(*get_wall_voxel(state, walk + vi2 { 1, -1 }) & WallVoxel::left)) &&
																!(!+*get_wall_voxel(state, walk + vi2 {  0, -1 }) && !+(*get_wall_voxel(state, walk + vi2 { -1, -1 }) & WallVoxel::bottom) && !+(*get_wall_voxel(state, walk + vi2 { 0, -2 }) & WallVoxel::left)) &&
																!(!+*get_wall_voxel(state, walk + vi2 {  0,  1 }) && !+(*get_wall_voxel(state, walk) & WallVoxel::left) && !+(*get_wall_voxel(state, walk + vi2 { -1, 1 }) & WallVoxel::bottom))
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

												state->game.lucia_position.z          = LUCIA_HEIGHT;
												state->game.lucia_fov                 = TAU / 3.0f;
												state->game.lucia_stamina             = 1.0f;
												state->game.lucia_health              = 1.0f;
												state->game.interpolated_lucia_health = 1.0f;

												state->game.monster_timeout = 8.0f;

												state->game.creepy_sound_countdown = rng(&state->seed, CREEPY_SOUND_MIN_TIME, CREEPY_SOUND_MAX_TIME);

												for (ItemType type = ItemType::ITEM_START; type != ItemType::ITEM_END; type = static_cast<ItemType>(+type + 1))
												{
													spawn_item(state, type);
												}

												i32 leftover_count = ARRAY_CAPACITY(state->game.item_buffer) - state->game.item_count;
												FOR_RANGE(leftover_count)
												{
													spawn_item(state);
												}

												state->game.hud.cursor = VIEW_RES / 2.0f;

												FOR_RANGE(y, ARRAY_CAPACITY(state->game.hud.circuit_breaker.switches))
												{
													FOR_RANGE(x, ARRAY_CAPACITY(state->game.hud.circuit_breaker.switches[0]))
													{
														state->game.hud.circuit_breaker.switches[y][x].position =
															(VIEW_RES - CIRCUIT_BREAKER_HUD_DIMENSIONS - CIRCUIT_BREAKER_SWITCH_DIMENSIONS) / 2.0f + CIRCUIT_BREAKER_MARGINS +
																vf2
																{
																	(CIRCUIT_BREAKER_VOLTAGE_DISPLAY_X - (VIEW_RES.x - CIRCUIT_BREAKER_HUD_DIMENSIONS.x) / 2.0f - 2.0f * CIRCUIT_BREAKER_MARGINS.x) / ARRAY_CAPACITY(state->game.hud.circuit_breaker.switches[0]) * (x + 0.5f),
																	(                                                  CIRCUIT_BREAKER_HUD_DIMENSIONS.y         - 2.0f * CIRCUIT_BREAKER_MARGINS.y) / ARRAY_CAPACITY(state->game.hud.circuit_breaker.switches   ) * (y + 0.5f)
																};

														state->game.hud.circuit_breaker.switches[y][x].voltage  = 1 + rng(&state->seed, 0, 6);
														state->game.hud.circuit_breaker.max_voltage            += state->game.hud.circuit_breaker.switches[y][x].voltage;
													}
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
											} break;
										}
									} break;

									case WindowType::power:
									{
										switch (+WindowButtonFamily::POWER_START + +button_index)
										{
											case WindowButtonFamily::power_no:
											{
												tm.window_type = WindowType::null;
											} break;

											case WindowButtonFamily::power_yes:
											{
												return UpdateCode::terminate;
											} break;
										} break;
									} break;
								}
							}
						}

						FOR_ELEMS(it, window_data.slider_buffer, window_data.slider_count)
						{
							ASSERT(tm.window_type == WindowType::settings);

							if (norm(tm.cursor - tm.window_position - hadamard_multiply(it->start_uv_position + vf2 { it->u_length * (state->settings_slider_values[it_index] - it->min_value) / (it->max_value - it->min_value), 0.0f }, window_data.dimensions)) < COMPUTER_SLIDER_KNOB_RADIUS)
							{
								tm.cursor_dragging_slider = static_cast<WindowSliderFamily>(+WindowSliderFamily::SETTINGS_START + it_index);
							}
						}
					}
					else
					{
						goto OUTSIDE_WINDOW;
					}
				}
				else
				{
					OUTSIDE_WINDOW:;

					FOR_ELEMS(it, WINDOW_ICON_DATA)
					{
						if (in_rect(tm.cursor, it->position, it->dimensions))
						{
							WindowType clicked_window_type = static_cast<WindowType>(+WindowType::TYPE_START + it_index);

							if (clicked_window_type == tm.window_type)
							{
								tm.window_type = WindowType::null;
							}
							else
							{
								tm.window_type          = clicked_window_type;
								tm.window_velocity      = { 0.0f, 0.0f };
								tm.window_position      = (DISPLAY_RES - WINDOW_DATA[+tm.window_type - +WindowType::TYPE_START].dimensions) / 2.0f;
							}

							break;
						}
					}
				}
			}
			else if (RELEASED(Input::left_mouse))
			{
				tm.cursor_dragging_window = false;
				tm.cursor_dragging_slider = WindowSliderFamily::null;
			}

			tm.cursor_velocity  = dampen(tm.cursor_velocity, 0.8f * platform->cursor_delta / SECONDS_PER_UPDATE, 64.0f, SECONDS_PER_UPDATE);
			tm.cursor          += tm.cursor_velocity * SECONDS_PER_UPDATE;

			constexpr f32 CURSOR_PADDING = 2.0f;
			vf2 min_region = vx2(CURSOR_PADDING);
			vf2 max_region = DISPLAY_RES - vx2(CURSOR_PADDING);

			if (tm.cursor_dragging_window)
			{
				aliasing window_data = WINDOW_DATA[+tm.window_type - +WindowType::TYPE_START];
				min_region = { max(min_region.x, -tm.cursor_rel_holding_position.x - window_data.dimensions.x + 2.0f * COMPUTER_TITLE_BAR_HEIGHT), max(min_region.y, COMPUTER_TASKBAR_HEIGHT - tm.cursor_rel_holding_position.y - window_data.dimensions.y) };
				max_region = { min(max_region.x, DISPLAY_RES.x - tm.cursor_rel_holding_position.x - COMPUTER_TITLE_BAR_HEIGHT), min(max_region.y, DISPLAY_RES.y - tm.cursor_rel_holding_position.y - window_data.dimensions.y - COMPUTER_TITLE_BAR_HEIGHT) };
			}

			if (tm.cursor.x < min_region.x || tm.cursor.x > max_region.x)
			{
				tm.cursor_velocity.x = 0.0f;
				tm.cursor.x          = clamp(tm.cursor.x, min_region.x, max_region.x);
			}
			if (tm.cursor.y < min_region.y || tm.cursor.y > max_region.y)
			{
				tm.cursor_velocity.y = 0.0f;
				tm.cursor.y          = clamp(tm.cursor.y, min_region.y, max_region.y);
			}

			if (tm.window_type != WindowType::null)
			{
				aliasing window_data = WINDOW_DATA[+tm.window_type - +WindowType::TYPE_START];

				if (tm.cursor_dragging_window)
				{
					tm.window_velocity = tm.cursor_velocity;
					tm.window_position = tm.cursor + tm.cursor_rel_holding_position;
				}
				else
				{
					tm.window_velocity  = dampen(tm.window_velocity, { 0.0f, 0.0f }, 32.0f, SECONDS_PER_UPDATE);
					tm.window_position += tm.window_velocity * SECONDS_PER_UPDATE;

					if (tm.window_position.x < 2.0f * COMPUTER_TITLE_BAR_HEIGHT - window_data.dimensions.x || tm.window_position.x > DISPLAY_RES.x - COMPUTER_TITLE_BAR_HEIGHT)
					{
						tm.window_position.x = clamp(tm.window_position.x, 2.0f * COMPUTER_TITLE_BAR_HEIGHT - window_data.dimensions.x, static_cast<f32>(DISPLAY_RES.x - COMPUTER_TITLE_BAR_HEIGHT));
						tm.window_velocity.x = 0.0f;
					}

					if (tm.window_position.y < COMPUTER_TASKBAR_HEIGHT - window_data.dimensions.y || tm.window_position.y > DISPLAY_RES.y - COMPUTER_TITLE_BAR_HEIGHT - window_data.dimensions.y)
					{
						tm.window_position.y = clamp(tm.window_position.y, COMPUTER_TASKBAR_HEIGHT - window_data.dimensions.y, DISPLAY_RES.y - COMPUTER_TITLE_BAR_HEIGHT - window_data.dimensions.y);
						tm.window_velocity.y = 0.0f;
					}
				}

				if (tm.cursor_dragging_slider != WindowSliderFamily::null)
				{
					ASSERT(IN_RANGE(+tm.cursor_dragging_slider, +WindowSliderFamily::SETTINGS_START, +WindowSliderFamily::SETTINGS_END));

					aliasing slider = window_data.slider_buffer[+tm.cursor_dragging_slider - +WindowSliderFamily::SETTINGS_START];

					state->settings_slider_values[+tm.cursor_dragging_slider - +WindowSliderFamily::SETTINGS_START] =
						lerp
						(
							slider.min_value,
							slider.max_value,
							clamp
							(
								(tm.cursor.x - tm.window_position.x - slider.start_uv_position.x * window_data.dimensions.x) / (slider.u_length * window_data.dimensions.x),
								0.0f,
								1.0f
							)
						);
				}
			}
		} break;

		case StateContext::game:
		{
			if (HOLDING(Input::down))
			{
				state->game.lucia_health -= 0.1f * SECONDS_PER_UPDATE;
			}
			if (HOLDING(Input::up))
			{
				state->game.lucia_health += 0.1f * SECONDS_PER_UPDATE;
			}

			DEBUG_once // @TEMP@
			{
				state->game.lucia_position.xy = get_position_of_wall_side(state->game.door_wall_side, 1.0f);
				//state->game.lucia_position.xy = get_position_of_wall_side(state->game.circuit_breaker_wall_side, 1.0f);
				//state->game.lucia_position.xy = { 1.0f, 1.0f };
				//state->game.lucia_position = { 64.637268f, 26.6f, 1.3239026f };
				//state->game.lucia_angle    = 5.3498487f;
				//state->game.monster_position = { 66.295441f, 25.49999f, 1.2878139f };
				//state->game.monster_normal   = { -0.83331096f, 0.55280459f };
				state->game.hud.inventory.array[0][0].type = ItemType::night_vision_goggles;
				state->game.hud.inventory.array[0][0].night_vision_goggles.power = 1.0f;
				state->game.hud.inventory.array[0][1].type = ItemType::flashlight;
				state->game.hud.inventory.array[0][2].type = ItemType::first_aid_kit;
				state->game.hud.inventory.array[0][3].type = ItemType::radio;

				//state->game.lucia_health = max(state->game.lucia_health - 0.4f, 0.0f);
			}

			Mix_VolumeChunk(state->game.audio.drone                                      , static_cast<i32>(MIX_MAX_VOLUME *         state->game.ceiling_lights_keytime  * (1.0f - state->game.exiting_keytime)));
			Mix_VolumeChunk(state->game.audio.drone_low                                  , static_cast<i32>(MIX_MAX_VOLUME * (1.0f - state->game.ceiling_lights_keytime) * (1.0f - state->game.exiting_keytime)));
			Mix_VolumeChunk(state->game.audio.drone_loud                                 , static_cast<i32>(MIX_MAX_VOLUME * clamp(1.0f / (norm_sq(ray_to_closest(state->game.lucia_position.xy, get_position_of_wall_side(state->game.circuit_breaker_wall_side, 0.25f))) + 1.0f), 0.0f, 1.0f)));
			Mix_VolumeChunk(state->game.audio.heartbeats[state->game.heartbeat_sfx_index], static_cast<i32>(MIX_MAX_VOLUME * clamp((state->game.heart_bpm - 30.0f) / 80.0f, 0.0f, 1.0f)));

			state->game.hand_on_state     = HandOnState::null;
			state->game.hand_hovered_item = 0;

			if (state->game.lucia_health > 0.0f)
			{
				state->game.entering_keytime = clamp(state->game.entering_keytime + SECONDS_PER_UPDATE / 1.0f, 0.0f, 1.0f);

				vf2 wasd = { 0.0f, 0.0f };

				if (state->game.hud.type != HudType::circuit_breaker)
				{
					if (HOLDING(Input::s)) { wasd.x -= 1.0f; }
					if (HOLDING(Input::w)) { wasd.x += 1.0f; }
					if (HOLDING(Input::d)) { wasd.y -= 1.0f; }
					if (HOLDING(Input::a)) { wasd.y += 1.0f; }

					if (+wasd)
					{
						constexpr f32 MIN_PORTION = 0.1f;
						state->game.lucia_velocity += 38.0f * rotate(normalize(wasd), state->game.lucia_angle) * (1.0f + MIN_PORTION - powf(1.0f - state->game.lucia_stamina, 8)) / (1.0f + MIN_PORTION) * SECONDS_PER_UPDATE;
					}
				}

				if (+wasd && HOLDING(Input::shift) && !state->game.lucia_out_of_breath)
				{
					state->game.lucia_velocity        = dampen(state->game.lucia_velocity, { 0.0f, 0.0f }, FRICTION * 0.7f, SECONDS_PER_UPDATE);
					state->game.lucia_stamina         = clamp(state->game.lucia_stamina - SECONDS_PER_UPDATE / 60.0f * (1.0f + (1.0f - square(1.0f - 2.0f * state->game.lucia_sprint_keytime)) * 4.0f), 0.0f, 1.0f);
					state->game.lucia_sprint_keytime  = clamp(state->game.lucia_sprint_keytime + SECONDS_PER_UPDATE / 1.5f, 0.0f, 1.0f);
					if (state->game.lucia_stamina == 0.0f)
					{
						state->game.lucia_out_of_breath = true;
					}
					else
					{
						state->game.lucia_fov = dampen(state->game.lucia_fov, TAU * 0.35f, 1.5f, SECONDS_PER_UPDATE);
					}
				}
				else
				{
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
							state->game.lucia_fov      = dampen(state->game.lucia_fov, TAU * 0.2f, 1.0f, SECONDS_PER_UPDATE);
							state->game.lucia_velocity = dampen(state->game.lucia_velocity, { 0.0f, 0.0f }, FRICTION * 1.5f, SECONDS_PER_UPDATE);
						}
					}
					else
					{
						state->game.lucia_fov      = dampen(state->game.lucia_fov, TAU * 0.25f, 4.0f, SECONDS_PER_UPDATE);
						state->game.lucia_velocity = dampen(state->game.lucia_velocity, { 0.0f, 0.0f }, FRICTION, SECONDS_PER_UPDATE);
					}
				}

				state->game.lucia_head_bob_keytime = mod(state->game.lucia_head_bob_keytime + (0.05f + 0.3f * norm(state->game.lucia_velocity)) * SECONDS_PER_UPDATE, 1.0f);
				state->game.lucia_position.xy      = move(state, state->game.lucia_position.xy, state->game.lucia_velocity * SECONDS_PER_UPDATE);

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

				FOR_ELEMS(it, state->game.item_buffer, state->game.item_count)
				{
					it->velocity = dampen(it->velocity, { 0.0f, 0.0f }, FRICTION, SECONDS_PER_UPDATE);
					if (+it->velocity)
					{
						it->position.xy = move(state, it->position.xy, it->velocity * SECONDS_PER_UPDATE);
					}
					it->position.z = lerp(0.15f, state->game.lucia_position.z, clamp(1.0f - norm_sq(ray_to_closest(state->game.lucia_position.xy, it->position.xy)) / 36.0f, 0.0f, 1.0f)) + sinf(state->time * 3.0f) * 0.025f;

					vf2 ray      = ray_to_closest(it->position.xy, state->game.lucia_position.xy);
					f32 distance = norm(ray);
					if (distance > 0.001f)
					{
						it->normal = dampen(it->normal, ray / distance, 2.0f, SECONDS_PER_UPDATE);
					}
				}

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
					state->game.hud.cursor_velocity  = dampen(state->game.hud.cursor_velocity, 0.3f * platform->cursor_delta / SECONDS_PER_UPDATE, 64.0f, SECONDS_PER_UPDATE);
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
						constexpr vf2 INVENTORY_HUD_DIM = vi2 { ARRAY_CAPACITY(state->game.hud.inventory.array[0]), ARRAY_CAPACITY(state->game.hud.inventory.array) } * static_cast<f32>(INVENTORY_DIM + INVENTORY_PADDING);

						if (PRESSED(Input::left_mouse))
						{
							constexpr vf2 INVENTORY_HUD_DIMENSIONS = vf2 { static_cast<f32>(ARRAY_CAPACITY(state->game.hud.inventory.array[0])), static_cast<f32>(ARRAY_CAPACITY(state->game.hud.inventory.array)) } * (INVENTORY_DIM + INVENTORY_PADDING);
							if (in_rect(state->game.hud.cursor, VIEW_RES / 2.0f - INVENTORY_HUD_DIMENSIONS / 2.0f, INVENTORY_HUD_DIMENSIONS))
							{
								state->game.hud.inventory.click_position = state->game.hud.cursor;

								vi2 inventory_box_coordinates =
									{
										static_cast<i32>(roundf(( state->game.hud.cursor.x + (INVENTORY_HUD_DIM.x - INVENTORY_DIM - VIEW_RES.x) / 2.0f) / (INVENTORY_DIM + INVENTORY_PADDING))),
										static_cast<i32>(roundf((-state->game.hud.cursor.y + (INVENTORY_HUD_DIM.y - INVENTORY_DIM + VIEW_RES.y) / 2.0f) / (INVENTORY_DIM + INVENTORY_PADDING)))
									};

								if
								(
									IN_RANGE(inventory_box_coordinates.x, 0, ARRAY_CAPACITY(state->game.hud.inventory.array[0])) &&
									IN_RANGE(inventory_box_coordinates.y, 0, ARRAY_CAPACITY(state->game.hud.inventory.array   )) &&
									in_rect_centered(state->game.hud.cursor, (VIEW_RES + conjugate(vx2(INVENTORY_DIM) - INVENTORY_HUD_DIM)) / 2.0f + conjugate(inventory_box_coordinates) * (INVENTORY_DIM + INVENTORY_PADDING), vxx(vx2(INVENTORY_DIM))) &&
									state->game.hud.inventory.array[inventory_box_coordinates.y][inventory_box_coordinates.x].type != ItemType::null
								)
								{
									state->game.hud.inventory.selected_item = &state->game.hud.inventory.array[inventory_box_coordinates.y][inventory_box_coordinates.x];
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
									if (in_rect_centered(state->game.hud.cursor, VIEW_RES / 2.0f, INVENTORY_HUD_DIM))
									{
										vi2 inventory_box_coordinates =
											{
												static_cast<i32>(roundf(( state->game.hud.cursor.x + (INVENTORY_HUD_DIM.x - INVENTORY_DIM - VIEW_RES.x) / 2.0f) / (INVENTORY_DIM + INVENTORY_PADDING))),
												static_cast<i32>(roundf((-state->game.hud.cursor.y + (INVENTORY_HUD_DIM.y - INVENTORY_DIM + VIEW_RES.y) / 2.0f) / (INVENTORY_DIM + INVENTORY_PADDING)))
											};

										if
										(
											IN_RANGE(inventory_box_coordinates.x, 0, ARRAY_CAPACITY(state->game.hud.inventory.array[0])) &&
											IN_RANGE(inventory_box_coordinates.y, 0, ARRAY_CAPACITY(state->game.hud.inventory.array   )) &&
											in_rect_centered(state->game.hud.cursor, (VIEW_RES + conjugate(vx2(INVENTORY_DIM) - INVENTORY_HUD_DIM)) / 2.0f + conjugate(inventory_box_coordinates) * (INVENTORY_DIM + INVENTORY_PADDING), vxx(vx2(INVENTORY_DIM)))
										)
										{
											aliasing dropped_on = state->game.hud.inventory.array[inventory_box_coordinates.y][inventory_box_coordinates.x];
											if (&dropped_on != state->game.hud.inventory.selected_item)
											{
												if (dropped_on.type == ItemType::null)
												{
													// @NOTE@ Item move.

													dropped_on                                    = *state->game.hud.inventory.selected_item;
													state->game.hud.inventory.selected_item->type = ItemType::null;

													FOR_ELEMS(it, state->game.holdings)
													{
														if (*it == state->game.hud.inventory.selected_item)
														{
															*it = &dropped_on;
															break;
														}
													}
												}
												else
												{
													// @NOTE@ Item combine.

													bool32 combined        = false;
													Item   combined_result = {};

													if (!combined)
													{
														Item* cheap_batteries;
														Item* flashlight;
														if (check_combine(&cheap_batteries, ItemType::cheap_batteries, &flashlight, ItemType::flashlight, &dropped_on, state->game.hud.inventory.selected_item))
														{
															combined = true;

															Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.eletronical, 0);
															state->game.notification_message = "(You replaced the batteries in the flashlight.)";
															state->game.notification_keytime = 1.0f;

															combined_result = *flashlight;
															combined_result.flashlight.power = min(combined_result.flashlight.power + 0.25f, 1.0f);

															if (state->game.holding.flashlight == state->game.hud.inventory.selected_item)
															{
																state->game.holding.flashlight = &dropped_on;
															}
														}
													}

													if (!combined)
													{
														Item* military_grade_batteries;
														Item* night_vision_goggles;
														if (check_combine(&military_grade_batteries, ItemType::military_grade_batteries, &night_vision_goggles, ItemType::night_vision_goggles, &dropped_on, state->game.hud.inventory.selected_item))
														{
															combined = true;

															Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.eletronical, 0);
															state->game.notification_message = "(You replaced the batteries in the night vision goggles.)";
															state->game.notification_keytime = 1.0f;

															combined_result = *night_vision_goggles;
															combined_result.night_vision_goggles.power = min(combined_result.night_vision_goggles.power + 0.25f, 1.0f);

															if (state->game.holding.night_vision_goggles == state->game.hud.inventory.selected_item)
															{
																state->game.holding.night_vision_goggles = &dropped_on;
															}
														}
													}

													if (!combined)
													{
														Item* military_grade_batteries;
														Item* flashlight;
														if (check_combine(&military_grade_batteries, ItemType::military_grade_batteries, &flashlight, ItemType::flashlight, &dropped_on, state->game.hud.inventory.selected_item))
														{
															combined = true;

															Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.shock, 0);
															state->game.notification_message = "(You fried your fingers trying to put military grade batteries into a cheap dollar store flashlight.)";
															state->game.notification_keytime = 1.0f;

															state->game.hud.type = HudType::null;
															state->game.lucia_health          -= 0.1f;
															state->game.blur_value            += 0.5f;
															state->game.flash_stun_activation += 1.0f;

															if (state->game.holding.flashlight == flashlight)
															{
																state->game.holding.flashlight = 0;
															}
														}
													}

													if (!combined)
													{
														Item* cheap_batteries;
														Item* radio;
														if (check_combine(&cheap_batteries, ItemType::cheap_batteries, &radio, ItemType::radio, &dropped_on, state->game.hud.inventory.selected_item))
														{
															combined = true;

															Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.eletronical, 0);
															state->game.notification_message = "(You put some batteries in the radio.)";
															state->game.notification_keytime = 1.0f;

															combined_result = *radio;
															combined_result.radio.power = min(combined_result.radio.power + 0.25f, 1.0f);

															if (state->game.holding.radio == state->game.hud.inventory.selected_item)
															{
																state->game.holding.radio = &dropped_on;
															}
														}
													}

													if (combined)
													{
														state->game.hud.inventory.selected_item->type = ItemType::null;
														dropped_on                                    = combined_result;
													}
													else
													{
														state->game.notification_message = "\"I'm not sure how these fit.\"";
														state->game.notification_keytime = 1.0f;
													}
												}
											}
										}
									}
									else
									{
										// @NOTE@ Item drop.

										Item* dropped = allocate_item(state);
										*dropped = *state->game.hud.inventory.selected_item;
										dropped->velocity    = polar(state->game.lucia_angle + (0.5f - state->game.hud.cursor.x / VIEW_RES.x) * state->game.lucia_fov) * rng(&state->seed, 12.0f, 16.0f);
										dropped->position.xy = move(state, state->game.lucia_position.xy, dropped->velocity * SECONDS_PER_UPDATE);
										dropped->position.z  = state->game.lucia_position.z;
										dropped->normal      = polar(-state->game.lucia_angle);

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

										case ItemType::military_grade_batteries:
										{
											state->game.notification_message = "\"I can feel the volts in this thing!\"";
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

												if (state->game.holding.night_vision_goggles)
												{
													Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.pick_up_heavy, 0);
													state->game.holding.night_vision_goggles = 0;
												}
											}
											else
											{
												state->game.notification_message = "\"The flashlight is dead.\"";
												state->game.notification_keytime = 1.0f;
											}
										} break;

										case ItemType::night_vision_goggles:
										{
											if (state->game.hud.inventory.selected_item == state->game.holding.night_vision_goggles)
											{
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.pick_up_heavy, 0);
												state->game.holding.night_vision_goggles = 0;
											}
											else if (state->game.hud.inventory.selected_item->night_vision_goggles.power)
											{
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.pick_up_heavy, 0);
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.night_vision_goggles_on, 0);
												state->game.holding.night_vision_goggles = state->game.hud.inventory.selected_item;
												state->game.holding.flashlight           = 0;
											}
											else
											{
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.switch_toggle, 0);
												state->game.notification_message = "\"The night vision goggles are dead.\"";
												state->game.notification_keytime = 1.0f;
											}
										} break;

										case ItemType::eye_drops:
										{
											if (state->game.holding.night_vision_goggles)
											{
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.acid_burn, 0);
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.shock, 0);
												state->game.notification_message                = "(You dumped the whole eye drop container onto the night vision goggles.)";
												state->game.notification_keytime                = 1.0f;
												state->game.blur_value                         += 1.0f;
												state->game.flash_stun_activation              += 2.0f;
												state->game.holding.night_vision_goggles->type  = ItemType::null;
												state->game.holding.night_vision_goggles        = 0;
											}
											else
											{
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.acid_burn, 0);
												state->game.notification_message  = "(You dumped the whole eye drop container into your eyes.)";
												state->game.notification_keytime  = 1.0f;
												state->game.eye_drops_activation += 1.0f;
												state->game.blur_value           += 0.5f;
											}

											state->game.hud.inventory.selected_item->type = ItemType::null;
										} break;

										case ItemType::first_aid_kit:
										{
											if (state->game.lucia_health > FIRST_AID_KIT_THRESHOLD)
											{
												state->game.notification_message = "\"I don't think this is supposed to be eaten... yet.\"";
												state->game.notification_keytime = 1.0f;
											}
											else if (rng(&state->seed) < 0.85f)
											{
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.first_aid_kit, 0);
												state->game.notification_message = "(You wrapped the bandages and poured the antiseptic.)";
												state->game.notification_keytime = 1.0f;
												state->game.lucia_state          = LuciaState::healed;
												state->game.lucia_state_keytime  = 1.0f;
												state->game.lucia_health         = min(state->game.lucia_health + 0.5f, 1.0f);
												state->game.hud.inventory.selected_item->type = ItemType::null;
											}
											else
											{
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.acid_burn, 0);
												state->game.notification_message  = "(You ate the bandages and drank the antiseptic.)";
												state->game.notification_keytime  = 1.0f;
												state->game.lucia_state           = LuciaState::hit;
												state->game.lucia_state_keytime   = 1.0f;
												state->game.lucia_health          = max(state->game.lucia_health - 0.15f, 0.0f);
												state->game.blur_value           += 0.25f;
												state->game.hud.inventory.selected_item->type = ItemType::null;
											}
										} break;

										case ItemType::radio:
										{
											if (state->game.hud.inventory.selected_item == state->game.holding.radio && state->game.radio_keytime > 0.0f)
											{
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.squelch, 0);

												FOR_ELEMS(it, state->game.audio.radio_clips)
												{
													Mix_VolumeChunk(*it, 0);
												}

												state->game.holding.radio = 0;
											}
											else if (state->game.hud.inventory.selected_item->radio.power)
											{
												if (state->game.radio_keytime == 0.0f)
												{
													state->game.radio_keytime = 1.0f;
													state->game.holding.radio = state->game.hud.inventory.selected_item;

													i32 unplayed_count = 0;
													FOR_ELEMS(it, state->game.played_radio_clips)
													{
														Mix_VolumeChunk(state->game.audio.radio_clips[it_index], static_cast<i32>(MIX_MAX_VOLUME));
														if (!*it)
														{
															unplayed_count += 1;
														}
													}

													if (unplayed_count == 0)
													{
														FOR_ELEMS(it, state->game.played_radio_clips)
														{
															*it = false;
														}
														unplayed_count = ARRAY_CAPACITY(state->game.audio.radio_clips);
													}

													i32 index = rng(&state->seed, 0, unplayed_count);
													FOR_ELEMS(it, state->game.played_radio_clips)
													{
														if (!*it)
														{
															index -= 1;

															if (index < 0)
															{
																*it = true;
																Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.radio_clips[it_index], 0);
																break;
															}
														}
													}
												}
												else
												{
													Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.squelch, 0);
													state->game.notification_message = "\"No signal right now.\"";
													state->game.notification_keytime = 1.0f;
												}
											}
											else
											{
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.switch_toggle, 0);
												state->game.notification_message = "\"The radio is dead.\"";
												state->game.notification_keytime = 1.0f;
											}
										} break;
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
							if (HOLDING(Input::left_mouse))
							{
								state->game.hud.paper.velocity = state->game.hud.cursor_velocity;
							}
							else
							{
								state->game.hud.paper.velocity         = dampen(state->game.hud.paper.velocity, { 0.0f, 0.0f }, 8.0f, SECONDS_PER_UPDATE);
								state->game.hud.paper.scalar_velocity += 3.0f * platform->scroll;
							}

							state->game.hud.paper.scalar_velocity = dampen(state->game.hud.paper.scalar_velocity, 0.0f, 16.0f, SECONDS_PER_UPDATE);
							state->game.hud.paper.scalar         += state->game.hud.paper.scalar_velocity * state->game.hud.paper.scalar * SECONDS_PER_UPDATE;

							if (PAPER_DATA[state->game.hud.paper.index].min_scalar > state->game.hud.paper.scalar || state->game.hud.paper.scalar > PAPER_DATA[state->game.hud.paper.index].max_scalar)
							{
								state->game.hud.paper.scalar_velocity = 0.0f;
								state->game.hud.paper.scalar          = clamp(state->game.hud.paper.scalar, PAPER_DATA[state->game.hud.paper.index].min_scalar, PAPER_DATA[state->game.hud.paper.index].max_scalar);
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
									FOR_ELEMS(it, state->game.hud.circuit_breaker.flat_switches)
									{
										if (in_rect(state->game.hud.cursor, it->position, CIRCUIT_BREAKER_SWITCH_DIMENSIONS))
										{
											Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.circuit_breaker_switch, 0);

											if (it->active)
											{
												it->active                                      = false;
												state->game.hud.circuit_breaker.active_voltage -= it->voltage;
											}
											else
											{
												it->active                                      = true;
												state->game.hud.circuit_breaker.active_voltage += it->voltage;
											}

											if (state->game.hud.circuit_breaker.active_voltage == state->game.hud.circuit_breaker.goal_voltage)
											{
												Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.drone_on, 0);
												state->game.goal = GameGoal::escape;
											}
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

						lambda hand_on_heuristic =
							[&](vf2 position)
							{
								if (!exists_clear_way(state, state->game.lucia_position.xy, position))
								{
									return 0.0f;
								}

								vf2 ray      = ray_to_closest(state->game.lucia_position.xy, position);
								f32 distance = norm(ray);

								if (distance > 2.0f)
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

						vf3 hand_reach_position = { NAN, NAN };
						f32 best_heuristic = 0.0f;

						{
							constexpr vf2 DOOR_KNOB = { 0.525f, 0.9f };
							vf3 door_knob_position = vx3(get_position_of_wall_side(state->game.door_wall_side, 0.25f) + DOOR_KNOB.x * rotate90(state->game.door_wall_side.normal), DOOR_KNOB.y);

							f32 heuristic = hand_on_heuristic(door_knob_position.xy);
							if (best_heuristic < heuristic)
							{
								best_heuristic            = heuristic;
								state->game.hand_on_state = HandOnState::door;
								hand_reach_position       = door_knob_position;
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

						if (state->game.hand_on_state != HandOnState::null)
						{
							state->game.hand_position.xy = hand_reach_position.xy + ray_to_closest(hand_reach_position.xy, state->game.lucia_position.xy) / 2.0f;
							state->game.hand_position.z  = lerp(hand_reach_position.z, state->game.lucia_position.z, 0.75f);
						}

						if (PRESSED(Input::space) || PRESSED(Input::left_mouse))
						{
							switch (state->game.hand_on_state)
							{
								case HandOnState::door:
								{
									if (state->game.goal == GameGoal::escape)
									{
										boot_down_state(state);
										state->context_arena.used = 0;
										state->context            = StateContext::end;
										state->end                = {};
										boot_up_state(platform->renderer, state);
									}
									else
									{
										Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.door_budge, 0);

										state->game.notification_message =
											rng(&state->seed) < 0.35f
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

				if (state->game.hud.type == HudType::null)
				{
					state->game.lucia_angle_velocity = dampen(state->game.lucia_angle_velocity, -0.005f * platform->cursor_delta.x / SECONDS_PER_UPDATE, 16.0f, SECONDS_PER_UPDATE);
				}
				else
				{
					state->game.lucia_angle_velocity = dampen(state->game.lucia_angle_velocity, 0.0f, 16.0f, SECONDS_PER_UPDATE);
				}

				state->game.lucia_angle = mod(state->game.lucia_angle + state->game.lucia_angle_velocity * SECONDS_PER_UPDATE, TAU);

				state->game.hud.circuit_breaker.interpolated_voltage_velocity  = dampen(state->game.hud.circuit_breaker.interpolated_voltage_velocity, 32.0f * (state->game.hud.circuit_breaker.active_voltage - state->game.hud.circuit_breaker.interpolated_voltage), 16.0f, SECONDS_PER_UPDATE);
				state->game.hud.circuit_breaker.interpolated_voltage          += state->game.hud.circuit_breaker.interpolated_voltage_velocity * SECONDS_PER_UPDATE;

				if (state->game.hud.circuit_breaker.interpolated_voltage < 0.0f)
				{
					state->game.hud.circuit_breaker.interpolated_voltage          = 0.0f;
					state->game.hud.circuit_breaker.interpolated_voltage_velocity = 0.0f;
				}

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

				if (state->game.goal != GameGoal::find_door && state->game.monster_timeout > 0.0f)
				{
					state->game.monster_timeout = max(state->game.monster_timeout - SECONDS_PER_UPDATE, 0.0f);

					if (state->game.monster_timeout == 0.0f)
					{
						f32 farthest_distance = 0.0f;
						FOR_RANGE(y, MAP_DIM)
						{
							FOR_RANGE(x, MAP_DIM)
							{
								vf2 position = rng_open_position(state, { x, y });
								f32 distance = norm(ray_to_closest(position, state->game.lucia_position.xy));

								if (distance > farthest_distance)
								{
									farthest_distance               = farthest_distance;
									state->game.monster_position.xy = position;
								}
							}
						}
					}
				}

				if (state->game.monster_timeout == 0.0f)
				{
					vf2 ray_to_monster = ray_to_closest(state->game.monster_position.xy, state->game.lucia_position.xy);
					if (state->game.monster_chase_keytime)
					{
						if (exists_clear_way(state, state->game.monster_position.xy, state->game.lucia_position.xy))
						{
							state->game.monster_chase_keytime = 1.0f;
						}
						else
						{
							state->game.monster_chase_keytime = max(state->game.monster_chase_keytime - SECONDS_PER_UPDATE / 8.0f, 0.0f);

							if (state->game.monster_chase_keytime == 0.0f)
							{
								Mix_FadeOutMusic(4000);
							}
						}
					}
					else if (exists_clear_way(state, state->game.monster_position.xy, state->game.lucia_position.xy) && norm(ray_to_monster) < 24.0f)
					{
						Mix_FadeInMusic(state->game.music.chases[0], -1, 2000);
						Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.horror[0], 0);
						state->game.monster_chase_keytime = 1.0f;
					}


					vi2 updated_monster_path_goal = state->game.monster_path_goal;
					if (state->game.monster_chase_keytime)
					{
						updated_monster_path_goal = get_closest_open_path_coordinates(state, state->game.lucia_position.xy);
					}
					else
					{
						state->game.monster_roam_update_keytime -= SECONDS_PER_UPDATE / 16.0f;

						if (state->game.monster_roam_update_keytime <= 0.0f || norm(ray_to_closest(path_coordinates_to_position(state->game.monster_path_goal), state->game.lucia_position.xy)) > 8.0f)
						{
							state->game.monster_roam_update_keytime = 1.0f;
							updated_monster_path_goal = get_closest_open_path_coordinates(state, state->game.lucia_position.xy + vf2 { rng(&state->seed, -16.0f, 16.0f), rng(&state->seed, -16.0f, 16.0f) });
						}
					}

					if (updated_monster_path_goal != state->game.monster_path_goal)
					{
						while (state->game.monster_path)
						{
							state->game.monster_path = deallocate_path_coordinates_node(state, state->game.monster_path);
						}

						state->game.monster_path      = path_find(state, get_closest_open_path_coordinates(state, state->game.monster_position.xy), updated_monster_path_goal);
						state->game.monster_path_goal = updated_monster_path_goal;
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
							state->game.monster_velocity = dampen(state->game.monster_velocity, normalize(ray) * 5.0f, 3.0f, SECONDS_PER_UPDATE);
						}
					}
					else if (state->game.monster_chase_keytime)
					{
						vf2 ray = ray_to_closest(state->game.monster_position.xy, state->game.lucia_position.xy);
						if (norm(ray) > 1.0f)
						{
							state->game.monster_velocity = dampen(state->game.monster_velocity, normalize(ray) * 4.0f, 4.0f, SECONDS_PER_UPDATE);
						}
						else
						{
							// @NOTE@ Lucia hit.

							Mix_FadeOutMusic(2500);
							Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.horror[1], 0);
							state->game.monster_timeout       = 32.0f;
							state->game.monster_chase_keytime = 0.0f;
							state->game.blur_value            = 1.0f;
							state->game.lucia_state           = LuciaState::haunted;
							state->game.lucia_state_keytime   = 1.0f;
							state->game.lucia_health          = max(state->game.lucia_health - 0.25f, 0.0f);
							state->game.lucia_position.xy     = rng_open_position(state);
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

					state->game.monster_position.z = cosf(state->time * 3.0f) * 0.15f + WALL_HEIGHT / 2.0f;

					if (state->game.monster_chase_keytime)
					{
						vf2 ray      = ray_to_closest(state->game.monster_position.xy, state->game.lucia_position.xy);
						f32 distance = norm(ray);
						if (distance >= 0.001f)
						{
							state->game.monster_normal = normalize(dampen(state->game.monster_normal, ray / distance, 8.0f, SECONDS_PER_UPDATE));
						}
					}
					else
					{
						f32 distance = norm(state->game.monster_velocity);
						if (distance > 0.001f)
						{
							state->game.monster_normal = normalize(dampen(state->game.monster_normal, state->game.monster_velocity / distance, 8.0f, SECONDS_PER_UPDATE));
						}
					}
				}

				if (state->game.holding.flashlight)
				{
					state->game.flashlight_keytime += 0.00005f + 0.005f * norm(state->game.lucia_velocity) * SECONDS_PER_UPDATE;
					if (state->game.flashlight_keytime > 1.0f)
					{
						state->game.flashlight_keytime -= 1.0f;
					}

					state->game.holding.flashlight->flashlight.power = clamp(state->game.holding.flashlight->flashlight.power - SECONDS_PER_UPDATE / 150.0f, 0.0f, 1.0f);

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

				if (state->game.holding.night_vision_goggles)
				{
					state->game.holding.night_vision_goggles->night_vision_goggles.power = clamp(state->game.holding.night_vision_goggles->night_vision_goggles.power - SECONDS_PER_UPDATE / 90.0f, 0.0f, 1.0f);
					state->game.night_vision_goggles_activation                          = dampen(state->game.night_vision_goggles_activation, 1.0f, 4.0f, SECONDS_PER_UPDATE);

					if (state->game.holding.night_vision_goggles->night_vision_goggles.power == 0.0f)
					{
						state->game.holding.night_vision_goggles = 0;

						if (state->game.holding.night_vision_goggles == state->game.holding.night_vision_goggles)
						{
							Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.switch_toggle, 0);
							state->game.notification_message = "\"The night vision goggles died.\"";
							state->game.notification_keytime = 1.0f;
						}
					}
				}

				state->game.heart_bpm               = 63.5f + 80.0f * (1.0f - state->game.lucia_stamina) + 30.0f * clamp(1.0f - state->game.lucia_health, 0.0f, 1.0f);
				state->game.heart_pulse_keytime    += state->game.heart_bpm / 60.0f * SECONDS_PER_UPDATE;
				state->game.heart_pulse_time_since += SECONDS_PER_UPDATE;

				if (state->game.heart_pulse_keytime >= 1.0f)
				{
					Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.heartbeats[state->game.heartbeat_sfx_index], 0);

					state->game.heart_pulse_keytime               = 0.0f;
					state->game.heart_pulse_time_since            = 0.0f;
					state->game.heart_rate_display_update_keytime = 1.0f;
					state->game.heartbeat_sfx_index = (state->game.heartbeat_sfx_index + 1) % ARRAY_CAPACITY(state->game.audio.heartbeats);
				}

				state->game.notification_keytime = clamp(state->game.notification_keytime - SECONDS_PER_UPDATE / 8.0f, 0.0f, 1.0f);
			}

			// @NOTE@ Lucia dying.
			if (state->game.lucia_health <= 0.0f)
			{
				state->game.lucia_dying_keytime += SECONDS_PER_UPDATE / DEATH_DURATION;

				state->game.blur_value          = 2.0f;
				state->game.heart_bpm           = 0.0f;
				state->game.lucia_out_of_breath = false;
				state->game.hand_on_state       = HandOnState::null;
				state->game.hud.type            = HudType::null;

				state->game.lucia_angle_velocity = dampen(state->game.lucia_angle_velocity, 0.0f, 16.0f, SECONDS_PER_UPDATE);
				state->game.lucia_angle          = mod(state->game.lucia_angle + state->game.lucia_angle_velocity * SECONDS_PER_UPDATE, TAU);

				state->game.lucia_velocity    *= 0.9f;
				state->game.lucia_position.xy  = move(state, state->game.lucia_position.xy, state->game.lucia_velocity * SECONDS_PER_UPDATE);
				state->game.lucia_position.z   = dampen(state->game.lucia_position.z, 0.1f, 1.0f, SECONDS_PER_UPDATE);

				state->game.notification_keytime = clamp(state->game.notification_keytime - SECONDS_PER_UPDATE / 2.0f, 0.0f, 1.0f);

				if (state->game.lucia_dying_keytime >= 1.0f)
				{
					return UpdateCode::terminate;
				}
			}

			state->game.eye_drops_activation              = max(state->game.eye_drops_activation - SECONDS_PER_UPDATE / 45.0f, 0.0f);
			state->game.interpolated_eye_drops_activation = dampen(state->game.interpolated_eye_drops_activation, state->game.eye_drops_activation, 4.0f, SECONDS_PER_UPDATE);
			state->game.flash_stun_activation             = dampen(state->game.flash_stun_activation, 0.0f, 16.0f, SECONDS_PER_UPDATE);

			state->game.blur_value        = max(state->game.blur_value - SECONDS_PER_UPDATE / 8.0f, lerp(0.0f, 0.2f, square(1.0f - state->game.lucia_health)));
			state->game.interpolated_blur = dampen(state->game.interpolated_blur, state->game.blur_value, 4.0f, SECONDS_PER_UPDATE);

			// @TODO@ Frame-independent.
			state->game.heart_rate_display_update_keytime += SECONDS_PER_UPDATE / 0.025f;
			while (state->game.heart_rate_display_update_keytime >= 1.0f)
			{
				state->game.heart_rate_display_update_keytime                               -= 1.0f;
				state->game.heart_rate_display_values[state->game.heart_rate_display_index]  = cosf(64.0f * state->game.heart_pulse_time_since) * expf(-25.0f * state->game.heart_pulse_time_since);
				state->game.heart_rate_display_index                                         = (state->game.heart_rate_display_index + 1) % ARRAY_CAPACITY(state->game.heart_rate_display_values);
			}

			if (state->game.holding.radio)
			{
				state->game.holding.radio->radio.power = max(state->game.holding.radio->radio.power - SECONDS_PER_UPDATE / 120.0f, 0.0f);

				if (state->game.holding.radio->radio.power == 0.0f)
				{
					Mix_PlayChannel(+AudioChannel::unreserved, state->game.audio.squelch, 0);
					state->game.notification_message = "\"The radio died.\"";
					state->game.notification_keytime = 1.0f;
					state->game.holding.radio        = 0;

					FOR_ELEMS(clip, state->game.audio.radio_clips)
					{
						Mix_VolumeChunk(*clip, 0);
					}
				}
			}

			f32 battery_display_level = 0.0f;

			if (state->game.holding.flashlight)
			{
				battery_display_level = state->game.holding.flashlight->flashlight.power;

				state->game.flashlight_activation = dampen(state->game.flashlight_activation, sinf(TAU / 4.0f * (1.0f - powf(1.0f - state->game.holding.flashlight->flashlight.power, 16.0f))), 25.0f, SECONDS_PER_UPDATE);

				state->game.flashlight_ray.z = sinf(state->game.flashlight_keytime * TAU * 36.0f) * 0.05f;
			}
			else
			{
				state->game.flashlight_activation = dampen(state->game.flashlight_activation, 0.0f, 8.0f, SECONDS_PER_UPDATE);
			}

			if (state->game.holding.night_vision_goggles)
			{
				battery_display_level  = state->game.holding.night_vision_goggles->night_vision_goggles.power;

				state->game.night_vision_goggles_scan_line_keytime = mod(state->game.night_vision_goggles_scan_line_keytime + SECONDS_PER_UPDATE / 0.15f, 1.0f);

				state->game.blur_value = max(state->game.blur_value, 0.15f);

				state->game.night_vision_goggles_interpolated_ray_to_circuit_breaker =
					dampen
					(
						state->game.night_vision_goggles_interpolated_ray_to_circuit_breaker,
						ray_to_closest(state->game.lucia_position.xy, get_position_of_wall_side(state->game.circuit_breaker_wall_side, 1.0f)),
						8.0f,
						SECONDS_PER_UPDATE
					);

				state->game.night_vision_goggles_interpolated_ray_to_door =
					dampen
					(
						state->game.night_vision_goggles_interpolated_ray_to_door,
						ray_to_closest(state->game.lucia_position.xy, get_position_of_wall_side(state->game.door_wall_side, 1.0f)),
						8.0f,
						SECONDS_PER_UPDATE
					);
			}
			else
			{
				state->game.night_vision_goggles_activation = dampen(state->game.night_vision_goggles_activation, 0.0f, 8.0f, SECONDS_PER_UPDATE);
			}

			if (battery_display_level)
			{
				state->game.hud.status.battery_display_keytime = min(1.0f, state->game.hud.status.battery_display_keytime + SECONDS_PER_UPDATE / 0.25f);
				state->game.hud.status.battery_level_keytime   = dampen(state->game.hud.status.battery_level_keytime, battery_display_level, 8.0f, SECONDS_PER_UPDATE);
			}
			else
			{
				state->game.hud.status.battery_display_keytime = max(0.0f, state->game.hud.status.battery_display_keytime - SECONDS_PER_UPDATE / 0.25f);
				state->game.hud.status.battery_level_keytime   = dampen(state->game.hud.status.battery_level_keytime, 0.0f, 8.0f, SECONDS_PER_UPDATE);
			}

			FOR_ELEMS(it, state->game.animated_sprites)
			{
				age_animated_sprite(it, SECONDS_PER_UPDATE);
			}

			state->game.lucia_state_keytime  = max(state->game.lucia_state_keytime - SECONDS_PER_UPDATE / 1.5f, 0.0f);

			if (state->game.lucia_state_keytime == 0.0f)
			{
				if (state->game.lucia_health <= 0.25f)
				{
					state->game.lucia_state = LuciaState::anxious_wounded;
				}
				else if (state->game.lucia_health < FIRST_AID_KIT_THRESHOLD)
				{
					state->game.lucia_state = LuciaState::wounded;
				}
				else if (state->game.goal == GameGoal::find_door)
				{
					state->game.lucia_state = LuciaState::normal;
				}
				else
				{
					state->game.lucia_state = LuciaState::anxious;
				}
			}

			state->game.radio_keytime = max(state->game.radio_keytime - SECONDS_PER_UPDATE / 30.0f, 0.0f);

			state->game.interpolated_lucia_health = dampen(state->game.interpolated_lucia_health, state->game.lucia_health, 1.0f, SECONDS_PER_UPDATE);

			state->game.flashlight_ray.xy = dampen(state->game.flashlight_ray.xy, polar(state->game.lucia_angle + sinf(state->game.flashlight_keytime * TAU * 15.0f) * 0.1f), 16.0f, SECONDS_PER_UPDATE);
			state->game.flashlight_ray    = normalize(state->game.flashlight_ray);
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
					if (PRESSED(Input::space) || PRESSED(Input::left_mouse))
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

	set_color(platform->renderer, vf3 { 0.0f, 0.0f, 0.0f });
	SDL_RenderClear(platform->renderer);

	SDL_SetRenderTarget(platform->renderer, state->texture.display);
	set_color(platform->renderer, vf3 { 0.0f, 0.0f, 0.0f });
	SDL_RenderClear(platform->renderer);

	f32 blackout = 0.0f;

	switch (state->context)
	{
		case StateContext::title_menu:
		{
			aliasing tm = state->title_menu;

			render_texture(platform->renderer, tm.texture.desktop, { 0.0f, 0.0f }, DISPLAY_RES + vf2 { 0.0f, -COMPUTER_TASKBAR_HEIGHT });

			FOR_ELEMS(it, WINDOW_ICON_DATA)
			{
				if (it_index != +WindowType::power - +WindowType::TYPE_START)
				{
					render_texture(platform->renderer, tm.texture.icons[it_index], { it->position.x, DISPLAY_RES.y - it->dimensions.y - it->position.y }, it->dimensions);

					render_text
					(
						platform->renderer,
						state->font.major,
						{ it->position.x + it->dimensions.x / 2.0f, DISPLAY_RES.y - it->position.y },
						-0.75f,
						FC_ALIGN_CENTER,
						0.5f,
						{ 1.0f, 1.0f, 1.0f, 1.0f },
						"%s",
						it->name
					);
				}
			}

			if (+tm.window_type)
			{
				aliasing window_data = WINDOW_DATA[+tm.window_type - +WindowType::TYPE_START];

				set_color(platform->renderer, monochrome(0.5f));
				render_filled_rect(platform->renderer, vf2 { tm.window_position.x, DISPLAY_RES.y - COMPUTER_TITLE_BAR_HEIGHT - tm.window_position.y - window_data.dimensions.y }, { window_data.dimensions.x, COMPUTER_TITLE_BAR_HEIGHT });

				render_texture
				(
					platform->renderer,
					tm.texture.window_close,
					{
						tm.window_position.x + window_data.dimensions.x - COMPUTER_TITLE_BAR_HEIGHT,
						DISPLAY_RES.y - COMPUTER_TITLE_BAR_HEIGHT - tm.window_position.y - window_data.dimensions.y,
					},
					{ COMPUTER_TITLE_BAR_HEIGHT, COMPUTER_TITLE_BAR_HEIGHT }
				);

				if (+tm.window_type)
				{
					switch (tm.window_type)
					{
						case WindowType::credits:
						{
							set_color(platform->renderer, vf3 { 0.75f, 0.65f, 0.2f });
							render_filled_rect(platform->renderer, { tm.window_position.x, DISPLAY_RES.y - window_data.dimensions.y - tm.window_position.y }, window_data.dimensions);

							render_boxed_text
							(
								platform->renderer,
								state->font.minor,
								{ tm.window_position.x + 5.0f, DISPLAY_RES.y - window_data.dimensions.y - tm.window_position.y + 5.0f },
								window_data.dimensions - vf2 { 5.0f, 5.0f } * 2.0f,
								FC_ALIGN_LEFT,
								1.0f,
								{ 1.0f, 1.0f, 1.0f, 1.0f },
								"Programming done by Phuc Doan.\n\n"
								"Voice acting performed by Ren Stolebarger.\n\n"
								"Art drawn by Mila Matthews.\n\n"
								"Majority of sounds possibly stolen from SCP:CB."
							);
						} break;

						case WindowType::room_protocol:
						{
							set_color(platform->renderer, monochrome(0.4f));
							render_filled_rect(platform->renderer, { tm.window_position.x, DISPLAY_RES.y - window_data.dimensions.y - tm.window_position.y }, window_data.dimensions);

							render_text
							(
								platform->renderer,
								state->font.major,
								{ tm.window_position.x + window_data.dimensions.x * 0.5f, DISPLAY_RES.y - tm.window_position.y - window_data.dimensions.y * 0.7f },
								0.5f,
								FC_ALIGN_CENTER,
								1.5f,
								{ 1.0f, 1.0f, 1.0f, 1.0f },
								"ROOM"
							);
						} break;

						case WindowType::settings:
						{
							set_color(platform->renderer, vf3 { 0.05f, 0.1f, 0.15f });
							render_filled_rect(platform->renderer, { tm.window_position.x, DISPLAY_RES.y - window_data.dimensions.y - tm.window_position.y }, window_data.dimensions);

							render_text
							(
								platform->renderer,
								state->font.major,
								{ tm.window_position.x + window_data.dimensions.x * 0.05f, DISPLAY_RES.y - tm.window_position.y - window_data.dimensions.y * 0.9f },
								0.5f,
								FC_ALIGN_LEFT,
								1.0f,
								{ 1.0f, 1.0f, 1.0f, 1.0f },
								"SYSTEM SETTINGS"
							);
						} break;

						case WindowType::power:
						{
							set_color(platform->renderer, monochrome(0.8f));
							render_filled_rect(platform->renderer, { tm.window_position.x, DISPLAY_RES.y - window_data.dimensions.y - tm.window_position.y }, window_data.dimensions);

							render_text
							(
								platform->renderer,
								state->font.major,
								{ tm.window_position.x + window_data.dimensions.x * 0.5f, DISPLAY_RES.y - tm.window_position.y - window_data.dimensions.y * 0.75f },
								0.5f,
								FC_ALIGN_CENTER,
								0.8f,
								{ 1.0f, 1.0f, 1.0f, 1.0f },
								"Quit?"
							);
						} break;
					}

					FOR_ELEMS(it, window_data.button_buffer, window_data.button_count)
					{
						set_color(platform->renderer, it->bg_color);
						render_filled_rect(platform->renderer, { tm.window_position.x + it->rel_centered_uv_position.x * window_data.dimensions.x - it->dimensions.x / 2.0f, DISPLAY_RES.y - tm.window_position.y - it->rel_centered_uv_position.y * window_data.dimensions.y - it->dimensions.y / 2.0f }, it->dimensions);

						render_text
						(
							platform->renderer,
							state->font.major,
							{ tm.window_position.x + it->rel_centered_uv_position.x * window_data.dimensions.x, DISPLAY_RES.y - tm.window_position.y - it->rel_centered_uv_position.y * window_data.dimensions.y },
							0.5f,
							FC_ALIGN_CENTER,
							0.6f,
							monochrome(1.0f),
							"%s",
							it->text
						);
					}

					FOR_ELEMS(it, window_data.slider_buffer, window_data.slider_count)
					{
						ASSERT(tm.window_type == WindowType::settings);

						set_color(platform->renderer, monochrome(0.8f));
						render_line
						(
							platform->renderer,
							{
								tm.window_position.x + it->start_uv_position.x * window_data.dimensions.x,
								DISPLAY_RES.y - tm.window_position.y - it->start_uv_position.y * window_data.dimensions.y
							},
							{
								tm.window_position.x + (it->start_uv_position.x + it->u_length) * window_data.dimensions.x,
								DISPLAY_RES.y - tm.window_position.y - it->start_uv_position.y * window_data.dimensions.y
							}
						);

						set_color(platform->renderer, monochrome(1.0f));
						render_filled_circle
						(
							platform->renderer,
							{ tm.window_position.x + (it->start_uv_position.x + it->u_length * (state->settings_slider_values[it_index] - it->min_value) / (it->max_value - it->min_value)) * window_data.dimensions.x, DISPLAY_RES.y - tm.window_position.y - it->start_uv_position.y * window_data.dimensions.y },
							COMPUTER_SLIDER_KNOB_RADIUS
						);

						render_text
						(
							platform->renderer,
							state->font.major,
							{ tm.window_position.x + it->start_uv_position.x * window_data.dimensions.x - 25.0f, DISPLAY_RES.y - tm.window_position.y - it->start_uv_position.y * window_data.dimensions.y },
							0.5f,
							FC_ALIGN_RIGHT,
							0.6f,
							{ 1.0f, 1.0f, 1.0f, 1.0f },
							"%s",
							it->text
						);
					}
				}
			}

			set_color(platform->renderer, monochrome(0.3f));
			render_filled_rect(platform->renderer, { 0.0f, static_cast<f32>(DISPLAY_RES.y - COMPUTER_TASKBAR_HEIGHT) }, { static_cast<f32>(DISPLAY_RES.x), static_cast<f32>(COMPUTER_TASKBAR_HEIGHT) });

			render_texture
			(
				platform->renderer,
				tm.texture.icons[+WindowType::power - +WindowType::TYPE_START],
				{ WINDOW_ICON_DATA[+WindowType::power - +WindowType::TYPE_START].position.x, DISPLAY_RES.y - WINDOW_ICON_DATA[+WindowType::power - +WindowType::TYPE_START].dimensions.y - WINDOW_ICON_DATA[+WindowType::power - +WindowType::TYPE_START].position.y },
				WINDOW_ICON_DATA[+WindowType::power - +WindowType::TYPE_START].dimensions
			);

			render_texture
			(
				platform->renderer,
				tm.texture.cursor,
				{ tm.cursor.x, DISPLAY_RES.y - tm.cursor.y },
				{ 12.0f, 24.0f }
			);
		} break;

		case StateContext::game:
		{
			DEBUG_profiling_start(FRAME);
			DEFER { DEBUG_profiling_end_averaged_printf(FRAME, 64, "Frame : %f / %f (%fx of goal)\n", DEBUG_get_profiling(FRAME), 1.0f / 60.0f, DEBUG_get_profiling(FRAME) * 60.0f); };

			set_color(platform->renderer, monochrome(0.0f));
			SDL_RenderClear(platform->renderer);

			u32* new_view_pixels    = memory_arena_allocate<u32>(&state->transient_arena, VIEW_RES.x * VIEW_RES.y);
			u32* current_view_pixel = new_view_pixels;

			FOR_RANGE(x, VIEW_RES.x)
			{
				vf2 ray_horizontal = polar(state->game.lucia_angle + (0.5f - static_cast<f32>(x) / VIEW_RES.x) * state->game.lucia_fov);

				WallSide ray_casted_wall_side       = {};
				f32      wall_distance              = NAN;
				f32      wall_portion               = NAN;
				i32      wall_starting_y            = 0;
				i32      wall_ending_y              = 0;
				Image*   wall_overlay               = 0;
				vf2      wall_overlay_uv_position   = { NAN, NAN };
				vf2      wall_overlay_uv_dimensions = { NAN, NAN };
				bool32   wall_in_light              = true;

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
									&& 0.0f <= portion && portion <= 1.0f
									&& (!+ray_casted_wall_side.voxel || distance < wall_distance)
								)
								{
									ray_casted_wall_side.normal = voxel_data->normal;
									ray_casted_wall_side.voxel  = voxel_data->voxel;
									wall_distance               = distance;
									wall_portion                = portion;

									if (dot(ray_horizontal, voxel_data->normal) > 0.0f)
									{
										ray_casted_wall_side.normal *= -1.0f;
										wall_portion                 = 1.0f - wall_portion;
									}
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
								wall_overlay = &state->game.image.door;

								if (+(state->game.door_wall_side.voxel & (WallVoxel::back_slash | WallVoxel::forward_slash)))
								{
									wall_overlay_uv_position   = { 0.5f - 0.5f / 2.0f / SQRT2, 0.0f };
									wall_overlay_uv_dimensions = { 0.5f / SQRT2, 0.85f };
								}
								else
								{
									wall_overlay_uv_position   = { 0.25f, 0.0f };
									wall_overlay_uv_dimensions = { 0.5f, 0.85f };
								}
							}
							else if (equal_wall_sides(ray_casted_wall_side, state->game.circuit_breaker_wall_side))
							{
								wall_overlay = &state->game.image.circuit_breaker;

								if (+(state->game.door_wall_side.voxel & (WallVoxel::back_slash | WallVoxel::forward_slash)))
								{
									wall_overlay_uv_position   = { 0.5f - 0.30f / 2.0f / SQRT2, 0.25f };
									wall_overlay_uv_dimensions = { 0.30f / SQRT2, 0.50f };
								}
								else
								{
									wall_overlay_uv_position   = { 0.35f, 0.25f };
									wall_overlay_uv_dimensions = { 0.30f, 0.50f };
								}
							}
							else if (rng(static_cast<i32>((ray_casted_wall_side.coordinates.x + ray_casted_wall_side.coordinates.y) * 317.14f + ray_casted_wall_side.coordinates.y * 17102.012f + 962.0f)) < 0.1f)
							{
								f32 direction =
									dot
									(
										rotate90(ray_casted_wall_side.normal),
										normalize(ray_to_closest(get_position_of_wall_side(ray_casted_wall_side, 0.0f), get_position_of_wall_side(state->game.door_wall_side, 0.0f)))
									);

								constexpr f32 THRESHOLD = 0.7f;

								if (direction < -THRESHOLD)
								{
									wall_overlay = &state->game.image.wall_left_arrow;
								}
								else if (direction > THRESHOLD)
								{
									wall_overlay = &state->game.image.wall_right_arrow;
								}

								wall_overlay_uv_position   = { 0.0f, 0.0f };
								wall_overlay_uv_dimensions = { 1.0f, 1.0f };
							}

							if (wall_overlay && !IN_RANGE(wall_portion, wall_overlay_uv_position.x, wall_overlay_uv_position.x + wall_overlay_uv_dimensions.x))
							{
								wall_overlay = 0;
							}

							wall_in_light =
								state->game.monster_timeout == 0.0f
									&& dot(ray_to_closest(state->game.lucia_position.xy + ray_horizontal * wall_distance, state->game.monster_position.xy), ray_casted_wall_side.normal) > 0.0f
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

				RenderScanNode* render_scan_node = 0;

				__m128 m_delta_checks_x;
				__m128 m_delta_checks_y;
				{
					vf2 lucia_position_uv = state->game.lucia_position.xy / (MAP_DIM * WALL_SPACING);

					m_delta_checks_x = _mm_mul_ps(_mm_set_ps(0.0f, 1.0f, 0.0f, 1.0f), _mm_set_ps1((2.0f * roundf(lucia_position_uv.x) - 1.0f) * MAP_DIM * WALL_SPACING));
					m_delta_checks_y = _mm_mul_ps(_mm_set_ps(0.0f, 0.0f, 1.0f, 1.0f), _mm_set_ps1((2.0f * roundf(lucia_position_uv.y) - 1.0f) * MAP_DIM * WALL_SPACING));

					if (fabs(lucia_position_uv.x - roundf(lucia_position_uv.x)) > fabs(lucia_position_uv.y - roundf(lucia_position_uv.y)))
					{
						m_delta_checks_x = _mm_shuffle_ps(m_delta_checks_x, m_delta_checks_x, _MM_SHUFFLE(3, 1, 2, 0));
						m_delta_checks_y = _mm_shuffle_ps(m_delta_checks_y, m_delta_checks_y, _MM_SHUFFLE(3, 1, 2, 0));
					}
				}

				constexpr f32 SHADER_INV_EPSILON = 0.9f;

				__m128 m_lucia_x    = _mm_set_ps1(state->game.lucia_position.x);
				__m128 m_lucia_y    = _mm_set_ps1(state->game.lucia_position.y);
				__m128 m_ray_x      = _mm_set_ps1(ray_horizontal.x);
				__m128 m_ray_y      = _mm_set_ps1(ray_horizontal.y);
				__m128 m_max_scalar = _mm_set_ps1(+ray_casted_wall_side.voxel ? wall_distance : INFINITY);

				lambda scan =
					[&](Material material, Image image, vf3 position, vf2 normal, vf2 dimensions)
					{
						__m128 m_start_x          = _mm_add_ps(_mm_set_ps1(position.x + normal.y * dimensions.x / 2.0f), m_delta_checks_x);
						__m128 m_end_x            = _mm_add_ps(_mm_set_ps1(position.x - normal.y * dimensions.x / 2.0f), m_delta_checks_x);
						__m128 m_start_y          = _mm_add_ps(_mm_set_ps1(position.y - normal.x * dimensions.x / 2.0f), m_delta_checks_y);
						__m128 m_end_y            = _mm_add_ps(_mm_set_ps1(position.y + normal.x * dimensions.x / 2.0f), m_delta_checks_y);
						__m128 m_start_to_end_x   = _mm_sub_ps(m_end_x, m_start_x);
						__m128 m_start_to_end_y   = _mm_sub_ps(m_end_y, m_start_y);
						__m128 m_start_to_lucia_x = _mm_sub_ps(m_lucia_x, m_start_x);
						__m128 m_start_to_lucia_y = _mm_sub_ps(m_lucia_y, m_start_y);
						__m128 m_det              = _mm_sub_ps(_mm_mul_ps(m_ray_x, m_start_to_end_y), _mm_mul_ps(m_ray_y, m_start_to_end_x));
						__m128 m_scalar           = _mm_div_ps(_mm_sub_ps(_mm_mul_ps(m_start_to_lucia_y, m_start_to_end_x), _mm_mul_ps(m_start_to_lucia_x, m_start_to_end_y)), m_det);
						__m128 m_portion          = _mm_div_ps(_mm_sub_ps(_mm_mul_ps(m_start_to_lucia_y, m_ray_x         ), _mm_mul_ps(m_start_to_lucia_x, m_ray_y         )), m_det);
						i32    mask               = _mm_movemask_ps(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(m_scalar, m_zero), _mm_cmplt_ps(m_scalar, m_max_scalar)), _mm_and_ps(_mm_cmpge_ps(m_portion, m_zero), _mm_cmplt_ps(m_portion, m_one))));

						if (mask)
						{
							f32 scalars[4];
							f32 portions[4];
							_mm_storeu_ps(scalars, m_scalar);
							_mm_storeu_ps(portions, m_portion);

							FOR_RANGE(i, 4)
							{
								if (mask & (1 << i))
								{
									RenderScanNode** post_node = &render_scan_node;
									while (*post_node && (*post_node)->distance < scalars[i])
									{
										post_node = &(*post_node)->next_node;
									}

									RenderScanNode* new_node = memory_arena_allocate<RenderScanNode>(&state->transient_arena);
									new_node->material   = material;
									new_node->in_light   = state->game.monster_timeout == 0.0f && exists_clear_way(state, state->game.lucia_position.xy + ray_horizontal * scalars[i] * SHADER_INV_EPSILON, state->game.monster_position.xy);
									new_node->image      = image;
									new_node->normal     = normal;
									new_node->distance   = scalars[i];
									new_node->portion    = portions[i];
									new_node->next_node  = *post_node;
									new_node->starting_y = static_cast<i32>(VIEW_RES.y / 2.0f + HORT_TO_VERT_K / state->game.lucia_fov * (position.z - 0.5f * dimensions.y - state->game.lucia_position.z) / (scalars[i] + 0.1f));
									new_node->ending_y   = static_cast<i32>(VIEW_RES.y / 2.0f + HORT_TO_VERT_K / state->game.lucia_fov * (position.z + 0.5f * dimensions.y - state->game.lucia_position.z) / (scalars[i] + 0.1f));
									*post_node = new_node;
									break;
								}
							}
						}
					};

				memory_arena_checkpoint(&state->transient_arena);

				if (state->game.monster_timeout == 0.0f)
				{
					scan(Material::monster, get_image_of_frame(&state->game.animated_sprite.monster), state->game.monster_position, state->game.monster_normal, { 1.0f, 1.0f });

					constexpr i32 FIRE_COUNT = 3;
					FOR_RANGE(i, FIRE_COUNT)
					{
						vf3 fire_position = state->game.monster_position + vx3(polar(state->time + static_cast<f32>(i) / FIRE_COUNT * TAU), 0.0f);
						if (exists_clear_way(state, state->game.monster_position.xy, fire_position.xy))
						{
							scan(Material::fire, get_image_of_frame(&state->game.animated_sprite.fire), fire_position, normalize(ray_to_closest(state->game.monster_position.xy, state->game.lucia_position.xy)), { 1.0f, 1.0f });
						}
					}
				}

				if (state->game.hand_on_state != HandOnState::null)
				{
					scan(Material::hand, state->game.texture_sprite.hand.image, state->game.hand_position, normalize(ray_to_closest(state->game.hand_position.xy, state->game.lucia_position.xy)), { 0.1f, 0.1f });
				}

				FOR_ELEMS(it, state->game.item_buffer, state->game.item_count)
				{
					scan(Material::item, state->game.texture_sprite.default_items[+it->type - +ItemType::ITEM_START].image, it->position, it->normal, { 0.5f, 0.5f });
				}

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
								*current_view_pixel = pack_color(shader(state, scan_pixel.xyz, node->material, node->in_light, ray, vx3(node->normal, 0.0f), node->distance));
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
							wall_color =
								sample_at
								(
									&state->game.mipmap.wall,
									(distance / 4.0f + state->game.mipmap.wall.level_count * square(1.0f - fabsf(dot(ray, vx3(ray_casted_wall_side.normal, 0.0f))))) * (1.0f - state->game.interpolated_eye_drops_activation),
									{ wall_portion, y_portion }
								);
						}

						*current_view_pixel =
							pack_color
							(
								shader
								(
									state,
									wall_overlay_color.w == 0.0f
										? wall_color
										: wall_overlay_color.w == 1.0f
											? wall_overlay_color.xyz
											: lerp(wall_color, wall_overlay_color.xyz, wall_overlay_color.w),
									Material::wall,
									wall_in_light,
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

							precomputed f32 FLOOR_DIM = MAP_DIM * WALL_SPACING / roundf(MAP_DIM * WALL_SPACING / 4.0f);
							uv.x = mod(uv.x / FLOOR_DIM, 1.0f);
							uv.y = mod(uv.y / FLOOR_DIM, 1.0f);
						}
						else
						{
							f32 zk   = (WALL_HEIGHT - state->game.lucia_position.z) / ray.z;
							uv       = state->game.lucia_position.xy + zk * ray.xy;
							distance = sqrtf(norm_sq(uv - state->game.lucia_position.xy) + square(WALL_HEIGHT - state->game.lucia_position.z));
							normal   = { 0.0f, 0.0f, -1.0f };
							mipmap   = &state->game.mipmap.ceiling;
							material = Material::ceiling;

							precomputed f32 CEILING_DIM = MAP_DIM * WALL_SPACING / roundf(MAP_DIM * WALL_SPACING / 4.0f);
							uv.x = mod(uv.x / CEILING_DIM, 1.0f);
							uv.y = mod(uv.y / CEILING_DIM, 1.0f);
						}

						vf3 floor_ceiling_color =
							sample_at
							(
								mipmap,
								(distance / 16.0f + mipmap->level_count * square(1.0f - fabsf(dot(ray, normal)))) * (1.0f - state->game.interpolated_eye_drops_activation),
								uv
							);

						*current_view_pixel =
							pack_color
							(
								shader
								(
									state,
									floor_ceiling_color,
									material,
									state->game.monster_timeout == 0.0f && exists_clear_way(state, state->game.lucia_position.xy + ray.xy * distance * SHADER_INV_EPSILON, state->game.monster_position.xy),
									ray,
									normal,
									distance
								)
							);
					}

					NEXT_Y:;
					current_view_pixel += 1;
				}
			}

			u32* view_texture_pixels;
			i32  view_pitch_;
			SDL_LockTexture(state->game.texture.view, 0, reinterpret_cast<void**>(&view_texture_pixels), &view_pitch_);

			__m128 m_blur =
				state->game.interpolated_blur > 0.001f
					? _mm_set_ps1(1.0f - expf(-SECONDS_PER_UPDATE / state->game.interpolated_blur))
					: m_one;

			__m128 m_max_x = _mm_set_ps1(static_cast<f32>(VIEW_RES.x));

			__m128 m_night_vision_goggles_activation        = _mm_set_ps1(state->game.night_vision_goggles_activation);
			__m128 m_night_vision_goggles_scan_line_keytime = _mm_set_ps1(state->game.night_vision_goggles_scan_line_keytime);
			__m128 m_night_vision_goggles_low_scan          = _mm_set_ps1(0.95f);
			__m128 m_night_vision_goggles_r                 = _mm_set_ps1(0.0f);
			__m128 m_night_vision_goggles_g                 = _mm_set_ps1(2.5f);
			__m128 m_night_vision_goggles_b                 = _mm_set_ps1(0.0f);
			__m128 m_brightness                             = _mm_set_ps1(state->settings.brightness + 128.0f * square(state->game.flash_stun_activation));

			FOR_RANGE(y, VIEW_RES.y)
			{
				__m128 m_night_vision_goggles_scan_line = _mm_set_ps1(fabsf(0.5f - y % 3 / 3.0f));
				__m128 m_x                              = _mm_set_ps(3.0f, 2.0f, 1.0f, 0.0f);
				for (i32 x = 0; x < VIEW_RES.x; x += 4)
				{
					u32 old_view_colors[4];
					u32 new_view_colors[4];
					FOR_RANGE(xi, x, min(x + 4, VIEW_RES.x))
					{
						old_view_colors[xi - x] = view_texture_pixels[y * VIEW_RES.x + xi];
						new_view_colors[xi - x] = new_view_pixels[xi * VIEW_RES.y + (VIEW_RES.y - 1 - y)];
					}

					__m128i mi_rgba = _mm_loadu_si128(reinterpret_cast<__m128i*>(old_view_colors));
					__m128 m_old_r = _mm_div_ps(_mm_cvtepi32_ps(_mm_and_epi32(_mm_bsrli_si128(mi_rgba, 3), mi_byte_mask)), m_max_rgb);
					__m128 m_old_g = _mm_div_ps(_mm_cvtepi32_ps(_mm_and_epi32(_mm_bsrli_si128(mi_rgba, 2), mi_byte_mask)), m_max_rgb);
					__m128 m_old_b = _mm_div_ps(_mm_cvtepi32_ps(_mm_and_epi32(_mm_bsrli_si128(mi_rgba, 1), mi_byte_mask)), m_max_rgb);

					mi_rgba = _mm_loadu_si128(reinterpret_cast<__m128i*>(new_view_colors));
					__m128 m_new_r = _mm_div_ps(_mm_cvtepi32_ps(_mm_and_epi32(_mm_bsrli_si128(mi_rgba, 3), mi_byte_mask)), m_max_rgb);
					__m128 m_new_g = _mm_div_ps(_mm_cvtepi32_ps(_mm_and_epi32(_mm_bsrli_si128(mi_rgba, 2), mi_byte_mask)), m_max_rgb);
					__m128 m_new_b = _mm_div_ps(_mm_cvtepi32_ps(_mm_and_epi32(_mm_bsrli_si128(mi_rgba, 1), mi_byte_mask)), m_max_rgb);

					__m128 m_r = _mm_add_ps(_mm_mul_ps(m_old_r, _mm_sub_ps(m_one, m_blur)), _mm_mul_ps(m_new_r, m_blur));
					__m128 m_g = _mm_add_ps(_mm_mul_ps(m_old_g, _mm_sub_ps(m_one, m_blur)), _mm_mul_ps(m_new_g, m_blur));
					__m128 m_b = _mm_add_ps(_mm_mul_ps(m_old_b, _mm_sub_ps(m_one, m_blur)), _mm_mul_ps(m_new_b, m_blur));

					__m128 m_night_vision_non_green_channel = _mm_sub_ps(m_one, m_night_vision_goggles_activation);
					__m128 m_scan_line_delta                = _mm_mul_ps(_mm_sub_ps(_mm_div_ps(m_x, m_max_x), m_night_vision_goggles_scan_line_keytime), m_four);
					m_scan_line_delta = _mm_min_ps(_mm_max_ps(_mm_mul_ps(m_scan_line_delta, m_scan_line_delta), m_zero), m_one);

					__m128 m_avg_rgb = _mm_div_ps(_mm_add_ps(_mm_add_ps(m_r, m_g), m_b), m_three);

					__m128 m_t = _mm_add_ps(m_night_vision_goggles_low_scan, _mm_mul_ps(_mm_mul_ps(_mm_sub_ps(m_one, m_scan_line_delta), m_night_vision_goggles_scan_line), m_night_vision_goggles_activation));

					m_r = _mm_mul_ps(lerp(m_r, _mm_mul_ps(m_avg_rgb, _mm_mul_ps(m_night_vision_goggles_r, m_t)), m_night_vision_goggles_activation), m_brightness);
					m_g = _mm_mul_ps(lerp(m_g, _mm_mul_ps(m_avg_rgb, _mm_mul_ps(m_night_vision_goggles_g, m_t)), m_night_vision_goggles_activation), m_brightness);
					m_b = _mm_mul_ps(lerp(m_b, _mm_mul_ps(m_avg_rgb, _mm_mul_ps(m_night_vision_goggles_b, m_t)), m_night_vision_goggles_activation), m_brightness);

					mi_rgba =
						_mm_or_epi32
						(
							_mm_or_epi32
							(
								_mm_bslli_si128(_mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(m_r, m_zero), m_one), m_max_rgb)), 3),
								_mm_bslli_si128(_mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(m_g, m_zero), m_one), m_max_rgb)), 2)
							),
							_mm_bslli_si128(_mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(m_b, m_zero), m_one), m_max_rgb)), 1)
						);

					_mm_maskmoveu_si128(mi_rgba, _mm_castps_si128(_mm_cmplt_ps(m_x, m_max_x)), reinterpret_cast<char*>(view_texture_pixels + y * VIEW_RES.x + x));

					m_x = _mm_add_ps(m_x, m_four);
				}
			}

			SDL_UnlockTexture(state->game.texture.view);
			render_texture(platform->renderer, state->game.texture.view, { 0.0f, 0.0f }, { static_cast<f32>(VIEW_RES.x) / SCREEN_RES.x * DISPLAY_RES.x, static_cast<f32>(VIEW_RES.y) / SCREEN_RES.y * DISPLAY_RES.y });

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

							if (&state->game.hud.inventory.array[y][x] == state->game.hud.inventory.selected_item)
							{
								set_color(platform->renderer, vf3 { 0.8f, 0.8f, 0.3f });
							}
							else
							{
								FOR_ELEMS(it, state->game.holdings)
								{
									if (*it == &state->game.hud.inventory.array[y][x])
									{
										set_color(platform->renderer, vf3 { 0.6f, 0.6f, 0.2f });
										goto BREAK;
									}
								}

								set_color(platform->renderer, monochrome(0.5f));
							}


							BREAK:;
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
						(VIEW_RES - CIRCUIT_BREAKER_HUD_DIMENSIONS) / 2.0f,
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
								{ state->game.hud.circuit_breaker.switches[y][x].position.x, VIEW_RES.y - CIRCUIT_BREAKER_SWITCH_DIMENSIONS.y - state->game.hud.circuit_breaker.switches[y][x].position.y },
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
						render_line
						(
							platform->renderer,
							{ CIRCUIT_BREAKER_VOLTAGE_DISPLAY_X, (VIEW_RES.y + CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS.y) / 2.0f - static_cast<f32>(i) },
							{ CIRCUIT_BREAKER_VOLTAGE_DISPLAY_X + CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS.x, (VIEW_RES.y + CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS.y) / 2.0f - static_cast<f32>(i) }
						);
					}

					set_color(platform->renderer, monochrome(0.9f));
					render_filled_rect
					(
						platform->renderer,
						vf2 { CIRCUIT_BREAKER_VOLTAGE_DISPLAY_X, (VIEW_RES.y + CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS.y) / 2.0f - CIRCUIT_BREAKER_VOLTAGE_DISPLAY_DIMENSIONS.y * state->game.hud.circuit_breaker.goal_voltage / state->game.hud.circuit_breaker.max_voltage } - vf2 { 4.0f, 1.0f },
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

			constexpr vf2 HEART_RATE_MONITOR_DIMENSIONS  = { 65.0f, STATUS_HUD_HEIGHT * 0.6f };
			constexpr vf2 HEART_RATE_MONITOR_COORDINATES = vf2 { SCREEN_RES.x - 15.0f - HEART_RATE_MONITOR_DIMENSIONS.x, SCREEN_RES.y - (STATUS_HUD_HEIGHT + HEART_RATE_MONITOR_DIMENSIONS.y) / 2.0f };

			set_color(platform->renderer, monochrome(0.0f));
			render_rect(platform->renderer, HEART_RATE_MONITOR_COORDINATES + vf2 { -1.0f, -1.0f }, HEART_RATE_MONITOR_DIMENSIONS + vf2 { 2.0f, 2.0f });
			set_color(platform->renderer, vf3 { lerp(0.15f, 0.7f, clamp(1.0f - state->game.heart_pulse_time_since, 0.0f, 1.0f) * square(1.0f - state->game.interpolated_lucia_health)), 0.15f, 0.15f });
			render_filled_rect(platform->renderer, HEART_RATE_MONITOR_COORDINATES, HEART_RATE_MONITOR_DIMENSIONS);

			FOR_ELEMS(it, state->game.heart_rate_display_values, ARRAY_CAPACITY(state->game.heart_rate_display_values) - 1)
			{
				if (it_index + 1 != state->game.heart_rate_display_index)
				{
					set_color(platform->renderer, { mod(it_index - state->game.heart_rate_display_index, ARRAY_CAPACITY(state->game.heart_rate_display_values)) / static_cast<f32>(ARRAY_CAPACITY(state->game.heart_rate_display_values)), 0.0f, 0.0f, 1.0f });
					render_line
					(
						platform->renderer,
						HEART_RATE_MONITOR_COORDINATES + vxx(vf2 {  it_index         / static_cast<f32>(ARRAY_CAPACITY(state->game.heart_rate_display_values)) * HEART_RATE_MONITOR_DIMENSIONS.x, clamp(0.75f - * it      / 2.0f, 0.0f, 1.0f) * HEART_RATE_MONITOR_DIMENSIONS.y }),
						HEART_RATE_MONITOR_COORDINATES + vxx(vf2 { (it_index + 1.0f) / static_cast<f32>(ARRAY_CAPACITY(state->game.heart_rate_display_values)) * HEART_RATE_MONITOR_DIMENSIONS.x, clamp(0.75f - *(it + 1) / 2.0f, 0.0f, 1.0f) * HEART_RATE_MONITOR_DIMENSIONS.y })
					);
				}
			}

			constexpr f32 HEALTH_DISPLAY_WIDTH = 10.0f;

			set_color(platform->renderer, monochrome(0.0f));
			render_filled_rect(platform->renderer, HEART_RATE_MONITOR_COORDINATES + vf2 { -HEALTH_DISPLAY_WIDTH, 0.0f } + vf2 { -1.0f, -1.0f }, vf2 { HEALTH_DISPLAY_WIDTH, HEART_RATE_MONITOR_DIMENSIONS.y } + vf2 { 2.0f, 2.0f });
			set_color(platform->renderer, vf3 { lerp(0.1f, 1.0f, clamp(1.0f - square(state->game.heart_pulse_time_since), 0.0f, 1.0f)), 0.1f, 0.1f });
			render_filled_rect(platform->renderer, HEART_RATE_MONITOR_COORDINATES + vf2 { -HEALTH_DISPLAY_WIDTH, 0.0f }, { HEALTH_DISPLAY_WIDTH, HEART_RATE_MONITOR_DIMENSIONS.y });
			set_color(platform->renderer, vf3 { 0.1f, lerp(0.7f, 1.0f, clamp(1.0f - state->game.heart_pulse_time_since, 0.0f, 1.0f)), 0.1f });
			render_filled_rect(platform->renderer, HEART_RATE_MONITOR_COORDINATES + vf2 { -HEALTH_DISPLAY_WIDTH, HEART_RATE_MONITOR_DIMENSIONS.y * (1.0f - state->game.interpolated_lucia_health) }, { HEALTH_DISPLAY_WIDTH, HEART_RATE_MONITOR_DIMENSIONS.y * state->game.interpolated_lucia_health });

			SDL_SetRenderTarget(platform->renderer, state->texture.display);

			lambda draw_night_vision_goggles_text =
				[&](strlit name, vf2 ray)
				{
					f32 distance = norm(ray);
					render_text
					(
						platform->renderer,
						state->font.minor,
						{ VIEW_RES.x / SCREEN_RES.x * DISPLAY_RES.x * (0.5f - sign_angle(mod(argument(ray) - state->game.lucia_angle, TAU)) / state->game.lucia_fov), VIEW_RES.y * DISPLAY_RES.y * 0.5f / SCREEN_RES.y },
						0.5f,
						FC_ALIGN_CENTER,
						clamp(32.0f / (distance + 1.0f), 0.75f, 2.0f),
						{ 0.0f, 0.8f, 0.0f, clamp(32.0f / (distance + 1.0f), 0.0f, state->game.night_vision_goggles_activation) },
						"%s\n%.2fm",
						name, distance
					);
				};

			draw_night_vision_goggles_text("CIRCUIT_BREAKER", state->game.night_vision_goggles_interpolated_ray_to_circuit_breaker);
			draw_night_vision_goggles_text("\"EXIT_9341\"", state->game.night_vision_goggles_interpolated_ray_to_door);

			render_texture(platform->renderer, state->game.texture.screen, { 0.0f, 0.0f }, vxx(DISPLAY_RES));

			render_texture
			(
				platform->renderer,
				state->game.texture.lucia_states[+state->game.lucia_state],
				{ (SCREEN_RES.x - STATUS_HUD_HEIGHT) / 2.0f / SCREEN_RES.x * DISPLAY_RES.x, static_cast<f32>(SCREEN_RES.y - STATUS_HUD_HEIGHT) / SCREEN_RES.y * DISPLAY_RES.y },
				{ static_cast<f32>(STATUS_HUD_HEIGHT) / SCREEN_RES.x * DISPLAY_RES.x, static_cast<f32>(STATUS_HUD_HEIGHT) / SCREEN_RES.y * DISPLAY_RES.y }
			);

			if (state->game.notification_keytime)
			{
				render_boxed_text
				(
					platform->renderer,
					state->font.minor,
					{ DISPLAY_RES.x * 0.10f, DISPLAY_RES.y * 0.6f },
					{ DISPLAY_RES.x * 0.80f, DISPLAY_RES.y * 0.9f },
					FC_ALIGN_CENTER,
					1.0f,
					{ 1.0f, 1.0f, 1.0f, sinf(TAU / 4.0f * square(state->game.notification_keytime)) },
					"%s",
					state->game.notification_message
				);
			}

			if (state->game.lucia_dying_keytime)
			{
				blackout = clamp(square(state->game.lucia_dying_keytime * 1.1f), 0.0f, 1.0f);
			}
			else
			{
				blackout = 1.0f - state->game.entering_keytime;
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
				"%.2f %.2f\n%.2f %.2f\n%.2f %.2f\n%.2f\n%.2f\n%.2f",
				state->game.lucia_position.x, state->game.lucia_position.y,
				state->game.door_wall_side.coordinates.x * WALL_SPACING, state->game.door_wall_side.coordinates.y * WALL_SPACING,
				state->game.monster_position.x, state->game.monster_position.y,
				state->game.monster_timeout,
				state->game.monster_roam_update_keytime,
				state->game.monster_chase_keytime
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
		} break;
	}

	set_color(platform->renderer, { 0.0f, 0.0f, 0.0f, blackout });
	render_filled_rect(platform->renderer, { 0.0f, 0.0f }, vxx(DISPLAY_RES));

	SDL_SetRenderTarget(platform->renderer, 0);
	vf2 display_dimensions = DISPLAY_RES * min(static_cast<f32>(platform->window_dimensions.x) / DISPLAY_RES.x, static_cast<f32>(platform->window_dimensions.y) / DISPLAY_RES.y);
	render_texture(platform->renderer, state->texture.display, (platform->window_dimensions - display_dimensions) / 2.0f, display_dimensions);
}
