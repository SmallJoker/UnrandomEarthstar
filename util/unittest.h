#pragma once

class Image;

class Unittest {
public:
	Unittest();
	~Unittest();
	static int updateImage(Image *img, bool clear);

private:
	void checkSimilar();
	void similarOverall();
	void moveLink();
	
	
	Image *m_img;
};
