/*
TO GET GOING WITH PICO ON ARDUINO:

1. Download the post_install.sh and add execute permissions and execute as sudo
2. Follow the instructions on https://github.com/earlephilhower/arduino-pico

You may need to copy over the uf2 for the first time.
The blink.ino should work (note that the pico w and pico have different gpios for the LED)
 */
#define USE_DMA
#include <TFT_eSPI.h>
#include <Wire.h>
#include "zbitx.h"
extern "C" {
#include "pico.h"
#include "pico/time.h"
#include "pico/bootrom.h"
}

int freq = 7000000;
unsigned long now = 0;
unsigned long last_blink = 0;
char receive_buff[10000];
struct Queue q_incoming;

bool mouse_down = false;
uint8_t encoder_state = 0;
boolean encoder_switch = false;
unsigned long next_repeat_time = 0;
unsigned int last_wheel_moved = 0;
unsigned int wheel_count = 0;

int vfwd=0, vswr=0, vref = 0, vbatt=0;
int wheel_move = 0;

char message_buffer[100];

int enc_state(){
  return  (digitalRead(ENC_A)? 1:0) + (digitalRead(ENC_B) ? 2:0);
}

void on_enc(){

  uint8_t encoder_now = enc_state();
	if (encoder_now == encoder_state)
		return;

	if (enc_state() != encoder_now)
		return;
  
  if ((encoder_state == 0 && encoder_now == 1) 
    || (encoder_state == 1 && encoder_now == 3) 
    || (encoder_state == 3 && encoder_now == 2)
    || (encoder_state == 2 && encoder_now == 0)) {
    	wheel_move--;
			wheel_count++;
	}
  else if ((encoder_state == 0 && encoder_now == 2)
    || (encoder_state == 2 && encoder_now == 3)
    || (encoder_state == 3 && encoder_now == 1)
    || (encoder_state == 1 && encoder_now == 0)){
      wheel_move++;
			wheel_count++;
	}
  encoder_state = encoder_now;    
}

char last_sent[1000]={0};
int req_count = 0;
int total = 0;



//comannd tokenizer

static char cmd_label[FIELD_TEXT_MAX_LENGTH];
static char cmd_value[1000]; // this should be enough
static bool cmd_in_label = true;
static bool cmd_in_field = false;

void command_init(){
  cmd_label[0] = 0;
  cmd_value[0] = 0;
  //cmd_p = cmd_label;
  cmd_in_label = false;
  cmd_in_field = false;
}

boolean in_tx(){
	struct field *f = field_get("IN_TX");
	if (!f)
		return false;
	if (!strcmp(f->value, "0"))
		return false;
	else
		return true;
}

void set_bandwidth_strip(){
	struct field *f_span = field_get("SPAN");
	struct field *f_high = field_get("HIGH");
	struct field *f_low  = field_get("LOW");
	struct field *f_mode = field_get("MODE");
	struct field *f_pitch = field_get("PITCH");
	struct field *f_tx_pitch = field_get("TX_PITCH");

	if (!f_span || !f_high || !f_low || !f_mode || !f_pitch)
		return;

	int span = 25000;
	if (!strcmp(f_span->value, "10K"))
		span = 10000;
	else if (!strcmp(f_span->value, "6K"))
		span = 6000;
	else if (!strcmp(f_span->value, "2.5K"))
		span = 2500;	

	int high = (atoi(f_high->value) * 240)/span;
	int low = (atoi(f_low->value) * 240)/span;
	int pitch = (atoi(f_pitch->value) * 240)/span;
	int tx_pitch = (atoi(f_tx_pitch->value) * 240)/span;

/*	if (!strcmp(f_mode->value, "CW"))
		
	else if(!strcmp(f_mode->value, "CWR"))
		
	}
*/
	if (!strcmp(f_mode->value, "LSB") || !strcmp(f_mode->value, "CWR")){
		high = -high;
		low = -low;
		pitch = -pitch;
		tx_pitch = -tx_pitch;
	}
	
	waterfall_bandwidth(low, high, pitch, tx_pitch);
}


void command_tokenize(char c){

  if (c == COMMAND_START){
    cmd_label[0] = 0;
    cmd_value[0] = 0;
    //cmd_p = cmd_label;
    cmd_in_label = true;
    cmd_in_field = true;
  }
  else if (c == COMMAND_END){
		if (strlen(cmd_label)){
			struct field *f = field_get(cmd_label);
			if (!f)  // some are not really fields but just updates, like QSO
     		field_set(cmd_label, cmd_value, false);
      else if (f->last_user_change + 1000 < now || f->type == FIELD_TEXT)
     		field_set(cmd_label, cmd_value, false);
			if (!strcmp(cmd_label, "HIGH") || !strcmp(cmd_label, "LOW") || !strcmp(cmd_label, "PITCH")
				|| !strcmp(cmd_label, "SPAN") || !strcmp(cmd_label, "MODE"))
				set_bandwidth_strip();	
    }
    cmd_in_label = false;
    cmd_in_field = false;
  }
  else if (!cmd_in_field) // only:0 handle characters between { and }
    return;
  else if (cmd_in_label){
    //label is delimited by space
    if (c != ' ' && strlen(cmd_label) < sizeof(cmd_label)-1){
      int i = strlen(cmd_label);
      cmd_label[i++] = c;
      cmd_label[i]= 0;
    }
    else 
      cmd_in_label = false;
  }
  else if (!cmd_in_label && strlen(cmd_value) < sizeof(cmd_value) -1 ){
    int i = strlen(cmd_value);
    cmd_value[i++] = c;
    cmd_value[i] = 0;
  }
}

