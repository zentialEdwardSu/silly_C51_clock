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

typedef struct alarm{// alarm struct 
    uchar Hours;
    uchar Minutes;
    uchar isOn;
}S_alarm; 

// typedef struct currenttime{ // current time
//     uchar c_Hours;
//     uchar c_Minutes;
//     uchar c_Seconds;
// }S_currenttime;

// typedef struct timer{ // timer struct
//     uchar Seconds;
//     uchar Tenmilliseconds;
// }S_timer;

typedef enum displaymode{ // enum for display mode
    H_M,
    M_S,
    Timer,
}E_displaymode;

typedef enum keycls{ // enum for presseed key
    Nak,
    f1,
    f2,
    f3,
    f4,
}E_keycls;

typedef enum workmode{// enum for dorkmode
    clock,
    clock_adjust,
    alram_adjust,
    timer,
}E_workmode;

void init();
void delay(unsigned int count);
void Clock_trim_time();
// uchar Display_change_select_bit();
void Display_Display();
void Display_setbuf_by_mode();
void Display_setLatches();
E_keycls Input_transfer_key(uchar key);
E_keycls Input_get_key();

uchar code distab[23] ={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0x88,0x83,0xC6,0xA1,0x86,0x8E,0x8C,0xC1,0xCE,0x91,0x89,0xC7,0xFF};
uchar code distab_p[16] = {
    0x40,0x79,0x24,0x30,0x19,0x12,0x02,0x78,
    0x00,0x10,0x08,0x03,0x46,0x21,0x06,0x0e
};

uchar code bit_select_map[4] = { 0x8f,0x4f,0x2f,0x1f};

const uchar C_fixtime = 0x00;// clock correction factor

uchar R_10ms_counter = 0; // 10ms counter powered by T0 
uchar R_galarm_isOn = 0; //global alarm enable flag
uchar R_keypressed = -1; // storge key pressed -1 for no key was pressed 
uchar R_countkeydown = 0; // count keydown to ease
uchar R_ledbuffer[4] = {0}; // 4bit LED buffer 
E_keycls key = Nak;// final key cls
E_displaymode R_displaymode = H_M; // current display mode
E_workmode R_workmode = clock;// workmode 
// S_currenttime R_currenttime = ZEROCLOCK; // Hold runtime clock data (Init form 00:00:00)
// S_currenttime R_currenttime = {0,0,24}; // *test

uchar R_cHours = 0; // current time hour
uchar R_cMinutes = 59; // current time Minutes
uchar R_cSeconds = 55; // current time Seconds

uchar R_tSeconds = 0; // timer time seconds
uchar R_tMilliseconds = 0; // timer time mileseconds
S_alarm R_alarm[8] = {ZEROALARM}; // Hold runtime alarm groupInit as no clock

// uchar T_ex_seconds = 0; // external counter for seconds

int main(){
    init();
    //R_displaymode = M_S;
    // //*test
    // P1 = 1;

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
    //T0 as Main clock 
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
        case 0x1c:  return f1;    break;
        case 0x2c:  return f2;    break;
        case 0x34:  return f3;    break;
        case 0x38:  return f4;    break;
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
    delay(20);
    if (key_tmp == Input_transfer_key(KEYINPUT)){
        return key_tmp;
    }
    return Nak;
}

/**
 * @brief check legacy for clock
 * 
 */
void Clock_trim_time(){
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
    Clock_trim_time();
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
    for (i = 0;i < 4;i++){
        Display_setLatches();
        LEDOUT = distab[R_ledbuffer[i]];
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


void Clock_clockwalk() interrupt 1{
    TH0 = 0xdc; 
    TL0 = 0x00; 
    R_10ms_counter++;
    if(R_10ms_counter >= 100){
        R_10ms_counter = 0;
        R_cSeconds++;
        Clock_trim_time();
    }

}