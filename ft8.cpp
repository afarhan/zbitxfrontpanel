#include <TFT_eSPI.h>       // Hardware-specific library
#include "zbitx.h"
#include "logbook.h"

#define FT8_MAX 100 
struct ft8_message ft8_list[FT8_MAX];
void field_ft8_append(const char *msg);
void ft8_move_cursor(int by);
int ft8_next = 0;
int ft8_cursor = 0;
int ft8_id = 1;
unsigned long ft8_cursor_timeout = 0;
int ft8_top = 0;
unsigned long last_ft8_cursor_movement = 0;

void ft8_init(){
  memset(ft8_list, 0, sizeof(ft8_list));
	ft8_next = 0;
	ft8_cursor = 0;
	ft8_id = 1;
	ft8_cursor_timeout = 0;
	ft8_top = 0;
	last_ft8_cursor_movement = 0;
}

void ft8_select(){
	char buff[200];

	struct ft8_message *m = ft8_list + ft8_cursor;
	sprintf(message_buffer, "FT8 %s\n", m->data);
}

void ft8_update(const char *msg){
  //#G121145  16 -16 1797 ~ #GDG5YPR #RIZ2FOS #SJN55
  char buff[100], *p;

  struct ft8_message *m = ft8_list + ft8_next;
  
  strcpy(buff, msg);

  p = strtok(buff, " ");
  if(!p)return;  

  p = strtok(NULL, " "); //skip the confidence score
  p = strtok(NULL, " ");
  if (!p) return;
  m->signal_strength = atoi(p);
 
  p = strtok(NULL, " ");
  if (!p) return;
  m->frequency = atoi(p);
  m->id = ft8_id++; 
  
  p = strchr(msg, '~');
  if (!p)
    return;

  p+= 2; //skip the tilde and the next space

  if (strlen(p) >= FT8_MAX_DATA){
    return;
  }
  strcpy(m->data, msg);  
  ft8_next++;
  if (ft8_next >= FT8_MAX)
		ft8_next = 0;
	if (ft8_next == ft8_cursor)
		ft8_cursor = -1;
}

//by can be -ve
int ft8_new_index(int from, int by){
	int next = from + by;
	if (next < 0)
		next += FT8_MAX;
	if (next >= FT8_MAX)
		next -= FT8_MAX;
	return next;
}

void ft8_move_cursor(int by){
	int new_cursor = -1;

	//if no message was selected, we just pick the last received
	if (ft8_cursor == -1){
		new_cursor = ft8_new_index(ft8_next, -1);
	}
	if(by < 0){
		new_cursor = ft8_new_index(ft8_cursor, -1); 
		if (new_cursor == ft8_next|| ft8_list[new_cursor].id == 0 ||
			ft8_list[new_cursor].id > ft8_list[ft8_cursor].id) 
			new_cursor = ft8_cursor; 
	}
	else if (by > 0){
		new_cursor = ft8_new_index(ft8_cursor, +1);
		if (new_cursor == ft8_next || ft8_list[new_cursor].id == 0 ||
			ft8_list[new_cursor].id < ft8_list[ft8_cursor].id) 
			new_cursor = ft8_cursor;
	}
	//check that it is a valid message (ids start from 1, not zero)
	if (ft8_list[new_cursor].id > 0)
		ft8_cursor = new_cursor;
	last_ft8_cursor_movement = millis();
}

void ft8_draw(field *f){
  int count = f->h / screen_text_height(2);

	if (last_ft8_cursor_movement + 30000 < millis())
		ft8_cursor = -1;

	//Serial.printf("top: %d, cursor:%d, next %d\n", ft8_top, ft8_cursor, ft8_next);
	//display all the latest fields
	if (ft8_cursor == -1){
		//Adjust the top to show last messages
		ft8_top = ft8_new_index(ft8_next, -count);
	//	Serial.printf("Adjusting unselected ft8_top to %d\n", ft8_top);
	}
	else {
		if (ft8_cursor >= 0 && ft8_list[ft8_cursor].id < ft8_list[ft8_top].id && ft8_list[ft8_cursor].id > 0){
			ft8_top = ft8_cursor;
		//	Serial.print("BACK ");
		}
		else if (ft8_cursor >= 0 && ft8_list[ft8_cursor].id > ft8_list[ft8_new_index(ft8_top, count - 1)].id){
			ft8_top = ft8_new_index(ft8_cursor, -count + 1);
			Serial.print("FWD ");
		}
//		Serial.printf("Adjusted cursor to show with ft8_top at %d\n", ft8_top);
	}

	screen_fill_rect(f->x, f->y, f->w, f->h, TFT_BLACK);
  int index = ft8_top ; //ft8_next - count;
  if (index < 0)
    index += FT8_MAX;

  for (int i=0; i < count; i++){
    char buff[100], *p;
    int x = f->x+2;
		if (ft8_list[index].id){
			char slot = '0';
			char slot1 = ft8_list[index].data[6];
			char slot2 = ft8_list[index].data[7];
			if (slot1 == '0' && slot2 == '0')
				slot = '1';
			else if (slot1 == '1' && slot2 == '5')
				slot = '2';
			else if (slot1 == '3' && slot2 == '0')
				slot = '3';
			else
				slot = '4';
    	strcpy(buff+3, ft8_list[index].data + 12);
			buff[0] = '#';
			buff[1] = 'G';
			buff[2] = slot;
 	   for (char *p = strtok(buff, "#"); p; p = strtok(NULL, "#")){
  	    //F=white G=Green R=Red, S=Orange
    	  uint16_t color = TFT_WHITE;
      	switch(*p){
   	   case 'G':
    	    color = TFT_GREEN;
      	  break;
      	case 'R':
        	color = TFT_CYAN;
        	break;
   	   case 'S':
    	    color = TFT_YELLOW;
      	  break;
    	  default:
      	  color= TFT_WHITE;
        	break;
      	}
      	screen_draw_text(p+1, -1, x, f->y + (screen_text_height(2) * i), color, 2);
      	x += screen_text_width(p+1,2);
      	//Serial.print("|");Serial.print(p);
				if(index == ft8_cursor && f == f_selected)
      		screen_draw_rect(f->x+2, f->y + (screen_text_height(2) * i), f->w - 4, 16, TFT_WHITE);
    	}
    	//Serial.println("done");
    }
    
    index++;
    if (index >= FT8_MAX)
      index = 0;
  } 
}

void ft8_input(int input){
	if (input == ZBITX_KEY_DOWN){
		ft8_move_cursor(+1);
	}
	else if (input == ZBITX_KEY_UP)
		ft8_move_cursor(-1);
	else if (input == ZBITX_KEY_ENTER){
		Serial.println("Ft selecting");
		ft8_select();
	}
}
