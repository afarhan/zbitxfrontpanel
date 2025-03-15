/* Basic graphics routines, implemented in screen_gx */
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

#define ZBITX_FONT_SMALL 1
#define ZBITX_FONT_NORMAL 2
#define ZBITX_FONT_LARGE 4 

#define SCREEN_BACKGROUND_COLOR 0x31E7

#define ZBITX_KEY_UP 2
#define ZBITX_KEY_DOWN 3
#define ZBITX_KEY_ENTER 13 

//Some hardware definitions/pins
#define ENC_S 1
#define ENC_A 2
#define ENC_B 3
#define VFWD A1
#define VREF A0

//duration since the start of the controller in millis
extern unsigned long now;

void screen_init();
void screen_fill_rect(int x, int y, int w, int h, int color);
void screen_draw_rect(int x, int y, int w, int h, int color);
void screen_fill_round_rect(int x, int y, int w, int h, int color);
void screen_draw_round_rect(int x, int y, int w, int h, int color);
void screen_draw_text(const char *text, int length, int x, int y, int color, int font); 
int16_t screen_text_width(const char *text, uint8_t font);
int16_t screen_text_height(uint8_t font);
void screen_pixel(int x, int y, uint16_t color);
bool screen_read(uint16_t *x, uint16_t *y);
void screen_text_extents(int font, uint16_t *extents); //extents upto 127, below 32 are randomf
void screen_draw_mono(const char *text, int count, int x_at, int y_at, uint16_t color);

/* struct field holds a value of radio controls like volume, frequency, etc. */

#define FIELD_NUMBER 0
#define FIELD_BUTTON 1
#define FIELD_TOGGLE 2
#define FIELD_SELECTION 3
#define FIELD_TEXT 4
#define FIELD_STATIC 5
#define FIELD_CONSOLE 6
#define FIELD_KEY 7
#define FIELD_FREQ 8
#define FIELD_WATERFALL 9
#define FIELD_FT8 10
#define FIELD_LOGBOOK 11
#define FIELD_SMETER 12

#define FIELD_TEXT_MAX_LENGTH 32

#ifndef FIELDS_H
#define FIELDS_H
struct field {
  int16_t type;
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
  uint16_t  color_scheme;
  const char *label;
  char value[FIELD_TEXT_MAX_LENGTH];
	char selection[128];
	void (*draw)(struct field *)=NULL;
  void *data;
  bool is_visible;
  bool redraw;
  bool update_to_radio;
};

/* A general purpose, reentrant Q for signals and 
 *  non critical data that can be lost without much impact
 */
 
void q_init(struct Queue *p, int32_t length);
int q_length(struct Queue *p);
int32_t q_read(struct Queue *p);
int q_write(struct Queue *p, int32_t w);
void q_empty(struct Queue *p);
#define MAX_QUEUE 4000
struct Queue
{
  int id;
  int head;
  int tail;
  int  stall;
  int data[MAX_QUEUE+1];
  unsigned int underflow;
  unsigned int overflow;
  unsigned int max_q;
};

#define FT8_MAX_DATA 100
struct ft8_message {
  uint32_t  id;
  int8_t    signal_strength;
  int16_t  frequency;
  char data[FT8_MAX_DATA];  
};

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
#endif
#define MAX_LOGBOOK 200
/*Field manipulation functions */

extern struct field *field_list;
void field_clear_all();
void field_init();
void field_draw(struct field *f);
void field_set(const char *label, const char *value);
struct field *field_at(uint16_t x, uint16_t y);
struct field *field_get(const char *label);
void field_show(const char *label, bool turn_on);
struct field *field_get_selected();
struct field *field_select(const char *label);
void field_panel(char *field_list);
void field_draw_all(bool all);
void field_set_panel(const char *mode);
void field_update_to_radio(char *label);
char read_key();

void screen_waterfall_update(uint8_t *bins, int w, int h);
void screen_waterfall_draw(int x, int y, int w, int h);
void field_draw(struct field *f, bool all);

/* keyboard */
#define EDIT_STATE_ALPHA 0
#define EDIT_STATE_UPPER 1
#define EDIT_STATE_SYM 2

void keyboard_show(uint8_t key_mode);
char keyboard_read();
void keyboard__hide();
bool text_edit(char *label, char *value, uint8_t mode); //mode from the edit states liste above

void radio_display_show();

void waterfall_show(int x, int y, int w, int h);
void waterfall_update(uint8_t *bins, int w, int h);
bool in_tx();

/* specific field routines */
void field_logbook_draw(struct field *f);

#define CONSOLE_MAX_LINES 30
#define CONSOLE_MAX_COLUMNS 22
