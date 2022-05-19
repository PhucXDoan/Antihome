#define PRESSED(INPUT)  (!!platform->inputs[+(INPUT)].curr && !platform->inputs[+(INPUT)].prev)
#define HOLDING(INPUT)  (!!platform->inputs[+(INPUT)].curr)
#define RELEASED(INPUT) (!platform->inputs[+(INPUT)].curr && !!platform->inputs[+(INPUT)].prev)

global constexpr f32 TAU           = 6.28318530717f;
global constexpr f32 SQRT2         = 1.41421356237f;
global constexpr f32 INVSQRT2      = 0.70710678118f;
global constexpr i32 MIPMAP_LEVELS = 4;

// @TODO@ Use an actual RNG lol.
// @NOTE@ Is in interval [0, 65536).
global constexpr u16 RAND_TABLE[] =
	{
		0x9b9d, 0x4e65, 0x8ec9, 0x910d, 0xfdf3, 0x73dc, 0x424b, 0xc8f9, 0x30e9, 0x5477, 0xe845, 0xab16, 0x27a1, 0xe07a, 0x6803, 0xb8c1,
		0x9525, 0x1c29, 0xcf1f, 0xeae4, 0x7e41, 0x945e, 0xbbbf, 0x5ac7, 0x3e18, 0xc463, 0x165f, 0x7018, 0x15a4, 0xa666, 0xa8f1, 0xbc54,
		0xe604, 0x3393, 0x5cad, 0xa37b, 0x1880, 0x26cd, 0xb131, 0x3f59, 0x061f, 0x98b7, 0x93fd, 0xc762, 0xae82, 0x264f, 0x9355, 0xc0e8,
		0x5e46, 0x76bd, 0xd8f2, 0x936a, 0x0ccd, 0x8497, 0xd08d, 0x92ad, 0x2e69, 0xb158, 0x0317, 0x127f, 0xa3d3, 0xcf59, 0x1dc6, 0xbb4b,
		0x7ecb, 0xb664, 0x93c7, 0x8c17, 0xdfee, 0xd19c, 0x3bff, 0x6ac7, 0xd3f1, 0xd23c, 0x9b2f, 0x5fae, 0x078c, 0x8282, 0x9fe3, 0xa7fd,
		0xa653, 0xd182, 0x31ad, 0x4e64, 0xeb1f, 0x3da6, 0x133d, 0x6f45, 0x0938, 0x5ac8, 0xcd79, 0x8622, 0xe061, 0x6449, 0xe19e, 0xf63b,
		0x5440, 0xe667, 0x9be6, 0x54b6, 0x8061, 0x6e0b, 0x36f5, 0xdd0d, 0x27f1, 0xfbee, 0x407b, 0x3c76, 0xbaa5, 0xe8dc, 0x2a46, 0x4f8f,
		0x63ab, 0xda6c, 0x5950, 0xe9d2, 0xb825, 0x31b8, 0x2a7c, 0x49f0, 0x1df6, 0xd32a, 0xf42b, 0x68dd, 0xe859, 0x4041, 0x2fce, 0x72f9,
		0x8224, 0x9615, 0x888f, 0xfaef, 0xf191, 0xd627, 0x2dae, 0x69da, 0x2050, 0x9172, 0x5744, 0xe53a, 0xe4b3, 0x4a30, 0x9436, 0x80cd,
		0xb863, 0x7ac0, 0xd72b, 0xb683, 0xc820, 0x36d8, 0xeb86, 0x3bc5, 0xb49c, 0x20fc, 0xa0c6, 0x29c2, 0xd2f4, 0xa04e, 0x90b4, 0xe5f0,
		0x44df, 0x442b, 0xf29c, 0x734a, 0x00c9, 0x40c1, 0xbddf, 0x7fcd, 0x72db, 0x0fdf, 0x366f, 0x36e7, 0xff1e, 0x31a9, 0xd998, 0x67c7,
		0x328f, 0xbf19, 0xaa42, 0xe0ac, 0x6cf0, 0x6c4d, 0x7a87, 0x73b5, 0x165a, 0x3975, 0x99fb, 0x0df0, 0xc3fc, 0x8878, 0xe755, 0x01ab,
		0x2d9e, 0x19dc, 0x2f14, 0xd81a, 0xd4ba, 0x757a, 0xfe60, 0x58fd, 0xcaca, 0x4510, 0x2983, 0xfa28, 0xb899, 0x24e8, 0x90ac, 0x1a92,
		0x4b26, 0xb050, 0xd8e1, 0xcfc4, 0xaabd, 0x0394, 0x1a64, 0x59da, 0x96d1, 0x6ad2, 0x330d, 0x9e1a, 0x4f65, 0xac6f, 0x381d, 0x1c57,
		0xbc45, 0xab88, 0x0f90, 0x4e64, 0x1f1e, 0xbb90, 0xa3d7, 0xa04a, 0x2dd3, 0x468b, 0x71ac, 0xe1d1, 0xd59b, 0x8778, 0x38ed, 0xd353,
		0x5e6f, 0xd907, 0xbbcf, 0xb302, 0xda05, 0xd0df, 0x0cbf, 0x2efe, 0xe53e, 0xe37a, 0x7c8d, 0x9d9d, 0x38aa, 0xbd2d, 0x1a53, 0x0f2b,
		0x32a0, 0x7bf1, 0x1b46, 0x9c7e, 0x4edd, 0x629f, 0x163e, 0x96b4, 0x6584, 0x3e36, 0x6354, 0x3615, 0xb608, 0x1c16, 0xf10e, 0x0301,
		0x2937, 0xd751, 0x3537, 0xf89e, 0x4cd4, 0x2957, 0x891f, 0x0e02, 0x08ba, 0x2c0c, 0xf871, 0x2d8d, 0x4128, 0xb303, 0x57f4, 0xd352,
		0x4237, 0x1cb9, 0x56c9, 0x5afe, 0x364a, 0xa3f8, 0xd495, 0x6bfb, 0x0f84, 0xae4c, 0x99ed, 0x5c8f, 0xfba2, 0xeb6b, 0x1d31, 0x4e18,
		0x79a1, 0x3aaf, 0x1cbd, 0xae7c, 0x2dc4, 0xb8d2, 0xee0f, 0x80b3, 0x9c6f, 0x876f, 0x2689, 0x609c, 0xf114, 0x2797, 0x4b37, 0xa1aa,
		0x51f6, 0xff82, 0x1516, 0xbb8b, 0x7120, 0xeb30, 0xf391, 0x30d5, 0xe7a1, 0x703f, 0x49d1, 0xf6a2, 0xa8df, 0x330f, 0x9c87, 0xe51d,
		0xd436, 0xc0b7, 0x5b31, 0xab89, 0x4e09, 0x7923, 0xa824, 0xeca9, 0x1c7d, 0x426d, 0x1d1d, 0x71f5, 0x6ab9, 0xa53e, 0x3adc, 0xcdc3,
		0x17f3, 0xa949, 0xefef, 0xace4, 0x39f4, 0xc5a6, 0x1b16, 0xc720, 0xc0ae, 0xaca3, 0xd3d1, 0xd5cc, 0xe628, 0x9d40, 0x7ca0, 0x3635,
		0x867f, 0xe054, 0x012e, 0x0cc6, 0xf45f, 0x1b3b, 0x0c23, 0x256e, 0xf062, 0x4721, 0x745a, 0xd40b, 0x53e5, 0xa4c0, 0x2f8e, 0x7696,
		0x4e3b, 0x8d68, 0x0f37, 0x75a9, 0xc4d8, 0xd8e1, 0x2f9d, 0x7bbe, 0xc524, 0xfefe, 0xffee, 0xc746, 0xee9e, 0x7543, 0x9835, 0xcb9b,
		0xf7cf, 0xa8f3, 0xa606, 0x30f4, 0x609b, 0xfa30, 0xad31, 0xbbd7, 0x2a56, 0x8ce4, 0x637e, 0x5ff3, 0xb077, 0xbe95, 0x4c18, 0xa763,
		0xf76e, 0xa450, 0x42b6, 0x0db7, 0x1bee, 0x7e93, 0x5deb, 0xcec3, 0xe19a, 0x4552, 0x90cd, 0x6a68, 0x7d5f, 0xbf81, 0xd899, 0xc69f,
		0xe168, 0x544b, 0x6a1b, 0x11e0, 0x781c, 0x6b9f, 0xe59c, 0xf645, 0x90c7, 0xf833, 0x91fd, 0x094c, 0x1198, 0x30fd, 0x4206, 0x0af2,
		0x59ce, 0x6362, 0x2bef, 0x6a11, 0xff28, 0xb353, 0x481a, 0x8716, 0x44ba, 0x3ea1, 0x8eab, 0x0daf, 0x52de, 0x9656, 0xa848, 0x4adb,
		0xdcfd, 0x3de3, 0xafb0, 0x9a53, 0x2180, 0xfb19, 0x786c, 0xb781, 0x3c8e, 0x7083, 0xc054, 0x3e78, 0x20f4, 0xdf59, 0xfa62, 0x117b,
		0x7ac9, 0x8489, 0xbd24, 0xfccd, 0x5b47, 0x2a9a, 0xde3b, 0x7c1d, 0x7dc0, 0xfc17, 0xba5c, 0x9c4e, 0x5e9e, 0xbcfd, 0x6f0b, 0x82fd,
		0x517b, 0xf57b, 0xacac, 0x5be7, 0x5bb9, 0x6cc2, 0xbd39, 0xdb17, 0x4be8, 0x4eea, 0xeffb, 0x13ed, 0x2391, 0x0b62, 0xd9eb, 0x7b5f
	};

