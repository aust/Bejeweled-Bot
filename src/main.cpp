/* Bejeweled Bot
 * Author: Austin Burrow
 * Version: 1.0
 * Description:
 *	This Bejeweled bot is a HIGHLY modified version of the bot
 *  located at http://andrewroycarter.com/c/bejeweled-bot-written-in-c-win32-api/
 *  and redd.it/et87y
 *  It's basically rewritten from the ground up, including new features
 *  such as heuristics, screen capturing, time limits, advanced
 *  debugging, cell count, cell size, etc.
 */

#include <windows.h>
#include <ctime>
#include <cstdio>

COLORREF GetPixelRGB(int x, int y);

// Initiazes game
void InitGame();

// Deinits game
void ClearGame();

// Outputs debugging information into an HTML file for viewing
void DisplayGrid();

// Updates game state
void GetGems();

// Makes a move based on game state
void MakeMove();

// Copies grid to grid_temp
void CopyGrid();

// Simulates a left mouse click at its current position
void MouseLeftClick();

// Checks if there is a move available
bool CheckMatch(int gemx, int gemy, int newx, int newy);

// Moves gem at position gemx x gemy, to newx x newy
void MoveGem(int gemx, int gemy, int newx, int newy);

// Takes a new screenshot
void Screenshot();

// Displays a count down timer to the console
void CountDown(time_t seconds);

// Checks a color based on its RGB values separately, and
// allows for a given error threshold.
bool ColorCheck(COLORREF c1, COLORREF c2);

// *****************************************
// Configuration Start
// *****************************************

// Debug file. Used for dumping output HTML
const char* debug_path = "C:\\Users\\Austin\\Desktop\\out.html";

// Amount of time the game lasts (seconds)
const time_t time_limit = 60; 

// Time in milliseconds to "sleep" after each turn
// Honestly it's safe to keep it 0.
const int throttle_time = 100;  

const int cell_count = 8;

// Size of each gem cell.
const int cell_size = 40;

// Size of the box that we're going to take the average. MUST be
// less than cell_size
const int box_size = 2;

// Error threshold. This is how "picky" the bot is according
// to color value differences. The higher the number means the
// less picky it is. 15-30 should be good enough.
const int error_threshold = 15; 

// *****************************************
// Configuration End
// *****************************************

// Current error threshold. Changes when the bot is not able to
// find a solution at the current threshold. Changes back after
// every turn.
int current_error_threshold = error_threshold; 
int current_box_size = box_size;

// Current center of the gem, this is where we base
// our pixel coordinates of each cell
int current_center = (cell_size / 2) - (box_size / 2);

bool gem_matched = false;

COLORREF grid[cell_count][cell_count];
COLORREF temp_grid[cell_count][cell_count];

// Holds information about the starting cursor position
POINT game_origin = { 0 };

int screen_width = 0;
int screen_height = 0;
int screenshot_size = 0;
char* screenshot_buffer = 0;

int main()
{
	// Display a few instructions
	printf("Instructions:\nWhen ready, hit enter. You will have five seconds "
		"to place your mouse cursor in the upper left hand corner of the game board.\n"
		"\nTips: I would hit enter, then wait a few seconds and then click play. It might"
		" take a game or two to get your timing right.\n");
	system("pause");

	while (true)
	{
		CountDown(5);
		InitGame();

		// Play game for specified time_limit
		for (time_t start_time = time(0);
			start_time + time_limit - time(0) >= 0;)
		{
			GetGems();
			DisplayGrid();
			MakeMove();
			Sleep(throttle_time);
		}

		ClearGame();

		printf("Game over. . . Play again?\n");
		system("Pause");
	}
	
	return 0;
}

void CountDown(time_t seconds)
{
	printf("Starting in ");
	for (int i = 0; i < seconds; i++)
	{
		printf("%d ", seconds - i);
		Sleep(1000L);
	}
	printf("\n");
}

