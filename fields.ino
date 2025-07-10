#include <TFT_eSPI.h>       // Hardware-specific library
#include "zbitx.h"
#include "logbook.h"
#include "ft8.h"
#include "text_field.h"
#include "fields_list.h"
#include "console.h"

struct field *f_selected = NULL;
extern int vswr, vfwd, vref, vbatt;


//void field_draw(struct field *f, bool all);

struct field *field_list;

/* Fields are controls that hold radio's controls values, they are also buttons, text fields, multiple selections, etc.
 *  See zbitx.h for the different types of controls
 */

void field_init(){
  int count = 0;

  field_list = main_list;
	ft8_init();
	logbook_init();
	waterfall_init();
  for (struct field *f = field_list; f->type != -1; f++){
    if (count < FIELDS_ALWAYS_ON)
      f->is_visible = true;
    else
      f->is_visible = false;
    f->redraw = true;    
    count++;
		f->update_to_radio = false;
		f->last_user_change= 0;
  }

  screen_text_extents(2, font_width2);
  screen_text_extents(4, font_width4);
}

void field_clear_all(){
  int count = 0;

  for (struct field *f = field_list; f->type != -1; f++){
    if (count < FIELDS_ALWAYS_ON || (f_selected && f_selected->type == FIELD_TEXT && f->type == FIELD_KEY))
      f->is_visible = true;
    else
      f->is_visible = false;
    f->redraw = true;
    count++;
  }
}

struct field *field_get(const char *label){
  for (struct field *f = field_list; f->type != -1; f++)
    if (!strcmp(label, f->label))
      return f;
  return NULL;  
}

void field_post_to_radio(struct field *f){
	f->update_to_radio = true;
	f->last_user_change = now;
}

struct field *dialog_box(const char *title, char const *fields_list){
	char original_mode[30];
	char list[1000];

	struct field *f_original_selection = f_selected;
	strcpy(original_mode, field_get("MODE")->value);
	strcpy(list, fields_list);
		
	//clear every field
  for (struct field *f = field_list; f->type != -1; f++)
		f->is_visible = false;
		
	int last_blink = 0;
  char *p = strtok(list, "/");
	field_set("TITLE", title, false);
	field_show("TITLE", true);	

  while (p){
    field_show(p, true);
    p = strtok(NULL, "/");
		/* if (now > last_blink + BLINK_RATE){
			field_blink(-1);
			last_blink = now;
		}*/
  }
  screen_fill_rect(0,0,SCREEN_WIDTH, SCREEN_HEIGHT,SCREEN_BACKGROUND_COLOR);
	//screen_draw_text(title, -1, 10, 5, TFT_WHITE, 4);
	screen_fill_rect(0,30, SCREEN_WIDTH, 1, TFT_WHITE);	

	struct field *f_touched = NULL;
	f_selected = NULL;

	while(1){	
		f_touched = ui_slice();
		if (f_touched && f_touched->type == FIELD_BUTTON){
			break;	
		}
		delay(10);
	}
  screen_fill_rect(0,0,SCREEN_WIDTH, SCREEN_HEIGHT,SCREEN_BACKGROUND_COLOR);
	field_set_panel(original_mode);
	field_draw_all(1);
	f_selected = f_original_selection;
	return f_touched;
}

void field_set_panel(const char *mode){
  char list[100];
 
  field_clear_all();
	keyboard_hide(); //fwiw
  if (!strcmp(mode, "FT8")){
    strcpy(list,"ESC/F1/F2/F3/F4/F5/TX_PITCH/AUTO/TX1ST/FT8_REPEAT/FT8_LIST/WF");
	}
  else if (!strcmp(mode, "CW") || !strcmp(mode, "CWR")){
    strcpy(list, "ESC/F1/F2/F3/F4/F5/F6/F7/PITCH/WPM/TEXT/CONSOLE/WF");
	}  
  else
    strcpy(list, "MIC/TX/RX/WF/CONSOLE");  

  //clear the bottom row
  screen_fill_rect(0, SCREEN_HEIGHT - 48, SCREEN_WIDTH, 48, SCREEN_BACKGROUND_COLOR);
  //clear the right pane
  screen_fill_rect(240, 48, 240, SCREEN_HEIGHT - 96, SCREEN_BACKGROUND_COLOR);
  
  char *p = strtok(list, "/");
  while (p){
    field_show(p, true);
    p = strtok(NULL, "/");
  }
}

