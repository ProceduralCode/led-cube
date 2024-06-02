#include <stdint.h>
#include "../common/FPToolkit.c"
#include "../common/typedefs.h"
#include "../common/serial.h"
#include "../common/packet.h"
#include "../common/hsv.h"

void _screen_recalc(screen *s) {
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
	return (double2){
		(pos.x - s->screen_bounds_min.x) / s->viewscale_lens.x * SCREEN_WIDTH,
		(pos.y - s->screen_bounds_min.y) / s->viewscale_lens.y * SCREEN_HEIGHT,
	};
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
	double2 screen_pos1 = to_screen_space(s, pos1);
	double2 screen_pos2 = to_screen_space(s, pos2);
	G_rgb(color.x, color.y, color.z);
	G_line(screen_pos1.x, screen_pos1.y, screen_pos2.x, screen_pos2.y);
}

double2 screen_waitclick(screen *s) {
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

void ifs() {
	screen screen;
	screen_init(&screen);
	screen_set_view(&screen, (double2){0, 0.45}, 0.6); // fern
	int devices = serialInit();

	double2 pa = {0, 0};
	while (True) {
		screen_clear(&screen, (double3){0, 0, 0});
		
		// All these points are in terms of a unit square
		double2 point = {0, 0};
		RGB hsl;// = (double3){0.3, 0.4, 0.4};
		for (int i = 0; i < 1000000; i++) {
			double r = drand48();
			//printf("%lf: %lf,%lf\n", r, point.x, point.y);

			// Fern
			if (r < 0.8) { // Sets points to the upper right
				point = scale(point, (double2){0.90, 0.90}); // These points should have the most movement
				point = rotate(point, -2); // turn each point 2 degrees clockwise
				point = translate(point, (double2){0, 0.1}); // Adds an absolute 10% to the point's position
				 //hsl = lerp_double3(hsl, (double3){0.3, 0.4, 0.9}, 0.02);
				hsv2rgb(&hsl, 80, 40, 50);
			} else if (r < 0.9) { // Sets points to the left side
				point = scale(point, (double2){0.25, 0.25}); // Constrain the movement compared to other attractors
				point = rotate(point, 60); // Rotate left
				point = translate(point, (double2){0, 0.1}); // Allow movement to be more influenced by the translate
				//hsl = lerp_double3(hsl, (double3){0.3, 0.4, 0.1}, 0.3);
				hsv2rgb(&hsl, 120, 80, 100);
			} else if (r < 0.999) { // Righthand leaves. We use a sign change in the scale to flip the leaf.
				point = scale(point, (double2){-0.27, 0.27}); // Pull these points to the right side of the screen
				point = rotate(point, -60); // Rotate right
				point = translate(point, (double2){0, 0.05}); // Most of this placement will be affected by the scale
				//hsl = lerp_double3(hsl, (double3){0.3, 0.4, 0.1}, 0.3);
				hsv2rgb(&hsl, 120, 80, 100);
			} else { // Stem. Therefore these points move towards a single line.
				point = scale(point, (double2){0, 0.12}); // Don't make the stem too big or too small
				//hsl = lerp_double3(hsl, (double3){0.1, 0.4, 0.3}, 0.9);
				hsv2rgb(&hsl, 30, 90, 100);
			}

			double2 ss_point = to_screen_space(&screen, point);
			int sx = (int)ss_point.x;
			int sy = (int)ss_point.y;
			if (sx >= 0 && sx < SCREEN_WIDTH && sy >= 0 && sy < SCREEN_HEIGHT) {
				screen.pixels[sy][sx] = (double3){hsl.red/255.0, hsl.green/255.0, hsl.blue/255.0};
				//screen.pixels[sy][sx] = hsl2rgb(hsl);
			}
		}

		// Covert double3 to RGB and send
		if(devices > 0)
		{
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
					packet.flags = 1;
					for(int sy = 0; sy < PANEL_SIZE; sy++)
					{
						for(int sx = 0; sx < PANEL_SIZE; sx++)
						{
							int px = x0 + sx;
							int py = y0 + sy;
							packet.pixelMap[(sx + sy * PANEL_SIZE) * COLORS + 0] = screen.pixels[py][px].x * 255;
							packet.pixelMap[(sx + sy * PANEL_SIZE) * COLORS + 1] = screen.pixels[py][px].y * 255;
							packet.pixelMap[(sx + sy * PANEL_SIZE) * COLORS + 2] = screen.pixels[py][px].z * 255;
						}
					}
					serialWrite(j, (char*)&packet, sizeof(packet));
					if(packet.flags) serialRead(j, 0);
				}
			}
			for(int j = 0; j < devices; j++)
			{
				serialWrite(j, "d", 1);
				serialRead(j, 0);
			}
		}
		screen_draw(&screen);
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
}

int main() {
	ifs();
}