void DisplayGrid()
{
	FILE* f;
	
	// Attempt to open file for debugging
	if (fopen_s(&f, debug_path, "w"))
	{
		printf("Couldn't open the file located at debug_path!\n");
		return;
	}

	// Display average color table for RED
	fprintf(f, "<meta http-equiv=\"refresh\" content=\"1\"><h2>Average Red Colors</h2>"
		"<table cellpadding=0 cellspacing=0>");
	for (int i = 0; i < cell_count; i++)
	{
		fprintf(f, "<tr>");
		for (int j = 0; j < cell_count; j++)
		{
			COLORREF color = grid[i][j];
			fprintf(f, "  <td bgcolor=\"#%02X0000\" width=15 height=15></td>\n",
				GetRValue(color));
		}
		fprintf(f, "</tr>\n");
	}
	fprintf(f, "</table>");

	// Display average color table for GREEN
	fprintf(f, "<h2>Average Green Colors</h2>"
		"<table cellpadding=0 cellspacing=0>");
	for (int i = 0; i < cell_count; i++)
	{
		fprintf(f, "<tr>");
		for (int j = 0; j < cell_count; j++)
		{
			COLORREF color = grid[i][j];
			fprintf(f, "  <td bgcolor=\"#00%02X00\" width=15 height=15></td>\n",
				GetGValue(color));
		}
		fprintf(f, "</tr>\n");
	}
	fprintf(f, "</table>");

	// Display average color table for BLUE
	fprintf(f, "<h2>Average Blue Colors</h2>"
		"<table cellpadding=0 cellspacing=0>");
	for (int i = 0; i < cell_count; i++)
	{
		fprintf(f, "<tr>");
		for (int j = 0; j < cell_count; j++)
		{
			COLORREF color = grid[i][j];
			fprintf(f, "  <td bgcolor=\"#0000%02X\" width=15 height=15></td>\n",
				GetBValue(color));
		}
		fprintf(f, "</tr>\n");
	}
	fprintf(f, "</table>");

	// Display Heuristic Table
	fprintf(f, "<h2>Heuristic Table</h2>"
		"<table cellpadding=0 cellspacing=0>");
	for (int y = 0; y < cell_count; y++)
	{
		fprintf(f, "<tr>");
		for (int x = 0; x < cell_count; x++)
		{
			fprintf(f, "<td><table cellpadding=0 cellspacing=0>");
			for (int py = 0; py < box_size; py++)
			{
				fprintf(f, "<tr>");
				for (int px = 0; px < box_size; px++)
				{
					COLORREF color = GetPixelRGB(game_origin.x + (x * 40) + 15 + px,
						(game_origin.y + (y * 40) + 15 + py));
					fprintf(f, "<td bgcolor=\"#%02X%02X%02X\" width=1 height=1></td>",
						GetRValue(color), GetGValue(color), GetBValue(color));
				}
				fprintf(f, "</tr>");
			}
			fprintf(f, "</table></td>");
		}
		fprintf(f, "</tr>");
	}
	fprintf(f, "</table>");
	fclose(f);
}

void CopyGrid()
{
	// Copies main grid to a temporary grid for checking moves
	memcpy(temp_grid, grid, sizeof(temp_grid));
}

void MakeMove()
{
	int x;
	int y;

	// Reset heuristic
	if (gem_matched)
	{
		current_error_threshold = error_threshold;
		gem_matched = false;
	}

	for (y = 0; y < cell_count; y++)
	{
		for (x = 0; x < cell_count; x++)
		{
			// Try move left
			if (x - 1 >= 0)
			{
				CopyGrid();
				if (CheckMatch(x, y, x - 1, y))
				{
					MoveGem(x, y, x - 1, y);
					gem_matched = true;
					break;
				}
			}
			
			// Try move right
			if (x + 1 < cell_count)
			{
				CopyGrid();
				if (CheckMatch(x,y,x+1,y))
				{
					MoveGem(x, y, x + 1, y);
					gem_matched = true;
					break;
				}

			}
			// Try move up
			if (y - 1 >= 0)
			{
				CopyGrid();
				if (CheckMatch(x, y, x, y - 1))
				{
					MoveGem(x, y, x, y - 1);
					gem_matched = true;
					break;
				}

			}
			
			// Try move down
			if (y + 1 < cell_count)
			{
				CopyGrid();
				if (CheckMatch(x, y, x, y + 1))
				{
					MoveGem(x, y, x, y + 1);
					gem_matched = true;
					break;
				}
			}
		}

		if (gem_matched)
			break;
	}

	// If we didn't find a match, we'll need to make
	// our heuristic less picky. Increment threshold
	if (!gem_matched && current_error_threshold < 255)
	{
		// Before taking screenshot we'll want to reset the cursor
		// position to prevent hover animations of gems. This
		// could possibly help in the detection of matches
		SetCursorPos(game_origin.x, game_origin.y);

		current_error_threshold += 5;
		if (current_error_threshold >= 255)
		{
			printf("Need some help here.\n");
			current_error_threshold = 255;
		}
	}
}

