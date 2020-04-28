# UnrandomEarthstar

This application is based on a challenge that took longer to solve than expected.

License: MIT


## Problem

The input image is a puzzle consisting of quadratic tiles.

![simple.png](https://raw.githubusercontent.com/SmallJoker/UnrandomEarthstar/master/images/simple.png)


## Solution

This is the third attempt at sorting the tiles somehow.
Hint: bouncy-spring-sort worked okay-ish too, but unfolding
the mess in the center turned out to be very complicated.

1) Compare opposite sides of a tile with each other tile
2) Find the best matching sides
3) Link the two tiles as "neighbours"
4) Generate a 2D map of the newly connected tiles
5) If there are overlapping tiles: Undo linking

TODO: Teach the problem to try/error different links.


## Project Compiling

**Required libraries:**

 * PNG: libpng-dev (1.6.0+)
 * ZLIB: zlib1g-dev

And of course, CMake and a C++ compiler.

**Run the demo:**

	cmake .
	make
	./UnrandomEarthstar

Nobody said it's perfect, right?