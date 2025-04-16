#include <Arduino.h>
#include <TFT_eSPI.h>       // Hardware-specific library
#include "zbitx.h"
#include "text_field.h"

//keyboard mode
int8_t edit_mode = -1;
static char last_key = 0;

static int text_streaming = 0;

/* keyboard routines */

static uint8_t edit_state = EDIT_STATE_ALPHA;

static int measure_text(const char *text, int font){
	uint16_t *exts;
	if (font == ZBITX_FONT_LARGE)
		exts = font_width4;
	else if (font == ZBITX_FONT_NORMAL)
		exts = font_width2;
	else
		return 0;

	int width = 0;
	while(*text)
		width += exts[*text++];
	return width;
}

void key_draw(struct field *f){
	int background_color = TFT_SKYBLUE;
	int text_color = TFT_BLACK;
	char text[30];

	if (f->label[0] == '#' && f->label[0] == 0)
		return;

	if (!strcmp(f->label, "del"))
		background_color = TFT_RED;
	else if (!strcmp(f->label, "[x]")){
		background_color = TFT_BLACK;
		text_color = TFT_WHITE;
	}
	else if (!strcmp(f->label, "Start")){
		if (!text_streaming)
			background_color = TFT_YELLOW;
	}
	else if (!strcmp(f->label, "Stop")){
		if (text_streaming)
			background_color = TFT_YELLOW;
	}

	int x = f->x + f->w/2;
	if (edit_state == EDIT_STATE_SYM){
	 x -= measure_text(f->value, ZBITX_FONT_LARGE)/2;
		if (strlen(f->value) == 2 && f->value[0] == 'F')
			background_color = TFT_GREEN;
	}
	else
	 x -= measure_text(f->label, ZBITX_FONT_LARGE)/2;
		
	screen_fill_round_rect(f->x+2, f->y+2, f->w-4, f->h-4, background_color);

  if (strlen(f->value)){
    if(edit_state == EDIT_STATE_SYM)
    	screen_draw_text(f->value, -1, x, (f->y)+9, text_color, ZBITX_FONT_LARGE);
		else 
    	screen_draw_text(f->label, -1, x, (f->y)+9, text_color, ZBITX_FONT_LARGE);
	}
  else 
    screen_draw_text(f->label, -1, x, (f->y)+9, text_color, ZBITX_FONT_LARGE);
}

void keyboard_redraw(){
  struct field *f;
  for (f = field_list; f->type != -1; f++)
    if (f->type == FIELD_KEY)
      	f->redraw = true;
}

char keyboard_read(struct field *key){
  uint16_t x, y;
  
  if (!key)
    return 0;
  char c = 0;
  if(!strcmp(key->label, "[x]")){
		keyboard_hide();
		return 0;
	}


	if (edit_state == EDIT_STATE_SYM){
		if ( key->value[0] == 'F' && isdigit(key->value[1])){
			struct field *f = field_select(key->value);
			if (f)
				field_post_to_radio(f);
			return 0;
		}
		else if (!strcmp(key->value, "AR"))
			return '+';
		else if (!strcmp(key->value, "BT"))
			return '&';
	}

  if (!strcmp(key->label, "space"))
    c = ' ';
	else if (!strcmp(key->label, "Start")){
		struct field *f = field_get("Stop");
		if (f)
			f->redraw = 1;
		text_streaming = 1;
		key->redraw = 1;
		field_select("TEXT");
	}
	else  if (!strcmp(key->label, "Stop")){
		struct field *f = field_get("Start");
		if (f)
			f->redraw = 1;
		text_streaming = 0;
		key->redraw = 1;
		//clear text
		field_set("TEXT", "", true);		
	}
  else if (!strcmp(key->label, "Sym")){
		if (edit_state == EDIT_STATE_SYM)
			edit_state = EDIT_STATE_ALPHA;
		else 
    	edit_state = EDIT_STATE_SYM;
		keyboard_redraw();		
		return 0;
	}
  else if (!strcmp(key->label, "del"))
    c = 8;
  else {
    if (edit_state == EDIT_STATE_SYM || edit_mode == EDIT_STATE_SYM)
      c = key->value[0];
    else if (edit_state == EDIT_STATE_UPPER || edit_mode == EDIT_STATE_UPPER)
      c = toupper(key->label[0]);
    else 
      c = tolower(key->label[0]);
  } 
  delay(10); // debounce for 10 msec    

  last_key = c;
  return c;
}

