#include <TFT_eSPI.h>
#include "zbitx.h"

static uint8_t waterfall[240*200]; //Very Arbitrary!
static int bandwidth_stop, bandwidth_start, center_line;

void waterfall_bandwidth(int start, int stop, int center){
	if (start > stop){
		bandwidth_stop = start;
		bandwidth_start = stop;
	}
	else{
		bandwidth_start = start;
		bandwidth_stop = stop;
	}

	center_line = center;	
}

int waterfall_init(){  
  memset(waterfall, 0,  sizeof(waterfall));
	waterfall_bandwidth(-10,-40,-25);
	return 0;
}

uint16_t inline heat_map(int v){
  uint8_t r, g, b;

  v *= 4;
  /*
  if (v > 100)    //we limit ourselves to 100 db range
    v = 100;
  */
  r =g = b = 0;
  
  if (v < 32){                  // r = 0, g= 0, increase blue
    r = 0;
    g = 0;
    b = v; 
  }
  else if (v < 64){             // r = 0, increase g, blue is max
    r = 0;
    g = (v - 32);
    b = 0x3f; 
  }
  else if (v < 96){             // r = 0, g= max, decrease b
    r = 0;
    g = 0x1f; 
    b = (96-v); 
  }
  else if (v < 128){             // increase r, g = max, b = 0
    r = (v-96);
    g = 0x1f;
    b = 0; 
  }
  else {                       // r = max, decrease g, b = 0
    r = 0x1f;
    g = (100-v);
    b = 0; 
  }
  //0x3c00 = red, 0x1f = green, blue=0x1e00

   // it took two days to figure this out! the bitfields are all garbled on the ili9488
  return (g << 13) | (b << 8) | (r << 3) | (g >> 3);
}

int  waterfall_draw(struct field *f){

  screen_fill_rect(f->x, f->y, f->w, 48, TFT_BLACK);
	// clip the bandwidth strip
	int bx = f->x + f->w/2 + bandwidth_start;
	if (bx < f->x)
		bx = f->x;
	int bw = bandwidth_stop - bandwidth_start;
	if (bx + bw > f->x + f->w)
		bw = f->x + f->w - bx; 
	screen_fill_rect(bx, f->y, bw, 48, TFT_BLUE);

	int pitch = center_line + f->x + f->w/2;
	if (f->x < pitch && pitch < f->x + f->w)
		screen_draw_line(pitch, f->y, pitch, f->y + 48, TFT_RED);

  //the values are offset by 32, the space character
  //there are 250 values to be spread out
  int last_y = f->y + 48-waterfall[0];
  for (int i = 1; i <f->w; i++){
    int y_now = f->y + 48 - waterfall[i];
    if(y_now < f->y)
      y_now = f->y;
    screen_draw_line(f->x+i, last_y, f->x+i, y_now, 0x00FFFF00);
    last_y = y_now;
  }

  //the screen is bbbbbrrrrrrggggg
  uint8_t *wf = waterfall;
  double scale = 240.0/f->w;
  uint16_t line[SCREEN_WIDTH];

  // each waterfall line is stored as exactly 240 points wide
  //this has to be stretced or compressed with scale variable 
  for (int j = 48; j < f->h; j++){
    for (int i = 0; i < f->w; i++){
      uint16_t heat = heat_map((uint16_t)wf[(int16_t)(scale * i)]);
      line[i] = heat;
    }
   screen_bitblt(f->x, f->y+j, f->w, 1, line);
   wf += 240;
  }
	return 0;
}

//always 240 values!
int waterfall_update(struct field *f, char *value){
  uint8_t spectrum[250];
  if (f->w > sizeof(spectrum)){
    Serial.println("waterfall is too large");
    return - 1;
   }

  //scale the values to fit the width
  //adjust the offset by space character
  int count = strlen(value);
 	//we take 240 points on the waterfall 
 	//and zzom it in/out
 	double scale = (240.0)/count;
 	for (int i = 0; i < f->w; i++){
   int v = value[(int)(scale * i)]-32;
   spectrum[i] = v;
  }

  //scroll down the waterfall
  int waterfall_length = sizeof(waterfall);
  memmove(waterfall+240, waterfall, waterfall_length-240);
  for (int i = 240; i > 0; i--)
    waterfall[i-1] = spectrum[i];
	return 0;
}

int waterfall_fn(struct field *f, int method, void *ptr){
	switch(method){
	case FIELD_METHOD_DRAW:
		return waterfall_draw(f);
	case FIELD_METHOD_INPUT:
		return -1;
	case FIELD_METHOD_INIT:
		return waterfall_init();
	case FIELD_METHOD_UPDATE:
		return waterfall_update(f, (char *)ptr);
	}
	return -1;
}