struct Img
{
	vi2          dim;
	vf4*         rgba;
	SDL_Texture* texture;
};

struct Animated
{
	vi2  sprite_dim;
	u32* data;
	i32  frame_count;
	i32  current_index;
	f32  age_hertz;
	f32  age_keytime;
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

template <typename TYPE>
i32 sign(TYPE x)
{
	return (static_cast<TYPE>(0) < x) - (x < static_cast<TYPE>(0));
}

internal constexpr vf2 conjugate(vf2 v) { return {  v.x, -v.y }; }
internal constexpr vf2 rotate90(vf2 v)  { return { -v.y,  v.x }; }
internal vf2 rotate(vf2 v, f32 angle)  { return { v.x * cosf(angle) - v.y * sinf(angle), v.x * sinf(angle) + v.y * cosf(angle) }; }

internal constexpr f32 square(f32 x) { return x * x; }
internal constexpr f32 cube(f32 x) { return x * x * x; }

internal constexpr vf4 lerp(vf4 a, vf4 b, f32 t) { return a * (1.0f - t) + b * t; }
internal constexpr vf3 lerp(vf3 a, vf3 b, f32 t) { return a * (1.0f - t) + b * t; }
internal constexpr vf2 lerp(vf2 a, vf2 b, f32 t) { return a * (1.0f - t) + b * t; }
internal constexpr f32 lerp(f32 a, f32 b, f32 t) { return a * (1.0f - t) + b * t; }

internal constexpr f32 dampen(f32 a, f32 b, f32 k, f32 dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }
internal constexpr vf2 dampen(vf2 a, vf2 b, f32 k, f32 dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }
internal constexpr vf3 dampen(vf3 a, vf3 b, f32 k, f32 dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }
internal constexpr vf4 dampen(vf4 a, vf4 b, f32 k, f32 dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }

internal constexpr f32 dot(vf3 u, vf3 v) { return u.x * v.x + u.y * v.y + u.z * v.z; }
internal constexpr f32 dot(vf2 u, vf2 v) { return u.x * v.x + u.y * v.y;             }

internal constexpr vf3 cross(vf3 u, vf3 v) { return { u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x }; }

internal constexpr vf4 hadamard_multiply(vf4 u, vf4 v) { return { u.x * v.x, u.y * v.y, u.z * v.z, u.w * v.w }; }

internal constexpr vf2 hadamard_divide(vf2 u, vf2 v) { return { u.x / v.x, u.y / v.y }; }

internal constexpr vf3 monochrome(f32 x) { return { x, x, x }; }

internal f32 norm_sq(vf3 v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
internal f32 norm_sq(vf2 v) { return v.x * v.x + v.y * v.y            ; }

internal f32 norm(vf3 v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }
internal f32 norm(vf2 v) { return sqrtf(v.x * v.x + v.y * v.y            ); }

internal vf3 normalize(vf3 v) { return v / norm(v); }
internal vf2 normalize(vf2 v) { return v / norm(v); }

internal f32 distance(vf3 u, vf3 v) { return norm(u - v); }
internal f32 distance(vf2 u, vf2 v) { return norm(u - v); }

internal i32 mod(i32 x, i32 m) { return (x % m + m) % m; }
internal f32 mod(f32 x, f32 m) { f32 y = fmodf(x, m); return y < 0.0f ? y + m : y; }

internal vf2 polar(f32 angle) { return { cosf(angle), sinf(angle) }; }

internal bool32 in_rect(vf2 position, vf2 bottom_left, vf2 dimensions)
{
	return
		bottom_left.x <= position.x && position.x <= bottom_left.x + dimensions.x &&
		bottom_left.y <= position.y && position.y <= bottom_left.y + dimensions.y;
}

internal constexpr u32 pack_color(vf3 color)
{
	return
		(static_cast<u8>(color.x * 255.0f) << 24) |
		(static_cast<u8>(color.y * 255.0f) << 16) |
		(static_cast<u8>(color.z * 255.0f) <<  8) |
		0xFF;
}

internal constexpr u32 pack_color(vf4 color)
{
	return
		(static_cast<u8>(color.x * 255.0f) << 24) |
		(static_cast<u8>(color.y * 255.0f) << 16) |
		(static_cast<u8>(color.z * 255.0f) <<  8) |
		(static_cast<u8>(color.w * 255.0f) <<  0);
}

internal constexpr vf4 unpack_color(u32 pixel)
{
	return
		{
			static_cast<f32>((pixel >> 24) & 0xFF) / 255.0f,
			static_cast<f32>((pixel >> 16) & 0xFF) / 255.0f,
			static_cast<f32>((pixel >>  8) & 0xFF) / 255.0f,
			static_cast<f32>((pixel >>  0) & 0xFF) / 255.0f
		};
}

internal Img init_img(SDL_Renderer* renderer, strlit file_path)
{
	Img img;

	i32  iw;
	i32  ih;
	u32* stbimg = reinterpret_cast<u32*>(stbi_load(file_path, &iw, &ih, 0, STBI_rgb_alpha));
	DEFER { stbi_image_free(stbimg); };
	ASSERT(stbimg);

	img.dim  = { iw, ih };
	img.rgba = reinterpret_cast<vf4*>(malloc(img.dim.x * img.dim.y * sizeof(vf4)));

	FOR_RANGE(y, img.dim.y)
	{
		FOR_RANGE(x, img.dim.x)
		{
			u32 pixel = *(stbimg + y * img.dim.x + x);
			*(img.rgba + x * img.dim.y + (img.dim.y - 1 - y)) =
				{
					static_cast<f32>(pixel >>  0 & 0xFF) / 0xFF,
					static_cast<f32>(pixel >>  8 & 0xFF) / 0xFF,
					static_cast<f32>(pixel >> 16 & 0xFF) / 0xFF,
					static_cast<f32>(pixel >> 24 & 0xFF) / 0xFF
				};
		}
	}

	img.texture = IMG_LoadTexture(renderer, file_path);
	ASSERT(img.texture);

	return img;
}

internal Animated init_animated(strlit file_path, vi2 sheet_dim, f32 age_hertz)
{
	vi2  stbdim;
	u32* stbimg = reinterpret_cast<u32*>(stbi_load(file_path, &stbdim.x, &stbdim.y, 0, STBI_rgb_alpha));
	DEFER { stbi_image_free(stbimg); };
	ASSERT(stbimg);

	Animated animated;
	animated.sprite_dim    = { stbdim.x / sheet_dim.x, stbdim.y / sheet_dim.y };
	animated.data          = reinterpret_cast<u32*>(malloc(stbdim.x * stbdim.y * sizeof(u32)));
	animated.frame_count   = sheet_dim.x * sheet_dim.y;
	animated.current_index = 0;
	animated.age_hertz     = age_hertz;
	animated.age_keytime   = 0.0f;

	FOR_RANGE(i, sheet_dim.x * sheet_dim.y)
	{
		FOR_RANGE(x, animated.sprite_dim.x)
		{
			FOR_RANGE(y, animated.sprite_dim.y)
			{
				animated.data[i * animated.sprite_dim.x * animated.sprite_dim.y + x * animated.sprite_dim.y + y] = stbimg[(i / sheet_dim.x * animated.sprite_dim.y + y) * stbdim.x + (i % sheet_dim.x * animated.sprite_dim.x + x)];
			}
		}
	}

	return animated;
}

internal void deinit_animated(Animated* animated)
{
	free(animated->data);
}

internal vf4 sample_at(Animated* animated, vf2 uv)
{
	ASSERT(0.0f <= uv.x && uv.x <= 1.0f);
	ASSERT(0.0f <= uv.y && uv.y <= 1.0f);
	u32 pixel = animated->data[animated->current_index * animated->sprite_dim.x * animated->sprite_dim.y + static_cast<i32>(uv.x * (animated->sprite_dim.x - 1.0f)) * animated->sprite_dim.y + static_cast<i32>(uv.y * (animated->sprite_dim.y - 1.0f))];
	return
		{
			static_cast<f32>(pixel >>  0 & 0xFF) / 0xFF,
			static_cast<f32>(pixel >>  8 & 0xFF) / 0xFF,
			static_cast<f32>(pixel >> 16 & 0xFF) / 0xFF,
			static_cast<f32>(pixel >> 24 & 0xFF) / 0xFF
		};
}

internal void age_animated(Animated* animated, f32 delta_time)
{
	animated->age_keytime += delta_time * animated->age_hertz;
	if (animated->age_keytime >= 1.0f)
	{
		animated->current_index  = (animated->current_index + static_cast<i32>(animated->age_keytime)) % animated->frame_count;
		animated->age_keytime    -= static_cast<i32>(animated->age_keytime);
	}
}

internal void deinit_img(Img* img)
{
	free(img->rgba);
	SDL_DestroyTexture(img->texture);
}

internal vf4 img_color_at(Img* img, vf2 uv)
{
	ASSERT(0.0f <= uv.x && uv.x <= 1.0f);
	ASSERT(0.0f <= uv.y && uv.y <= 1.0f);
	return img->rgba[static_cast<i32>(uv.x * (img->dim.x - 1.0f)) * img->dim.y + static_cast<i32>(uv.y * (img->dim.y - 1.0f))];
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

	i32 l = static_cast<i32>(CLAMP(level, 0.0f, MIPMAP_LEVELS - 1.0f));
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

internal f32 rng(u32* seed)
{
	return RAND_TABLE[(RAND_TABLE[++*seed % ARRAY_CAPACITY(RAND_TABLE)] + *seed) % ARRAY_CAPACITY(RAND_TABLE)] / 65536.0f;
}

internal i32 rng(u32* seed, i32 start, i32 end)
{
	return static_cast<i32>(rng(seed) * (end - start) + start);
}

internal f32 rng(u32* seed, f32 start, f32 end)
{
	return rng(seed) * (end - start) + start;
}

internal constexpr f32 rng_static(u32 seed)
{
	return RAND_TABLE[seed * seed % ARRAY_CAPACITY(RAND_TABLE)] / 65536.0f;
}

internal constexpr i32 rng_static(u32 seed, i32 start, i32 end)
{
	return static_cast<i32>(rng_static(seed) * (end - start) + start);
}

internal constexpr f32 rng_static(u32 seed, f32 start, f32 end)
{
	return rng_static(seed) * (end - start) + start;
}
internal bool32 ray_cast_line(f32* scalar, f32* portion, vf2 position, vf2 ray, vf2 start, vf2 end)
{
	*scalar = ((start.x - end.x) * (start.y - position.y) - (start.y - end.y) * (start.x - position.x)) / ((start.x - end.x) * ray.y - (start.y - end.y) * ray.x);

	if (*scalar < 0.0f)
	{
		return false;
	}
	else
	{
		f32 portion_c = start.x * ray.y - start.y * ray.x;
		*portion = (portion_c + ray.x * position.y - ray.y * position.x) / (portion_c + ray.x * end.y - ray.y * end.x);

		return true;
	}
}

enum struct Orientation : u8
{
	collinear,
	clockwise,
	counterclockwise
};

bool32 is_point_on_line_segment(vf2 p, vf2 q, vf2 r)
{
	return q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) && q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y);
}

internal Orientation orientation_of(vf2 p, vf2 q, vf2 r)
{
	f32 n = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);

