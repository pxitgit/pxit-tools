//TargaImage.h - class used to create TARGA files or read TARGA files
#include <stdio.h>

class TargaImage {
public:
	TargaImage(int Width, int Height);  //Used to create Targa file.
	TargaImage(char *filename, int width, int height); //Used to read file.

	long *getFrame();
	void fillBox(int top, int bottom, int left, int right, long color);
	void writeFile(char *filename, int bpp = 32);
	void displayHeader();

private:
	long *frame;
	int _width, _height;
	FILE *tga;
	char filename[100];
	bool overwriteOK;

	void writeheader(FILE *, int bpp = 32);
	void readheader(FILE *);
	void decodeRLE(FILE *);
	void decode(FILE *);

	struct {      //Targa header format.
		char id_len;
		char map_type;
		char img_type;
		char map_first;
		char map_len;
		char map_entry_size;
		int x;
		int y;
		int width;
		int height;
		char bpp;
		char misc;
	} header;
};
