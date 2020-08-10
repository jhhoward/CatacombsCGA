#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "lodepng.h"
#include "Defines.h"
#include "FixedMath.h"

using namespace std;

#define GENERATED_DIR "Source/COTD/Generated/"

const char* spriteDataHeaderOutputPath = GENERATED_DIR "SpriteData.inc.h";
const char* spriteTypesHeaderOutputPath = GENERATED_DIR "SpriteTypes.h";
const char* sinTableHeaderOutputPath = GENERATED_DIR "LUT.inc.h";
const char* wallScalerHeaderOutputPath = GENERATED_DIR "WallScaler.inc.h";
const char* drawRoutinesHeaderOutputPath = GENERATED_DIR "DrawRoutines.inc.h";

void WriteWallScaler(ofstream& output);

#if USE_GRAPHICS_MODE
constexpr int numPaletteEntries = 16;
unsigned char paletteOutput[numPaletteEntries];

#if USE_COMPOSITE_COLOURS
unsigned char palette[numPaletteEntries * 3] =
{
	0x00, 0x00, 0x00,
	0x00, 0x6e, 0x31,
	0x31, 0x09, 0xff,
	0x00, 0x8a,	0xff,
	0xa7, 0x00, 0x31,
	0x76, 0x76, 0x76,
	0xec, 0x11, 0xff,
	0xbb, 0x92, 0xff,
	0x31, 0x5a, 0x00,
	0x00, 0xdb, 0x00,
	0x76, 0x76, 0x76,
	0x45, 0xf7, 0xbb,
	0xec, 0x63, 0x00,
	0xbb, 0xe4, 0x00,
	0xff, 0x7f, 0xbb,
	0xff, 0xff, 0xff
};

void GeneratePalette()
{
	for (int n = 0; n < numPaletteEntries; n++)
	{
		paletteOutput[n] = n | (n << 4);
	}
}
#else
constexpr int numBasePaletteEntries = 4;

unsigned char basePalette[4 * 3] =
{
	0,	0,	0,
	0x55,	0xff,	0xff,
	0xff,	0x55,	0xff,
	0xff,	0xff,	0xff
};

/*unsigned char basePalette[4 * 3] =
{
	0xff,	0xff,	0xff,
	0xaa,	0xaa,	0xaa,
	0x55,	0x55,	0x55,
	0x00,	0x00,	0x00,
};*/

unsigned char palette[16 * 4];

void GeneratePalette()
{
	int index = 0;

	for (int y = 0; y < numBasePaletteEntries; y++)
	{
		for (int x = 0; x < numBasePaletteEntries; x++)
		{
			palette[index * 3] = (basePalette[x * 3] / 2) + (basePalette[y * 3] / 2);
			palette[index * 3 + 1] = (basePalette[x * 3 + 1] / 2) + (basePalette[y * 3 + 1] / 2);
			palette[index * 3 + 2] = (basePalette[x * 3 + 2] / 2) + (basePalette[y * 3 + 2] / 2);

			paletteOutput[index] = (x) | (y << 2) | (x << 4) | (y << 6);
			index++;
		}
	}

	vector<unsigned char> pixels;
	for (int n = 0; n < numPaletteEntries; n++)
	{
		pixels.push_back(palette[n * 3 + 0]);
		pixels.push_back(palette[n * 3 + 1]);
		pixels.push_back(palette[n * 3 + 2]);
		pixels.push_back(255);
	}
	lodepng::encode("pal.png", pixels, 4, 4);
}
#endif

#else
constexpr int numPaletteEntries = 16;
unsigned char palette[numPaletteEntries * 3] =
{
	0,	0,	0,
	0,	0,	0xaa,
	0,	0xaa,	0,
	0,	0xaa,	0xaa,
	0xaa,	0,	0,
	0xaa,	0,	0xaa,
	0xaa,	0x55,	0,
	0xaa,	0xaa,	0xaa,
	0x55,	0x55,	0x55,
	0x55,	0x55,	0xff,
	0x55,	0xff,	0x55,
	0x55,	0xff,	0xff,
	0xff,	0x55,	0x55,
	0xff,	0x55,	0xff,
	0xff,	0xff,	0x55,
	0xff,	0xff,	0xff
};
#endif

