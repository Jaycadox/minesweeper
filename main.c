#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#define SUPPORT_FILEFORMAT_BMP      1
#include <raylib.h>

#define GAME_WIDTH 480
#define GAME_HEIGHT 530
#define TITLE_HEIGHT 50

#define BOARD_SIZE 16
static unsigned char MINE_COUNT = 40;

static unsigned char TILE_PNG[] = {
#include "./assets/tile_c.h"
};

static unsigned char BLANK_TILE_PNG[] = {
#include "./assets/blank_tile_c.h"
};

static unsigned char FLAG_TILE_PNG[] = {
#include "./assets/flag_tile_c.h"
};

static unsigned char MINE_TILE_PNG[] = {
#include "./assets/mine_c.h"
};

#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))

enum tile_type {
	TILE_TYPE_CLOSE,
	TILE_TYPE_OPEN,
	TILE_TYPE_FLAG,
};

struct tile {
	enum tile_type type;
	bool mine;
	unsigned char nearby_mines;
};

struct game {
	struct tile tiles[BOARD_SIZE * BOARD_SIZE];
	bool mines_generated;
	unsigned short num_flags;
	bool game_over;
	bool won;
	double time_start;
	double time_end;
};

static struct game GAME = {0};

void draw_menu (Texture2D* flag);
void draw_grid (void);
void handle_inputs (void);
void reset_game (void);

void draw_tiles (Texture2D* tile, Texture2D* blank_tile, Texture2D* flag_tile, Texture2D* mine_tile);
int main (void) {
	// Perform some initial housework
	SetRandomSeed(time(NULL));
	//SetTraceLogLevel(LOG_WARNING);
	InitWindow(GAME_WIDTH, GAME_HEIGHT, "Minesweeper");
	SetTargetFPS(24);
	reset_game();

	Image tile_img = LoadImageFromMemory(".png", TILE_PNG, ARRAYSIZE(TILE_PNG));
	Texture2D tile = LoadTextureFromImage(tile_img);

	Image blank_tile_img = LoadImageFromMemory(".png", BLANK_TILE_PNG, ARRAYSIZE(BLANK_TILE_PNG));
	Texture2D blank_tile = LoadTextureFromImage(blank_tile_img);

	Image flag_tile_img = LoadImageFromMemory(".png", FLAG_TILE_PNG, ARRAYSIZE(FLAG_TILE_PNG));
	Texture2D flag_tile = LoadTextureFromImage(flag_tile_img);

	Image mine_tile_img = LoadImageFromMemory(".png", MINE_TILE_PNG, ARRAYSIZE(MINE_TILE_PNG));
	Texture2D mine_tile = LoadTextureFromImage(mine_tile_img);

	while (!WindowShouldClose()) {
		double time = GetTime();
		if (!GAME.mines_generated) {
			GAME.time_start = time;
			GAME.time_end = time;	
		} else if (!GAME.game_over) {
			GAME.time_end = time;
		}

		BeginDrawing();
		ClearBackground(BLACK);

		handle_inputs();

		draw_menu(&flag_tile);
		draw_tiles(&tile, &blank_tile, &flag_tile, &mine_tile);

		EndDrawing();
	}

	CloseWindow();
}

void reset_game (void) {
	GAME = (struct game) {0};
	GAME.num_flags = MINE_COUNT;
}

void generate_mines (void) {
	for (unsigned char i = 0; i < MINE_COUNT; ++i) {
		while (1) {
			unsigned char idx = GetRandomValue(0, 255);
			if (GAME.tiles[idx].type == TILE_TYPE_CLOSE && !GAME.tiles[idx].mine) {
				GAME.tiles[idx].mine = true;
				break;
			}
		}
	}

	// Compute number of nearby mines for each empty cell
	for (unsigned char x = 0; x < BOARD_SIZE; ++x) {
		for (unsigned char y = 0; y < BOARD_SIZE; ++y) {
			unsigned char idx = (y * BOARD_SIZE) + x;

			signed char start_x_off = -1;
			signed char end_x_off = 1;

			signed char start_y_off = -1;
			signed char end_y_off = 1;

			if (y == 0) {
				start_y_off = 0;
			} else if (y == BOARD_SIZE - 1) {
				end_y_off = 0;
			}

			if (x == 0) {
				start_x_off = 0;
			} else if (x == BOARD_SIZE - 1) {
				end_x_off = 0;
			}

			unsigned char num_mines = 0;

			for (signed char x_off = start_x_off; x_off <= end_x_off; ++x_off) {
				for (signed char y_off = start_y_off; y_off <= end_y_off; ++y_off) {
					unsigned char off_idx = (y + y_off) * BOARD_SIZE + (x + x_off);
					if(GAME.tiles[off_idx].mine) {
						++num_mines;
					}
				}
			}
			GAME.tiles[idx].nearby_mines = num_mines;			

		}
	}


	GAME.mines_generated = true;
}

