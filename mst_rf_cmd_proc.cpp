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

uint8_t mst_compose_rf_rsp_echo(uint8_t *inbuf,uint8_t *outbuf)
{
  uint8_t ret=16;
  *(outbuf+0) = 0xa0;
  *(outbuf+1) = ret;
  *(outbuf+2) = *(inbuf+2);
  *(outbuf+3) = *(inbuf+3);
  //
  *(outbuf+4) = 0xaa;
  *(outbuf+5) = 4;
  *(outbuf+6) = *(inbuf+6);
  *(outbuf+7) = *(inbuf+7);
  //
  *(outbuf+8) = *(inbuf+8);
  *(outbuf+9) = *(inbuf+9);
  *(outbuf+10) = *(inbuf+10);
  *(outbuf+11) = *(inbuf+11);  
  //
  *(outbuf+12) = *(inbuf+12);
  *(outbuf+13) = *(inbuf+13);
  *(outbuf+14) = *(inbuf+14);
  *(outbuf+15) = 0xaa;//*(inbuf+15);    
  return ret;
}