unsigned char matchColour(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	if (a == 0)
	{
		return 0xff;
	}

	unsigned char closest = 0;
	int closestDistance = -1;

	for (int n = 0; n < numPaletteEntries; n++)
	{
		int distance = (r - palette[n * 3]) * (r - palette[n * 3])
			+ (g - palette[n * 3 + 1]) * (g - palette[n * 3 + 1])
			+ (b - palette[n * 3 + 2]) * (b - palette[n * 3 + 2]);
		if (closestDistance == -1 || distance < closestDistance)
		{
			closestDistance = distance;
			closest = (unsigned char)(n);
		}
	}

	return closest;
}

enum class ImageColour
{
	Transparent,
	Black,
	White
};

ImageColour CalculateColour(vector<uint8_t>& pixels, unsigned int x, unsigned int y, unsigned int pitch)
{
	unsigned int index = (y * pitch + x) * 4;
	if(pixels[index] == 0 && pixels[index + 1] == 0 && pixels[index + 2] == 0 && pixels[index + 3] == 255)
	{
		return ImageColour::Black;
	}
	else if (pixels[index] == 255 && pixels[index + 1] == 255 && pixels[index + 2] == 255 && pixels[index + 3] == 255)
	{
		return ImageColour::White;
	}

	return ImageColour::Transparent;
}

void EncodeSprite3D(ofstream& typefs, ofstream& fs, const char* inputPath, const char* variableName)
{
	vector<uint8_t> pixels;
	unsigned width, height;
	unsigned error = lodepng::decode(pixels, width, height, inputPath);
	
	if(error)
	{
		cout << inputPath << " : decoder error " << error << ": " << lodepng_error_text(error) << endl;
		return;
	}
	
	if(height != 16)
	{
		cout << inputPath << " : sprite must be 16 pixels high" << endl;
		return;
	}
	if((width % 16) != 0)
	{
		cout << inputPath << " : sprite width must be multiple of 16 pixels" << endl;
		return;
	}
	
	vector<uint16_t> colourMasks;
	vector<uint16_t> transparencyMasks;
	
	for(unsigned x = 0; x < width; x++)
	{
		uint16_t colour = 0;
		uint16_t transparency = 0;
		
		for(unsigned y = 0; y < height; y++)
		{
			uint16_t mask = (1 << y);
			ImageColour pixelColour = CalculateColour(pixels, x, y, width);
			
			if(pixelColour == ImageColour::Black)
			{
				// Black
				transparency |= mask;
			}
			else if (pixelColour == ImageColour::White)
			{
				// White
				transparency |= mask;
				colour |= mask;
			}
		}
		
		colourMasks.push_back(colour);
		transparencyMasks.push_back(transparency);
	}
	
	unsigned int numFrames = width / 16;

	typefs << "// Generated from " << inputPath << endl;
	typefs << "constexpr uint8_t " << variableName << "_numFrames = " << dec << numFrames << ";" << endl;
	typefs << "extern const uint16_t " << variableName << "[];" << endl;

	fs << "// Generated from " << inputPath << endl;
	fs << "constexpr uint8_t " << variableName << "_numFrames = " << dec << numFrames << ";" << endl;
	fs << "extern const uint16_t " << variableName << "[] PROGMEM =" << endl;
	fs << "{" << endl << "\t";
	
	for(unsigned x = 0; x < width; x++)
	{
		// Interleaved transparency and colour
		fs << "0x" << hex << transparencyMasks[x] << ",0x" << hex << colourMasks[x];
	
		if(x != width - 1)
		{
			fs << ",";
		}
	}
	
	fs << endl;	
	fs << "};" << endl;	
}

