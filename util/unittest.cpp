#include "unittest.h"
#include "image.h"
#include "headers.h"
#include "tile.h"

#include <algorithm> // std::sort
#include <vector>

const char *testfile = "images/simple.png";


Unittest::Unittest()
{
	m_img = new Image(testfile);
	m_img->read(v2u16(4, 4));
	m_img->smoothen();

	checkSimilar();
	similarOverall();
	moveLink();

	updateImage(m_img, false);

	//closestMatchLoop(img);

	exit(0);
}

Unittest::~Unittest()
{
	delete m_img;
}

int Unittest::updateImage(Image *img, bool clear)
{
	static std::vector<v2s16> oldpos;
	if (oldpos.size() != g_pool.size())
		oldpos.resize(g_pool.size());

	Tile *center;
	int n_tiles = Tile::sortAllUnsafe(center);
	img->plotTile(center, clear);
	img->save("images/out.png");

	return n_tiles;
}


void Unittest::checkSimilar()
{
	auto compare_tiles = [](const std::string &name, Tile *t1, Tile *t2) {
		TILE_POS best_face = TP_TOTAL;
		int distance = t1->getDistance(t2, &best_face);
		LOG(name << ": distance=" << distance << ", face=" << (int)best_face);
	};

	compare_tiles("hair     ", g_pool[ 5], g_pool[ 6]);
	compare_tiles("white_top", g_pool[ 6], g_pool[ 9]);
	compare_tiles("aliasing ", g_pool[10], g_pool[11]); // walkway
}

void Unittest::similarOverall()
{
	typedef std::pair<Tile *, int> tilediff_t;
	std::vector<tilediff_t> ranking;

	Tile *main = g_pool[2];
	LOG("Testing matches with tile " << PP(main->original_pos));
	for (Tile *tile : g_pool) {
		if (tile == main)
			continue;

		TILE_POS face;
		int diff = main->getDistance(tile, &face);
		if (diff >= 0)
			ranking.push_back(tilediff_t(tile, diff));
	}

	std::sort(ranking.begin(), ranking.end(),
			[](const tilediff_t &a, const tilediff_t &b) {
		return a.second < b.second;
	});

	int n = 0;
	for (tilediff_t &result : ranking) {
		LOG("MATCH " << PP(result.first->original_pos)
			<< " diff=" << result.second);
		// << " @ " << (int)face);
		if (++n > 7)
			break;
	}
}

void Unittest::moveLink()
{
	/*Tile *t1 = g_pool[5];
	Tile *t2 = g_pool[6];
	TILE_POS best_face = TP_RIGHT;
	int distance;

	LOG("pos1=" << PP(t1->pos) << " pos2=" << PP(t2->pos));
	t1->moveTo(t2->pos - tile_pos_to_dir[best_face]);
	t1->link(t2);
	LOG("pos1=" << PP(t1->pos) << " pos2=" << PP(t2->pos));
	distance = t1->isNeighbour(t2, &best_face);
	LOG("neighbour1? " << distance << " face=" << (int)best_face);
	distance = t2->isNeighbour(t1, &best_face);
	LOG("neighbour2? " << distance << " face=" << (int)best_face);

	distance = g_pool[4]->getDistance(t2, &best_face);
	LOG("Simple distance: " << distance << " face: " << (int)best_face);
	g_pool[4]->moveTo(t2->pos - tile_pos_to_dir[best_face]);
	g_pool[4]->link(t2);*/
}
