#pragma once

#include "headers.h"
#include <functional>
#include <unordered_map>
#include <vector>

#define SEGNUM 16

class Tile;
extern std::unordered_map<Tile *, v2s16> *g_mapdata;
extern std::vector<Tile *> g_pool;


enum TILE_POS : uint8_t {
	TP_LEFT,
	TP_TOP,
	TP_RIGHT,
	TP_BOTTOM,
	TP_TOTAL
};

extern v2s16 tile_pos_to_dir[TP_TOTAL];

inline TILE_POS swapTilePos(int pos)
{
	return (TILE_POS)((pos + 2) & (TP_TOTAL - 1));
}

typedef std::function<void(Tile *)> tilecall_t;

class Face {
public:
	int getDistance(const Face &other) const;

	uint8_t colors[SEGNUM];
	uint8_t variance;
};

class Tile {
public:
	Tile(const v2u16 &tilepos, const v2u16 &original);

	static Tile *getAtPos(const v2s16 &pos);

	// out: link successful?
	bool link(Tile *other, TILE_POS face);
	bool unlink(TILE_POS face);

	int getDistance(Tile *other, TILE_POS *best_match) const;
	Tile *getNeighbour(TILE_POS face);
	int getDistanceAll() const;

	int recursiveExecS(tilecall_t func);
	int getWeight();
	int getDimensions(v2s16 &dim_min, v2s16 &dim_max);
	bool makeMap(v2s16 pos);
	static void dumpMap();
	static int sortAllUnsafe(Tile *&center);

	// 0 = neighbour/myself
	// 1 = from previous recursive action
	inline bool getSeenDiff() const { return s_seen_max - m_seen; };
	static void pushSeen() { ++s_seen_max; }
	static void popSeen();

	v2u16 original_pos;

	Face faces[TP_TOTAL];
	int link_count;

private:
	static int s_seen_max;

	void checkLinks();

	Tile *neighbours[TP_TOTAL];
	int m_seen = false;
};
