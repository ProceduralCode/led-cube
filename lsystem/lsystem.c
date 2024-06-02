#include "../common/FPToolkit.c"
#include "../common/defines.h"
#include "../common/typedefs.h"
#include "../common/hsv.h"

#define PAGESIZE 4096
#define PAGES 2
#define STACKTOP PAGESIZE

typedef struct _StackElement_t
{
    double x,y;
    double rot;
} StackElement_t;

RGB g_screen[SCREEN_WIDTH][SCREEN_HEIGHT];
int g_index;
StackElement_t g_stack[STACKTOP];
char g_string[PAGESIZE*PAGES] = {'X', 'F'};
char g_stringCopy[PAGESIZE*PAGES];

void _screen_recalc(screen *s) {
	// s->viewscale_lens[0] = 1 / s->viewscale;
	// s->viewscale_lens[1] = 1 / s->viewscale * SCREEN_HEIGHT / SCREEN_WIDTH;
	// s->screen_bounds[0] = s->viewpoint[0] - s->viewscale_lens[0]/2;
	// s->screen_bounds[1] = s->viewpoint[0] + s->viewscale_lens[0]/2;
	// s->screen_bounds[2] = s->viewpoint[1] - s->viewscale_lens[1]/2;
	// s->screen_bounds[3] = s->viewpoint[1] + s->viewscale_lens[1]/2;
	s->viewscale_lens = (double2){
		1 / s->viewscale,
		1 / s->viewscale * SCREEN_HEIGHT / SCREEN_WIDTH,
	};
	s->screen_bounds_min = (double2){
		s->viewpoint.x - s->viewscale_lens.x/2,
		s->viewpoint.y - s->viewscale_lens.y/2,
	};
	s->screen_bounds_max = (double2){
		s->viewpoint.x + s->viewscale_lens.x/2,
		s->viewpoint.y + s->viewscale_lens.y/2,
	};

}

void screen_init(screen *s) {
	s->pixels = malloc(sizeof(double3) * SCREEN_WIDTH * SCREEN_HEIGHT);
	G_init_graphics(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void screen_set_view(screen *s, double2 viewpoint, double viewscale) {
	s->viewpoint = viewpoint;
	s->viewscale = viewscale;
	_screen_recalc(s);
}

void screen_zoom_towards(screen *s, double2 pos, double zoom_factor) {
	s->viewscale *= zoom_factor;
	s->viewpoint = (double2){
		(s->viewpoint.x - pos.x) / zoom_factor + pos.x,
		(s->viewpoint.y - pos.y) / zoom_factor + pos.y,
	};
	_screen_recalc(s);
}

double2 to_view_space(screen *s, double2 pos) {
	return (double2){
		pos.x / SCREEN_WIDTH  * s->viewscale_lens.x + s->screen_bounds_min.x,
		pos.y / SCREEN_HEIGHT * s->viewscale_lens.y + s->screen_bounds_min.y,
	};
}

double2 to_screen_space(screen *s, double2 pos) {
    double x = (pos.x - s->screen_bounds_min.x) / s->viewscale_lens.x * SCREEN_WIDTH;
    double y = (pos.y - s->screen_bounds_min.y) / s->viewscale_lens.y * SCREEN_HEIGHT;
    if(x > SCREEN_WIDTH) x = SCREEN_WIDTH;
    if(y > SCREEN_HEIGHT) y = SCREEN_HEIGHT;
	return (double2){x,y};
}

void screen_clear(screen *s, double3 color) {
	G_rgb(color.x, color.y, color.z);
	G_clear();
	for (int y = 0; y < SCREEN_HEIGHT; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			s->pixels[y][x] = color;
		}
	}
}

void screen_draw(screen *s) {
	for (int y = 0; y < SCREEN_HEIGHT; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			double3 col = s->pixels[y][x];
			G_rgb(col.x, col.y, col.z);
			G_point(x, y);
		}
	}
}

void screen_draw_pix(screen *s, double2 pos, double3 color) {
	double2 screen_pos = to_screen_space(s, pos);
	G_rgb(color.x, color.y, color.z);
	G_point(screen_pos.x, screen_pos.y);
}

void screen_draw_line(screen *s, double2 pos1, double2 pos2, double3 color) {
    static int degree;
    double2 screen_pos1 = to_screen_space(s, pos1);
    double2 screen_pos2 = to_screen_space(s, pos2);
    RGB rgb;
    hsv2rgb(&rgb, degree, 100, 100);
    G_rgb(rgb.red/255.0, rgb.green/255.0, rgb.blue/255.0);
    double delta_x = screen_pos2.x - screen_pos1.x;
    double delta_y = screen_pos2.y - screen_pos1.y;
    for(double t = 0; t <= 1.0; t = t + 0.01)
    {
        int x = screen_pos1.x + t*delta_x;
        int y = screen_pos1.y + t*delta_y;
        if ((x < 0) || (y < 0) || (x >= SCREEN_WIDTH) || (y >= SCREEN_HEIGHT)) break;
        g_screen[x][y] = rgb;
        G_point(x,y);
    }
    degree = degree + 1;
}

