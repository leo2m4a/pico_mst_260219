#include <stdio.h>
#include "pico/stdlib.h"
#include "global_def.h"
#include "mst_rf_cmd_proc.h"

uint8_t mst_rf_cmd_process(uint8_t *inbuf,uint8_t *rspbuf,uint8_t *rspleng,uint8_t *dispbuf)
{
  uint8_t ret = 0;
  uint8_t cmd = *(inbuf+0);
  uint8_t sid = *(inbuf+3);
  uint8_t subcmd = *(inbuf+4);
  uint8_t addr = *(inbuf+5);
  if(cmd == 0xaa)//receive slv ack
  {
    if(subcmd == 0xaa)
    {
      //save to 
      /*
      *(dispbuf+0) = 6;
      //convert for display
      *(dispbuf+1) = *(inbuf+4);//TS
      *(dispbuf+2) = *(inbuf+5);//sid
      *(dispbuf+3) = *(inbuf+6);//temp
      *(dispbuf+4) = *(inbuf+7);
      *(dispbuf+5) = *(inbuf+8);
      *(dispbuf+6) = *(inbuf+9);
      */
     *(rspleng+0) = mst_compose_rf_rsp_echo(inbuf,rspbuf);
     ret = 0x1;
    }
  }
  else if(cmd == 0x89)
  {
    if(subcmd == 0x11)//write
    {
      if(addr == 4)//rly,io
      {
        *(rspleng+0) = mst_compose_ur_rsp_w4(inbuf,rspbuf);
        //compose uart response
        ret |= 0x1;
      }
    }
    else if(subcmd == 0x35)//read
    {}
  }
  
  //display receive data at lcd,fill display buffer
  *(dispbuf+0) = 7;
  *(dispbuf+1) = *(inbuf+0);//sid
  //*(dispbuf+2) = *(inbuf+5);//fcnt
  *(dispbuf+2) = *(inbuf+1);//*(inbuf+6);//TH
  *(dispbuf+3) = *(inbuf+2);//*(inbuf+7);//TL
  *(dispbuf+4) = *(inbuf+3);
  *(dispbuf+5) = *(inbuf+4);
  *(dispbuf+6) = *(inbuf+10);
  
  ret  |= 0x2;
  return ret;
}

uint8_t mst_compose_ur_rsp_w4(uint8_t *inbuf,uint8_t *outbuf)
{
  uint8_t ret=12;
  *(outbuf+0) = 0x88;
  *(outbuf+1) = ret;
  *(outbuf+2) = *(inbuf+2);
  *(outbuf+3) = *(inbuf+3);
  //
  *(outbuf+4) = 0xaa;
  *(outbuf+5) = 4;
  *(outbuf+6) = *(inbuf+5);
  *(outbuf+7) = *(inbuf+6);
  //
  *(outbuf+8) = *(inbuf+7);
  *(outbuf+9) = *(inbuf+8);
  *(outbuf+10) = 0;
  *(outbuf+11) = 0;  
  return ret;
}

// slave poll 回應 → host 的 echo 封包
// [fix] 由 16 bytes 加長到 24 bytes：原本 byte 15 固定填 0xAA、長度只到 15，
//       slave 回應裡的 DS18B20（inbuf[15..16]）傳不到 host。
//
//  byte : 0    1    2    3    4    5    6     7     8    9   10    11    12    13    14    15   16   17-22  23
//  value: A0   18   MID  SID  AA   04   tmpH  tmpL  rly  io  adc0  humH  humL  adcH  adcL  dsH  dsL  00     AA
//  (byte 6-16 直接取自 slave 回應 inbuf[6..16]；byte 0-14 位置與舊 16-byte 格式相同)
uint8_t mst_compose_rf_rsp_echo(uint8_t *inbuf,uint8_t *outbuf)
{
  uint8_t ret=24;
  int i;
  *(outbuf+0) = 0xa0;
  *(outbuf+1) = ret;
  *(outbuf+2) = *(inbuf+2);
  *(outbuf+3) = *(inbuf+3);
  //
  *(outbuf+4) = 0xaa;
  *(outbuf+5) = 4;
  // 6..16: tempH, tempL, rly, io, adc0, humH, humL, adcH, adcL, dsH, dsL
  for(i=6;i<=16;i++)
  {*(outbuf+i) = *(inbuf+i);}
  // 17..22: 保留
  for(i=17;i<=22;i++)
  {*(outbuf+i) = 0;}
  *(outbuf+23) = 0xaa;
  return ret;
}