void flood_fill (unsigned char idx) {
	struct tile* t = &GAME.tiles[idx];
	if (t->type == TILE_TYPE_CLOSE) {
		t->type = TILE_TYPE_OPEN;
	}
	
	bool requires_zero_nearby = t->nearby_mines != 0;

	// Look up, if possible
	if (idx >= BOARD_SIZE) {
		struct tile* tt = &GAME.tiles[idx - BOARD_SIZE];
		if ((!requires_zero_nearby || tt->nearby_mines == 0) && tt->type == TILE_TYPE_CLOSE) {
			flood_fill(idx - BOARD_SIZE);
		}
	}
	// Look down, if possible
	if (idx / BOARD_SIZE != BOARD_SIZE - 1) {
		struct tile* tt = &GAME.tiles[idx + BOARD_SIZE];
		if ((!requires_zero_nearby || tt->nearby_mines == 0) && tt->type == TILE_TYPE_CLOSE) {
			flood_fill(idx + BOARD_SIZE);
		}
	}

	// Look left, if possible
	if (idx % BOARD_SIZE != 0) {
		struct tile* tt = &GAME.tiles[idx - 1];
		if ((!requires_zero_nearby || tt->nearby_mines == 0) && tt->type == TILE_TYPE_CLOSE) {
			flood_fill(idx - 1);
		}
	}
		
	// Look right, if possible
	if (idx % BOARD_SIZE != (BOARD_SIZE - 1)) {
		struct tile* tt = &GAME.tiles[idx + 1];
		if ((!requires_zero_nearby || tt->nearby_mines == 0) && tt->type == TILE_TYPE_CLOSE) {
			flood_fill(idx + 1);
		}
	}
}

void handle_inputs (void) {
	if (IsKeyPressed(KEY_R)) {
		reset_game();		
	}

	if (!GAME.mines_generated) {
		if (IsKeyPressed(KEY_UP) && MINE_COUNT < 100) {
			++MINE_COUNT;	
			GAME.num_flags = MINE_COUNT;
		}
		if (IsKeyPressed(KEY_DOWN) && MINE_COUNT > 1) {
			--MINE_COUNT;	
			GAME.num_flags = MINE_COUNT;
		}
	}

	if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) && !GAME.game_over) {
		enum tile_type type = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ? TILE_TYPE_OPEN : TILE_TYPE_FLAG;
		if (type == TILE_TYPE_FLAG && GAME.num_flags == 0) {
			return;
		}

		// Get row and col of mouse
		Vector2 pos = GetMousePosition();
		if (pos.y <= TITLE_HEIGHT) {
			// Mouse is in title bar, not over the grid
			return;
		}
		pos.y -= TITLE_HEIGHT; // Normalize mouse pos so the grid starts at 0, 0

		unsigned char x = pos.x / (GAME_WIDTH / BOARD_SIZE);
		unsigned char y = pos.y / (GAME_WIDTH / BOARD_SIZE);

		unsigned char idx = y * BOARD_SIZE + x;
		if (!(GAME.tiles[idx].type == TILE_TYPE_OPEN && type == TILE_TYPE_FLAG)) {
			if (type == TILE_TYPE_FLAG && GAME.tiles[idx].type != TILE_TYPE_FLAG) {
				--GAME.num_flags;
			} else if (type == TILE_TYPE_OPEN && GAME.tiles[idx].type == TILE_TYPE_FLAG) {
				++GAME.num_flags;
			}

			if (type == TILE_TYPE_OPEN && GAME.tiles[idx].mine) {
				// Game lost
				GAME.game_over = true;
				for (unsigned short i = 0; i < (BOARD_SIZE*BOARD_SIZE); ++i) {
					if (GAME.tiles[i].mine) {
						GAME.tiles[i].type = TILE_TYPE_OPEN;
					}
				}
			}
			GAME.tiles[idx].type = type;

			// Check to see if player won, zero remaining flags, with all flags on mines
			if (GAME.num_flags == 0 && type == TILE_TYPE_FLAG) {
				GAME.won = true;
				GAME.game_over = true;
				for (unsigned short i = 0; i < (BOARD_SIZE*BOARD_SIZE); ++i) {
					if (GAME.tiles[i].mine && GAME.tiles[i].type != TILE_TYPE_FLAG) {
						GAME.won = false;
						GAME.game_over = false;
						printf("%i\n", i);
						break;
					}
				}
			}

			if (!GAME.mines_generated) {
				generate_mines();
			}
			if (type == TILE_TYPE_OPEN) {
				flood_fill(idx);
			}
		}

	}
}

