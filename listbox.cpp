#include <Arduino.h>
#include <TFT_eSPI.h>
#include "zbitx.h"
#include "structures.h"
#include "listbox.h"

//the list box structure remains private to the implementation

struct listbox{
	int top_index;
	int selection; 
	int sort_style = 0;
	struct node *list;
	//this is called whenever a listbox item has been selected
	void(*notify)(struct field *f, int notification, void *ptr);
};

void lb_test();

int lb_reset(struct field *f){
	struct listbox *lb = (struct listbox *)f->value;	
	int count = 0;
	
	struct node *p = lb->list;
	while(p){
		struct node *q = p->next;
		node_free(p);
		p = q;
		count++;
	}
	lb->top_index = 0;
	lb->selection = 0;
	lb->list= NULL;		

	return count;
}

int lb_init(struct field *f, void(*notify)(struct field *f, int notification, void *ptr)) {
	struct listbox *lb = (struct listbox *)f->value;	

	lb->top_index = 0;
	lb->selection = 0;
	if (lb->list)
		lb_reset(f);
	lb->list = NULL;
	lb->notify = notify;

/*
	if(!strcmp(f->label, "CONTACTS")){
		lb_insert(f, "#RVU2XZ", -1);
		lb_insert(f, "#RVU3VWR", -1);
		lb_insert(f, "#WW7PUA", -1);
		lb_insert(f, "#WVU2DXA", -1);
		lb_dump(f);
	}

	
	if (!strcmp(f->label, "MESSAGES")){
		lb_insert(f, "#S   VU3VWR #D2025/04/05 0830:#\n#W   hi,meet 10:30?", -1);
		lb_insert(f, "#SVU2ESE #D2025/04/05 0853:#\n#Wr.bring zbix\n", -1);
		lb_insert(f, "#S    VU3VWR #D2025/04/05 0830:#\n#W   car broke down. shall we meet tmrw at 11 am?", -1);
	}
*/
	return 0;
}

static struct node *lb_node_at(struct field *f, int index){
	struct listbox *lb = (struct listbox *)f->value;	
	if (f->type != FIELD_LISTBOX)
		return NULL;

	int count = 0;
	struct node *p = lb->list;
	while (p){
		if (count == index){
			//Serial.printf("node at %d is %d\n", index, p->id);
			return p;
		}
		count++;
		p = p->next;
		/* if (p)
			Serial.printf("moved to %d\n", p->id); */
	}
	return NULL;
}

int lb_count(struct field *f){
	struct listbox *lb = (struct listbox *)f->value;	
	if (f->type != FIELD_LISTBOX)
		return -1;

	int count = 0;
	for (struct node *p = lb->list; p; p = p->next)
		count++;
	return count;
}

int lb_insert(struct field *f, const char *update_str, int index){
	struct listbox *lb = (struct listbox *)f->value;	
	if (f->type != FIELD_LISTBOX)
		return -1;

	//allocate a new node and string for it
	const char *p = theap_dup(update_str);
	if (!p)
		return -1;
	struct node *pnew = node_alloc();
	if (!pnew)
		return -1;
	pnew->ptr = (void *)p;

	struct node *list = lb->list;
	if (index == -1)
		index = lb_count(f);
	struct node *prev = lb_node_at(f, index - 1);

	if (prev == NULL){
		pnew->next = lb->list;	
		lb->list = pnew;
	}
	else {
		pnew->next = prev->next;
		prev->next = pnew;
	}
	return 0;
}

const char *lb_string_at(struct field *f, int index){
	struct listbox *lb = (struct listbox *)f->value;	
	if (f->type != FIELD_LISTBOX)
		return NULL;

	struct node *p = lb_node_at(f, index);
	if (p) 
			return (char *)p->ptr;		
	return NULL;
}

void lb_remove(struct field *f, int index){
	struct listbox *lb = (struct listbox *)f->value;	
	if (f->type != FIELD_LISTBOX)
		return;

	if (index == 0){
		if (!lb->list)
			return;
		struct node *p = lb->list;
		struct node *next = lb->list->next;
		theap_free((char *)p->ptr);
		node_free(p);
		lb->list = next;
		return;
	}

	struct node *prev = lb_node_at(f, index - 1);
	if(!prev)
		return;
	struct node *p = prev->next;
	if (!p)
		return;
	prev->next = p->next;
	theap_free((char *)p->ptr);
	node_free(p);
}

void lb_dump(struct field *f){
	struct listbox *lb = (struct listbox *)f->value;	
	struct node *list = lb->list;

	if (!list)
		Serial.println(" *list is empty");

	while(list){
		Serial.printf(" %d: %s\n", list->id, (char *)list->ptr);
		list = list->next;
	}
}