	if (n == 0.0f)
	{
		return Orientation::collinear;
	}
	else if (n > 0.0f)
	{
		return Orientation::clockwise;
	}
	else
	{
		return Orientation::counterclockwise;
	}
}

bool32 is_line_segment_intersecting(vf2 p1, vf2 q1, vf2 p2, vf2 q2)
{
	Orientation o1 = orientation_of(p1, q1, p2);
	Orientation o2 = orientation_of(p1, q1, q2);
	Orientation o3 = orientation_of(p2, q2, p1);
	Orientation o4 = orientation_of(p2, q2, q1);

	return
		o1 != o2 && o3 != o4
		|| o1 == Orientation::collinear && is_point_on_line_segment(p1, p2, q1)
		|| o2 == Orientation::collinear && is_point_on_line_segment(p1, q2, q1)
		|| o3 == Orientation::collinear && is_point_on_line_segment(p2, p1, q2)
		|| o4 == Orientation::collinear && is_point_on_line_segment(p2, q1, q2);
}

struct CollisionData
{
	bool16 exists;
	bool16 inside;
	vf2    displacement;
	vf2    normal;
};

internal CollisionData prioritize_collision(CollisionData a, CollisionData b)
{
	if (a.exists)
	{
		if (b.exists && norm(a.displacement) * (a.inside ? 1.0f : -1.0f) < norm(b.displacement) * (b.inside ? 1.0f : -1.0f))
		{
			return b;
		}
		else
		{
			return a;
		}
	}
	else
	{
		return b;
	}
}

