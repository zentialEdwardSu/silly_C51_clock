/**
 * @file main.c
 * @author EdwardSu(zentialeds@gmail.com)
 * @brief silly clock on AT89C51
 * @version 0.1-beta
 * @date 2022-12-14
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <reg51.h>
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
void Beep();
void delay(unsigned int count);
void Clock_lint_time();
// uchar Display_change_select_bit();
void Display_Display();
void Display_setbuf_by_mode();
void Display_setLatches();
void Display_transfer_code();
void Display_blink();
void Display_aison_tips();
E_keycls Input_transfer_key(uchar key);
E_keycls Input_get_key();
void Input_key_map();
E_adjust_pos Adjclock_change_adj(E_adjust_pos cadj_pos);
void Adjclock_adj_up();
void Adjclock_adj_down();

void Adjalarm_lint_alarm();
void Adjalarm_adj_up();
void Adjalarm_change_state();
E_adjust_pos Adjalarm_change_adj(E_adjust_pos adj_pos);

void Timer_start();
void Timer_pause();
void Timer_reset();
void Timer_lint();

uchar code distab[23] ={
    0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,
    0x80,0x90,0x88,0x83,0xC6,0xA1,0x86,0x8E,
    0x8C,0xC1,0xCE,0x91,0x89,0xC7,0xFF
};
uchar code bit_select_map[4] = { 0x8f,0x4f,0x2f,0x1f};

const uchar C_fixtime = 0x12;// wmode_clock correction factor

uchar R_10ms_counter = 0; // 10ms counter powered by T0 
uchar R_galarm_isOn = 0; //global alarm enable flag
E_keycls R_keypressed = Nak; // storge key pressed -1 for no key was pressed 
uchar R_countkeydown = 0; // count keydown to ease
uchar R_ledbuffer[4] = {0}; // 4bit LED buffer 
E_keycls R_key = Nak;// final key cls
E_displaymode R_displaymode = H_M; // current display mode
E_adjust_pos R_adjustpos = ad_Nad; // adjust position
E_workmode R_workmode = wmode_clock;// workmode 
// S_currenttime R_currenttime = ZEROCLOCK; // Hold runtime wmode_clock data (Init form 00:00:00)

uchar R_cHours = 10; // current time hour
uchar R_cMinutes = 59; // current time Minutes
uchar R_cSeconds = 50; // current time Seconds

uchar R_tSeconds = 0; // wmode_timer time seconds
uchar R_tMilliseconds = 0; // wmode_timer time mileseconds
// S_alarm R_alarm[8] = {ZEROALARM}; // Hold runtime alarm groupInit as no wmode_clock
uchar R_aHours = 11; // seted alarm hour
uchar R_aMinutes = 1; // seted alarm minutes
uchar R_aisON = 0; // is alarm enable
uchar R_adisplay_tips = 0; // display on/off tips
uchar R_aonAlarm = 0; // is on alarm 

uchar R_tpressed_count = 0; // *Unused start/stop or set zero
uchar R_tisON = 0;// control timer
// uchar T_ex_seconds = 0; // external counter for seconds

uchar R_isblink = 0;// if blink before
uchar R_500ms_counter = 0; // counter for 0.5s
uchar R_ontimebeep_counter = 0; // counter for beep times

sbit BEEP = P2^0;

int main(){
    init();
    //R_displaymode = M_S;

    while (1){

        Display_setbuf_by_mode();
        if(R_aonAlarm && !R_ontimebeep_counter){
            Beep();
        }
        if(R_aisON && R_aMinutes == R_cMinutes && R_aHours == R_cHours)   R_aonAlarm=1;
		
        switch(R_workmode){
            case wmode_clock: TR0 = 1;  break;
            case wmode_timer: R_displaymode = Timer;   break;
            case wmode_alram_adjust: 
                R_displaymode = H_M; 
                R_adjustpos = R_adjustpos == ad_Nad?ad_H:R_adjustpos;
                break;
            case wmode_clock_adjust: R_adjustpos = ad_H; TR0 = 0; break;
        }
        Input_key_map();
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
    //T1 power key sacan
    TH1 = 254;
    TL1 = 254;
    ET1 = 1;
    TR1 = 1;//

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
        case 0x1c:  return key_f4;    break;
        case 0x2c:  return key_f3;    break;
        case 0x34:  return key_f2;    break;
        case 0x38:  return key_f1;    break;
        default :   return Nak;   break;   
    }
}

/**
 * @brief Get the key pressed
 * @bug Unused
 * @return E_keycls 
 */