void MoveGem(int gemx, int gemy, int newx, int newy)
{
	//moves mouse to moveable gem, clicks it, then clicks legal move
	SetCursorPos(game_origin.x + ((gemx * 40) + 20),
		game_origin.y + ((gemy * 40) + 20));
	MouseLeftClick();
	SetCursorPos(game_origin.x + ((newx * 40) + 20),
		game_origin.y + ((newy * 40) + 20));
	MouseLeftClick();
}

bool ColorCheck(COLORREF c1, COLORREF c2)
{
	bool red_match = false;
	bool blue_match = false;
	bool green_match = false;

	int c1_red = GetRValue(c1);
	int c2_red = GetRValue(c2);
	int c1_green = GetGValue(c1);
	int c2_green = GetGValue(c2);
	int c1_blue = GetBValue(c1);
	int c2_blue = GetBValue(c2);

	if (c2_red < c1_red + error_threshold && c2_red > c1_red - error_threshold)
		red_match = true;

	if (c2_green < c1_green + error_threshold && c2_green > c1_green - error_threshold)
		green_match = true;

	if (c2_blue < c1_blue + error_threshold && c2_blue > c1_blue - error_threshold)
		blue_match = true;

	return red_match && blue_match && green_match;
}

bool CheckMatch(int gemx, int gemy, int newx, int newy)
{
	// Gets color of gem to look of matches
	COLORREF c = temp_grid[gemy][gemx];
	temp_grid[gemy][gemx] = temp_grid[newy][newx];
	temp_grid[newy][newx] = c;

	int i;
	int matched = 0;

	// Column match
	for (i = 0; i < cell_count && matched < 3; i++)
	{
		if (ColorCheck(temp_grid[i][newx], c))
			matched++;
		else
			matched = 0;
	}

	if (matched >= 3)
		return true;

	// Row match
	for (i = 0, matched = 0; i <= cell_count && matched < 3; i++)
	{

		if (ColorCheck(temp_grid[newy][i], c))
			matched++;
		else
			matched = 0;
	}

	if (matched >= 3)
		return true;

	return false;
}

// Averages each cell's color
void GetGems()
{
	int y, py;
	int x, px;

	COLORREF color;

	int r_avg = 0;
	int g_avg = 0;
	int b_avg = 0;

	Screenshot();

	for (y = 0; y < cell_count; y++)
	{
		for (x = 0; x < cell_count; x++)
		{
			// Reset averages for cell
			r_avg = 0;
			g_avg = 0;
			b_avg = 0;

			for (py = 0; py < box_size; py++)
			{
				for (px = 0; px < box_size; px++)
				{
					// Get pixel by calculating it relative to the game_origin
					// (game_origin) then the current cell_size (x * 40) then by the
					// current_center of the cell, finally by the position
					color = GetPixelRGB(game_origin.x + (x * 40) + current_center + px,
						(game_origin.y + (y * 40) + current_center + py));

					// Add up R, G, B separately
					r_avg += GetRValue(color);
					g_avg += GetGValue(color);
					b_avg += GetBValue(color);
				}
			}

			// Calculate averages, then construct RGB COLORREF
			grid[y][x] = RGB(r_avg / box_size * box_size,
				g_avg / box_size * box_size,
				b_avg / box_size * box_size);
		}
	}

}

void InitGame()
{
	// Get current cursor position
	GetCursorPos(&game_origin);

	// Current error threshold. Changes when the bot is not able to
	// find a solution at the current threshold. Changes back after
	// every turn.
	current_error_threshold = error_threshold; 
	current_box_size		= box_size;
	current_center          = (cell_size / 2) - (box_size / 2);

	gem_matched = false;

	// Get screen metrics
	screen_width = GetSystemMetrics(SM_CXSCREEN);
	screen_height = GetSystemMetrics(SM_CYSCREEN);

	// Initialize screenshot bitmap buffer
	screenshot_size = screen_width * screen_height * 4;
	screenshot_buffer = new char[screenshot_size];
}

void ClearGame()
{
	// Zero out memory
	ZeroMemory(&game_origin, sizeof(POINT));
	ZeroMemory(grid, sizeof(grid));
	ZeroMemory(temp_grid, sizeof(temp_grid));

	screen_width = 0;
	screen_height = 0;

	// Cleanup bitmap buffer
	screenshot_size = 0;
	delete[] screenshot_buffer;
	screenshot_buffer = 0;
}

