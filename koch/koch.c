#include "../common/FPToolkit.c"
#include "../common/defines.h"
#include "../common/typedefs.h"
#include "../common/packet.h"
#include "../common/serial.h"

// This program has major issues with stack overflows
// We set these defines in order to constrain the problem
#define PAGESIZE 4096
#define PAGES 32
#define STRING_MAX PAGESIZE*PAGES
#define STACKTOP PAGESIZE
#define MAX_ITERATIONS 6U

char g_string[STRING_MAX] = {'f'};
char g_stringCopy[STRING_MAX];

void _screen_recalc(screen *s)
{
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

void screen_init(screen *s)
{
    s->pixels = malloc(sizeof(double3) * SCREEN_WIDTH * SCREEN_HEIGHT);
    G_init_graphics(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void screen_set_view(screen *s, double2 viewpoint, double viewscale)
{
    s->viewpoint = viewpoint;
    s->viewscale = viewscale;
    _screen_recalc(s);
}

void screen_zoom_towards(screen *s, double2 pos, double zoom_factor)
{
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

void screen_clear(screen *s, double3 color)
{
    G_rgb(color.x, color.y, color.z);
    G_clear();
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            s->pixels[y][x] = color;
        }
    }
}

void screen_draw(screen *s)
{
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            double3 col = s->pixels[y][x];
            G_rgb(col.x, col.y, col.z);
            G_point(x, y);
        }
    }
}

void screen_draw_pix(screen *s, double2 pos, double3 color)
{
    double2 screen_pos = to_screen_space(s, pos);
    G_rgb(color.x, color.y, color.z);
    G_point(screen_pos.x, screen_pos.y);
}

void screen_draw_line(screen *s, double2 pos1, double2 pos2, double3 color) {
    double2 screen_pos1 = to_screen_space(s, pos1);
    double2 screen_pos2 = to_screen_space(s, pos2);
    G_rgb(color.x, color.y, color.z);
    double delta_x = screen_pos2.x - screen_pos1.x;
    double delta_y = screen_pos2.y - screen_pos1.y;
    for(double t = 0; t <= 1.0; t = t + 0.01)
    {
        int x = screen_pos1.x + t*delta_x;
        int y = screen_pos1.y + t*delta_y;
        if ((x < 0) || (y < 0) || (x >= SCREEN_WIDTH) || (y >= SCREEN_HEIGHT)) break;
        s->pixels[y][x] = (double3){color.x, color.y, color.z};
        G_point(x,y);
    }
}

double2 screen_waitclick(screen *s)
{
    double p[2];
    G_wait_click(p);
    double2 pos = (double2){p[0], p[1]};
    return to_view_space(s, pos);
}

double2 scale(double2 point, double2 scale) {
    return (double2){
        point.x * scale.x,
        point.y * scale.y,
    };
}
double2 translate(double2 point, double2 offset) {
    return (double2){
        point.x + offset.x,
        point.y + offset.y,
    };
}
double2 rotate(double2 point, double angle) {
    double rad = angle * M_PI / 180;
    return (double2){
        point.x * cos(rad) - point.y * sin(rad),
        point.x * sin(rad) + point.y * cos(rad),
    };
}