void keyboard_show(uint8_t mode){
  edit_mode = mode;
	struct field *f;

  screen_fill_rect(0, 128, 480, 192, TFT_BLACK);

  for (f = field_list; f->type != -1; f++)
		if (f->type == FIELD_KEY)
			field_show(f->label, true);
	
	if (f_selected){
		if (strcmp(f_selected->label, "TEXT")){
			field_show("Start", false);
			field_show("Stop", false);
		}
	}
	field_draw_all(true);
}

void keyboard_hide(){
	struct field *f;

  for (f = field_list; f->type != -1; f++)
		if (f->type == FIELD_KEY)
			field_show(f->label, false);

	if (edit_mode == -1)
		return;
  edit_mode = -1;
	field_draw_all(true);
}

char *text_editor_get_visible(struct field *f){
	char *p = f->value + strlen(f->value);
	int ext = 0;
	do {
		ext += font_width2[*p];
		if (ext >= f->w - 10)
			break;
		p--;
	}while(p >= f->value);
	return p;
}

int cursor_on = 0;
void field_blink(int blink_state){
	if (!f_selected)
		return;
	if (f_selected->type != FIELD_TEXT)
		return;

	if (blink_state == 0)
		cursor_on = true;
	else if (blink_state == 1)
		cursor_on = false;
	else {
		if (cursor_on)
			cursor_on = 0;
		else
			cursor_on = 1;
	}
		
	char *p = text_editor_get_visible(f_selected);
	if (cursor_on){		
    screen_fill_rect(f_selected->x+3+measure_text(p, ZBITX_FONT_NORMAL)+1, 
			f_selected->y+4,1, 
    	screen_text_height(ZBITX_FONT_NORMAL)-3, TFT_BLACK);
	}
	else {
    screen_fill_rect(f_selected->x+3+measure_text(p, ZBITX_FONT_NORMAL)+1, f_selected->y+4,1, 
    	screen_text_height(ZBITX_FONT_NORMAL)-3, TFT_WHITE);
	}
}

void text_draw(struct field *f){

	char *p = text_editor_get_visible(f);

  if (!strlen(f->value))
    screen_draw_text(f->label, -1, (f->x)+4, (f->y)+4, TFT_CYAN, ZBITX_FONT_NORMAL);
  else
    screen_draw_text(p, -1, (f->x)+4, (f->y)+3, TFT_WHITE, ZBITX_FONT_NORMAL);

	if (f == f_selected)
		field_blink(1);
	else
		field_blink(0);
}

void static field_text_editor(char keystroke){
  if (!f_selected)
    return;
  
  if (f_selected->type != FIELD_TEXT)
    return;

  int l = strlen(f_selected->value);
  if (keystroke == 8){
		if(l > 0) //backspace?
    	f_selected->value[l-1] = 0;
	}
  else if (l < FIELD_TEXT_MAX_LENGTH - 1){        
    f_selected->value[l] = keystroke;
    f_selected->value[l+1] = 0;
  }
  f_selected->redraw = true;
	field_post_to_radio(f_selected);
}

void text_input(struct field *key){

		if (f_selected->type != FIELD_TEXT)
			return;

    char c = keyboard_read(key);
    last_key = c;
		if (c > 0){
    	field_text_editor(c);
			//hold updating to radio if the streaming is turned off
			if (text_streaming == 0 && !strcmp(f_selected->label, "TEXT"))
				f_selected->update_to_radio = false;
			f_selected->redraw = true;
		}
}

