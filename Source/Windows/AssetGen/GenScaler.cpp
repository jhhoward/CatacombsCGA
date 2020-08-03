#include <iostream>
#include <fstream>
#include <vector>

#define USE_TILT_CODE 0

using namespace std;

constexpr int floorColour = 7;
constexpr int ceilingColour = 8;
constexpr int displayHeight = 40;
constexpr int horizon = 20;
constexpr int outlineColour = 0;
constexpr int displayWidth = 80;

void WriteWallScaler(ofstream& output)
{
#if USE_TILT_CODE
	output << "void far RenderWallSlice(unsigned char far* p, char h, unsigned char s, unsigned char u, unsigned char l) {" << endl;
#else
	output << "void RenderWallSlice(unsigned char far* p, unsigned char s, unsigned char u, unsigned char l) {" << endl;
#endif
	output << "  unsigned char ou = " << outlineColour << " | (u << 4);" << endl;
	output << "  unsigned char uu = u | (u << 4);" << endl;
	output << "  unsigned char uo = u | " << (outlineColour << 4) << ";" << endl;
	output << "  unsigned char ol = " << outlineColour << " | (l << 4);" << endl;
	output << "  unsigned char ll = l | (l << 4);" << endl;
	output << "  unsigned char lo = l | " << (outlineColour << 4) << ";" << endl;

	int outline = 0;
	int upper = 1;
	int lower = 2;
	int floor = 3;
	int ceiling = 4;

	// OU
	// UU
	// UO
	// OL
	// LL
	// LO

#if USE_TILT_CODE
	output << " switch(h) {" << endl;
	for (int h = -2; h <= 2; h++)
	{
		int horizon = 20 + h;

		output << "  case " << h << ":" << endl;
#endif
		output << "  switch(s) {" << endl;

		for (int s = 0; s < 64; s++)
		{
			output << "   case " << s << ": ";
			int address = 0;
			vector<int> wall;

			for (int n = 0; n < s * 2; n++)
			{
				if (n == 0 || n == s * 2 - 1)
				{
					wall.push_back(outline);
				}
				else
				{
					if (n < 2 * s / 3 || n > 4 * s / 3)
					{
						wall.push_back(upper);
					}
					else
					{
						wall.push_back(lower);
					}
				}
			}
			if (wall.size() > 0)
			{
				wall[2 * s / 3] = outline;
				wall[4 * s / 3] = outline;
			}

			for (int y = 0; y < displayHeight; y++)
			{
				output << "p[" << address << "]=";

				int first = 0, second = 0;

				if (y < horizon - s)
				{
					first = ceiling;
				}
				else if (y >= horizon + s)
				{
					first = floor;
				}
				else
				{
					int v = y - (horizon - s);
					first = wall[v];
				}

				y++;

				if (y < horizon - s)
				{
					second = ceiling;
				}
				else if (y >= horizon + s)
				{
					second = floor;
				}
				else
				{
					int v = y - (horizon - s);
					second = wall[v];
				}

				if (first == ceiling && second == floor)
				{
					output << (ceilingColour | (floorColour << 4));
				}
				else if (first == ceiling && second == ceiling)
				{
					output << (ceilingColour | (ceilingColour << 4));
				}
				else if (first == ceiling && second == outline)
				{
					output << (ceilingColour | (outlineColour << 4));
				}
				else if (first == outline && second == outline)
				{
					output << (outlineColour | (outlineColour << 4));
				}
				else if (first == outline && second == upper)
				{
					output << "ou";
				}
				else if (first == upper && second == upper)
				{
					output << "uu";
				}
				else if (first == upper && second == outline)
				{
					output << "uo";
				}
				else if (first == outline && second == lower)
				{
					output << "ol";
				}
				else if (first == lower && second == lower)
				{
					output << "ll";
				}
				else if (first == lower && second == outline)
				{
					output << "lo";
				}
				else if (first == outline && second == floor)
				{
					output << (outlineColour | (floorColour << 4));
				}
				else if (first == floor && second == floor)
				{
					output << (floorColour | (floorColour << 4));
				}
				else
				{
					printf("???\n");
				}

				output << ";";

				address += displayWidth * 2;
			}

			output << "   break;" << endl;
		}

		output << "  }" << endl;

#if USE_TILT_CODE		
		output << "  break;" << endl;
	}
	output << "}" << endl;
#endif

	output << "}" << endl;
	output << endl;
}