//set from the radio to the front panel
void field_set(const char *label, const char *value, bool update_to_radio){
  struct field *f;

  //translate a few fields 
  if (!strcmp(label, "9") || !strcmp(label, "10") || !strcmp(label, "5"))
    f = field_get("CONSOLE");
  else if (!strcmp(label, "6") || !strcmp(label, "7"))
    f = field_get("FT8_LIST");
	else if (!strcmp(label, "QSO")){
		f = field_get("LOGB");
		logbook_update(value);
	}
  else 
    f = field_get(label);

  if (!f)
    return;

  if (update_to_radio)
		field_post_to_radio(f);

	if (!strcmp(f->label, "IN_TX") || !strcmp(f->label, "RIT") 
		|| !strcmp(f->label, "VFO")){
		if (strcmp(f->value, value)){
			struct field *f = field_get("FREQ");
			f->redraw = true;
		}
	}
	
   //these are messages of FT8
  if(!strcmp(f->label, "FT8_LIST")){
    ft8_update(value);
    f->redraw = true;
  }
	
  //cw decoded text
  else if (!strcmp(f->label, "CONSOLE")){
    console_update(f, label, value);  
    f->redraw = true;
  }
  else if (!strcmp(f->label, "WF")){
    uint8_t spectrum[250];
    if (f->w > sizeof(spectrum)){
      Serial.println("#waterfall is too large");
      return;
    }
    //scale the values to fit the width
    //adjust the offset by space character
    int count = strlen(value);
    //we take 240 points on the waterfall 
    //and zzom it in/out
    double scale = count/240.0;
    for (int d = 0; d < f->w; d++){
			int i = (scale * d);
      int v = value[i]-32;
      spectrum[d] = v;
    }
    //always 250 points
    waterfall_update(f, spectrum);
  }
  //else if (strlen(value) < FIELD_TEXT_MAX_LENGTH - 1){
	else {
    if (!strcmp(label, "MODE"))
      field_set_panel(value);
    strcpy(f->value, value);
  }
  f->redraw = true;
}

void field_show(const char *label, bool turn_on){
  struct field *f = field_get(label);
  if(!f){
      Serial.printf("#%s field not found", label);
      return;
  }
  if (turn_on)
    f->is_visible = true;
  else
    f->is_visible = false;
  f->redraw = true;
}

struct field *field_at(uint16_t x, uint16_t y){
  for (struct field *f = field_list; f->type != -1; f++)
      if (f->x < x && x  < f->x-2 + f->w && f->y < y && y < f->y + f->h -2 && f->is_visible){
        return f; 
      }
  return NULL;    
}

//this is the user selecting a field (or a simulated touch on the field)
struct field *field_select(const char *label){
  struct field *f = field_get(label);

  //if you have hit outside a field, then deselect the last one anyway
  if (!f){
    f_selected = NULL;
    return NULL;
  }

	if (!strcmp(f->label, "WF")){
		if (!strcmp(f->value, "ON"))
			strcpy(f->value, "OFF");
		else
			strcpy(f->value, "ON");
		f->update_to_radio = true;
	}

	//some fields should not get focus anyway 
	if (!strcmp(f->label, "WF") || !strcmp(f->label, "CONSOLE"))
		return NULL;

	if (!strcmp(f->label, "MENU")){
		dialog_box("Radio", "10M/12M/15M/17M/20M/30M/40M/60M/80M/AGC/VFO/SPLIT/CLOSE/SHUTDOWN");
		return NULL;
	}

	if (!strcmp(f->label, "SET")){
		dialog_box("Settings", "MY CALL/MYCALLSIGN/MY GRID/MYGRID/PASS KEY/PASSKEY/CW_INPUT/CW_DELAY/SIDETONE/CLOSE");
		return NULL;
	}

	if (!strcmp(f->label, "[x]")){
		keyboard_hide();
	}

	//this can get recusrive, it is calling field_select() again
  Serial.print("KHK: "), Serial.println(f->label);

