#include <Arduino.h>
#include <TFT_eSPI.h>
#include "zbitx.h"
#include "logbook.h"

//static int log_top_index = 0;
//static int log_selection = 0;

struct logbook_entry {
  uint32_t qso_id;
  char date_utc[11]; //ex: 20250102 yyyy/mm/dd
  char time_utc[11]; //ex: 1305 hh:mm
  uint32_t frequency;
  char mode[10];
  char callsign[10];
  char rst_sent[10];
  char rst_recv[10];
  char exchange_sent[10];
  char exchange_recv[10];
};
#define MAX_LOGBOOK 200
struct logbook_entry logbook[MAX_LOGBOOK];

struct listbox{
	int top_index;
	int selection; 
};

/* logbook routines */
int logbook_init(struct field *f){
	struct listbox *lb = (struct listbox *)f->value;	
	lb->top_index = 0;
	lb->selection = 0;
	memset(logbook, 0, sizeof(logbook));
	//Serial.println("Initializing the logbook");
	return 0;
}

void logbook_edit(struct logbook_entry *e){
	char entry[100];

	sprintf(entry, "%s on %s at %s hrs\nMode: %s\nFreq: %d KHz", 
		e->callsign, e->date_utc, e->time_utc,
		e->mode, e->frequency);
	int qso_id = e->qso_id;
	char qso_str[10];
	sprintf(qso_str, "%d", e->qso_id);
	field_set("MESSAGE", entry, true); 
	struct field *f = dialog_box("Delete log entry?",  "MESSAGE/DELETE/CANCEL");
	if (f){
		if (!strcmp(f->label, "DELETE")){
			field_set("QSODEL", qso_str, true); 
			struct field *f = field_get("QSODEL");
			field_select("QSODEL");
			memset(logbook, 0, sizeof(logbook));
		}
	}
}

struct logbook_entry *logbook_get(int qso_id){
	for (int i = 0; i < MAX_LOGBOOK; i++)
		if (logbook[i].qso_id == qso_id)
			return logbook + i;
	//we are here because the matching qso_id wasn't found
	return NULL;
}

// ex update_str = "QSO 3|FT8|21074|2023-05-04|0639|VU2ESE|-16|MK97|LZ6DX|-11|KN23||"
int logbook_update(struct field *f, const char *update_str){
  uint32_t qso_id, frequency;
  char date_utc[11], time_utc[5], mode[10], my_callsign[10], rst_sent[10], 
		rst_recv[10], exchange_sent[10], exchange_recv[10], contact_callsign[10],
		buff[200], *record;

	Serial.println(update_str);
	strcpy(buff, update_str);
	record = buff;
	//read the QSO id  
	char *p = strsep(&record, "|");
	if (!p)
		return -1;
	qso_id = atoi(p);

	// read the mode 
	p = strsep(&record, "|");
	if (!p)
		return -1;
	if (strlen(p) > 0 && strlen(p) < 10)
		strcpy(mode, p);	
	else
		return -1;

	//read the frequency
	p = strsep(&record, "|");
	frequency = atoi(p);
	if (!p || frequency < 10 || frequency > 4000000000)
		return -1;
	frequency = atoi(p);

	//read the date
	p = strsep(&record, "|");
	if (!p || strlen(p) != 10)
		return -1;
	strcpy(date_utc, p);

	//read the time
	p = strsep(&record, "|");
	if (!p || strlen(p) != 4)	
		return -1;
	strcpy(time_utc, p);

	p = strsep(&record, "|");
	if (!p || strlen(p) < 4 || strlen(p) > 9)
		return -1;
	strcpy(my_callsign, p);
	
	// read the sent report
	p = strsep(&record, "|");
	if (!p || strlen(p) < 2 || strlen(p) > 9)
		return -1;
	strcpy(rst_sent, p);

	// read the sent exchange 
	p = strsep(&record, "|");
	if (p && strlen(p) <= 9)
		strcpy(exchange_sent, p);
	else
		exchange_sent[0] = 0;

	p = strsep(&record, "|");
	if (!p || strlen(p) < 4 || strlen(p) > 9)
		return -1;
	strcpy(contact_callsign, p);

	//read the recv report
	p = strsep(&record, "|");
	if(!p || strlen(p) < 2 || strlen(p) > 9)
		return -1;
	strcpy(rst_recv, p); 

	//read the recv exchange 
	p = strsep(&record, "|");
	if(p  && strlen(p) <= 9)
		strcpy(exchange_recv, p); 
	else
		exchange_recv[0] = 0;

	//Serial.printf("parsed: id:%d / date:%s / time:%s/ freq:%d/ mode: %s/ call:%s/ sent:%s/ recv:%s\n", 
	//	qso_id, date_utc, time_utc, frequency, mode, contact_callsign, rst_sent, rst_recv);

	logbook_entry *e = logbook_get(qso_id);
	if (!e){
    //Serial.printf("Didnt find a matching entry for %d\n", qso_id);
		//find an empty entry
		for (int i = 0; i < MAX_LOGBOOK; i++)
			if (logbook[i].qso_id == 0){
				e = logbook + i;
				break;
			}
	}
	//if we don't have enough space for the rest ...
	if (!e){
    //Serial.println("clearing the logbook");
    //the table is too full, we clear it all out.
		memset(logbook, 0, sizeof(logbook));
		e = logbook;
	}	
	e->qso_id = qso_id;
	strcpy(e->date_utc, date_utc);
	strcpy(e->time_utc, time_utc);
	e->frequency = frequency;
	strcpy(e->mode, mode);
  strcpy(e->rst_recv, rst_recv);
  strcpy(e->exchange_recv, exchange_recv);
  strcpy(e->rst_sent, rst_sent);
	strcpy(e->exchange_sent, exchange_sent);
	strcpy(e->callsign, contact_callsign);

	//Serial.println("entered");
	return 0;
}
	