COLORREF GetPixelRGB(int x, int y)
{
	// Bitmap is laid out into a 1D array UPSIDE DOWN (screen_height - y) gives us
	// the flipped image, which is what we need
	// (screen_height - y - 1) * Width * 4 Gives us the row
	// X * 4 gives us the specific column
	// Note that inside memory, the bitmap is laid out as 0x00rrggbb
	// We need to flip it around
	int row = ((screen_height - y - 1) * screen_width * 4);
	int col = (x * 4);
	COLORREF bgr = *((int*)(&screenshot_buffer[row + col]));
	return RGB(GetBValue(bgr), GetGValue(bgr), GetRValue(bgr));
}

// SendInput: http://msdn.microsoft.com/en-us/library/ms646310(VS.85).aspx
// INPUT: http://msdn.microsoft.com/en-us/library/ms646270(v=VS.85).aspx
void MouseLeftClick()
{
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.mouseData = 0;
	input.mi.dx =  0;
	input.mi.dy = 0;
	input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	SendInput(1, &input, sizeof(input));

	input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput(1, &input, sizeof(input));
}

// Modified from http://msdn.microsoft.com/en-us/library/dd183402(v=vs.85).aspx
void Screenshot()
{
	HWND hWnd = GetDesktopWindow();
	HDC hdcScreen;
    HDC hdcWindow;
    HDC hdcMemDC = 0;
    HBITMAP hbmScreen = 0;
    BITMAP bmpScreen;

    // Retrieve the handle to a display device context for the client 
    // area of the window. 
    hdcScreen = GetDC(0);
    hdcWindow = GetDC(hWnd);

    // Create a compatible DC which is used in a BitBlt from the window DC
    hdcMemDC = CreateCompatibleDC(hdcWindow); 

    if (!hdcMemDC)
    {
        MessageBox(hWnd, "CreateCompatibleDC has failed", "Failed", MB_OK);
        goto done;
    }

    // Get the client area for size calculation
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);

    //This is the best stretch mode
    SetStretchBltMode(hdcWindow,HALFTONE);

    //The source DC is the entire screen and the destination DC is the current window (HWND)
    if (!StretchBlt(hdcWindow, 0, 0, rcClient.right, rcClient.bottom, 
		hdcScreen, 0, 0, GetSystemMetrics (SM_CXSCREEN),
		GetSystemMetrics (SM_CYSCREEN), SRCCOPY))
    {
        MessageBox(hWnd,  "StretchBlt has failed", "Failed", MB_OK);
        goto done;
    }
    
    // Create a compatible screenshot_buffer from the Window DC
    hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);
    
    if (!hbmScreen)
    {
        MessageBox(hWnd, "CreateCompatibleBitmap Failed", "Failed", MB_OK);
        goto done;
    }

    // Select the compatible screenshot_buffer into the compatible memory DC.
    SelectObject(hdcMemDC,hbmScreen);
    
    // Bit block transfer into our compatible memory DC.
    if (!BitBlt(hdcMemDC, 0, 0, rcClient.right - rcClient.left,
		rcClient.bottom - rcClient.top, hdcWindow, 0, 0, SRCCOPY))
    {
        MessageBox(hWnd, "BitBlt has failed", "Failed", MB_OK);
        goto done;
    }

    // Get the BITMAP from the HBITMAP
    GetObject(hbmScreen,sizeof(BITMAP),&bmpScreen);
   
    BITMAPINFOHEADER bi;
     
    bi.biSize = sizeof(BITMAPINFOHEADER);    
    bi.biWidth = bmpScreen.bmWidth;    
    bi.biHeight = bmpScreen.bmHeight;  
    bi.biPlanes = 1;    
    bi.biBitCount = 32;    
    bi.biCompression = BI_RGB;    
    bi.biSizeImage = 0;  
    bi.biXPelsPerMeter = 0;    
    bi.biYPelsPerMeter = 0;    
    bi.biClrUsed = 0;    
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

    // Gets the "bits" from the screenshot_buffer and copies them into a buffer 
    // which is pointed to by lpbitmap.
    GetDIBits(hdcWindow, hbmScreen, 0,
        (UINT)bmpScreen.bmHeight,
        screenshot_buffer,
        (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    //Clean up
done:
    DeleteObject(hbmScreen);
    DeleteObject(hdcMemDC);
    ReleaseDC(0,hdcScreen);
    ReleaseDC(hWnd,hdcWindow);
}