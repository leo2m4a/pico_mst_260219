#ifndef _MST_UR_PROC_H
#define _MST_UR_PROC_H
uint8_t mst_compose_rf_bcn(uint8_t *,uint8_t,uint8_t,uint8_t);
uint8_t mst_compose_rf_poll(uint8_t *,uint8_t );
uint8_t mst_compose_rf_write_a4(uint8_t *outbuf,uint8_t sid,uint8_t wen_rly,uint8_t wval_rly);

uint8_t mst_ur_rsp_echo(uint8_t *,uint8_t ,uint8_t);
uint8_t mst_ur_rsp_write_mst_a0(uint8_t *);
uint8_t mst_ur_rsp_write_mst_a1(uint8_t *);
uint8_t mst_ur_rsp_write_slv_a4(uint8_t *,uint8_t );
uint8_t mst_ur_cmd_process(uint8_t *inbuf,uint8_t *,uint8_t *,uint8_t *,uint8_t *);


extern uint32_t gPARAM_fcnt_1s;
extern TimeData Stru_curTime;
#endif