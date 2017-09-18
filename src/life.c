/* C Library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
/* Ncurses */
#include <ncurses.h>
/* dmatrix */
#include <dmatrix.h>

/* Game variables */
#define TIME_STEP	0.08
#define TIME_STEP_DIFF	0.02

/* Cell states */
#define CELL_DEAD	0
#define CELL_ALIVE	1

/* Grids */
#define TMP_GRID	0
#define REAL_GRID	1
#define GRID_W		128
#define GRID_H		128

/* Colors */
#define WHITE		0
#define BLACK		1

/* Window dimensions */
#define WINDOW_W	768
#define WINDOW_H	768

/* Game state flags */
#define QUIT		0
#define PAUSE		1
#define DRAW		2

/* Macro fucntions */
#define BIT(n)		(1 << (n))

struct life {
	/* [Grid] */
	int *grid;
	int *grid_tmp;
	int grid_h, grid_w;

	/* [Simulation flags] */
	double time_step;
	double timer;
	uint8_t flags;
};

const uint8_t colors[][3] = { { 0, 0, 0 }, { 58, 168, 220 } };

int
get_cell(struct life *conway, int y, int x, int grid)
{
	y += conway->grid_h;
	x += conway->grid_w;

	if (grid) {
		return conway->grid[(y % conway->grid_h) * conway->grid_w + (x % conway->grid_w)];

	} else {
		return conway->grid_tmp[(y % conway->grid_h) * conway->grid_w + (x % conway->grid_w)];
	}
}

void
set_cell(struct life *conway, int y, int x, int state, int grid)
{
	if (grid) {
		conway->grid[(y % conway->grid_h) * conway->grid_w + (x % conway->grid_w)] = state;

	} else {
		conway->grid_tmp[(y % conway->grid_h) * conway->grid_w + (x % conway->grid_w)] = state;
	}
}

int
neighbour_cnt(struct life *conway, int y, int x)
{
	int i, j, n;

	n = 0;

	for (i = -1; i != 2; ++i) {
		for (j = -1; j != 2; ++j) {

			if (get_cell(conway, y + i, x + j, TMP_GRID) == CELL_ALIVE) {
				++n;
			}
		}
	}

	return n - get_cell(conway, y, x, TMP_GRID);
}

int
update_cell(struct life *conway, int y, int x)
{
	int nb;

	nb = neighbour_cnt(conway, y, x);
	if (get_cell(conway, y, x, TMP_GRID)) {
		return nb < 2 || nb > 3 ? CELL_DEAD : CELL_ALIVE;

	} else {
		return nb == 3 ? CELL_ALIVE : CELL_DEAD;
	}
}

void
time_tick(struct life *conway)
{
	int i, j;

	memcpy(conway->grid_tmp, conway->grid, conway->grid_h * conway->grid_w * sizeof(int));

	for (i = 0; i != conway->grid_h; ++i) {
		for (j = 0; j != conway->grid_w; ++j) {
			set_cell(conway, i, j, update_cell(conway, i, j), REAL_GRID);
		}
	}
}

void
empty_grid(struct life *conway)
{
	int i;

	for (i = 0; i != conway->grid_h * conway->grid_w; ++i) {
		conway->grid[i] = 0;
	}
}

void
randomize_grid(struct life *conway)
{
	int i;

	for (i = 0; i != conway->grid_h * conway->grid_w; ++i) {
		conway->grid[i] = rand() % 2;
	}
}

void
init_life(struct life *conway)
{
	conway->grid = malloc(conway->grid_h * conway->grid_w * sizeof(int));
	conway->grid_tmp = malloc(conway->grid_h * conway->grid_w * sizeof(int));
	conway->flags |= BIT(DRAW);
	conway->timer = glfwGetTime();
	conway->time_step = TIME_STEP;

	randomize_grid(conway);
}

void
key_callback(GLFWwindow *win, int key, int scancode, int action, int mods)
{
	struct life *conway;

	conway = (struct life *) glfwGetWindowUserPointer(win);

	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
		case GLFW_KEY_Q:
			conway->flags |= BIT(QUIT);
			break;

		case GLFW_KEY_SPACE:
			conway->flags ^= BIT(PAUSE);
			break;

		case GLFW_KEY_R:
			randomize_grid(conway);
			break;

		case GLFW_KEY_E:
			empty_grid(conway);
			break;

		case GLFW_KEY_UP:
		case GLFW_KEY_K:
			conway->time_step -= TIME_STEP_DIFF;
			break;

		case GLFW_KEY_DOWN:
		case GLFW_KEY_J:
			conway->time_step += TIME_STEP_DIFF;
			break;
		}
	}
}

int
main(int argc, char **argv)
{
	struct life conway;
	struct dot_matrix dm;
	uint8_t scr_buf[GRID_H][GRID_W][3];
	double x, y;
	int i, j, ix, iy;

	srand(time(NULL));

	/* Create window */
	memset(scr_buf[0][0], 2, sizeof scr_buf);
	dm.scr_buf_h = GRID_H;
	dm.scr_buf_w = GRID_W;
	dm.scr_buf = scr_buf[0][0];
	create_matrix(&dm, WINDOW_W, WINDOW_H);

	/* Some GLFW stuff */
	glfwSetWindowUserPointer(dm.win, &conway);
	glfwSetKeyCallback(dm.win, key_callback);
	glfwSwapInterval(1);

	/* Initialize game of struct life */
	memset(&conway, 0, sizeof(conway));
	conway.grid_h = GRID_H;
	conway.grid_w = GRID_W;
	init_life(&conway);

	while (!glfwWindowShouldClose(dm.win) && !(conway.flags & BIT(QUIT))) {
		glfwPollEvents();

		/* Handle mouse input */
		if (glfwGetMouseButton(dm.win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			conway.flags |= BIT(DRAW);
			
			glfwGetCursorPos(dm.win, &x, &y);
			ix = (x / (double)WINDOW_W) * GRID_W;
			iy = (y / (double)WINDOW_H) * GRID_H;
			
			conway.grid[iy * GRID_W + ix] = 1;

		} else if (glfwGetMouseButton(dm.win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
			conway.flags |= BIT(DRAW);
			
			glfwGetCursorPos(dm.win, &x, &y);
			ix = (x / (double)WINDOW_W) * GRID_W;
			iy = (y / (double)WINDOW_H) * GRID_H;
			
			conway.grid[iy * GRID_W + ix] = 0;
		}

		/* Update if not paused */
		if (!(conway.flags & BIT(PAUSE)) &&
		    ((glfwGetTime() - conway.timer) >= conway.time_step)) {
			time_tick(&conway);

			conway.timer = glfwGetTime();
			conway.flags |= BIT(DRAW);
		}

		/* Update screen buffer only if necessary */
		if (conway.flags & BIT(DRAW)) {
			for (i = 0; i != GRID_H; ++i) {
				for (j = 0; j != GRID_W; ++j) {
					/*
					scr_buf[i][j][1] = 255 * conway.grid[i * GRID_W + j];
					*/
					memcpy(scr_buf[i][j], &colors[conway.grid[i * GRID_W + j]], sizeof (uint8_t) * 3);
				}
			}
			
			update_matrix(&dm);
		}

		/* Draw */
		render_matrix(&dm);
		glfwSwapBuffers(dm.win);
	}

	free_matrix(&dm);

	return 0;
}