internal CollisionData collide_thick_line(vf2 position, vf2 ray, vf2 start, vf2 end, f32 padding)
{
	vf2 n  = normalize(rotate90(end - start));
	f32 d0 = dot(start + n * padding - position, n);
	f32 d1 = dot(start - n * padding - position, n);

	constexpr f32 EPSILON = 0.001f;
	if (fabsf(d0) <= EPSILON) { d0 = 0.0f; }
	if (fabsf(d1) <= EPSILON) { d1 = 0.0f; }

	if (sign(d0) == sign(d1))
	{
		f32 den = (end.y - start.y) * ray.x - (end.x - start.x) * ray.y;
		f32 k0  = ((end.x - start.x) * (position.y - start.y - n.y * padding) - (end.y - start.y) * (position.x - start.x - n.x * padding)) / den;
		f32 k1  = ((end.x - start.x) * (position.y - start.y + n.y * padding) - (end.y - start.y) * (position.x - start.x + n.x * padding)) / den;

		if (k0 < k1)
		{
			if (0.0f < k0 && k0 <= 1.0f)
			{
				CollisionData data;
				data.exists       = true;
				data.inside       = false;
				data.displacement = k0 * ray;
				data.normal       = n;
				return data;
			}
			else
			{
				CollisionData data;
				data.exists = false;
				return data;
			}
		}
		else if (0.0f < k1 && k1 <= 1.0f)
		{
			CollisionData data;
			data.exists       = true;
			data.inside       = false;
			data.displacement = k1 * ray;
			data.normal       = -n;
			return data;
		}
		else
		{
			CollisionData data;
			data.exists = false;
			return data;
		}
	}
	else if (d0 == 0.0f && dot(ray, n) >= 0.0f || d1 == 0.0f && dot(ray, n) <= 0.0f)
	{
		CollisionData data;
		data.exists = false;
		return data;
	}
	else if (fabsf(d0) < fabsf(d1))
	{
		CollisionData data;
		data.exists       = true;
		data.inside       = true;
		data.displacement = d0 * n;
		data.normal       = n;
		return data;
	}
	else
	{
		CollisionData data;
		data.exists       = true;
		data.inside       = true;
		data.displacement = d1 * n;
		data.normal       = -n;
		return data;
	}
}

