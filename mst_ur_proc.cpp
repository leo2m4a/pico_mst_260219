#include <stdio.h>
#include "pico/stdlib.h"
#include "global_def.h"
#include "mst_ur_proc.h"
#include "hardware/i2c.h"
#include "usr_lib/at24c32.h"


extern uint8_t gPARAM_ACT_DATA[8];
extern int at24c32_write_page(i2c_inst_t *i2c, uint8_t dev_addr, uint16_t word_addr, const uint8_t *data, size_t len);
extern int at24c32_read(i2c_inst_t *i2c, uint8_t dev_addr, uint16_t word_addr, uint8_t *data, size_t len);
//55 0c 00 ff 00 00 00 00 00 00 00 00  AA 0C 02 00 00 00 06 07 00 00 00 00
//78 0c 01 83 11 04 01 01 00 00 00 00  89 0C 02 83 11 04 00 00 00 00 00 00 
uint8_t mst_compose_rf_bcn(uint8_t *val,uint8_t sec,uint8_t min,uint8_t hour)
{
  uint8_t ret = 12;
  
  *(val+0) = 0x55;
  *(val+1) = ret;
  *(val+2) = MY_MID;
  *(val+3) = 0xff;  
  *(val+4) = 0;//subcmd
  *(val+5) = sec;
  *(val+6) = min;
  *(val+7) = hour;
  *(val+8) = gPARAM_fcnt_1s;
  *(val+9) = 0x9;
  *(val+10) = 0xa;
  *(val+11) = 0xb;  

  #if 0
   bcn->hdr = 0x55;
   bcn->length = 0xc;
   bcn->mid = 0x01;
   bcn->sid = 0xff;
   bcn->subcmd = 0;
   bcn->sec = Stru_curTime.sec;
   bcn->min = Stru_curTime.min;
   bcn->hour = Stru_curTime.hour;
   bcn->fcnt = gPARAM_fcnt_1s;
   bcn->data9 = 0;
   bcn->data10 = 0;
   bcn->data11 = 0;
   #endif
 //  (*bcn->hdr)=0x55;
   return ret;
}
uint8_t mst_compose_rf_poll(uint8_t *val,uint8_t sid)
{
  uint8_t ret = 12;
  *(val+0) = 0x55;
  *(val+1) = ret;
  *(val+2) = MY_MID;
  *(val+3) = sid;  
  *(val+4) = 0xaa;//subcmd
  *(val+5) = 0;
  *(val+6) = 0;
  *(val+7) = 0;
  *(val+8) = 0;
  *(val+9) = 0x9;
  *(val+10) = 0xa;
  *(val+11) = 0xb;  

 
   return ret;
}
uint8_t mst_compose_rf_write_a4(uint8_t *outbuf,uint8_t sid,uint8_t wen_rly,uint8_t wval_rly)
{
  uint8_t ret=12;
  
  *(outbuf+0) = 0x78;
  *(outbuf+1) = ret;
  *(outbuf+2) = MY_MID;//mid
  *(outbuf+3) = sid;//sid
//
  *(outbuf+4) = 0x11;//subcmd
  *(outbuf+5) = 4;//addr
  *(outbuf+6) = wen_rly;
  *(outbuf+7) = wval_rly;
//
  *(outbuf+8)  = 0;
  *(outbuf+9)  = 0;
  *(outbuf+10) = 0;
  *(outbuf+11) = 0;    
  return ret;  
}
/////
uint8_t mst_ur_rsp_echo(uint8_t *outbuf,uint8_t ret_code4,uint8_t ret_code5)
{
  uint8_t ret=12;
  
  *(outbuf+0) = 0xaa;
  *(outbuf+1) = ret;
  *(outbuf+2) = MY_MID;
  *(outbuf+3) = 0;
//
  *(outbuf+4) = ret_code4;
  *(outbuf+5) = ret_code5;
  *(outbuf+6) = 6;
  *(outbuf+7) = 0xbe;
//
  *(outbuf+8) = 0xef;
  *(outbuf+9) = 0;
  *(outbuf+10) = 0x6a;
  *(outbuf+11) = 0xe2;  
  
  return ret;
}
uint8_t mst_ur_rsp_rw_eep(uint16_t word_addr,uint8_t *dbuf,uint8_t *outbuf)
{
  uint8_t ret=16;
  
  *(outbuf+0) = 0x92;
  *(outbuf+1) = ret;
  *(outbuf+2) = MY_MID;
  *(outbuf+3) = 0;//sid
//
  *(outbuf+4) = 0x11;
  *(outbuf+5) = word_addr >>8 ;
  *(outbuf+6) = (uint8_t)word_addr & 0xff;
  *(outbuf+7) = *(dbuf+0);
  //
  *(outbuf+8) = *(dbuf+1);
  *(outbuf+9) = *(dbuf+2);
  *(outbuf+10) = *(dbuf+3);
  *(outbuf+11) = *(dbuf+4);
  //
  *(outbuf+12) = *(dbuf+5);    
  *(outbuf+13) = *(dbuf+6);    
  *(outbuf+14) = *(dbuf+7);
  *(outbuf+15) = 0;
   

  return ret;
}

