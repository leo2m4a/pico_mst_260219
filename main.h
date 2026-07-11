//parameter
uint32_t gPARAM_fcnt_100ms;
uint32_t gPARAM_fcnt_1s;
uint32_t gPARAM_actflag;

uint32_t gVAR_btn_u=0;
uint32_t gVAR_btn_d=0;

//volatile bool timer_fired = false;
uint8_t gPARAM_UR1RX_buf[32];
uint8_t gPARAM_UR1RX_buf_lat[32];
uint8_t gPARAM_UR1RX_buf_cnt;
uint8_t gPARAM_UR1RX_buf_leng=32;
uint8_t gPARAM_UR1RX_buf_ptr = 0;



uint8_t gPARAM_UR1TX_buf[32];
uint8_t gPARAM_UR1TX_buf_leng;
//radio
uint8_t gPARAM_RFTX_buf[32];
uint8_t gPARAM_RFTX_buf_leng;
uint8_t gPARAM_RFRX_buf[32];
uint8_t gPARAM_RFRX_buf_lat[32];
uint8_t gDisp_BUF_2[8];
uint8_t gDisp_BUF_3[8];
//
uint8_t gPARAM_APOLL_SID[8]= {0x81,0x82,0x83,0x84,0,0,0,0};
uint8_t gPARAM_APOLL_en;
uint8_t gPARAM_APOLL_cnt;
uint8_t gPARAM_APOLL_cnt_lmt;
uint8_t gPARAM_APOLL_RXDATA[16];

uint8_t gPARAM_ACT_DATA[8];

uint8_t gSCH_RLY[24 * 4];
