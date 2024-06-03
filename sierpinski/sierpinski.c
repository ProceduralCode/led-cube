#include "../common/FPToolkit.c"
#include "../common/defines.h"
#include "../common/typedefs.h"
#include "../common/hsv.h"
#include "../common/packet.h"
#include "../common/serial.h"

// This program has major issues with stack overflows
// We set these defines in order to constrain the problem
#define PAGESIZE 4096
#define PAGES 1
#define STRING_MAX PAGESIZE*PAGES
#define STACKTOP PAGESIZE
#define MAX_ITERATIONS 4U

typedef struct _StackElement_t
{
    double x,y;
    double rot;
} StackElement_t;

RGB g_screen[SCREEN_HEIGHT][SCREEN_WIDTH];
int g_index;
StackElement_t g_stack[STACKTOP];
char g_string[STRING_MAX] = {'X', 'F'};
char g_stringCopy[STRING_MAX];
uint32_t g_hsvDegree;

void _screen_recalc(screen *s)
{
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
    RGB rgb;
    hsv2rgb(&rgb, g_hsvDegree, 100, 100);
    G_rgb(rgb.red/255.0, rgb.green/255.0, rgb.blue/255.0);
    double delta_x = screen_pos2.x - screen_pos1.x;
    double delta_y = screen_pos2.y - screen_pos1.y;
    for(double t = 0; t <= 1.0; t = t + 0.01)
    {
        int x = screen_pos1.x + t*delta_x;
        int y = screen_pos1.y + t*delta_y;
        if ((x < 0) || (y < 0) || (x >= SCREEN_WIDTH) || (y >= SCREEN_HEIGHT)) break;
        g_screen[y][x] = rgb;
        G_point(x,y);
    }
    g_hsvDegree = g_hsvDegree + 1;
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

int F(char *const str)
{
    const char prod[] = "F-G+F+G-F";
    const int size = strlen(prod);
    strcpy(str, prod);
    return size;
}

int G(char *const str)
{
    const char prod[] = "GG";
    const int size = strlen(prod);
    strcpy(str, prod);
    return size;
}

void stringBuilder()
{
    int i = 0;
    int size = 0;
    while(g_string[i] != '\0')
    {
        switch(g_string[i])
        {
            case 'F':
                size += F(g_stringCopy+size);
                break;
            case 'G':
                size += G(g_stringCopy+size);
                break;
            case '+':
                g_stringCopy[size] = '+';
                size++;
                break;
            case '-':
                g_stringCopy[size] = '-';
                size++;
                break;
            case '[':
                g_stringCopy[size] = '[';
                size++;
                break;
            case ']':
                g_stringCopy[size] = ']';
                size++;
                break;
            default:
                printf("%c has no rule\n");
        }
        i++; //i should not exceed the size of the array
        if(size > STRING_MAX)
        {
            printf("Exceeded the string array by %d bytes\n", size - STRING_MAX);
            exit(0);
        }
    }

    g_stringCopy[size] = '\0'; // Ensure a null terminator
    strcpy(g_string, g_stringCopy); // Replace the old string
    memset(g_stringCopy, '\0', strlen(g_stringCopy)); // zero out the copy string for next time
}

void production(screen *screen, turtle *t)
{
    int i = 0;
    while(g_string[i] != '\0')
    {
        switch(g_string[i])
        {
            case 'F':
                t_move(t, screen, 0.02);
                break;
            case 'G':
                t_move(t, screen, 0.02);
                break;
            case '+':
                t_rot(t, -120.0);
                break;
            case '-':
                t_rot(t, 120.0);
                break;
            case '[':
                stackPush(t->pos.x, t->pos.y, t->rot);
                break;
            case ']':
                stackPop(&t->pos.x, &t->pos.y, &t->rot);
                break;
            default:
                printf("%c is an invalid production\n");
        }
        i++; //i should not exceed the size of the array
    }
}

int main(int argc, char *argv[])
{
    int iterations = 0;
    screen screen;
    turtle t;
    screen_init(&screen);
    screen_set_view(&screen, (double2){0.15, 0.135}, 2.0);
    double2 pa = {0, 0};

    int devices = serialInit();

    while (True)
    {
        // Reset the display
        screen_clear(&screen, (double3){0, 0, 0});
        t_reset(&t);

        // Reset the string builder
        memset(g_string, '\0', strlen(g_string));
        memset(g_stringCopy, '\0', strlen(g_stringCopy));
        memset(g_stack, 0, sizeof(g_stack));

        // Axiom
        g_string[0] = 'F';
        g_string[1] = '-';
        g_string[2] = 'G';
        g_string[3] = '-';
        g_string[4] = 'G';
        printString();

        // Set the starting position on the color wheel
        g_hsvDegree = 130;
        memset(g_screen, 0, sizeof(g_screen));

        // Go through the productions
        for(int i = 0; i < iterations; i++)
        {
            stringBuilder();
            printString();
        }
        production(&screen, &t);

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
                            packet.pixelMap[sy][sx].red = g_screen[py][px].red;
                            packet.pixelMap[sy][sx].green = g_screen[py][px].green;
                            packet.pixelMap[sy][sx].blue = g_screen[py][px].blue;
                            //if(g_screen[py][px].green != 0) printf("%3d,%3d --- %3d,%3d\n", sx, sy, px, py);
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
        if (key == 'i') { pa_off = (double2){0, 0.02}; }
        if (key == 'k') { pa_off = (double2){0, -0.02}; }
        if (key == 'j') { pa_off = (double2){-0.02, 0}; }
        if (key == 'l') { pa_off = (double2){0.02, 0}; }
        if (key == '=') { iterations = iterations%MAX_ITERATIONS + 1; }
        if (key == '-') { iterations = (iterations-1)%MAX_ITERATIONS; } // This is the best we can do without adding if statements
        pa = translate(pa, pa_off);
        printf("pa: (%f, %f)\n", pa.x, pa.y);

        if (offset.x != 0 || offset.y != 0 || zoom_factor != 1) {
            screen_set_view(&screen, (double2){
                screen.viewpoint.x + offset.x,
                screen.viewpoint.y + offset.y,
            }, screen.viewscale * zoom_factor);
            printf("viewpoint: (%f, %f) %f\n", screen.viewpoint.x, screen.viewpoint.y, screen.viewscale);
        }
    }

    serialDeinit();

    return 0;
}