uint8_t mst_ur_rsp_read_eep(uint16_t word_addr,uint8_t *dbuf,uint8_t *outbuf)
{
  uint8_t ret=16;
  
  *(outbuf+0) = 0x92;
  *(outbuf+1) = ret;
  *(outbuf+2) = MY_MID;
  *(outbuf+3) = 0;//sid
//
  *(outbuf+4) = 0x11;
  *(outbuf+5) = 0 ;
  *(outbuf+6) = *(dbuf+0);
  *(outbuf+7) = *(dbuf+1);
//
  *(outbuf+8) = *(dbuf+2);
  *(outbuf+9) = *(dbuf+3);
  *(outbuf+10) = *(dbuf+4);
  *(outbuf+11) = *(dbuf+5);    
  //
  *(outbuf+12) = *(dbuf+6);    
  *(outbuf+13) = *(dbuf+7);
  *(outbuf+14) = 0;
  *(outbuf+15) = 0;    

  return ret;
}
uint8_t mst_ur_rsp_write_mst_a0(uint8_t *outbuf)
{
  uint8_t ret=12;
  
  *(outbuf+0) = 0x7a;
  *(outbuf+1) = ret;
  *(outbuf+2) = MY_MID;
  *(outbuf+3) = 0;//sid
//
  *(outbuf+4) = 0x11;
  *(outbuf+5) = 0 ;
  *(outbuf+6) = Stru_curTime.hour ;
  *(outbuf+7) = Stru_curTime.min;
//
  *(outbuf+8)  = Stru_curTime.sec;
  *(outbuf+9)  = Stru_curTime.month;
  *(outbuf+10) = Stru_curTime.day;
  *(outbuf+11) = 0;    
  return ret;
}
uint8_t mst_ur_rsp_write_mst_a1(uint8_t *outbuf)
{
  uint8_t ret=12;
  
  *(outbuf+0) = 0x7a;
  *(outbuf+1) = ret;
  *(outbuf+2) = MY_MID;
  *(outbuf+3) = 0;//sid
//
  *(outbuf+4) = 0x11;
  *(outbuf+5) = 1;
  *(outbuf+6) = Stru_curTime.year ;
  *(outbuf+7) = Stru_curTime.month;
//
  *(outbuf+8)  = Stru_curTime.day;
  *(outbuf+9)  = Stru_curTime.dow;
  *(outbuf+10) = 0;
  *(outbuf+11) = 0;    
  return ret;
}
uint8_t mst_ur_rsp_write_slv_a4(uint8_t *outbuf,uint8_t sid)
{
  uint8_t ret=12;
  
  *(outbuf+0) = 0x89;
  *(outbuf+1) = ret;
  *(outbuf+2) = MY_MID;
  *(outbuf+3) = sid;//sid
//
  *(outbuf+4) = 0x11;
  *(outbuf+5) = 4;
  *(outbuf+6) = 0;
  *(outbuf+7) = 0;
//
  *(outbuf+8)  = 0;
  *(outbuf+9)  = 0;
  *(outbuf+10) = 0;
  *(outbuf+11) = 0;    
  return ret;
}


