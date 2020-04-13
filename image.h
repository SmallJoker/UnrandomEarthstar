#pragma once

#include "headers.h"


#define PNG_DEBUG 3
#include <png.h>

// >1 to draw a huge image to show most tiles
#define DBG_SCALE 3
// Show border segments of the drawn tiles
#define DBG_BLUR


class Tile;

class Image {
public:
	Image(const std::string &filename);
	~Image();
	void read(const v2u16 &n_tiles);
	void smoothen();
	void save(const std::string &filename);
	void debugColorize(Tile *tile, uint8_t color, bool source = false);
	void plotTile(Tile *tile, bool clear = false);

	void close();


	v2u16 size;
private:
	uint8_t getAverage(const v2u16 &start, v2u16 end);

	FILE *m_file = nullptr;
	png_struct *m_png = nullptr;
	png_info *m_info = nullptr;
	uint8_t **m_image = nullptr;
	uint8_t **m_output = nullptr;
	v2u16 m_tilesize;
	int m_bpp;
};
