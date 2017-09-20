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
#define TIME_STEP	0.04
#define TIME_STEP_DIFF	0.02

/* Grid */
#define TMP_GRID	0
#define REAL_GRID	1
#define GRID_W		320
#define GRID_H		180
#define MAX_CELL_TYPES	3

/* Window dimensions */
#define WINDOW_W	1280
#define WINDOW_H	720

/* Game state flags */
#define QUIT		0
#define PAUSE		1
#define DRAW		2
#define BORDER		3

/* Macro fucntions */
#define BIT(n)		(1 << (n))
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

struct life {
	/* [Grid] */
	int *grid;
	int *grid_tmp;
	int grid_h, grid_w;
	int cell_types;

	/* [Simulation flags] */
	double time_step;
	double timer;
	uint8_t flags;
};

const uint8_t colors[][3] = { { 0, 0, 0 },
			      { 58, 168, 220 },
			      { 220, 20, 68 },
			      { 220, 168, 68 } };

int
get_cell(struct life *conway, int y, int x, int grid)
{
	if (conway->flags & BIT(BORDER) &&
	    ((y < 0 || y >= conway->grid_h || x < 0 || x >= conway->grid_w))) {
		return 0;
	}

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
	if (conway->flags & BIT(BORDER) &&
	    ((y < 0 || y >= conway->grid_h || x < 0 || x >= conway->grid_w))) {
		return;
	}

	if (grid) {
		conway->grid[(y % conway->grid_h) * conway->grid_w + (x % conway->grid_w)] = state;

	} else {
		conway->grid_tmp[(y % conway->grid_h) * conway->grid_w + (x % conway->grid_w)] = state;
	}
}

int
neighbour_cnt(struct life *conway, int y, int x, int *hi_cnt)
{
	int i, j, n, cell;
	uint8_t n_cnt[conway->cell_types];

	for (i = 0; i != conway->cell_types; ++i) {
		n_cnt[i] = 0;
	}

	n = 0;
	for (i = -1; i != 2; ++i) {
		for (j = -1; j != 2; ++j) {
			if ((cell = get_cell(conway, y + i, x + j, TMP_GRID)) != 0) {
				++n_cnt[cell - 1];
				++n;
			}
		}
	}

	*hi_cnt = 0;
	for (i = 0; i != conway->cell_types; ++i) {
		if (n_cnt[i] > n_cnt[*hi_cnt]) {
			*hi_cnt = i;
		}
	}

	++*hi_cnt;

	return n - (get_cell(conway, y, x, TMP_GRID) ? 1 : 0);
}

int
update_cell(struct life *conway, int y, int x)
{
	int nb, hi_cnt;

	nb = neighbour_cnt(conway, y, x, &hi_cnt);
	if (get_cell(conway, y, x, TMP_GRID)) {
		return nb < 2 || nb > 3 ? 0 : hi_cnt;

	} else {
		return nb == 3 ? hi_cnt : 0;
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
		conway->grid[i] = rand() % (1 + conway->cell_types);
	}
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
	uint8_t scr_buf[GRID_H * GRID_W][3];
	double x, y;
	int i, ix, iy, win_w, win_h, arg;

	/* Random seed */
	srand(time(NULL));

	/* Default values */
	win_w = WINDOW_W;
	win_h = WINDOW_H;
	dm.scr_buf_w = GRID_W;
	dm.scr_buf_h = GRID_H;

	conway.flags |= BIT(DRAW) | BIT(PAUSE);
	conway.timer = glfwGetTime();
	conway.time_step = TIME_STEP;
	conway.cell_types = 1;

	for (i = 1; i != argc; ++i) {
		if (argv[i][0] == '-') {
			if (i < argc - 1) {
				arg = atoi(argv[i + 1];
			}

			switch (argv[i][1]) {
			case 'x':
				win_w = arg;
				break;

			case 'y':
				win_h = arg;
				break;
				
			case 'w':
				dm.scr_buf_w = arg;
				break;

			case 'h':
				dm.scr_buf_h = arg;
				break;

			case 'c':
				conway.cell_types = MAX(MIN(arg, MAX_CELL_TYPES), 1);
				break;

			case 'b':
				conway.flags |= BIT(BORDER);
				break;
			}
		}
	}

	/* Create window */
	memset(scr_buf[0], 0, sizeof scr_buf);
	dm.scr_buf = scr_buf[0];
	create_matrix(&dm, win_w, win_h);

	/* Initialize game of struct life */
	conway.grid_h = dm.scr_buf_h;
	conway.grid_w = dm.scr_buf_w;
	conway.grid = malloc(conway.grid_h * conway.grid_w * sizeof(int));
	conway.grid_tmp = malloc(conway.grid_h * conway.grid_w * sizeof(int));
	randomize_grid(&conway);

	/* Some GLFW stuff */
	glfwSetWindowUserPointer(dm.win, &conway);
	glfwSetKeyCallback(dm.win, key_callback);
	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(dm.win) && !(conway.flags & BIT(QUIT))) {
		glfwPollEvents();

		/* Handle mouse input */
		if (glfwGetMouseButton(dm.win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			conway.flags |= BIT(DRAW);
			
			glfwGetCursorPos(dm.win, &x, &y);
			ix = (x / (double)win_w) * conway.grid_w;
			iy = (y / (double)win_h) * conway.grid_h;
			
			conway.grid[iy * GRID_W + ix] = 1;

		} else if (glfwGetMouseButton(dm.win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
			conway.flags |= BIT(DRAW);
			
			glfwGetCursorPos(dm.win, &x, &y);
			ix = (x / (double)win_w) * conway.grid_w;
			iy = (y / (double)win_h) * conway.grid_h;
			
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
			for (i = 0; i != conway.grid_w * conway.grid_h; ++i) {
				memcpy(scr_buf[i], &colors[conway.grid[i]], sizeof (uint8_t) * 3);
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

