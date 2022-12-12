#include<reg51.h>
#include <intrins.h> 

#define uchar unsigned char
#define uint unsigned int

#define ZEROCLOCK {0,0,0}
#define ZEROALARM {0,0,0}
#define KEYINPUT P3
#define BITSELECT P2
#define LEDOUT P0
#define count10ms 256-9

// typedef struct alarm{// alarm struct 
//     uchar Hours;
//     uchar Minutes;
//     uchar isOn;
// }S_alarm; 

// typedef struct currenttime{ // current time
//     uchar c_Hours;
//     uchar c_Minutes;
//     uchar c_Seconds;
// }S_currenttime;

// typedef struct wmode_timer{ // wmode_timer struct
//     uchar Seconds;
//     uchar Tenmilliseconds;
// }S_wmode_timer;

typedef enum displaymode{ // enum for display mode
    H_M,
    M_S,
    Timer,
}E_displaymode;

typedef enum keycls{ // enum for presseed key
    Nak,
    key_f1,
    key_f2,
    key_f3,
    key_f4,
}E_keycls;

typedef enum workmode{// enum for dorkmode
    wmode_clock,
    wmode_clock_adjust,
    wmode_alram_adjust,
    wmode_timer,
}E_workmode;

typedef enum adjust_pos{
    ad_Nad,
    ad_H,
    ad_M,
    ad_S,
}E_adjust_pos;

void init();
void delay(unsigned int count);
void Clock_lint_time();
// uchar Display_change_select_bit();
void Display_Display();
void Display_setbuf_by_mode();
void Display_setLatches();
void Display_transfer_code();
E_keycls Input_transfer_key(uchar key);
E_keycls Input_get_key();
void Input_key_map();
E_adjust_pos Adjclock_change_adj(E_adjust_pos cadj_pos);
void Adjclock_adj_up();
void Adjclock_adj_down();

uchar code distab[23] ={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0x88,0x83,0xC6,0xA1,0x86,0x8E,0x8C,0xC1,0xCE,0x91,0x89,0xC7,0xFF};
uchar code bit_select_map[4] = { 0x8f,0x4f,0x2f,0x1f};

const uchar C_fixtime = 0x00;// wmode_clock correction factor

uchar R_10ms_counter = 0; // 10ms counter powered by T0 
uchar R_galarm_isOn = 0; //global alarm enable flag
uchar R_keypressed = -1; // storge key pressed -1 for no key was pressed 
uchar R_countkeydown = 0; // count keydown to ease
uchar R_ledbuffer[4] = {0}; // 4bit LED buffer 
E_keycls R_key = Nak;// final key cls
E_displaymode R_displaymode = H_M; // current display mode
E_adjust_pos R_adjustpos = ad_Nad; // adjust position
E_workmode R_workmode = wmode_clock;// workmode 
// S_currenttime R_currenttime = ZEROCLOCK; // Hold runtime wmode_clock data (Init form 00:00:00)

uchar R_cHours = 0; // current time hour
uchar R_cMinutes = 59; // current time Minutes
uchar R_cSeconds = 55; // current time Seconds

uchar R_tSeconds = 0; // wmode_timer time seconds
uchar R_tMilliseconds = 0; // wmode_timer time mileseconds
// S_alarm R_alarm[8] = {ZEROALARM}; // Hold runtime alarm groupInit as no wmode_clock
uchar R_aHours = 0; // seted alarm hour
uchar R_aMinutes = 0; // seted alarm minutes
uchar R_aisON = 0; // is alarm enable

uchar R_tpressed_count = 0; // start/stop or set zero
uchar R_tisON = 0;// control timer
// uchar T_ex_seconds = 0; // external counter for seconds

int main(){
    init();
    //R_displaymode = M_S;

    while (1){

        Display_setbuf_by_mode();
		
        Display_Display();

    }
    
}

/**
 * @brief delay 1ms function
 * 
 * @param count 
 */
void delay(unsigned int count){
	unsigned char i;
    // count = count *2;
	while (count-- != 0)
		for (i = 0; i < 120; i++);
}

/**
 * @brief Init 
 * 
 */
void init(){
    TMOD = 0x11; //0010 0010
    EA = 1;
    //T0 as Main wmode_clock 
    TH0 = 0xdc; 
    TL0 = 0x00; 
    ET0 = 1;
    TR0 = 1;//start

}

/**
 * @brief transfer key to key_enum
 * 
 * @param key 
 * @return E_keycls 
 */
E_keycls Input_transfer_key(uchar key){
    key = key & 0x3c; // high 5
    switch (key){
        case 0x1c:  return key_f1;    break;
        case 0x2c:  return key_f2;    break;
        case 0x34:  return key_f3;    break;
        case 0x38:  return key_f4;    break;
        default :   return Nak;   break;   
    }
}

/**
 * @brief Get the key pressed
 * 
 * @return E_keycls 
 */
E_keycls Input_get_key(){
    E_keycls key_tmp = Nak;
    key_tmp = Input_transfer_key(KEYINPUT);
    delay(10);
    if (key_tmp == Input_transfer_key(KEYINPUT)){
        return key_tmp;
    }
    return Nak;
}

/**
 * @brief mtach keymap for individual workmode
 * 
 */