void draw_menu (Texture2D* flag) {
	DrawRectangle(0, 0, GAME_WIDTH, TITLE_HEIGHT, DARKGRAY);
	char buff[12] = {0};
	snprintf(buff, sizeof(buff), "%i", GAME.num_flags);

	DrawRectangle(5 + 2, 5 + 2, 30, 30, BLACK);
	DrawTextureEx(*flag, (Vector2){5, 5}, 0.0f, 2.0f, WHITE);
	DrawText(buff, (GAME_WIDTH / BOARD_SIZE) + 12 + 2, 12 + 2, 20, BLACK);
	DrawText(buff, (GAME_WIDTH / BOARD_SIZE) + 12, 12, 20, WHITE);
	
	const char* reset_text = GAME.mines_generated ? "'R' to reset." : "by jayphen";

	DrawText(reset_text, 5 + 2, 36 + 2, 15, GAME.game_over ? ColorAlpha(GAME.won ? BLACK : BLACK, 0.2f * sinf(GetTime() * 20.0f) + 0.8f) : BLACK);
	DrawText(reset_text, 5, 36, 15, GAME.game_over ? ColorAlpha(GAME.won ? GREEN : YELLOW, 0.2f * sinf(GetTime() * 20.0f) + 0.8f) : WHITE);

	DrawRectangle(GAME_WIDTH / 4 - 8 + 2, 6 + 2, 90, 40, BLACK);
	DrawRectangle(GAME_WIDTH / 4 - 8, 6, 90, 40, GetColor(0x181818FF));

	unsigned int total_seconds_played = (unsigned int)(GAME.time_end - GAME.time_start);
	unsigned int minutes = total_seconds_played / 60;
	unsigned int seconds_in_min = total_seconds_played % 60;
	snprintf(buff, sizeof(buff), "%02i:%02i", minutes, seconds_in_min);
	int length = (90 - MeasureText(buff, 30)) / 2;
	DrawText(buff, GAME_WIDTH / 4 + length - 7, 12, 30, RED);

	DrawText("MINESWEEPER", GAME_WIDTH / 2 + 2, 12 + 2, 30, BLACK);
	DrawText("MINESWEEPER", GAME_WIDTH / 2, 12, 30, WHITE);
}

void draw_tiles (Texture2D* tile, Texture2D* blank_tile, Texture2D* flag_tile, Texture2D* mine_tile) {
	for (unsigned char x = 0; x < BOARD_SIZE; ++x) {
		unsigned int x_offset = (GAME_WIDTH / BOARD_SIZE) * x;
		for (unsigned char y = 0; y < BOARD_SIZE; ++y) {
			unsigned int y_offset = TITLE_HEIGHT + (GAME_WIDTH / BOARD_SIZE) * y;
			unsigned char idx = (y * BOARD_SIZE) + x;
			struct tile* t = &GAME.tiles[idx];
			Texture2D* tex = NULL;

			bool should_draw_text = false;
			switch(t->type) {
				case TILE_TYPE_OPEN:
					tex = t->mine ? mine_tile : blank_tile;
					should_draw_text = !t->mine;
					break;
				case TILE_TYPE_CLOSE:
					tex = tile;
					break;
				case TILE_TYPE_FLAG:
					tex = flag_tile;
					break;
			}
			DrawTextureEx(*tex, (Vector2){x_offset, y_offset}, 0.0f, 2.0f, WHITE);
			if (should_draw_text && t->nearby_mines != 0) {
				char buff[4] = {0};
				snprintf(buff, sizeof(buff), "%i", t->nearby_mines);
				unsigned char text_offset = 8;
				if (t->nearby_mines == 1) {
					text_offset = 12;
				}
				Color col = WHITE;
				switch (t->nearby_mines) {
					case 1: col = BLUE; break;
					case 2: col = GREEN; break;
					case 3: col = RED; break;
					case 4: col = DARKBLUE; break;
					case 5: col = VIOLET; break;
				}

				DrawText(buff, x_offset + text_offset + 2, y_offset + 1 + 2, 30, BLACK);
				DrawText(buff, x_offset + text_offset, y_offset + 1, 30, col);
			}

		}
	}

}

