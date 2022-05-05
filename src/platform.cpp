#include <sys/stat.h>
#include "unified.h"
#include "platform.h"

global constexpr strlit DLL_FILE_PATH      = EXE_DIR "Antihome.dll";
global constexpr strlit DLL_TEMP_FILE_PATH = EXE_DIR "Antihome.dll.temp"; // @TODO@ Do files get written in the exe file directory or the data directory?

global byte*                dll;
global time_t               dll_modification_time;
global PrototypeInitialize* initialize;
global PrototypeBootUp*     boot_up;
global PrototypeBootDown*   boot_down;
global PrototypeUpdate*     update;
global PrototypeRender*     render;

internal time_t fetch_dll_modification_time(void)
{
	struct stat file_status;
	return
		stat(DLL_FILE_PATH, &file_status)
			? time_t {}
			: file_status.st_mtime;
}

internal void reload_dll(void)
{
	if (dll)
	{
		SDL_UnloadObject(dll);
	}

	SDL_RWops* src      = SDL_RWFromFile(DLL_FILE_PATH     , "r");
	SDL_RWops* des      = SDL_RWFromFile(DLL_TEMP_FILE_PATH, "w");
	i64        src_size = SDL_RWsize(src);
	byte*      buffer   = reinterpret_cast<byte*>(SDL_calloc(1, src_size));

	SDL_RWread (src, buffer, src_size, 1);
	SDL_RWwrite(des, buffer, src_size, 1);
	SDL_RWclose(src);
	SDL_RWclose(des);
	SDL_free(buffer);

	dll                   = reinterpret_cast<byte*>(SDL_LoadObject(DLL_TEMP_FILE_PATH));
	dll_modification_time = fetch_dll_modification_time();
	initialize            = reinterpret_cast<PrototypeInitialize*>(SDL_LoadFunction(dll, "initialize"));
	boot_up               = reinterpret_cast<PrototypeBootUp*>    (SDL_LoadFunction(dll, "boot_up"));
	boot_down             = reinterpret_cast<PrototypeBootDown*>  (SDL_LoadFunction(dll, "boot_down"));
	update                = reinterpret_cast<PrototypeUpdate*>    (SDL_LoadFunction(dll, "update"));
	render                = reinterpret_cast<PrototypeRender*>    (SDL_LoadFunction(dll, "render"));
}