/////
uint8_t mst_ur_cmd_process(uint8_t *inbuf,uint8_t *ur_out_buf,uint8_t *ur_rsp_leng,uint8_t *rf_out_buf,uint8_t *rf_send_leng)
{
  uint8_t ret=0;
  uint8_t cmd = *(inbuf+0);
  uint8_t sid = *(inbuf+3);
  uint8_t subcmd = *(inbuf+4);
  uint8_t addr   = *(inbuf+5);
  uint8_t addr_l = *(inbuf+6);
  uint16_t word_addr = (addr<<8)|addr_l;
  uint8_t datatemp[8];
  
  if(cmd == 0x55)
  {
    if(subcmd == 0x0)
    {
      *(ur_rsp_leng+0) = mst_ur_rsp_echo(ur_out_buf,*(inbuf+4),*(inbuf+5));
      ret |= 0x1;
    }
    else if(subcmd == 0xaa)
    {
      *(ur_rsp_leng+0) = mst_ur_rsp_echo(ur_out_buf,0x55,0xaa);
      //compose rf poll
      *(rf_send_leng+0) = mst_compose_rf_poll(rf_out_buf,sid);
      ret |= 0x2;
    }
  }
  else if(cmd == 0x69) //host control master
  {
	  if(subcmd == 0x11) //write 
	  {
	  	if(addr == 0)
      {
        //update time,sync to rtc
        Stru_curTime.hour  = *(inbuf+6);
        Stru_curTime.min   = *(inbuf+7);
        Stru_curTime.sec   = 0;
        Stru_curTime.month   = *(inbuf+8);
        Stru_curTime.day  = *(inbuf+9);
        Stru_curTime.dow   = *(inbuf+10);
        Stru_curTime.year   = *(inbuf+11)+2020;

        //Stru_curTime.caldone=1;
        *(ur_rsp_leng+0) = mst_ur_rsp_write_mst_a0(ur_out_buf);
        ret |= 0x1;
        ret |= 0x4;
        gPARAM_ACT_DATA[0] = 0x0;
      }
      else if(addr == 1)
      {
        Stru_curTime.year  = *(inbuf+6);
        Stru_curTime.month = *(inbuf+7);
        Stru_curTime.day   = *(inbuf+8);
        Stru_curTime.dow   = *(inbuf+9);
        //dow
        *(ur_rsp_leng+0) = mst_ur_rsp_write_mst_a1(ur_out_buf);
        ret |= 0x1;
        ret |= 0x4;
        gPARAM_ACT_DATA[0] = 0x1;
        Stru_curTime.caldone=1;
      }
      //else if(addr >= 0x20)&&(addr >= 0x40))
      //{
      //  ur_w_ee_page(*(inbuf+5),(inbuf+6));
      //}
	  }
    else if(subcmd == 0x35) //read
    {
      
    }    
  }
  else if(cmd == 0x78)//host control slv via master
  {
     if(subcmd == 0x11) //write 
	  {
      if(addr == 0)
      {
        
      }
	  	else if(addr == 4)//rly
      {
        //ur resp ok
        *(ur_rsp_leng+0) = mst_ur_rsp_write_slv_a4(ur_out_buf,sid);
        //compose rf tx data
        *(rf_send_leng+0) = mst_compose_rf_write_a4(rf_out_buf,sid,*(inbuf+6),*(inbuf+7));
        
        ret |= 0x3;
      }
	  }
    else if(subcmd == 0x35) //read
    {
      
    }    
  }
  else if(cmd == 0x83) //eep8
  {
	  if(subcmd == 0x11) //write 
	  {
      at24c32_write_page(I2C_PORT, AT24C32_ADDR, word_addr,(inbuf+7),8);
      
      //resp
      *(ur_rsp_leng+0) = mst_ur_rsp_rw_eep( word_addr,(inbuf+7),ur_out_buf);
      ret |= 0x1;
    }
    else if(subcmd == 0x35) //read
    {
      at24c32_read(I2C_PORT, AT24C32_ADDR,word_addr,&datatemp[0],8);
      //
      *(ur_rsp_leng+0) = mst_ur_rsp_rw_eep(word_addr,&datatemp[0],ur_out_buf);
      ret |= 0x1;
    }
    else{}
  }
  return ret;
}