/*
	UI related functions
*/

int lb_draw(struct field *f){
	int nlines = f->h / CONTROL_TEXT_HEIGHT;
	char buff[100];

	if (f->type != FIELD_LISTBOX)
		return -1;

	struct listbox *lb = (struct listbox *)f->value;	
	screen_fill_rect(f->x, f->y, f->w, f->h, TFT_BLACK);
  screen_draw_round_rect(f->x+1, f->y+1, f->w-2, f->h-2, TFT_WHITE);

	if(!lb->list)
		return 0;
	
	lb_dump(f);

	int y = f->y;
	int count = lb_count(f);
	if (lb->selection < 0)
		lb->selection = 0;
	if (lb->selection >= count)
		lb->selection = count - 1;
	
	if (lb->selection < lb->top_index)
		lb->top_index = lb->selection;
	if (lb->selection >= lb->top_index + nlines)
		lb->top_index = lb->selection - nlines + 1; 

	int i = lb->top_index;
	struct node *p = lb_node_at(f, lb->top_index); 

	//Serial.printf("drawing lb: count %d, sel %d\n", lb_count(f), lb->selection);
	while (p && y < f->y + f->h){
   	strcpy(buff, (char *)p->ptr);
		int x = f->x + 4;
 	  for (char *p = strtok(buff, "#"); p; p = strtok(NULL, "#")){
  	  //F=white G=Green R=Red, S=Orange
      uint16_t color = TFT_WHITE;
     	switch(*p){
   	  case 'G':
        color = TFT_GREEN;
     	  break;
			case 'D':
				color = TFT_DARKGREY;
				break;
     	case 'R':
       	color = TFT_CYAN;
       	break;
   	  case 'S':
        color = TFT_YELLOW;
     	  break;
			case '\n':
				Serial.printf("line break!!!\n");
				y+= CONTROL_TEXT_HEIGHT;
				x = f->x + 4;
				break;
      default:
     	  color= TFT_WHITE;
       	break;
     	}
     	screen_draw_text(p+1, -1, x, y, color, 2);
     	x += screen_text_width(p+1,2);
			//draw the selection
 		}
		if(i == lb->selection)
   		screen_draw_rect(f->x+1, y, f->w-2, 
				CONTROL_TEXT_HEIGHT, TFT_WHITE);
		i++;
		y += CONTROL_TEXT_HEIGHT;
		p = p->next;	
   	//Serial.println("done");
  }
	f->redraw = 0;
	return 0;
}	

int lb_input(struct field *f, int input){
	struct listbox *lb = (struct listbox *)f->value;	

	//Serial.printf("lb_input %d\n", input);
	
	if (input == ZBITX_KEY_UP && lb->selection > 0){
		lb->selection--;
	}
	if (input == ZBITX_KEY_DOWN && lb->selection < lb_count(f) -1){
		lb->selection++;
	}
	if (input == ZBITX_KEY_ENTER){
		//go to the selected node
		struct node *p = lb->list;
		int count = 0;
		while(p && count++ < lb->selection)
			p = p->next;
		if (p)
			lb->notify(f, FIELD_NOTIFY_SELECT, p->ptr);	
	}
	//Serial.printf("lb sel: %d\n", lb->selection);
	f->redraw = 1;
	lb_dump(f);
	return 0;
}

int lb_fn(struct field *f, int method, void *ptr){

	switch(method){
	case FIELD_METHOD_DRAW:
		return lb_draw(f);
	case FIELD_METHOD_INPUT:
		return lb_input(f, (int)ptr);
	case FIELD_METHOD_CLEAR:
		return lb_reset(f);
	case FIELD_METHOD_INIT:
		return lb_init(f, NULL);
	/* case FIELD_METHOD_UPDATE:
		return lb_insert(f, (char *)ptr, -1);*/
	}
	return -1;
}

/*
void  lb_test(){
	//wait for the serial portn to connect
	delay(10000);

	struct field *f = field_get("CONTACTS");

	Serial.printf("list has %d\n", lb_count(f));
	lb_dump(f);

	lb_insert(f, "first append", -1);
	lb_insert(f, "second append", -1);
	lb_insert(f, "third append", -1);
	lb_dump(f);

	Serial.println("removed at index 1");
	lb_remove(f, 1); 
	lb_dump(f);

	Serial.println("added fourth at 0, fifth at 2");
	lb_insert(f, "fourth at 0", 0);
	lb_insert(f, "fifth at 2", 2);
	lb_dump(f);

	Serial.println("removing at 0\n");
	lb_remove(f, 0);
	lb_dump(f);

	Serial.println("removing all");
	lb_reset(f);
	lb_dump(f);
}
*/