// add colons between hours and minutes
// show the exchange
// have a start index and scroll with the mouse input
int logbook_draw(struct field *f){
	int nlines = f->h / 16;
	char buff[100];

	struct listbox *lb = (struct listbox *)f->value;	
	//Serial.printf("logbook_draw of %s: top:%d, sel %d\n", f->label, lb->top_index, lb->selection); 

	//Serial.println("logbook_draw");
	screen_fill_rect(f->x, f->y, f->w, f->h, TFT_BLACK);
	int y = f->y;
	if (lb->selection < lb->top_index)
		lb->top_index = lb->selection;
	if (lb->selection >= lb->top_index + nlines)
		lb->top_index = lb->selection - nlines + 1; 

	//Serial.printf("logbook start at %d\n", lb->top_index);
	for (int line = 0; line < nlines; line++){
		struct logbook_entry *e = logbook + line + lb->top_index;
		int x = f->x;

    if (!e->date_utc[0])
      continue;
/*
		sprintf(buff, "%d", e->qso_id);
		screen_draw_text(buff, -1, x, y, TFT_CYAN, 2);
		x += 30;
*/		
		screen_draw_text(e->date_utc, -1, x, y, TFT_LIGHTGREY, 2);
		x += 88;

		//int time_utc = atoi(e->time_utc);
		//sprintf(buff, "%02d:%02d", time_utc / 60, time_utc % 60);
		//screen_draw_text(buff, -1, x, y, TFT_CYAN, 2);
		screen_draw_text(e->time_utc, -1, x, y, TFT_CYAN, 2);
		x += 45;

		sprintf(buff, "%d", logbook[line].frequency);
		screen_draw_text(buff, -1, x, y, TFT_WHITE, 2);
		x += 55;

		screen_draw_text(e->mode, -1, x, y, TFT_WHITE, 2);
		x += 40;

		screen_draw_text(e->callsign, -1, x, y, TFT_WHITE, 2);
		x += 80;

		strcpy(buff, e->rst_sent);strcat(buff, " ");strcat(buff, e->exchange_sent);
		screen_draw_text(buff, -1, x, y, TFT_LIGHTGREY, 2);
		x += 80;

		strcpy(buff, e->rst_recv);strcat(buff, " ");strcat(buff, e->exchange_recv);
		screen_draw_text(buff, -1, x, y, TFT_LIGHTGREY, 2);

		if (f == f_selected &&  lb->top_index + line == lb->selection)
			screen_draw_rect(f->x, y, f->w, 16, TFT_WHITE);
		y += 16;
	}
	//Serial.println("logbook_drawn");
	return 0;
}

int logbook_input(struct field *f, int input){
	struct listbox *lb = (struct listbox *)f->value;	

	if (input == ZBITX_KEY_UP && lb->selection> 0){
		lb->selection--;
	}
	if (input == ZBITX_KEY_DOWN && lb->selection < MAX_LOGBOOK -1){
		lb->selection++;
	}
	if (input == ZBITX_KEY_ENTER){
		//Serial.print("logbook edit");
		logbook_edit(logbook+lb->selection);
	}
	return 0;
}

int logbook_fn(struct field *f, int method, void *ptr){
	switch(method){
	case FIELD_METHOD_DRAW:
		return logbook_draw(f);
	case FIELD_METHOD_INPUT:
		return logbook_input(f, (int)ptr);
	case FIELD_METHOD_INIT:
		Serial.printf("init %s\n", f->label);
		return logbook_init(f);
	case FIELD_METHOD_UPDATE:
		Serial.printf("update %s\n", f->label);
		return logbook_update(f, (char *)ptr);
	}
	return -1;
}