int main(int, char**)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		DEBUG_printf("SDL_Error: '%s'\n", SDL_GetError());
		ASSERT(false);
		exit(-1);
	}
	DEFER { SDL_Quit(); };

	if (TTF_Init() == -1)
	{
		DEBUG_printf("TTF_Error: '%s'\n", TTF_GetError());
		ASSERT(false);
		exit(-1);
	}
	DEFER { TTF_Quit(); };

	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 8, 512) == -1)
	{
		DEBUG_printf("MIX_Error: '%s'\n", Mix_GetError());
		ASSERT(false);
		exit(-1);
	}
	DEFER { Mix_CloseAudio(); };

	SDL_Window* window = SDL_CreateWindow("Antihome", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_DIM.x, WIN_DIM.y, 0);
	DEFER { SDL_DestroyWindow(window); };
	if (!window)
	{
		DEBUG_printf("SDL_Error: '%s'\n", SDL_GetError());
		ASSERT(false);
		exit(-1);
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	DEFER { SDL_DestroyRenderer(renderer); };
	if (!renderer)
	{
		DEBUG_printf("SDL_Error: '%s'\n", SDL_GetError());
		ASSERT(false);
		exit(-1);
	}

	{
		SDL_Surface* icon = SDL_LoadBMP(DATA_DIR "icon.bmp");
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}

	Platform platform        = {};
	platform.renderer        = renderer;
	platform.memory_capacity = MEBIBYTES_OF(1);
	platform.memory          = reinterpret_cast<byte*>(malloc(platform.memory_capacity));
	DEFER { free(platform.memory); };

	{
		i32 cursor_x;
		i32 cursor_y;
		SDL_GetMouseState(&cursor_x, &cursor_y);
		platform.cursor = { cursor_x - platform.cursor.x, WIN_DIM.y - 1.0f - cursor_y - platform.cursor.y };
	}

	reload_dll();
	DEFER { SDL_UnloadObject(dll); };

	initialize(&platform);
	boot_up(&platform);
	DEFER { boot_down(&platform); };

	u64 performance_count = SDL_GetPerformanceCounter();
	f32 frame_time        = 0.0f;
	while (true)
	{
		u64 new_performance_count = SDL_GetPerformanceCounter();
		frame_time += static_cast<f32>(new_performance_count - performance_count) / SDL_GetPerformanceFrequency();
		performance_count = new_performance_count;

		for (SDL_Event event; SDL_PollEvent(&event);)
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
				case SDL_KEYUP:
				{
					if (!event.key.repeat)
					{
						switch (event.key.keysym.sym)
						{
							case SDLK_a         : ++platform.inputs[+Input::a        ].curr; break;
							case SDLK_d         : ++platform.inputs[+Input::d        ].curr; break;
							case SDLK_e         : ++platform.inputs[+Input::e        ].curr; break;
							case SDLK_q         : ++platform.inputs[+Input::q        ].curr; break;
							case SDLK_s         : ++platform.inputs[+Input::s        ].curr; break;
							case SDLK_w         : ++platform.inputs[+Input::w        ].curr; break;
							case SDLK_0         : ++platform.inputs[+Input::n0       ].curr; break;
							case SDLK_1         : ++platform.inputs[+Input::n1       ].curr; break;
							case SDLK_2         : ++platform.inputs[+Input::n2       ].curr; break;
							case SDLK_3         : ++platform.inputs[+Input::n3       ].curr; break;
							case SDLK_4         : ++platform.inputs[+Input::n4       ].curr; break;
							case SDLK_5         : ++platform.inputs[+Input::n5       ].curr; break;
							case SDLK_6         : ++platform.inputs[+Input::n6       ].curr; break;
							case SDLK_7         : ++platform.inputs[+Input::n7       ].curr; break;
							case SDLK_8         : ++platform.inputs[+Input::n8       ].curr; break;
							case SDLK_9         : ++platform.inputs[+Input::n9       ].curr; break;
							case SDLK_LEFT      : ++platform.inputs[+Input::left     ].curr; break;
							case SDLK_RIGHT     : ++platform.inputs[+Input::right    ].curr; break;
							case SDLK_DOWN      : ++platform.inputs[+Input::down     ].curr; break;
							case SDLK_UP        : ++platform.inputs[+Input::up       ].curr; break;
							case SDLK_SPACE     : ++platform.inputs[+Input::space    ].curr; break;
							case SDLK_BACKSPACE : ++platform.inputs[+Input::backspace].curr; break;
							case SDLK_RSHIFT    :
							case SDLK_LSHIFT    : ++platform.inputs[+Input::shift    ].curr; break;
							case SDLK_ESCAPE    : ++platform.inputs[+Input::escape   ].curr; break;
							case SDLK_RETURN    : ++platform.inputs[+Input::enter    ].curr; break;
						}
					}
				} break;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				{
					switch (event.button.button)
					{
						case SDL_BUTTON_LEFT  : ++platform.inputs[+Input::left_mouse ].curr; break;
						case SDL_BUTTON_RIGHT : ++platform.inputs[+Input::right_mouse].curr; break;
					}
				} break;

				case SDL_MOUSEWHEEL:
				{
					platform.scroll += event.wheel.preciseY * (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f);
				} break;

				case SDL_WINDOWEVENT:
				{
					if (event.window.event == SDL_WINDOWEVENT_CLOSE)
					{
						goto TERMINATE;
					}
				} break;
			}
		}

		if (fetch_dll_modification_time() != dll_modification_time)
		{
			for (struct stat stat_; stat(EXE_DIR "LOCK.tmp", &stat_) == 0;);

			boot_down(&platform);
			reload_dll();
			boot_up(&platform);
			frame_time = 0.0f;
		}

		if (frame_time >= SECONDS_PER_UPDATE)
		{
			do
			{
				FOR_ELEMS(it, platform.inputs)
				{
					if (it->curr)
					{
						it->curr -= it->prev;
					}
					else
					{
						it->curr = it->prev;
					}
				}

				i32 cursor_delta_x;
				i32 cursor_delta_y;
				SDL_GetRelativeMouseState(&cursor_delta_x, &cursor_delta_y);
				platform.cursor_delta = { static_cast<f32>(cursor_delta_x), -static_cast<f32>(cursor_delta_y) };

				i32 cursor_x;
				i32 cursor_y;
				SDL_GetMouseState(&cursor_x, &cursor_y);
				platform.cursor = { static_cast<f32>(cursor_x), WIN_DIM.y - 1.0f - cursor_y };

				if (update(&platform) == UpdateCode::terminate)
				{
					goto TERMINATE;
				}

				FOR_ELEMS(it, platform.inputs)
				{
					it->prev = it->curr % 2;
					it->curr = 0;
				}

				frame_time -= SECONDS_PER_UPDATE;
			}
			while (frame_time >= SECONDS_PER_UPDATE);

			FOR_ELEMS(it, platform.inputs)
			{
				if (it->curr)
				{
					it->curr -= it->prev;
				}
				else
				{
					it->curr = it->prev;
				}
			}

			render(&platform);

			FOR_ELEMS(it, platform.inputs)
			{
				it->prev = it->curr % 2;
				it->curr = 0;
			}

			platform.scroll = 0.0f;
		}

		SDL_Delay(1);
	}

	TERMINATE:

	return 0;
}