void EncodeTextures(ofstream& typefs, ofstream& fs, const char* inputPath, const char* variableName)
{
	vector<uint8_t> pixels;
	unsigned width, height;
	unsigned error = lodepng::decode(pixels, width, height, inputPath);
	
	if(error)
	{
		cout << inputPath << " : decoder error " << error << ": " << lodepng_error_text(error) << endl;
		return;
	}
	
	if(height != 16)
	{
		cout << inputPath << " : texture must be 16 pixels high" << endl;
		return;
	}
	if((width % 16) != 0)
	{
		cout << inputPath << " : texture width must be multiple of 16 pixels" << endl;
		return;
	}
	
	vector<uint16_t> colourMasks;
	
	for(unsigned x = 0; x < width; x++)
	{
		uint16_t colour = 0;
		
		for(unsigned y = 0; y < height; y++)
		{
			uint16_t mask = (1 << y);
			ImageColour pixelColour = CalculateColour(pixels, x, y, width);
			
			if(pixelColour != ImageColour::Black)
			{
				// White
				colour |= mask;
			}
		}
		
		colourMasks.push_back(colour);
	}
	
	unsigned int numTextures = width / 16;

	typefs << "// Generated from " << inputPath << endl;
	typefs << "constexpr uint8_t " << variableName << "_numTextures = " << dec << numTextures << ";" << endl;
	typefs << "extern const uint16_t " << variableName << "[];" << endl;

	fs << "// Generated from " << inputPath << endl;
	fs << "constexpr uint8_t " << variableName << "_numTextures = " << dec << numTextures << ";" << endl;
	fs << "extern const uint16_t " << variableName << "[] PROGMEM =" << endl;
	fs << "{" << endl << "\t";
	
	for(unsigned x = 0; x < width; x++)
	{
		fs << "0x" << hex << colourMasks[x];
	
		if(x != width - 1)
		{
			fs << ",";
		}
	}
	
	fs << endl;	
	fs << "};" << endl;	
}

void EncodeHUDElement(ofstream& typefs, ofstream& fs, const char* inputPath, const char* variableName)
{
	vector<uint8_t> pixels;
	unsigned width, height;
	unsigned error = lodepng::decode(pixels, width, height, inputPath);

	if (error)
	{
		cout << inputPath << " : decoder error " << error << ": " << lodepng_error_text(error) << endl;
		return;
	}

	if (height != 8)
	{
		cout << inputPath << " : height must be 8 pixels" << endl;
		return;
	}

	vector<uint8_t> colourMasks;

	for (unsigned y = 0; y < height; y += 8)
	{
		for (unsigned x = 0; x < width; x++)
		{
			uint8_t colour = 0;

			for (unsigned z = 0; z < 8 && y + z < height; z++)
			{
				uint8_t mask = (1 << z);

				ImageColour pixelColour = CalculateColour(pixels, x, y + z, width);

				if (pixelColour == ImageColour::White)
				{
					// White
					colour |= mask;
				}
			}

			colourMasks.push_back(colour);
		}
	}

	typefs << "// Generated from " << inputPath << endl;
	typefs << "extern const uint8_t " << variableName << "[];" << endl;

	fs << "// Generated from " << inputPath << endl;
	fs << "extern const uint8_t " << variableName << "[] PROGMEM =" << endl;
	fs << "{" << endl;

	for (unsigned x = 0; x < colourMasks.size(); x++)
	{
		fs << "0x" << hex << (int)(colourMasks[x]);

		if (x != colourMasks.size() - 1)
		{
			fs << ",";
		}
	}

	fs << endl;
	fs << "};" << endl;
}

