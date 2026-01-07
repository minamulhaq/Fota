#ifndef _INCLUDE_FSM_HPP__
#define _INCLUDE_FSM_HPP__

typedef enum 
{
    READ_ID,
    READ_LENGTH,
    READ_PAYLOAD,
    READ_CRC,
} fsm_state_t;

#endif // _INCLUDE_FSM_HPP__