	if (!strcmp(f->label, "OPEN")){
		f_selected = NULL;
		field_post_to_radio(f);
		logbook_init();
		struct field *f = dialog_box("Logbook", "LOGB/FINISH");
		return NULL;
	}
	else if (!strcmp(f->label, "FINISH")){
		f_selected = NULL;
	}
	else if (!strcmp(f_selected->label, "SAVE")){
		logbook_init();
	}
    else if (!strcmp(f->label, "SHUTDOWN")) {
      f_selected = NULL;
      struct field *f = dialog_box("Continue with Shutdown?", "CANCEL/CONFIRM");
      Serial.println("Checking for confirmation");
      f_selected = NULL;
      return NULL;
    }
    else if (!strcmp(f->label, "CONFIRM")) {
      f_selected = NULL;
      Serial.println("Yup, shutdown the radio");
      f->label = "SHUTDOWN";
      field_post_to_radio(f);
      dialog_box("Please wait...", "WAITFORSHUTDOWN");
  	  f_selected = NULL;
      return NULL;
  	}

/*
	if (!strcmp(f_selected->label, "VFO")){
		if (!strcmp(f_selected->value, "A"))
			field_set("FREQ", field_get("VFOA")->value, true);
		else if (!strcmp(f_selected->value, "B"))
			field_set("FREQ", field_get("VFOB")->value, true);
	}
	*/

  if (f_selected)
    f_selected->redraw = true; // redraw without the focus
  
  if (f->type == FIELD_KEY){
		text_input(f);
    return NULL;
  } 
    
  //redraw the deselected field again to remove the focus
	if (f->type != FIELD_BUTTON && f->type != FIELD_KEY){
  	f_selected = f;
  	f->redraw = true;
	}
	else
		f_selected = NULL;

  //if a selection field is selected, move to the next selection
  if(f->type == FIELD_SELECTION){
    char *p, *prev, *next, b[100], *first, *last;
    // get the first and last selections
    strcpy(b, f->selection);
    p = strtok(b, "/");
    first = p;
    //search the current text in the selection
    prev = NULL;
    strcpy(b, f->selection);
    p = strtok(b, "/");
    while(p){
      if (!strcmp(p, f->value))
        break;
      else
        prev = p;
      p = strtok(NULL, "/");
    }
    //set to the first option
    if (p == NULL){
      if (first)
        strcpy(f->value, first);
    }
    else { //move to the next selection
      p = strtok(NULL,"/");
      if (p)
        strcpy(f->value, p);
      else
        strcpy(f->value, first); // roll over
    }
  }
  else if (f->type == FIELD_TEXT){
    keyboard_show(EDIT_STATE_UPPER);
  }
	else if (f->type == FIELD_BUTTON){
		field_input(ZBITX_BUTTON_PRESS);
	}

  // emit the new value of the field to the radio
	field_post_to_radio(f);
  return f;
}

struct field *field_get_selected(){
  return f_selected;
}


/* Field : FREQ */
char* freq_with_separators(const char* freq_str){

  int freq = atoi(freq_str);
  int f_mhz, f_khz, f_hz;
  char temp_string[11];
  static char return_string[11];

  f_mhz = freq / 1000000;
  f_khz = (freq - (f_mhz*1000000)) / 1000;
  f_hz = freq - (f_mhz*1000000) - (f_khz*1000);

  sprintf(temp_string,"%d",f_mhz);
  strcpy(return_string,temp_string);
  strcat(return_string,".");
  if (f_khz < 100){
    strcat(return_string,"0");
  }
  if (f_khz < 10){
    strcat(return_string,"0");
  }
  sprintf(temp_string,"%d",f_khz);
  strcat(return_string,temp_string);
  strcat(return_string,".");
  if (f_hz < 100){
    strcat(return_string,"0");
  }
  if (f_hz < 10){
    strcat(return_string,"0");
  }
  sprintf(temp_string,"%d",f_hz);
  strcat(return_string,temp_string);
  return return_string;
}

