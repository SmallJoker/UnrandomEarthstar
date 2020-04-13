#include "headers.h"
#include "image.h"
#include "tile.h"
#include "util/timer.h"
#include <fstream>
#include <unordered_map>

Image::Image(const std::string &filepath)
{
	// Open & check
	{
		m_file = fopen(filepath.c_str(), "rb");

		if (!m_file)
			ERROR("Cannot open/find file");

		uint8_t header[8];
		if (fread(header, 1, 8, m_file) != 8 || png_sig_cmp(header, 0, 8))
			ERROR("Not a PNG file");
	}

	m_png  = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	m_info = png_create_info_struct(m_png);

	if (setjmp(png_jmpbuf(m_png)))
		ERROR("Cannot set scope to current routine");

	png_init_io(m_png, m_file);
	png_set_sig_bytes(m_png, 8); // Header already read
	png_set_user_limits(m_png, 0xFFFF, 0xFFFF);
	png_read_png(m_png, m_info,
		PNG_TRANSFORM_STRIP_16 |
		PNG_TRANSFORM_STRIP_ALPHA |
		PNG_TRANSFORM_PACKING |
		PNG_TRANSFORM_BGR, nullptr);

	this->size.X = png_get_image_width(m_png, m_info);
	this->size.Y = png_get_image_height(m_png, m_info);

	auto color_type = png_get_color_type(m_png, m_info);
	m_bpp = png_get_rowbytes(m_png, m_info) / size.X;

	LOG("Loaded image " << filepath << std::endl
		<< "\tSize:        " << PP(size) << std::endl
		<< "\tColor type:  " << (int)color_type << std::endl
		<< "\tDepth:       " << (int)png_get_bit_depth(m_png, m_info) << std::endl
		<< "\tBytes/Pixel: " << m_bpp);
}

Image::~Image()
{
	png_destroy_read_struct(&m_png, &m_info, nullptr);
	fclose(m_file);
	m_png = nullptr;
	m_info = nullptr;
	m_file = nullptr;

	if (m_output) {
		for (int y = 0; y < size.Y; ++y)
			delete[] m_output[y];

		delete[] m_output;
	}
}

void Image::read(const v2u16 &n_tiles)
{
	if (setjmp(png_jmpbuf(m_png)))
		ERROR("Cannot set scope to current routine");

	if (m_output) {
		for (int y = 0; y < size.Y; ++y)
			delete[] m_output[y];

		delete[] m_output;
	}
	g_pool.clear();

	Timer t_("Image::read");
	g_pool.reserve(n_tiles.X * n_tiles.Y);
	m_output = new uint8_t*[size.Y * DBG_SCALE];

	// Reading
	{
		size_t bytes_per_row = png_get_rowbytes(m_png, m_info);
		LOG("Bytes per row: " << bytes_per_row);

		m_image = png_get_rows(m_png, m_info);

		// Copy
		for (int y = 0; y < size.Y * DBG_SCALE; ++y){
			m_output[y] = new uint8_t[bytes_per_row * DBG_SCALE];
			for (size_t x = 0; x < bytes_per_row * DBG_SCALE; ++x)
				m_output[y][x] = 0x22;
		}
	}
	
	// Parse it!
	m_tilesize = size / n_tiles;
	LOG("Reading " << PP(n_tiles) << " tiles, tilesize=" << PP(m_tilesize));

	// Map to find other tiles easily
	std::unordered_map<uint32_t, Tile *> tiles;
	tiles.reserve(n_tiles.X * n_tiles.Y);

	v2u16 total_segs = n_tiles * SEGNUM;

	Tile *tile;
	for (int y = 0; y < total_segs.Y; ++y)
	for (int x = 0; x < total_segs.X; ++x) {
		v2u16 seg_pos(x & (SEGNUM - 1), y & (SEGNUM - 1));
		// Faces for X and Y: "Edge"/Corner cases
		Vector2D<uint8_t> facenum(TP_TOTAL, TP_TOTAL);

		if (seg_pos.Y == 0)
			facenum.X = TP_TOP;
		else if (seg_pos.Y == (SEGNUM - 1))
			facenum.X = TP_BOTTOM;

		if (seg_pos.X == 0)
			facenum.Y = TP_LEFT;
		else if (seg_pos.X == (SEGNUM - 1))
			facenum.Y = TP_RIGHT;

		// Ignore center segments
		if (facenum == Vector2D<uint8_t>(TP_TOTAL, TP_TOTAL))
			continue;

		v2u16 tile_pos(x / SEGNUM, y / SEGNUM);
		v2u16 image_pos = size / total_segs * v2u16(x, y);

		// Get or create new
		auto it = tiles.find(tile_pos.getHash());
		if (it != tiles.end()) {
			tile = it->second;
		} else {
			tile = new Tile(tile_pos, image_pos);
			tiles[tile_pos.getHash()] = tile;
			VERBOSE("Add tile " << tile_pos.getHash());
			g_pool.push_back(tile);
		}

		uint8_t avg = getAverage(image_pos, image_pos + m_tilesize / SEGNUM);

		VERBOSE(PP(image_pos) << " avg=" << (int)avg << std::endl
			<< "\tfaceX=" << (int)facenum.X
			<< "\tfaceY=" << (int)facenum.Y
			<< "\tSegm =" << PP(seg_pos));

		if (facenum.X != TP_TOTAL)
			tile->faces[facenum.X].colors[seg_pos.X] = avg;
		if (facenum.Y != TP_TOTAL)
			tile->faces[facenum.Y].colors[seg_pos.Y] = avg;
	}

	if (g_pool.size() != (size_t)n_tiles.X * n_tiles.Y)
		ERROR("Image parser is broken");
}