double2 screen_waitclick(screen *s) {
	double p[2];
	G_wait_click(p);
	double2 pos = (double2){p[0], p[1]};
	return to_view_space(s, pos);
}

void t_reset(turtle *t) {
	t->hsl = (double3){0.3, 0.4, 0.4};
	t->pos = (double2){0, 0};
	t->rot = 0;
	t->pen_state = 0;
}
void t_move(turtle *t, screen *s, double dist) {
	double2 new_pos = (double2){
		t->pos.x + dist * cos(t->rot),
		t->pos.y + dist * sin(t->rot),
	};
	if (t->pen_state == 0) {
		screen_draw_line(s, t->pos, new_pos, t->hsl);
	}
	t->pos = new_pos;
}
void t_rot(turtle *t, double ang) {
	t->rot += ang * M_PI / 180.0;
}
void t_pen(turtle *t, int state) {
	t->pen_state = state;
}
rule *get_rule(char var, rule *rules) {
	for (int i = 0; rules[i].var != '\0'; i++) {
		if (rules[i].var == var) {
			return &rules[i];
		}
	}
	return NULL;
}

void stackPush(double x, double y, double theta)
{
    if(g_index == (STACKTOP-1))
    {
        printf("ERROR: tried to push to full stack\n");
        return;
    }
    g_stack[g_index] = (StackElement_t){x, y, theta};
    g_index++;
}

void stackPop(double *const x, double *const y, double *const theta)
{
    if(g_index == 0)
    {
        printf("ERROR: tried to pop from an empty stack\n");
        return;
    }
    g_index--;
    *x = g_stack[g_index].x;
    *y = g_stack[g_index].y;
    *theta = g_stack[g_index].rot;
}

void printString()
{
    if(g_string[0] == '\0') return;

    int i = 0;
    while(g_string[i] != '\0')
    {
        printf("%c", g_string[i]);
        i++;
    }
    printf("\n");
}

int X(char *const str)
{
    const char prod[] = "F+[[X]-X]-F[-FX]+X";
    const int size = strlen(prod);
    //printf("str is \"%s\" adding \"%s\" %d\n", str, prod, size);
    strcpy(str, prod);
    return size;
}

int F(char *const str)
{
    const char prod[] = "FF";
    const int size = strlen(prod);
    //printf("str is \"%s\" adding \"%s\" %d\n", str, prod, size);
    strcpy(str, prod);
    return size;
}

void production(screen *screen, turtle *t)
{
    int i = 0;
    int size = 0;
    while(g_string[i] != '\0')
    {
        switch(g_string[i])
        {
            case 'X':
                size += X(g_stringCopy+size);
                break;
            case 'F':
                size += F(g_stringCopy+size);
                t_move(t, screen, 0.02);
                break;
            case '+':
                g_stringCopy[size] = '+';
                t_rot(t, 25);
                size++;
                break;
            case '-':
                g_stringCopy[size] = '-';
                t_rot(t, -25);
                size++;
                break;
            case '[':
                g_stringCopy[size] = '[';
                stackPush(t->pos.x, t->pos.y, t->rot);
                size++;
                break;
            case ']':
                g_stringCopy[size] = ']';
                stackPop(&t->pos.x, &t->pos.y, &t->rot);
                size++;
                break;
            default:
                printf("%c is an invalid production\n");
        }
        i++; //i should not exceed the size of the array
    }

    g_stringCopy[size] = '\0'; // Ensure a null terminator
    strcpy(g_string, g_stringCopy); // Replace the old string
    memset(g_stringCopy, '\0', strlen(g_stringCopy)); // zero out the copy string for next time
}

int main(int argc, char *argv[])
{
    argc--;
    argv = argv+1;

    if(argc != 0)
    {
        int iterations = atoi(argv[0]);

        if(iterations == 1) printf("Doing %d iteration\n", iterations);
        else printf("Doing %d iterations\n", iterations);

        if(iterations > 0)
        {
            screen screen;
            screen_init(&screen);
            screen_set_view(&screen, (double2){0.0, 0.5}, 0.6);
            screen_clear(&screen, (double3){0, 0, 0});
            turtle t;
            t_reset(&t);

            iterations--;
            printString();
            for(int i = 0; i < iterations; i++)
            {
                production(&screen, &t);
                printString();
            }
            int key = G_wait_key();
        }
    }
    else
    {
        printf("This program takes in a number on the commandline\n");
    }

    return 0;
}
