#include "../common/FPToolkit.c"
#include "../common/typedefs.h"
#include "../common/math.c"
#include "../common/packet.h"
#include "../common/serial.h"

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

double3 mandelbrot_pixel_color(double x, double y, double a) {
	double z_real = x, z_imag = y;
	double z_real_new, z_imag_new;
	int max_iter = 255;
	for (int i = 0; i < max_iter; i++) {
		z_real_new = z_real * z_real - z_imag * z_imag + x;
		z_imag_new = 2 * z_real * z_imag + y;
		z_real = z_real_new;
		z_imag = z_imag_new;
		// We use 4 because the mandelbrot exists on a square plane from -2 to +2
		if (z_real * z_real + z_imag * z_imag > 4)
		{
			double iters = (double)i / max_iter;
			return hsl2rgb((double3){iters, 0.5, iters});
		}
	}
	return (double3){0, 0, 0};
}

void mandelbrot() {
	screen screen;
	screen_init(&screen);
	// screen_set_view(&screen, (vec2){-0.5, 0}, 0.25);
	screen_set_view(&screen, (double2){-1.17, 0.3}, 8);

	// Set up the arduinos
	int devices = serialInit();

	double a = 0;
	double a_off = 0.1;
	while (True) {
		for (int y = 0; y < SCREEN_HEIGHT; y++) {
			for (int x = 0; x < SCREEN_WIDTH; x++) {
				screen.pixels[y][x] = mandelbrot_pixel_color(
					lerp(screen.screen_bounds_min.x, screen.screen_bounds_max.x, (double)x/SCREEN_WIDTH),
					lerp(screen.screen_bounds_min.y, screen.screen_bounds_max.y, (double)y/SCREEN_HEIGHT),
					a
				);
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
					for(int sy = 0; sy < PANEL_SIZE; sy++)
					{
						for(int sx = 0; sx < PANEL_SIZE; sx++)
						{
							int px = x0 + sx;
							int py = y0 + sy;
							packet.pixelMap[sy][sx].red = screen.pixels[py][px].x * 255.0;
							packet.pixelMap[sy][sx].green = screen.pixels[py][px].y * 255.0;
							packet.pixelMap[sy][sx].blue = screen.pixels[py][px].z * 255.0;
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
		screen_draw(&screen);
		int key = G_wait_key();
		// printf("key: %d\n", key);

		double move_amt = 0.05 / screen.viewscale;
		double2 offset = (double2){0, 0};
		double zoom_factor = 1;

		if (key == ' ') { break; }
		if (key == 'w') { offset = (double2){0, move_amt}; }
		if (key == 's') { offset = (double2){0, -move_amt}; }
		if (key == 'a') { offset = (double2){-move_amt, 0}; }
		if (key == 'd') { offset = (double2){move_amt, 0}; }
		if (key == 'e') { zoom_factor *= 1.2; }
		if (key == 'q') { zoom_factor /= 1.2; }
		if (key == 'j') { a += a_off; }
		if (key == 'l') { a -= a_off; }
		if (key == 'i') { a_off *= 1.2; }
		if (key == 'k') { a_off /= 1.2; }

		if (offset.x != 0 || offset.y != 0 || zoom_factor != 1) {
			screen_set_view(&screen, (double2){
				screen.viewpoint.x + offset.x,
				screen.viewpoint.y + offset.y,
			}, screen.viewscale * zoom_factor);
			// printf("viewpoint: (%f, %f)\n", screen.viewpoint.x, screen.viewpoint.y);
			// printf("viewscale: %f\n", screen.viewscale);
		}

		// vec2 click = screen_waitclick(&screen);
		// screen_zoom_towards(&screen, click, 1.1);
	}
}

int main(int argc, char *argv[]) {
	mandelbrot();
}