internal CollisionData collide_circle(vf2 position, vf2 ray, vf2 center, f32 radius)
{
	f32 norm_sq_ray  = norm_sq(ray);
	f32 discriminant = norm_sq_ray * radius * radius - square(ray.x * (position.y - center.y) - ray.y * (position.x - center.x));

	if (discriminant <= 0.0f)
	{
		CollisionData data;
		data.exists = false;
		return data;
	}
	else
	{
		f32 k0 = (dot(center - position, ray) - sqrtf(discriminant)) / norm_sq_ray;
		f32 k1 = (dot(center - position, ray) + sqrtf(discriminant)) / norm_sq_ray;

		if (k0 < 0.0f && k1 < 0.0f || dot(position - center, ray) >= 0.0f)
		{
			CollisionData data;
			data.exists = false;
			return data;
		}
		else if (sign(k0) == sign(k1))
		{
			if (min(k0, k1) <= 1.0f)
			{
				CollisionData data;
				data.exists       = true;
				data.inside       = false;
				data.displacement = min(k0, k1) * ray;
				data.normal       = normalize(position + data.displacement - center);
				return data;
			}
			else
			{
				CollisionData data;
				data.exists = false;
				return data;
			}
		}
		else
		{
			CollisionData data;
			data.exists       = true;
			data.inside       = true;
			data.displacement = normalize(position - center) * radius + center - position;
			data.normal       = data.displacement + position - center;
			return data;
		}
	}
}

