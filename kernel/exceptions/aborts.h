#ifndef ABORTS_H
#define ABORTS_H


extern bool probe_address;
extern uint32_t undef_stack[256];
extern uint32_t abort_stack[256];
extern uint32_t fiq_stack[256];


bool install_exception_handlers();



void set_exception_vectors(bool high);




































#endif