void EncodeSprite2D(ofstream& typefs, ofstream& fs, const char* inputPath, const char* variableName)
{
	vector<uint8_t> pixels;
	unsigned width, height;
	unsigned error = lodepng::decode(pixels, width, height, inputPath);
	
	if(error)
	{
		cout << inputPath << " : decoder error " << error << ": " << lodepng_error_text(error) << endl;
		return;
	}
	
	unsigned x1 = 0, y1 = 0;
	unsigned x2 = width, y2 = height;
	
	// Crop left
	bool isCropping = true;
	for(unsigned x = x1; x < x2 && isCropping; x++)
	{
		for(unsigned y = y1; y < y2; y++)
		{
			ImageColour colour = CalculateColour(pixels, x, y, width);
			
			if(colour != ImageColour::Transparent)
			{
				isCropping = false;
				break;
			}
		}
		
		if(isCropping)
		{
			x1++;
		}
	}

	// Crop right
	isCropping = true;
	for(unsigned x = x2 - 1; x >= x1 && isCropping; x--)
	{
		for(unsigned y = y1; y < y2; y++)
		{
			ImageColour colour = CalculateColour(pixels, x, y, width);
			
			if(colour != ImageColour::Transparent)
			{
				isCropping = false;
				break;
			}
		}
		
		if(isCropping)
		{
			x2--;
		}
	}

	// Crop top
	isCropping = true;
	for(unsigned y = y1; y < y2 && isCropping; y++)
	{
		for(unsigned x = x1; x < x2; x++)
		{
			ImageColour colour = CalculateColour(pixels, x, y, width);
			
			if(colour != ImageColour::Transparent)
			{
				isCropping = false;
				break;
			}
		}
		
		if(isCropping)
		{
			y1++;
		}
	}

	// Crop bottom
	isCropping = true;
	for(unsigned y = y2 - 1; y >= y1 && isCropping; y--)
	{
		for(unsigned x = x1; x < x2; x++)
		{
			ImageColour colour = CalculateColour(pixels, x, y, width);
			
			if(colour != ImageColour::Transparent)
			{
				isCropping = false;
				break;
			}
		}
		
		if(isCropping)
		{
			y2--;
		}
	}
	
	vector<uint8_t> colourMasks;
	vector<uint8_t> transparencyMasks;
	
	for(unsigned y = y1; y < y2; y += 8)
	{
		for(unsigned x = x1; x < x2; x++)
		{
			uint8_t colour = 0;
			uint8_t transparency = 0;
			
			for(unsigned z = 0; z < 8 && y + z < height; z++)
			{
				uint8_t mask = (1 << z);
				
				ImageColour pixelColour = CalculateColour(pixels, x, y + z, width);
				
				if(pixelColour == ImageColour::Black)
				{
					// Black
					transparency |= mask;
				}
				else if (pixelColour == ImageColour::White)
				{
					// White
					transparency |= mask;
					colour |= mask;
				}
			}
			
			colourMasks.push_back(colour);
			transparencyMasks.push_back(transparency);
		}
	}

	typefs << "// Generated from " << inputPath << endl;
	typefs << "extern const uint8_t " << variableName << "[];" << endl;

	fs << "// Generated from " << inputPath << endl;
	fs << "extern const uint8_t " << variableName << "[] PROGMEM =" << endl;
	fs << "{" << endl;
	fs << "\t" << dec << (x2 - x1) << ", " << dec << (y2 - y1) << "," << endl << "\t";
	
	for(unsigned x = 0; x < colourMasks.size(); x++)
	{
		fs << "0x" << hex << (int)(colourMasks[x]) << ",";
		fs << "0x" << hex << (int)(transparencyMasks[x]);
	
		if(x != colourMasks.size() - 1)
		{
			fs << ",";
		}
	}
	
	fs << endl;	
	fs << "};" << endl;	
	
/*	fs << "const uint8_t " << variableName << "[] PROGMEM =" << endl;
	fs << "{" << endl;
	fs << "\t" << dec << (x2 - x1) << ", " << dec << (y2 - y1) << "," << endl << "\t";
	
	for(unsigned x = 0; x < colourMasks.size(); x++)
	{
		fs << "0x" << hex << (int)(colourMasks[x]);
	
		if(x != colourMasks.size() - 1)
		{
			fs << ",";
		}
	}
	
	fs << endl;	
	fs << "};" << endl;	
	fs << "const uint8_t " << variableName << "_mask[] PROGMEM =" << endl;
	fs << "{" << endl << "\t";
	
	for(unsigned x = 0; x < transparencyMasks.size(); x++)
	{
		fs << "0x" << hex << (int)(transparencyMasks[x]);
	
		if(x != transparencyMasks.size() - 1)
		{
			fs << ",";
		}
	}

	fs << endl;	
	fs << "};" << endl;	
	*/
}