// I2c routines
// we separate out the updates with \n character
void wire_text(char *text){
	char i2c_buff2[200];

  int l = strlen(text);
  if (l > 255){
    Serial.printf("#Wire sending[%s] is too long\n", text);
    return;
  }

	i2c_buff2[0] = l;
  strcpy(i2c_buff2+1,text);

	Wire1.write(i2c_buff2, l+1); //include the last zero
	strcpy(last_sent, text);
	req_count++;
	total += l;
}

char buff_i2c_req[200];
void on_request(){
  char c;
  //just update a single field, buttons first
	if (message_buffer[0] != 0){
    strcpy(buff_i2c_req, message_buffer);
		wire_text(buff_i2c_req);
		message_buffer[0] = 0;
		return;
	}

	//check if any button has been pressed
  for (struct field *f = field_list; f->type != -1; f++){
    if (f->update_to_radio && f->type == FIELD_BUTTON){
			f->update_to_radio = false;
      sprintf(buff_i2c_req, "%s %s", f->label, f->value);
			wire_text(buff_i2c_req);
      return;
    }
	}
	//then the rest
  for (struct field *f = field_list; f->type != -1; f++){
    if (f->update_to_radio){
			f->update_to_radio = false;
      sprintf(buff_i2c_req, "%s %s", f->label, f->value);
			wire_text(buff_i2c_req);
      return;
    }
	}

	char buff[50];
	sprintf(buff, "vbatt %d\npower %d\nvswr %d\n", vbatt, vfwd, vswr);
  wire_text(buff);
}

int dcount = 0;
void on_receive(int len){
  uint8_t r;
  dcount += len;
  while(len--){
    q_write(&q_incoming, (int32_t)Wire1.read());
	}
}

#define AVG_N 10 

void measure_voltages(){
  char buff[30];
  int f, r, b;

	static unsigned long next_update = 0;
	unsigned long now = millis();

	if (now < next_update)
		return;

  f = (56 * analogRead(A0))/460;
  r = (56 *analogRead(A1))/460;
  b = (500 * analogRead(A2))/278;

	vbatt = b;

	if (f > vfwd)
		vfwd = f;
	else
  	vfwd = ((vfwd * AVG_N) + f)/(AVG_N + 1);

	if (r > vref)
		vref = r;
	else
  	vref = ((vref * AVG_N) + r)/(AVG_N + 1);

	vswr = (10*(vfwd + vref))/(vfwd-vref);

	// update only once in a while
	next_update = now + 50;
}

/* it returns the field that was last selected */

struct field *ui_slice(){
  uint16_t x, y;
	struct field *f_touched = NULL;

	//check if messages need to be processed
  while(q_length(&q_incoming))
    command_tokenize((char)q_read(&q_incoming));
	if (now > last_blink + BLINK_RATE){
		field_blink(-1);
		last_blink = now;
	}

  // check the encoder state
	if (digitalRead(ENC_S) == HIGH && encoder_switch == true){
			encoder_switch = false;		
	}
	if (digitalRead(ENC_S) == LOW && encoder_switch == false){
		encoder_switch = true;
		field_input(ZBITX_KEY_ENTER);
	}

	int step_size = 3;
  if (f_selected && !strcmp(f_selected->label, "FREQ"))
    step_size = 1;

	if (wheel_move > step_size){
    field_input(ZBITX_KEY_UP);
		wheel_move = 0;
		last_wheel_moved = now;
  }
  else if (wheel_move < -step_size){
    field_input(ZBITX_KEY_DOWN);
		wheel_move = 0;
		last_wheel_moved = now;
  }
  //redraw everything
  field_draw_all(false);
 
  if (!screen_read(&x, &y)){
      mouse_down = false;
    return NULL;
  }

  //check for user input
  struct field *f = field_at(x, y);
  if (!f)
    return NULL;
  //do selection only if the touch has started
  if (!mouse_down){
    field_select(f->label);
		next_repeat_time = millis() + 1500;
		f_touched = f;
	}
	else if (next_repeat_time < millis() && f->type == FIELD_KEY){
    field_select(f->label);
		next_repeat_time = millis() + 300;
	}
     
  mouse_down = true;
	return f_touched; //if any ...
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
	/* while(!Serial)
		delay(100); */
  q_init(&q_incoming);
  screen_init();
  field_init();
  field_clear_all();
  command_init();
  field_set("MODE","CW", false);

  q_init(&q_incoming);
  Wire1.setSDA(6);
  Wire1.setSCL(7);
  Wire1.begin(0x0a);
  Wire1.setClock(400000L);
  Wire1.onReceive(on_receive);
  Wire1.onRequest(on_request);

  receive_buff[0] = 0;

	pinMode(ENC_S, INPUT_PULLUP);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  encoder_state = digitalRead(ENC_A) + (2 * digitalRead(ENC_B));

	attachInterrupt(ENC_A, on_enc, CHANGE);
	attachInterrupt(ENC_B, on_enc, CHANGE);

	field_set("9", "zBitx firmware v1.08\nWaiting for the zBitx to start...\n", false);

	if (digitalRead(ENC_S) == LOW)
		reset_usb_boot(0,0); //invokes reset into bootloader mode
}

void simulate_waterfall(){
	uint8_t noise[240];

	for(int i = 0; i < 240; i++){
		noise[i] = analogRead(A0)/8;
	}
	struct field *f = field_get("WF");
	waterfall_update(f, noise);
	if (f){
		f->redraw = true;	
		waterfall_draw(f);
	}
}

int count = 0;
// the loop function runs over and over again forever
void loop() {
	now = millis();

  count++;
//  if (count % 400)
//    simulate_waterfall();
  ui_slice();

  measure_voltages();
  delay(1);
}