internal CollisionData collide_pill(vf2 position, vf2 ray, vf2 start, vf2 end, f32 padding)
{
	CollisionData data = collide_thick_line(position, ray, start, end, padding);

	if (data.exists)
	{
		f32 portion = dot(position + data.displacement - start, (end - start) / norm_sq(end - start));
		data.exists = 0.0f <= portion && portion <= 1.0f;
	}

	return prioritize_collision(prioritize_collision(data, collide_circle(position, ray, start, padding)), collide_circle(position, ray, end, padding));
}

internal void set_color(SDL_Renderer* renderer, vf3 color)
{
	SDL_SetRenderDrawColor
	(
		renderer,
		static_cast<u8>(color.x * 255.0f),
		static_cast<u8>(color.y * 255.0f),
		static_cast<u8>(color.z * 255.0f),
		255
	);
}

internal void set_color(SDL_Renderer* renderer, vf4 color)
{
	SDL_SetRenderDrawColor
	(
		renderer,
		static_cast<u8>(color.x * 255.0f),
		static_cast<u8>(color.y * 255.0f),
		static_cast<u8>(color.z * 255.0f),
		static_cast<u8>(color.w * 255.0f)
	);
}

internal i32 iterate_repeated_movement(Platform* platform, Input negative_input, Input positive_input, f32* current_repeat_countdown, f32 repeat_threshold = 0.3f, f32 repeat_frequency = 0.1f)
{
	i32 delta = 0;

	if (HOLDING(negative_input) != HOLDING(positive_input))
	{
		if (*current_repeat_countdown <= 0.0f)
		{
			if (HOLDING(negative_input))
			{
				delta                     = -1;
				*current_repeat_countdown =
					PRESSED(negative_input)
						? repeat_threshold
						: repeat_frequency;
			}
			else
			{
				delta                     = 1;
				*current_repeat_countdown =
					PRESSED(positive_input)
						? repeat_threshold
						: repeat_frequency;
			}
		}
	}
	else
	{
		*current_repeat_countdown = 0.0f;
	}

	return delta;
}