bool LoadPNGAsPaletted(const char* path, vector<unsigned char>& output, unsigned int& width, unsigned int& height)
{
	vector<unsigned char> image;

	unsigned error = lodepng::decode(image, width, height, path);

	if (!error)
	{
		cout << "Dimensions: " << width << ", " << height << endl;

		for (unsigned int n = 0; n < width * height; n++)
		{
			unsigned char col = matchColour(image[n * 4], image[n * 4 + 1], image[n * 4 + 2], image[n * 4 + 3]);
			output.push_back(col);
		}
		return true;
	}
	return false;
}

#if USE_GRAPHICS_MODE
void GenerateWeaponRoutine(ofstream& output, const char* routineName, const char* sourceImagePath)
{
	vector<unsigned char> palettedImage;
	unsigned int width, height;

	if (!LoadPNGAsPaletted(sourceImagePath, palettedImage, width, height))
	{
		return;
	}

	output << "void Draw" << routineName << "(backbuffer_t p, unsigned char x, unsigned char y) {" << endl;
	output << "  p += x;" << endl;
	output << "  p += " << (80 * (VIEWPORT_HEIGHT - height)) << " + (y * 80) ;" << endl;
	output << "  switch(y) {" << endl;

	int yCase = 0;

	for (int y = height - 1; y >= 0; y --)
	{
		output << "   case " << yCase << ":" << endl;
		yCase++;
		output << "    ";
		for (unsigned int x = 0; x < width; x++)
		{
			int index = ((y) * 80 + x);
			int col = palettedImage[y * width + x];

			if (col != 0xff)
			{
				int outputCol = paletteOutput[col]; // col | (col << 2) | (col << 4) | (col << 6);
				output << "p[" << index << "]=" << outputCol << ";";
			}
		}
		output << endl;
	}
	output << "   break;" << endl;
	output << "  }" << endl;
	output << endl;

	output << "}" << endl;
}
#else
void GenerateWeaponRoutine(ofstream& output, const char* routineName, const char* sourceImagePath)
{
	vector<unsigned char> palettedImage;
	unsigned int width, height;

	if (!LoadPNGAsPaletted(sourceImagePath, palettedImage, width, height))
	{
		return;
	}

	output << "void Draw" << routineName << "(backbuffer_t p, unsigned char x, unsigned char y) {" << endl;
	output << "  p += (x << 1);" << endl;
	output << "  p += " << (160 * (20 - height / 2)) << " + ((y >> 1) * 160) ;" << endl;
	output << "  switch(y) {" << endl;

	int yCase = 0;

	for (int y = height - 1; y >= 0; y -= 2)
	{
		output << "   case " << yCase << ":" << endl;
		yCase += 2;
		output << "    ";
		for (unsigned int x = 0; x < width; x++)
		{
			int index = ((y / 2) * 80 + x) * 2 + 1;
			int lower = palettedImage[y * width + x];
			int upper = palettedImage[(y - 1) * width + x];

			if (upper != 0xff && lower != 0xff)
			{
				int col = upper | (lower << 4);
				output << "p[" << index << "]=" << col << ";";
			}
			else if (upper != 0xff)
			{
				output << "p[" << index << "]=(p[" << index << "]&0xf0)|" << upper << ";";
			}
			else if (lower != 0xff)
			{
				output << "p[" << index << "]=(p[" << index << "]&0xf)|" << (lower << 4) << ";";
			}
		}
		output << endl;
	}
	output << "   break;" << endl;

	yCase = 1;

	for (int y = height - 2; y >= 0; y -= 2)
	{
		output << "   case " << yCase << ":" << endl;
		yCase += 2;
		output << "    ";
		for (unsigned int x = 0; x < width; x++)
		{
			int index = ((y / 2) * 80 + x) * 2 + 1;
			int lower = palettedImage[y * width + x];
			int upper = y > 0 ? palettedImage[(y - 1) * width + x] : 0xff;

			if (upper != 0xff && lower != 0xff)
			{
				int col = upper | (lower << 4);
				output << "p[" << index << "]=" << col << ";";
			}
			else if (upper != 0xff)
			{
				output << "p[" << index << "]=(p[" << index << "]&0xf0)|" << upper << ";";
			}
			else if (lower != 0xff)
			{
				output << "p[" << index << "]=(p[" << index << "]&0xf)|" << (lower << 4) << ";";
			}
		}
		output << endl;
	}

	output << "   break;" << endl;
	output << "  }" << endl;
	output << endl;

	output << "}" << endl;
}
#endif

