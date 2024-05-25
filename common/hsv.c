//#include <stdio.h>
#include <stdint.h>
#include "hsv.h"

/*****************************************************
 * hue - positionon the color wheel in degrees
 * saturation - colorfulness expressed as a percent
 * value - brightness expressed as a percentage
 ****************************************************/
void hsv2rgb(RGB *const rgb, uint16_t hue, uint8_t saturation, uint8_t value)
{
	float h = ((hue % 60) + 1) / 60.0;
	float chroma = (saturation/100.0) * (value/100.0);
	hue = hue % 360;
	//printf("hue: %3d, saturation: %3d, value: %3d, ", hue, saturation, value);
	if(hue < 60)
	{
		rgb->red = 255;
		rgb->green = h * chroma * 255;
		rgb->blue = 0;
		//printf("h: %1.3f, c: %1.3f\n", h, chroma);
	}
	else if(hue < 120)
	{
		rgb->red = (1-h) * chroma * 255;
		rgb->green = 255;
		rgb->blue = 0;
		//printf("h: %1.3f, c: %1.3f\n", 1-h, chroma);
	}
	else if(hue < 180)
	{
		rgb->red = 0;
		rgb->green = 255;
		rgb->blue = h * chroma * 255;
		//printf("h: %1.3f, c: %1.3f\n", h, chroma);
	}
	else if(hue < 240)
	{
		rgb->red = 0;
		rgb->green = (1-h) * chroma * 255;
		rgb->blue = 255;
		//printf("h: %1.3f, c: %1.3f\n", 1-h, chroma);
	}
	else if(hue < 300)
	{
		rgb->red = h * chroma * 255;;
		rgb->green = 0;
		rgb->blue = 255;
		//printf("h: %1.3f, c: %1.3f\n", h, chroma);
	}
	else
	{
		rgb->red = 255;
		rgb->green = 0;
		rgb->blue = (1-h) * chroma * 255;
		//printf("h: %1.3f, c: %1.3f\n", 1-h, chroma);
	}
}