void Input_key_map(){
    switch(R_workmode){
        case wmode_clock:
            switch (R_key)
            {
                case key_f1:    R_displaymode == H_M?M_S:H_M;   break;
                case key_f2:    R_workmode = wmode_clock_adjust;    break;
                case key_f3:    R_workmode = wmode_alram_adjust;    break;
                case key_f4:    R_workmode = wmode_timer;   break;
            }
            break;
        case wmode_clock_adjust:
            switch (R_key)
            {
                case key_f1:    R_adjustpos = Adjclock_change_adj(R_adjustpos); break;
                case key_f2:    Adjclock_adj_up();  break;
                case key_f3:    Adjclock_adj_up();  break;
                case key_f4:    R_workmode = wmode_clock;   break;
            }
            break;
        case wmode_alram_adjust:
            switch (R_key)
            {
                case key_f1:    R_adjustpos = Adjclock_change_adj(R_adjustpos); break;
                case key_f2:    break;
                case key_f3:    break;
                case key_f4:    R_workmode = wmode_clock;   break;
                
            }
            break;
        case wmode_timer:
            switch (R_key)
            {
                case key_f1:
                    /* code */
                    break;
                case key_f2:
                    break;
                case key_f3:
                    break;
                case key_f4:
                    R_workmode = wmode_clock;   break;
                
            }
            
    }
}

/**
 * @brief check legacy for wmode_clock
 * 
 */
void Clock_lint_time(){
    //R_cSeconds = T_ex_seconds;
    if(R_cSeconds>=60){
        R_cSeconds = 0;
        R_cMinutes += 1;
    }
    if(R_cMinutes>=60){
        R_cMinutes = 0;
        R_cHours += 1;
    }
    if(R_cHours >= 24){
        R_cHours = 0;
    }
    R_cSeconds < 0?0:R_cSeconds;
    R_cMinutes < 0?0:R_cMinutes;
    R_cHours < 0?0:R_cHours;
}

/**
 * @brief chanage adjust_pos and maped display mode 
 * 
 * @param cadj_pos 
 * @return E_adjust_pos 
 */
E_adjust_pos Adjclock_change_adj(E_adjust_pos cadj_pos){
    ++cadj_pos > ad_S?ad_H:cadj_pos;
    if(cadj_pos >= ad_M) R_displaymode = M_S;

    return cadj_pos;
}

/**
 * @brief add one to current adjust_pos
 * 
 */
void Adjclock_adj_up(){
    switch(R_adjustpos){
        case ad_H: R_cHours++;break;
        case ad_M: R_cMinutes++;break;
        case ad_S: R_cSeconds++;break;
    }
    Clock_lint_time();
}
/**
 * @brief minus one to current adjust_pos
 * 
 */
void Adjclock_adj_down(){
    switch(R_adjustpos){
        case ad_H: R_cHours--;break;
        case ad_M: R_cMinutes--;break;
        case ad_S: R_cSeconds--;break;
    }
    Clock_lint_time();
}
// /**
//  * @brief return next bit select value for digital Tube
//  * 
//  * @return uchar 
//  */
// uchar Display_change_select_bit(){
//     if(T_tmp_dis_bit_count < 4){
//         T_tmp_dis_bit_count++;
//         return T_tmp_dis_bit_table[T_tmp_dis_bit_count];
//     }else{
//         T_tmp_dis_bit_count = 0;
//         return T_tmp_dis_bit_table[T_tmp_dis_bit_count];
//     }
// }

/**
 * @brief set dled buffer by display mode
 * 
 */
void Display_setbuf_by_mode(){
    Clock_lint_time();
    switch (R_displaymode){
        case H_M:
            R_ledbuffer[0] = R_cHours/10;
            R_ledbuffer[1] = R_cHours%10;
            R_ledbuffer[2] = R_cMinutes/10;
            R_ledbuffer[3] = R_cMinutes%10;
            break;
        case M_S:
            R_ledbuffer[0] = R_cMinutes/10;
            R_ledbuffer[1] = R_cMinutes%10;
            R_ledbuffer[2] = R_cSeconds/10;
            R_ledbuffer[3] = R_cSeconds%10;
            // P1 = 1;
            break;
        case Timer:
            R_ledbuffer[0] = R_tSeconds/10;
            R_ledbuffer[1] = R_tSeconds%10;
            R_ledbuffer[2] = R_tMilliseconds/10;
            R_ledbuffer[3] = R_tMilliseconds%10;
            break;
        
        default:
            break;
    }
}

/**
 * @brief Update LED using R_ledbuffer
 * 
 */
void Display_Display(){
    
    uchar i; 
    Display_transfer_code();
    for (i = 0;i < 4;i++){
        Display_setLatches();
        LEDOUT = R_ledbuffer[i];
        BITSELECT = ~bit_select_map[i];
        delay(1);
        Display_setLatches();
    }
}


/**
 * @brief set Latches to enable
 * 
 */
void Display_setLatches(){
    P0 = 0xff;
}

/**
 * @brief transfer buffer to displayable
 * 
 */
void Display_transfer_code(){
    uchar i;
    for (i = 0;i <4; i++){
        R_ledbuffer[i] = distab[R_ledbuffer[i]];
        if(i == 1)  R_ledbuffer[i]-=0x80; //divide 2 2 by point 
    }
}


void Clock_clockwalk() interrupt 1{
    TH0 = 0xdc; 
    TL0 = 0x00; 
    R_10ms_counter++;
    if(R_10ms_counter >= 100){
        R_10ms_counter = 0;
        R_cSeconds++;
        Clock_lint_time();
    }
    R_key = Input_transfer_key();

}