void t_reset(turtle *t)
{
    t->hsl = (double3){0.3, 0.4, 0.4};
    t->pos = (double2){0, 0};
    t->rot = M_PI/4.0;
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
rule *get_rule(char var, rule *rules)
{
    for (int i = 0; rules[i].var != '\0'; i++) {
        if (rules[i].var == var) {
            return &rules[i];
        }
    }
    return NULL;
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

int main(int argc, char *argv[])
{
	screen screen;
	screen_init(&screen);
	screen_set_view(&screen, (double2){0.5, 0.2}, 1);

	turtle t;
	rule rules[] = {
		{'f', "f+f--f+f"},
		{'+', "+"},
		{'-', "-"},
	};
	double rot = 60;
	int iterations = 0;
    int devices = serialInit();

	while (True) {
		screen_clear(&screen, (double3){0, 0, 0});

		t_reset(&t);

        // Reset the string builder
        memset(g_string, '\0', strlen(g_string));
        memset(g_stringCopy, '\0', strlen(g_stringCopy));

        // Axiom
        strcpy(g_string, "f--f--f--");

        // generate string
        int rule_cnt = sizeof(rules) / sizeof(rule);
        for (int i = 0; i < iterations; i++) {
            int nidx = 0;
            for (int idx = 0; idx < STRING_MAX; idx++) {
                if (g_string[idx] == '\0') { break; }
                rule *r = get_rule(g_string[idx], rules);
                int repl_len = strlen(r->repl);
                for (int j = 0; j < repl_len; j++) {
                    g_stringCopy[nidx+j] = r->repl[j];
                }
                nidx += repl_len;
            }
            g_stringCopy[nidx] = '\0';
            strcpy(g_string, g_stringCopy);
        }

        printf("%s\n", g_string);

		int idx = 0;
		while (g_string[idx] != '\0') {
			switch (g_string[idx]) {
				case 'f':
					t_move(&t, &screen, 0.01);
					break;
				case '+':
					t_rot(&t, rot);
					break;
				case '-':
					t_rot(&t, -rot);
					break;
			}
			idx++;
		}

		if(devices > 0)
        {
            // Print to the LED Matracies
            CommandDrawPanel_t packet;
            int section = PANELS/devices;
            for(int j = 0; j < devices; j++)
            {
                for(int i = 0; i < section; i++)
                {
                    int x0 = (i % PANELS_HORIZONTAL) * PANEL_SIZE;
                    int y0 = ((i + j*section) / PANELS_HORIZONTAL) * PANEL_SIZE;
                    packet.panelId = i+1;
                    packet.bufferId = 0;
                    for(int sy = 0; sy < PANEL_SIZE; sy++)
                    {
                        for(int sx = 0; sx < PANEL_SIZE; sx++)
                        {
                            int px = x0 + sx;
                            int py = y0 + sy;
                            packet.pixelMap[sy][sx].red = screen.pixels[py][px].x * 255;
                            packet.pixelMap[sy][sx].green = screen.pixels[py][px].y * 255;
                            packet.pixelMap[sy][sx].blue = screen.pixels[py][px].z * 255;
                        }
                    }
                    serialWrite(j, (char*)&packet, sizeof(packet));
                    serialRead(j, 0);
                }
            }
            for(int j = 0; j < devices; j++)
            {
                serialWrite(j, "d", 1);
                serialRead(j, 0);
            }
        }

		int key = G_wait_key();

		double move_amt = 0.05 / screen.viewscale;
		//printf("viewscale: %f\n", screen.viewscale);
		double2 offset = (double2){0, 0};
		double2 pa_off = (double2){0, 0};
		double zoom_factor = 1;

		if (key == ' ') { break; }
		if (key == 'w') { offset = (double2){0, move_amt}; }
		if (key == 's') { offset = (double2){0, -move_amt}; }
		if (key == 'a') { offset = (double2){-move_amt, 0}; }
		if (key == 'd') { offset = (double2){move_amt, 0}; }
		if (key == 'e') { zoom_factor *= 1.2; }
		if (key == 'q') { zoom_factor /= 1.2; }
		if (key == '=') { iterations = (iterations+1)%MAX_ITERATIONS; }
        if (key == '-') { iterations = (iterations-1)%MAX_ITERATIONS; } // This is the best we can do without adding if statements

		if (offset.x != 0 || offset.y != 0 || zoom_factor != 1) {
			screen_set_view(&screen, (double2){
				screen.viewpoint.x + offset.x,
				screen.viewpoint.y + offset.y,
			}, screen.viewscale * zoom_factor);
			//printf("viewpoint: (%f, %f) %f\n", screen.viewpoint.x, screen.viewpoint.y, screen.viewscale);
		}
	}
}