#define MAX_SPRITE_HEIGHT 20

#if USE_GRAPHICS_MODE
void GenerateSpriteRoutine(ofstream& output, ofstream& typeOutput, const char* routineName, const char* sourceImagePath)
{
	vector<unsigned char> palettedImage;
	unsigned int width, height;

	if (!LoadPNGAsPaletted(sourceImagePath, palettedImage, width, height))
	{
		return;
	}

	typeOutput << "void far Draw" << routineName << "(backbuffer_t p, unsigned char x, unsigned char s);" << endl;

	output << "void far Draw" << routineName << "(backbuffer_t p, unsigned char x, unsigned char s) {" << endl;
	output << " switch(s) {" << endl;

	for (int s = 1; s < MAX_SPRITE_HEIGHT; s++)
	{
		vector<unsigned char> scaledSprRaw;
		for (int y = 0; y < s * 2; y++)
		{
			int v = (y * height) / (s * 2);
			if (v >= height)
				v = height - 1;

			for (int x = 0; x < s * 2; x++)
			{
				int u = (x * width) / (s * 2);
				if (u >= width)
					u = width - 1;
				scaledSprRaw.push_back(palettedImage[v * width + u]);
			}
		}

		vector<unsigned char> scaledSpr;
		unsigned char outlineCol = 0;

		for (int y = 0; y < s * 2; y++)
		{
			for (int x = 0; x < s * 2; x++)
			{
				unsigned char col = scaledSprRaw[y * s * 2 + x];
				if (col == 0xff)
				{
					if (x > 0 && scaledSprRaw[y * s * 2 + x - 1] != 0xff)
					{
						col = outlineCol;
					}
					else if (x < s * 2 - 1 && scaledSprRaw[y * s * 2 + x + 1] != 0xff)
					{
						col = outlineCol;
					}
					else if (y > 0 && scaledSprRaw[(y - 1) * s * 2 + x] != 0xff)
					{
						col = outlineCol;
					}
					else if (y < s * 2 - 1 && scaledSprRaw[(y + 1) * s * 2 + x] != 0xff)
					{
						col = outlineCol;
					}
				}
				else if (x == 0 || y == 0 || x == s * 2 - 1 || y == s * 2 - 1)
				{
					col = outlineCol;
				}

				scaledSpr.push_back(col);
			}
		}


		output << "  case " << s << ":" << endl;
		output << "  switch(x) {" << endl;

		for (int x = 0; x < s * 2; x++)
		{
			output << "   case " << x << ": ";

			int outAddress = (VIEWPORT_HEIGHT / 2 - s) * 80;

			for (int y = 0; y < s * 2; y++)
			{
				unsigned char col = scaledSpr[y * s * 2 + x];

				if (outAddress >= 0 && col != 0xff)
				{
					int outputCol = paletteOutput[col]; // col | (col << 2) | (col << 4) | (col << 6);
					output << "p[" << outAddress << "]=" << outputCol << ";";
				}

				outAddress += 80;

				if (outAddress >= VIEWPORT_HEIGHT * 80)
				{
					break;
				}
			}
			output << " break;" << endl;
		}

		output << "  }" << endl;
		output << "  break;" << endl;
	}

	output << " }" << endl;
	output << "}" << endl;
	output << endl;

}
#else
void GenerateSpriteRoutine(ofstream& output, ofstream& typeOutput, const char* routineName, const char* sourceImagePath)
{
	vector<unsigned char> palettedImage;
	unsigned int width, height;

	if (!LoadPNGAsPaletted(sourceImagePath, palettedImage, width, height))
	{
		return;
	}

	typeOutput << "void far Draw" << routineName << "(backbuffer_t p, unsigned char x, unsigned char s);" << endl;

	output << "void far Draw" << routineName << "(backbuffer_t p, unsigned char x, unsigned char s) {" << endl;
	output << " switch(s) {" << endl;

	for (int s = 1; s < MAX_SPRITE_HEIGHT; s++)
	{
		vector<unsigned char> scaledSprRaw;
		for (int y = 0; y < s * 2; y++)
		{
			int v = (y * height) / (s * 2);
			if (v >= height)
				v = height - 1;

			for (int x = 0; x < s * 2; x++)
			{
				int u = (x * width) / (s * 2);
				if (u >= width)
					u = width - 1;
				scaledSprRaw.push_back(palettedImage[v * width + u]);
			}
		}

		vector<unsigned char> scaledSpr;
		unsigned char outlineCol = 0;

		for (int y = 0; y < s * 2; y++)
		{
			for (int x = 0; x < s * 2; x++)
			{
				unsigned char col = scaledSprRaw[y * s * 2 + x];
				if (col == 0xff)
				{
					if (x > 0 && scaledSprRaw[y * s * 2 + x - 1] != 0xff)
					{
						col = outlineCol;
					}
					else if (x < s * 2 - 1 && scaledSprRaw[y * s * 2 + x + 1] != 0xff)
					{
						col = outlineCol;
					}
					else if (y > 0 && scaledSprRaw[(y - 1) * s * 2 + x] != 0xff)
					{
						col = outlineCol;
					}
					else if (y < s * 2 - 1 && scaledSprRaw[(y + 1) * s * 2 + x] != 0xff)
					{
						col = outlineCol;
					}
				}
				else if (x == 0 || y == 0 || x == s * 2 - 1 || y == s * 2 - 1)
				{
					col = outlineCol;
				}

				scaledSpr.push_back(col);
			}
		}


		output << "  case " << s << ":" << endl;
		output << "  switch(x) {" << endl;

		for (int x = 0; x < s * 2; x++)
		{
			output << "   case " << x << ": ";

			int outAddress = (10 - s / 2) * 160;

			for (int y = 0; y < s * 2; y++)
			{
				unsigned char upper = scaledSpr[y * s * 2 + x];
				y++;
				unsigned char lower = scaledSpr[y * s * 2 + x];

				if (outAddress >= 0)
				{
					if (upper != 0xff && lower != 0xff)
					{
						int combined = (upper | (lower << 4)) & 0xff;
						output << "p[" << outAddress << "]=" << combined << ";";
					}
					else if (upper != 0xff)
					{
						output << "p[" << outAddress << "]=(p[" << outAddress << "]&0xf0)|" << (int)upper << ";";
					}
					else if (lower != 0xff)
					{
						output << "p[" << outAddress << "]=(p[" << outAddress << "]&0x0f)|" << (int)(lower << 4) << ";";
					}
				}

				outAddress += 160;

				if (outAddress >= 20 * 160)
				{
					break;
				}
			}

			output << " break;" << endl;
		}

		output << "  }" << endl;
		output << "  break;" << endl;
	}

	output << " }" << endl;
	output << "}" << endl;
	output << endl;

}
#endif