// E_keycls Input_get_key(){
//     E_keycls key_tmp = Nak;
//     key_tmp = Input_transfer_key(KEYINPUT);
//     delay(10);
//     if (key_tmp == Input_transfer_key(KEYINPUT)){
//         return key_tmp;
//     }
//     return Nak;
// }

/**
 * @brief mtach keymap for individual workmode
 * 
 */
void Input_key_map(){
    switch(R_workmode){
        case wmode_clock:
            switch (R_key)
            {
                case key_f1:    R_displaymode = R_displaymode == H_M?M_S:H_M;   break;
                case key_f2:    R_workmode = wmode_clock_adjust;    break;
                case key_f3:
                    if(R_aonAlarm){
                        R_aonAlarm = 0; //stop beeeeep when on alarm
                        R_aisON = 0;
                        break;
                    }
                    R_workmode = wmode_alram_adjust;    
                    break;
                case key_f4:    R_adjustpos=ad_Nad;  R_workmode = wmode_timer;   break;
            }
            break;
        case wmode_clock_adjust:
            switch (R_key)
            {
                case key_f1:    
                    R_adjustpos++;
                    R_adjustpos = R_adjustpos > ad_S?ad_H:R_adjustpos;
                    if(R_adjustpos >= ad_M) R_displaymode = M_S;
                    if(R_adjustpos < ad_M) R_displaymode = H_M;
                    
                    break;
                case key_f2:    Adjclock_adj_up();  break;
                case key_f3:    Adjclock_adj_down();  break;
                case key_f4:    R_adjustpos=ad_Nad; R_workmode = wmode_clock;   R_displaymode = H_M;    break;
            }
            break;
        case wmode_alram_adjust:
            switch (R_key)
            {
                case key_f1:    R_adjustpos = R_adjustpos == ad_H?ad_M:ad_H; break;
                case key_f2:    Adjalarm_adj_up();	break;
                case key_f3:    Adjalarm_change_state();break;
                case key_f4:    R_adjustpos=ad_Nad; R_workmode = wmode_clock;   R_displaymode = H_M;    break;
                
            }
            break;
        case wmode_timer:
            switch (R_key)
            {
                case key_f1:    Timer_start(); break;
                case key_f2:    Timer_pause(); break;
                case key_f3:    Timer_reset(); break;
                case key_f4:    R_adjustpos=ad_Nad; R_workmode = wmode_clock;   R_displaymode = H_M; break;
            }
            
    }
    R_key = Nak;
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

        R_ontimebeep_counter = R_cHours;
    }
    if(R_cHours >= 24){
        R_cHours = 0;
    }
    R_cSeconds = R_cSeconds < 0?0:R_cSeconds;
    R_cMinutes = R_cMinutes< 0?0:R_cMinutes;
    R_cHours = R_cHours < 0?0:R_cHours;
}

/**
 * @brief chanage adjust_pos and maped display mode 
 * 
 * @param cadj_pos 
 * @return E_adjust_pos 
 * @bug 
 */