internal void render_circle(SDL_Renderer* renderer, vf2 center, f32 radius)
{
	ASSERT(radius < 256.0f);

	vf2 p     = { radius - 1.0f, 0.0f };
	vi2 t     = { 1, 1 };
	f32 error = t.x - 2.0f * radius;

	while (p.x >= p.y)
	{
		SDL_RenderDrawPointF(renderer, center.x + p.x, center.y - p.y);
		SDL_RenderDrawPointF(renderer, center.x + p.x, center.y + p.y);
		SDL_RenderDrawPointF(renderer, center.x - p.x, center.y - p.y);
		SDL_RenderDrawPointF(renderer, center.x - p.x, center.y + p.y);
		SDL_RenderDrawPointF(renderer, center.x + p.y, center.y - p.x);
		SDL_RenderDrawPointF(renderer, center.x + p.y, center.y + p.x);
		SDL_RenderDrawPointF(renderer, center.x - p.y, center.y - p.x);
		SDL_RenderDrawPointF(renderer, center.x - p.y, center.y + p.x);

		if (error <= 0.0f)
		{
			p.y   += 1.0f;
			error += t.y;
			t.y   += 2;
		}

		if (error > 0.0f)
		{
			p.x   -= 1.0f;
			t.x   += 2;
			error += t.x - 2.0f * radius;
		}
	}
}

