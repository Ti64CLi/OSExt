#ifndef TIMER_H
#define TIMER_H



/*
	Timer IDs
	fast timer = 0
	first timer = 1
	second timer = 2
*/

struct timer_state {
	uint32_t load;
	uint32_t control;
	uint32_t bgload;
};

// the individual timer in the module doesn't matter, it's a setting for both
uint32_t timer_get_clockselect(uint32_t timermodule,uint32_t timer);
void timer_set_clockselect(uint32_t timermodule,uint32_t timer,uint32_t select);


void timer_save_state(uint32_t timermodule,uint32_t timer,struct timer_state *state);
void timer_resume_state(uint32_t timermodule,uint32_t timer,struct timer_state *state);

void timer_return_os(uint32_t timermodule,uint32_t timer,struct timer_state *state);


void timer_enable(uint32_t timermodule,uint32_t timer);
void timer_disable(uint32_t timermodule,uint32_t timer);

uint32_t timer_value(uint32_t timermodule,uint32_t timer);
void timer_set_load(uint32_t timermodule,uint32_t timer,uint32_t load);
void timer_set_bg_load(uint32_t timermodule,uint32_t timer,uint32_t bgload);

// 0 for 16 bit, 1 for 32 bit
void timer_set_size(uint32_t timermodule,uint32_t timer,uint32_t size);


void timer_set_prescaler(uint32_t timermodule,uint32_t timer,uint8_t prescale);
void timer_set_mode(uint32_t timermodule,uint32_t timer,uint8_t mode);


bool timer_irq_status(uint32_t timermodule,uint32_t timer);
void timer_irq_clear(uint32_t timermodule,uint32_t timer);



bool timer_irq_enabled(uint32_t timermodule,uint32_t timer);
void timer_set_irq_enabled(uint32_t timermodule,uint32_t timer,bool irq);

void timer_set_oneshot(uint32_t timermodule,uint32_t timer,bool oneshot);
bool timer_is_oneshot(uint32_t timermodule,uint32_t timer);



#endif