E_adjust_pos Adjclock_change_adj(E_adjust_pos cadj_pos){
    cadj_pos++;
    cadj_pos = cadj_pos > ad_S?ad_H:cadj_pos;
    if(cadj_pos >= ad_M) R_displaymode = M_S;
    if(cadj_pos < ad_M) R_displaymode = H_M;

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
 * 
 * @brief minus one to current adjust_pos clock
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

/**
 * @brief minus one to current adjust_pos alarm
 * 
 */
void Adjalarm_adj_up(){
    switch(R_adjustpos){
        case ad_H: R_aHours++;break;
        case ad_M: R_aMinutes++;break;
    }
    Adjalarm_lint_alarm();
}

/**
 * @brief add one to current adjust_pos alarm
 * @bug unused
 */
// void Adjalarm_adj_down(){
//     switch(R_adjustpos){
//         case ad_H: R_aHours--;break;
//         case ad_M: R_aMinutes--;break;
//     }
// }

/**
 * @brief change clock state
 * 
 */
void Adjalarm_change_state(){
    R_aisON = R_aisON == 0?1:0;
    R_adisplay_tips = 1;
}

/**
 * @brief show alarm on/off tips
 * 
 */
void Display_aison_tips(){
    //R_adisplay_tips = 0;
    R_ledbuffer[0] = 0xff;
    R_ledbuffer[3] = 0xff;
    R_ledbuffer[1] = distab[12];
    switch(R_aisON){
        case 1:     R_ledbuffer[2] = distab[1];    break;
        case 0:     R_ledbuffer[2] = distab[0];    break;
    }
}

/**
 * @brief change adj pos for alarm adjust
 * 
 * @param adj_pos 
 * @return E_adjust_pos 
 * @bug unused
 */
// E_adjust_pos Adjalarm_change_adj(E_adjust_pos adj_pos){
//     adj_pos = adj_pos == ad_H?ad_M:ad_H;

//     return adj_pos;
// }

/**
 * @brief linter for alarm
 * 
 */
void Adjalarm_lint_alarm(){
    R_aHours = R_aHours > 24? 0:R_aHours;
    R_aMinutes = R_aMinutes >= 60? 0:R_aMinutes;
    R_aHours  = R_aHours < 0? 0:R_aHours;
    R_aMinutes = R_aMinutes< 0? 0:R_aMinutes;
}

/**
 * @brief start timer 
 * 
 */
void Timer_start(){
    Timer_reset();
    R_tisON = 1;
}

/**
 * @brief pause timer
 * 
 */
void Timer_pause(){
    R_tisON = 0;
}

/**
 * @brief reset timer
 * 
 */
void Timer_reset(){
    R_tSeconds = 0;
    R_tMilliseconds = 0;
}

/**
 * @brief Linte for timer
 * 
 */
void Timer_lint(){
    if(R_tMilliseconds > 99){
        R_tMilliseconds = 0;
        R_tSeconds += 1;
    }
    R_tSeconds = R_tSeconds > 59? 0:R_tSeconds;
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
    switch (R_workmode){
        case wmode_clock:    Clock_lint_time();   break;
			case wmode_timer: Timer_lint(); break;
    }

    switch (R_displaymode){
        case H_M:
            if(R_workmode == wmode_clock){
                R_ledbuffer[0] = R_cHours/10;
                R_ledbuffer[1] = R_cHours%10;
                R_ledbuffer[2] = R_cMinutes/10;
                R_ledbuffer[3] = R_cMinutes%10;
            }else if(R_workmode == wmode_alram_adjust){
                R_ledbuffer[0] = R_aHours/10;
                R_ledbuffer[1] = R_aHours%10;
                R_ledbuffer[2] = R_aMinutes/10;
                R_ledbuffer[3] = R_aMinutes%10;
            }
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
    Display_blink();
    if(R_adisplay_tips){
        Display_aison_tips();
    }
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

void Beep(){
	if(BEEP == 1){
        BEEP = 0; 
    }else{
        BEEP = 1;
    }
    delay(50);
}

/**
 * @brief set ledbuff 0xff when 1 is enable
 * 
 */
void Display_blink(){
    if(R_isblink){
        switch (R_adjustpos){
            case ad_H:
                R_ledbuffer[0] = 0xff;
                R_ledbuffer[1] = 0xff;
                break;
            case ad_M:
                if(R_displaymode == M_S){
                    R_ledbuffer[0] = 0xff;
                    R_ledbuffer[1] = 0xff;
                }else{
                    R_ledbuffer[2] = 0xff;
                    R_ledbuffer[3] = 0xff;
                }
                break;
            case ad_S:
                R_ledbuffer[2] = 0xff;
                R_ledbuffer[3] = 0xff;
                break;
        }
    }
}

void Clock_clockwalk() interrupt 1{
    TH0 = 0xdc; 
    TL0 = 0x00 + C_fixtime; 
    R_10ms_counter++;
    R_500ms_counter++;
    if(R_10ms_counter >= 100){
        R_10ms_counter = 0;
        R_cSeconds++;
        Clock_lint_time();
    }
    if(R_500ms_counter >= 50){
        R_500ms_counter = 0;
        if(R_adjustpos != ad_Nad){
            R_isblink = R_isblink ==0?1:0;
        }
        if(R_ontimebeep_counter){
            Beep();
            R_ontimebeep_counter-=1;
        }
        R_adisplay_tips = R_adisplay_tips == 1?0:0;
    }
    if (R_tisON){
        R_tMilliseconds += 1;
    }
}

void power_sacan() interrupt 3{
    E_keycls tmp = Nak;
    KEYINPUT = 0xff;
    tmp = Input_transfer_key(KEYINPUT);
    if(tmp != Nak){
        if(R_keypressed == Nak){
            R_keypressed = tmp;
        }else if(R_keypressed == tmp){
            R_key = R_keypressed;
            R_keypressed = Nak;
        }else{
            R_keypressed = Nak;
        }
    }

}