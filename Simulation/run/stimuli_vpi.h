#ifndef _SAMPLE_H_ 
#define _SAMPLE_H_ 

#include <vpi_user.h> 

typedef struct t_hook_record { 
    vpiHandle   obj; 
    int         p_key; 
    int         key; 
    PLI_BYTE8   *user_data; 
    
    vpiHandle   cb; 
} s_hook_record, *p_hook_record; 

#endif // _SAMPLE_H_
