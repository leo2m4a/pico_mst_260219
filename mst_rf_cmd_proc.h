#ifndef _MST_RF_CMD_PROC_H_
#define _MST_RF_CMD_PROC_H_

extern uint8_t gDisp_BUF_2[8];
extern uint8_t gDisp_BUF_3[8];
uint8_t mst_rf_cmd_process(uint8_t *,uint8_t *,uint8_t *,uint8_t *);
uint8_t mst_compose_ur_rsp_w4(uint8_t *,uint8_t *);
uint8_t mst_compose_rf_rsp_echo(uint8_t *,uint8_t *);
#endif