void freq_draw(){
  struct field *f = field_get("FREQ");
  struct field *rit = field_get("RIT");
  struct field *split = field_get("SPLIT");
  struct field *vfo = field_get("VFO");
  struct field *vfo_a = field_get("VFOA");
  struct field *vfo_b = field_get("VFOB");
  struct field *rit_delta = field_get("RIT_DELTA");

  char buff[20];
  char temp_str[20];

  //update the vfos
  if (vfo->value[0] == 'A')
      strcpy(vfo_a->value, f->value);
  else
      strcpy(vfo_b->value, f->value);

	if (!strcmp(split->value, "ON")){
    if (!in_tx()){
      strcpy(temp_str, vfo_a->value);
      sprintf(buff, "R:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+19, TFT_CYAN, ZBITX_FONT_LARGE);
    }
    else {
      strcpy(temp_str, vfo_b->value);
      sprintf(buff, "T:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+19, TFT_CYAN, ZBITX_FONT_LARGE);
    }
  }
  else if (!strcmp(rit->value, "ON")){
    if (!in_tx()){
      sprintf(temp_str, "%d", (atoi(f->value) + atoi(rit_delta->value)));
      sprintf(buff, "R:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+19, TFT_CYAN, ZBITX_FONT_LARGE);
    }
    else {
      sprintf(buff, "T:%s", freq_with_separators(f->value));
      screen_draw_text(buff, -1, f->x+5 , f->y+19, TFT_CYAN, ZBITX_FONT_LARGE);
    }
  }
  else if (!strcmp(vfo->value, "A")){
    if (!in_tx()){
      /*strcpy(temp_str, vfo_b->value);
      sprintf(buff, "B:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+1, TFT_BLACK, ZBITX_FONT_LARGE);*/
      sprintf(buff, "A:%s", freq_with_separators(f->value));
      screen_draw_text(buff, -1, f->x+5 , f->y+19, TFT_CYAN, ZBITX_FONT_LARGE);
    } else {
      /*strcpy(temp_str, vfo_b->value);
      sprintf(buff, "B:%s", freq_with_separators(temp_str));
      screen_draw_text(buff,  -1,f->x+5 , f->y+1, TFT_BLACK, ZBITX_FONT_LARGE);*/
      sprintf(buff, "TX:%s", freq_with_separators(f->value));
      screen_draw_text(buff,  -1,f->x+5, f->y+19, TFT_CYAN, ZBITX_FONT_LARGE);
    }
  }
  else{ /// VFO B is active
    if (!in_tx()){
      strcpy(temp_str, vfo_a->value);
      //sprintf(temp_str, "%d", vfo_a_freq);
      sprintf(buff, "A:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+1, TFT_BLACK, ZBITX_FONT_LARGE);
      sprintf(buff, "B:%s", freq_with_separators(f->value));
      screen_draw_text(buff, -1, f->x+5 , f->y+19, TFT_CYAN, ZBITX_FONT_LARGE);
    }else {
      strcpy(temp_str, vfo_a->value);
      //sprintf(temp_str, "%d", vfo_a_freq);
      sprintf(buff, "A:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+1, TFT_BLACK, ZBITX_FONT_LARGE);
      sprintf(buff, "TX:%s", freq_with_separators(f->value));
      screen_draw_text(buff, -1, f->x+5 , f->y+19, TFT_CYAN, ZBITX_FONT_LARGE);
    }
  }
}


void field_draw_cursor(uint16_t x, int y){
  if (!f_selected)
    return;
  if (f_selected->type != FIELD_TEXT)
    return;
  if ((now % 600) > 300)
    screen_fill_rect(f_selected->x+3+screen_text_width(f_selected->value, ZBITX_FONT_NORMAL)+1, f_selected->y+4, 
      screen_text_width("J", ZBITX_FONT_NORMAL), screen_text_height(ZBITX_FONT_NORMAL)-3, TFT_WHITE);
  else 
    screen_fill_rect(f_selected->x+3+screen_text_width(f_selected->value, ZBITX_FONT_NORMAL)+1, f_selected->y+4,
      screen_text_width("J", ZBITX_FONT_NORMAL), screen_text_height(ZBITX_FONT_NORMAL)-3, TFT_BLACK);
}


void smeter_draw(struct field *f){
	char temp_str[100];
	static int count = 0;

	if (count++ % 25)
		return;

	screen_fill_rect(f->x, f->y, f->w, f->h, TFT_BLACK);

	int v = vbatt/10;
	sprintf(temp_str, "+%d.%dv", v/10, v%10);
	screen_draw_text(temp_str, -1, f->x + 155, f->y+1, TFT_WHITE, 1);

	struct field *f_tx = field_get("IN_TX");
	if (!f_tx){
		Serial.println("#IN_TX field is missing");
		return;
	}
	int in_tx = atoi(f_tx->value);
	 if (in_tx){
		sprintf(temp_str, "%d W            SWR %d.%d", vfwd/10, vswr/10, vswr%10); 
		screen_draw_text(temp_str, -1, f->x + 3, f->y + 1, TFT_WHITE, 2);
		screen_draw_rect(f->x + 33,  f->y + 2, 60, 12, TFT_YELLOW);
		screen_fill_rect(f->x + 34,  f->y + 3, vfwd, 10, TFT_RED);
		return;
	}

	int s = atoi(field_get("SMETER")->value)/200;
	for (int i = 0 ; i < 6; i++){
		int color = TFT_DARKGREY;
		if (s >= i){
			switch(i){
				case 0: color=TFT_YELLOW;break;
				case 1: color=TFT_YELLOW;break;
				case 2: color=TFT_GREEN;break;
				case 3: color=TFT_GREEN;break;
				case 4: color=TFT_GREEN;break;
				case 5: color=TFT_ORANGE;break;
				case 6: color=TFT_RED;break;
				case 7: color=TFT_RED;break;
			}	
		}
		screen_fill_rect(f->x + 5 + (i * 20), f->y+1, 18, 5, color);
		temp_str[1] = 0;
		if (i < 5)
			temp_str[0] = '1' + (i *2);
		else
			strcpy(temp_str, "+20");
		screen_draw_text(temp_str, -1, f->x + 5 + (i * 20), f->y+7, TFT_WHITE, 1);
		//screen_draw_text(temp_str, -1, f->x + 130, f->y+6, TFT_WHITE, 1);
	}
}

void field_static_draw(field *f){
	char *p, text_line[FIELD_TEXT_MAX_LENGTH];

	p = f->value;
	int y = f->y;
	int i = 0;
	while(*p){	
		if (*p == '\n'){
			text_line[i] = 0;
			screen_draw_text(text_line, -1, f->x, y, TFT_WHITE, 2);
			y += 18;
			i = 0;
		}
		else if (*p != '\n' && i < sizeof(text_line) -1)
			text_line[i++] = *p;
		p++;
	}
	if (i > 0){
		text_line[i] = 0;
		screen_draw_text(text_line, -1, f->x, y, TFT_WHITE, 2);
	}
}

void field_title_draw(field *f){
	screen_draw_text(f->value, -1, f->x,  f->y, TFT_WHITE, 4);
}

void field_draw(struct field *f){
	struct field *f2;
	if (f->draw != NULL){
		f->draw(f);
		return;
	}
	if (f->type != FIELD_WATERFALL && f->type != FIELD_FT8 && f->type != FIELD_LOGBOOK && f->type != FIELD_KEY
		&& f->type != FIELD_STATIC && f->type != FIELD_TITLE &&
		f->type != FIELD_CONSOLE){
    //skip the background fill for the console on each character update
    screen_fill_round_rect(f->x+2, f->y+2, f->w-4, f->h-4, f->color_scheme);
    if (f == f_selected /*|| f->type == FIELD_KEY*/)
      screen_draw_round_rect(f->x+2, f->y+2, f->w-4, f->h-4, TFT_WHITE);
  }
  switch(f->type){
    case FIELD_WATERFALL:
      waterfall_draw(f);
      break;
    case FIELD_KEY:
			key_draw(f);
      break;
    case FIELD_FREQ:
      freq_draw();
      break;
    case FIELD_TEXT:
			text_draw(f);
      break;
		case FIELD_STATIC:
			field_static_draw(f);
			break;
		case FIELD_TITLE:
			field_title_draw(f);
			break;
    case FIELD_CONSOLE:
      console_draw(f);
      break;
    case FIELD_FT8:
      ft8_draw(f);
      break;
		case FIELD_LOGBOOK:
			logbook_draw(f);
			break;
		case FIELD_SMETER:
			//smeter_draw(f);
			return; // don't fall into the default background painting
    default:
			{
			const char *plabel = strchr(f->label, '_');
			if (!plabel)
				plabel = f->label;
			else 
				plabel++; //advance beyond the '_'	
			
      if (!strlen(f->value))
        screen_draw_text(plabel, -1, (f->x)+8, (f->y)+15, TFT_WHITE, ZBITX_FONT_NORMAL);
      else 
        screen_draw_text(plabel, -1, (f->x)+8, (f->y)+6, TFT_CYAN, ZBITX_FONT_NORMAL);

      screen_draw_text(f->value, -1, (f->x)+8, f->y + 24, 
        TFT_WHITE, ZBITX_FONT_NORMAL);
			}
      break;
  }
}

// some input to the field
// in many cases (pressing a button), the focus may not be the field
void field_input(uint8_t input){

  if (!f_selected)
    return;

	// handle some of the buttons locally rather than propage them 
	// to the radio
	if (f_selected->type == FIELD_FT8){
		ft8_input(input);
  	f_selected->redraw = true; 
		return;
	}
  else if(f_selected->type == FIELD_SELECTION){
    char *p, *last, *next, b[100], *first;

    strcpy(b, f_selected->selection);
    p = strtok(b, "/");
    first = p;
    //search the current text in the selection
    last = NULL;
    while(p){
      if (!strcmp(p, f_selected->value))
        break;
      else
        last = p;
      p = strtok(NULL, "/");
    }
    if (p == NULL){
      if (first)
        strcpy(f_selected->value, first);
    }
    else if (input == ZBITX_KEY_DOWN){
      if (last){
        strcpy(f_selected->value, last);
      }
      else{
        while (p){
          p = strtok(NULL, "/");
          if (p)
            last = p;
        }
        strcpy(f_selected->value, last);
      }
      //REMOVE THIS IN PRODUCTION
      if (!strcmp(f_selected->label, "MODE")){
        field_set_panel(f_selected->value);
			}
    }
    else if (input == ZBITX_KEY_UP){
      //move to the next selection
      p = strtok(NULL,"/");
      if (p)
        strcpy(f_selected->value, p);
      else
        strcpy(f_selected->value, first); // roll over

      //REMOVE THIS IN PRODUCTION!
      if (!strcmp(f_selected->label, "MODE"))
        field_set_panel(f_selected->value);
    }
  } 
  else if (f_selected->type == FIELD_NUMBER){
    char buff[100];
    int min, max, step, v;
    strcpy(buff, f_selected->selection);
    v = atoi(f_selected->value);
    min = atoi(strtok(buff, "/"));
    max = atoi(strtok(NULL,"/"));
    step = atoi(strtok(NULL, "/"));
    if (v + step <= max && input == ZBITX_KEY_UP)
      v += step;
    else if (v - step >= min && input == ZBITX_KEY_DOWN)
      v -=  step;
    sprintf(f_selected->value, "%d", v);
  }
  else if (f_selected->type == FIELD_FREQ){
    char buff[100];
    int min, max, step, v;
    strcpy(buff, f_selected->selection);
    v = atoi(f_selected->value);
    min = atoi(strtok(buff, "/"));
    max = atoi(strtok(NULL,"/"));
    char *s = field_get("STEP")->value;
    if (!strcmp(s, "1K"))
      step = 1000;
    else if (!strcmp(s, "10K"))
      step = 10000;
    else if (!strcmp(s, "100H"))
      step = 100;
    else if (!strcmp(s, "500H"))
      step = 500;  
    else if (!strcmp(s, "10H"))
      step = 10;
    else
      step = 1;  
    
		struct field *f_rit = field_get("RIT");
		if (!f_rit){
			Serial.println("#RIT field is missing");
			return;
		}

		//on rit, we only increase the rit delta
		if (!strcmp(f_rit->value, "ON")){
			struct field *f_rit_delta = field_get("RIT_DELTA");
			int rit_delta = atoi(f_rit_delta->value);
			if (-25000 < rit_delta - step && rit_delta + step < 25000){
				if (input == ZBITX_KEY_UP)
					rit_delta += step;
				else if (input == ZBITX_KEY_DOWN)
					rit_delta -= step;    
			}
			else //reset the rit to zero 
				rit_delta = 0;

			field_post_to_radio(f_rit_delta);
			sprintf(f_rit_delta->value, "%d", rit_delta);
		}
		else { //normal case
			v = (v/step)*step;
			if (v + step <= max && input == ZBITX_KEY_UP)
				v += step;
			else if (v - step >= min && input == ZBITX_KEY_DOWN)
				v -=  step;    
    sprintf(f_selected->value, "%d", v);
		}
  }
	else if (f_selected->type == FIELD_LOGBOOK)
		logbook_input(input);
	//this propagates all buttons to the radio
	field_post_to_radio(f_selected);
  f_selected->redraw = true;
}

void field_draw_all(bool all){
  struct field *f;

  if (all)
    screen_fill_rect(0,0,SCREEN_WIDTH, SCREEN_HEIGHT,SCREEN_BACKGROUND_COLOR);
	
  for (f = field_list; f->type != -1; f++)
    if ((all || f->redraw) && f->is_visible){
			if (edit_mode == -1 || f->y + f->h < 144 || f->type == FIELD_KEY
				|| f_selected == f){
      	field_draw(f);
      	f->redraw = false;
			}
    }
	f = field_get("METERS");
	smeter_draw(f);
}