void GenerateSinTable()
{
	FILE* fs;
	fopen_s(&fs, sinTableHeaderOutputPath, "w");

	if (!fs)
		return;

	int16_t gen_sinTable[FIXED_ANGLE_MAX];

	for (int n = 0; n < FIXED_ANGLE_MAX; n++)
	{
		gen_sinTable[n] = FLOAT_TO_FIXED(sin(FIXED_ANGLE_TO_RADIANS(n)));
	}

	fprintf(fs, "const int16_t sinTable[] = {\n\t");
	for (int n = 0; n < FIXED_ANGLE_MAX; n++)
	{
		fprintf(fs, "%d", gen_sinTable[n]);
		if (n != FIXED_ANGLE_MAX - 1)
		{
			fprintf(fs, ",");
		}
	}
	fprintf(fs, "\n};\n\n");

	fclose(fs);
}

int main(int argc, char* argv[])
{
	/*ofstream dataFile;
	ofstream typeFile;

	dataFile.open(spriteDataHeaderOutputPath);
	typeFile.open(spriteTypesHeaderOutputPath);
	
	EncodeSprite3D(typeFile, dataFile, "Images/enemy.png", "skeletonSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/mage.png", "mageSpriteData");
//	EncodeSprite3D(typeFile, dataFile, "Images/skeleton.png", "skeletonSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/torchalt1.png", "torchSpriteData1");
	EncodeSprite3D(typeFile, dataFile, "Images/torchalt2.png", "torchSpriteData2");
	EncodeSprite3D(typeFile, dataFile, "Images/fireball2.png", "projectileSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/fireball.png", "enemyProjectileSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/entrance.png", "entranceSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/exit.png", "exitSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/urn.png", "urnSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/sign.png", "signSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/crown.png", "crownSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/coins.png", "coinsSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/scroll.png", "scrollSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/chest.png", "chestSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/chestopen.png", "chestOpenSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/potion.png", "potionSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/bat.png", "batSpriteData");
	EncodeSprite3D(typeFile, dataFile, "Images/spider.png", "spiderSpriteData");

	EncodeSprite2D(typeFile, dataFile, "Images/hand1.png", "handSpriteData1");
	EncodeSprite2D(typeFile, dataFile, "Images/hand2.png", "handSpriteData2");

	EncodeTextures(typeFile, dataFile, "Images/textures.png", "wallTextureData");

	EncodeHUDElement(typeFile, dataFile, "Images/font.png", "fontPageData");
	EncodeHUDElement(typeFile, dataFile, "Images/heart.png", "heartSpriteData");
	EncodeHUDElement(typeFile, dataFile, "Images/mana.png", "manaSpriteData");

	dataFile.close();
	typeFile.close();*/

#if USE_GRAPHICS_MODE
	GeneratePalette();
#endif
	GenerateSinTable();

	ofstream wallScalerFile;
	wallScalerFile.open(wallScalerHeaderOutputPath);
	WriteWallScaler(wallScalerFile);
	wallScalerFile.close();

	ofstream drawRoutinesFile;
	drawRoutinesFile.open(drawRoutinesHeaderOutputPath);
	GenerateWeaponRoutine(drawRoutinesFile, "Hand1", "Images/hand1c.png");
	GenerateWeaponRoutine(drawRoutinesFile, "Hand2", "Images/hand2c.png");
	drawRoutinesFile.close();

	ofstream dataFile;
	ofstream typeFile;
	typeFile.open(spriteTypesHeaderOutputPath);
	dataFile.open(spriteDataHeaderOutputPath);

	typeFile << "#pragma once" << endl << endl;

	GenerateSpriteRoutine(dataFile, typeFile, "Torch1", "Images/torch1c.png");
	GenerateSpriteRoutine(dataFile, typeFile, "PlayerProjectile", "Images/fireball2c.png");
	GenerateSpriteRoutine(dataFile, typeFile, "Enemy", "Images/enemyc.png");
	dataFile.close();

	return 0;
}