void Image::smoothen()
{
	for (Tile *tile : g_pool) {
		for (int i = 0; i < TP_TOTAL; ++i) {
			uint8_t min = 0xFF, max = 0;

			uint8_t *colors = tile->faces[i].colors;

			for (int j = 0; j < SEGNUM; ++j) {
				if (colors[j] < min)
					min = colors[j];
				if (colors[j] > max)
					max = colors[j];
			}

			tile->faces[i].variance = max - min;
		}
	}
}

void Image::save(const std::string &filepath)
{
	FILE *file = fopen(filepath.c_str(), "wb");

	if (!file)
		ERROR("Cannot upen file");

	png_struct *png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_info  *info = png_create_info_struct(m_png);

	if (setjmp(png_jmpbuf(png)))
		ERROR("Cannot set scope to current routine");

	png_init_io(png, file);
	// "Long-hand" for modifying the now hidden properies "width" and "height"
	// Direct access works in libpng 1.2.x, but not 1.6.x
	//m_info->width = size.X * DBG_SCALE;
	//m_info->height = size.Y * DBG_SCALE;
	png_set_IHDR(
		png,
		info,
		size.X * DBG_SCALE,
		size.Y * DBG_SCALE,
		png_get_bit_depth(m_png, m_info),
		png_get_color_type(m_png, m_info),
		png_get_interlace_type(m_png, m_info),
		png_get_compression_type(m_png, m_info),
		png_get_filter_type(m_png, m_info)
	);

	png_write_info(png, info);
	png_write_image(png, m_output);
	png_write_end(png, nullptr);

	fclose(file);

	png_destroy_write_struct(&png, &info);
}

uint8_t Image::getAverage(const v2u16 &start, v2u16 end)
{
	if (end.X > size.X)
		end.X = size.X;

	if (end.Y > size.Y)
		end.Y = size.Y;

	v2u16 diff = end - start;

	if (diff.X + diff.Y == 0)
		return 0;

	uint64_t avg = 0;
	for (int y = start.Y; y < end.Y; ++y) {
		uint8_t *row = m_image[y];
		for (int x = start.X; x < end.X; ++x) {
			avg += row[x * m_bpp]; // R
		}
	}

	avg = (avg + 1) / ((uint64_t)diff.X * diff.Y);

#ifdef DBG_BLUR
	for (int y = start.Y; y < end.Y; ++y) {
		uint8_t *row = m_image[y];
		for (int x = start.X; x < end.X; ++x) {
			row[x * m_bpp + 0] = avg;
			row[x * m_bpp + 1] = avg;
			row[x * m_bpp + 2] = avg;
			if (m_bpp == 4)
				row[x * m_bpp + 3] = 0xFF;
		}
	}
#endif

	return avg;
}

void Image::debugColorize(Tile *tile, uint8_t color, bool source)
{
	for (int y = 0; y < m_tilesize.Y; ++y) {
		uint8_t *row  = (source ? m_image : m_output)[tile->original_pos.Y + y];
		for (int x = 0; x < m_tilesize.X * m_bpp; ++x) {
			row[tile->original_pos.X * m_bpp + x] = color;
		}
	}
}

void Image::plotTile(Tile *start_tile, bool clear)
{
	LOG("Plot start from: " << PP(start_tile->original_pos));

	if (clear) {
		for (int y = 0; y < size.Y * DBG_SCALE; ++y)
			for (int x = 0; x < size.X * m_bpp * DBG_SCALE; ++x)
				m_output[y][x] = 0x22;
	}

	tilecall_t f_plot = [&] (Tile *tile) {
		auto it = g_mapdata->find(tile);
		if (it == g_mapdata->end()) {
			VERBOSE("Tile not mapped: " << PP(tile->original_pos));
			return;
		}

		v2s16 pos = it->second * v2s16(m_tilesize.X, m_tilesize.Y);

		VERBOSE("src=" << PP(tile->original_pos) << " dst=" << PP(pos));
		if (pos.X + m_tilesize.X > size.X * DBG_SCALE
				|| pos.Y + m_tilesize.X > size.Y * DBG_SCALE)
			return;


		for (int y = 0; y < m_tilesize.Y; ++y) {
			uint8_t *inp  = m_image[tile->original_pos.Y + y];
			uint8_t *outp = m_output[pos.Y + y];

			for (int x = 0; x < m_tilesize.X * m_bpp; ++x) {
				outp[pos.X * m_bpp + x] = 
					inp[tile->original_pos.X * m_bpp + x];
			}
		}
	};

	for (Tile *tile : g_pool)
		f_plot(tile);
}
