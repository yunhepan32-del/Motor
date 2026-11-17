#ifndef S6_PORT_H
#define S6_PORT_H
#include "s6_control.h"
uint32_t S6P_time(void);
void S6P_init(void);void S6P_start(void);void S6P_stop(void);
uint16_t S6P_preflight(void);
void S6P_read(S6_Input *x);void S6P_ack(const S6_Input *x);
void S6P_finishInput(S6_Input *x);
void S6P_overflow(uint16_t x[3]);void S6P_interruptAck(void);
void S6P_backgroundLock(void);void S6P_registers(uint16_t x[32]);
uint16_t S6P_stagePWM(const uint16_t compare[3],uint16_t requested);
#endif
