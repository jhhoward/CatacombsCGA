#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "../Arduboy3D/lodepng.h"

using namespace std;

const char* spriteDataHeaderOutputPath = "Source/Arduboy3D/Generated/SpriteData.inc.h";
const char* spriteTypesHeaderOutputPath = "Source/Arduboy3D/Generated/SpriteTypes.h";

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

int main(int argc, char* argv[])
{
	ofstream dataFile;
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
	typeFile.close();
	
	return 0;
}