internal void render_filled_circle(SDL_Renderer* renderer, vf2 center, f32 radius)
{
	ASSERT(radius < 256.0f);

	vf2 p     = { radius - 1.0f, 0.0f };
	vi2 t     = { 1, 1 };
	f32 error = t.x - 2.0f * radius;

	while (p.x >= p.y)
	{
		SDL_RenderDrawLineF(renderer, center.x - p.x, center.y - p.y, center.x + p.x, center.y - p.y);
		SDL_RenderDrawLineF(renderer, center.x - p.y, center.y - p.x, center.x + p.y, center.y - p.x);
		SDL_RenderDrawLineF(renderer, center.x - p.x, center.y + p.y, center.x + p.x, center.y + p.y);
		SDL_RenderDrawLineF(renderer, center.x - p.y, center.y + p.x, center.x + p.y, center.y + p.x);

		if (error <= 0.0f)
		{
			p.y   += 1.0f;
			error += t.y;
			t.y   += 2;
		}

		if (error > 0.0f)
		{
			p.x   -= 1.0f;
			t.x   += 2;
			error += t.x - 2.0f * radius;
		}
	}
}

internal void render_filled_rect(SDL_Renderer* renderer, vf2 bottom_left, vf2 dimensions)
{
	SDL_FRect rect = { bottom_left.x, bottom_left.y, dimensions.x, dimensions.y };
	SDL_RenderFillRectF(renderer, &rect);
}

internal void render_rect(SDL_Renderer* renderer, vf2 bottom_left, vf2 dimensions)
{
	SDL_FRect rect = { bottom_left.x, bottom_left.y, dimensions.x, dimensions.y };
	SDL_RenderDrawRectF(renderer, &rect);
}

internal void render_texture(SDL_Renderer* renderer, SDL_Texture* texture, vf2 position, vf2 dimensions)
{
	SDL_FRect rect = { position.x, position.y, dimensions.x, dimensions.y };
	SDL_RenderCopyF(renderer, texture, 0, &rect);
}

template <typename... ARGUMENTS>
internal void render_text(SDL_Renderer* renderer, FC_Font* font, vf2 coordinates, f32 baseline_offset, FC_AlignEnum alignment, f32 scalar, vf4 rgba, strlit fstr, ARGUMENTS... arguments)
{
	FC_DrawEffect
	(
		font,
		renderer,
		coordinates.x,
		coordinates.y - FC_GetBaseline(font) * scalar * baseline_offset,
		FC_MakeEffect
		(
			alignment,
			FC_MakeScale(scalar, scalar),
			FC_MakeColor
			(
				static_cast<u8>(rgba.x * 255.0f),
				static_cast<u8>(rgba.y * 255.0f),
				static_cast<u8>(rgba.z * 255.0f),
				static_cast<u8>(rgba.w * 255.0f)
			)
		),
		fstr,
		arguments...
	);
}

template <typename... ARGUMENTS>
internal void render_boxed_text(SDL_Renderer* renderer, FC_Font* font, vf2 coordinates, vf2 dimensions, FC_AlignEnum alignment, f32 scalar, vf4 rgba, strlit fstr, ARGUMENTS... arguments)
{
	FC_DrawBoxEffect
	(
		font,
		renderer,
		{ static_cast<i32>(coordinates.x), static_cast<i32>(coordinates.y), static_cast<i32>(dimensions.x / scalar), static_cast<i32>(dimensions.y) },
		FC_MakeEffect
		(
			alignment,
			FC_MakeScale(scalar, scalar),
			FC_MakeColor
			(
				static_cast<u8>(rgba.x * 255.0f),
				static_cast<u8>(rgba.y * 255.0f),
				static_cast<u8>(rgba.z * 255.0f),
				static_cast<u8>(rgba.w * 255.0f)
			)
		),
		fstr,
		arguments...
	);
}

internal void render_line(SDL_Renderer* renderer, vf2 start, vf2 end)
{
	ASSERT(fabsf(start.x - end.x) + fabsf(start.y - end.y) < 4096.0f);
	SDL_RenderDrawLineF(renderer, start.x, start.y, end.x, end.y);
}
