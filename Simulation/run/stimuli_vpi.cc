#include "stimuli_vpi.h"
#include "stimuli.h"

#include <stdlib.h>
#include <string.h>

static Path *p = NULL; 
static int n_path = -1; 
static char **monitored; 
static int n_mon; 
static int *m; 
static int state = 0, next_state = -1; 

PLI_INT32 stimuli_gen_calltf(PLI_BYTE8 *user_data) { 
    vpiHandle callh;
    s_vpi_value value;

    callh = vpi_handle(vpiSysTfCall, 0);

    value.format = vpiIntVal;
    int inst; 
    stimuli_gen(state, next_state, inst); 
    value.value.integer = inst;

    vpi_put_value(callh, &value, 0, vpiNoDelay);

    return 0;
}

PLI_INT32 stimuli_gen_sizetf(PLI_BYTE8 *user_data) { return(32); }

PLI_INT32 state_calltf(PLI_BYTE8 *user_data) { 
    vpiHandle callh;
    s_vpi_value value;

    callh = vpi_handle(vpiSysTfCall, 0);

    value.format = vpiIntVal;
    value.value.integer = next_state;

    vpi_put_value(callh, &value, 0, vpiNoDelay);

    return 0;
}

PLI_INT32 state_sizetf(PLI_BYTE8 *user_data) { return(7); }

typedef struct { 
    int start_idx; 
    int bits; 
} Idx; 

Idx get_index(char *name) { 
    Idx ret; ret.start_idx = -1; ret.bits = -1; 
    char *pch = strtok(name, "[:]"); int i = 0;  
    while (pch!=NULL) { 
        if (i==1) { 
            ret.start_idx = atoi(pch); 
            ret.bits = 1; 
        } 
        else if (i==2) {
            int end_idx = atoi(pch); 
            ret.bits = ret.start_idx - end_idx + 1; 
            ret.start_idx = end_idx; 
            return ret; 
        }
        pch = strtok(NULL, "[:]");

        i++; 
    }

    return ret;  
}

static int eval_calltf(PLI_BYTE8 *data) {
    s_vpi_value val; val.format = vpiIntVal; 
    vpiHandle signal_h; 

    // float sensitized_sum = 0.f; 
    // for (int n=0; n<n_path; n++) { 
    for (int n=0; n<0; n++) { 
        int sensitized = 0; 
        for (int i=0; i<p[n].len; i++) { 
            if(p[n].sensitized[i]) sensitized++; 
            else break; 
        }
        for (int i=0; i<p[n].len; i++) { 
            p[n].sensitized[i] = false; 
        }

        // if (sensitized==p[n].len) { 
        if (false) { 
            char str[256]; 
            strcpy(str, "tb_core.core."); 
            strcat(str, p[n].end_point); 
            // strcat(str, ".Q"); 
            printf("%s\n", str); 

            signal_h = vpi_handle_by_name(str, NULL); 
            vpi_get_value(signal_h, &val); 
            vpi_printf("\t[%d]: %s = [%d]->", n, p[n].end_point, val.value.integer); 
            val.value.integer = val.value.integer^1; 
            vpi_printf("[%d]\n", val.value.integer); 
            // vpi_put_value(signal_h, &val, NULL, vpiNoDelay); 
        } 
        // float sensitized_rate = (float) sensitized / p[n].len; 
        // sensitized_sum += sensitized_rate; 
    } 

    PLI_INT32 inst; 
    signal_h = vpi_handle_by_name("tb_core.monitor0.ex_param", NULL); 
    vpi_get_value(signal_h, &val); 
    inst = val.value.integer; 

    for (int n=0; n<n_mon; n++) { 
        char str[256]; 
        strcpy(str, "tb_core.core."); 
        Idx idx = get_index(monitored[n]); 
        strcat(str, monitored[n]); 
        // printf("%s\n", str); 
        signal_h = vpi_handle_by_name(str, NULL); 
        vpi_get_value(signal_h, &val); 
        if (idx.start_idx==-1) 
            m[n] = (int) val.value.integer; 
        else { 
            m[n] = (((int) val.value.integer)>>idx.start_idx)&((1<<idx.bits)-1); 
        }
    } 

    // remember(state, next_state, (float) sensitized_sum/n_path); 
    remember((int) inst, next_state, m, n_mon); 
    state = next_state; 

    return 0; 
}

PLI_INT32 plot_model_calltf(PLI_BYTE8 *data) { 
    plot_model(); 
    
    printf("INFO-VPI:\tOutput Model...\n"); 
    return 0; 
}

void tf_register() {
    s_vpi_systf_data tf; 

    tf.type         = vpiSysTask; 
    tf.tfname       = "$eval"; 
    tf.calltf       = eval_calltf; 
    tf.compiletf    = NULL; 
    tf.sizetf       = 0; 
    tf.user_data    = 0; 

    vpi_register_systf(&tf); 

    tf.type         = vpiSysTask; 
    tf.tfname       = "$plot"; 
    tf.calltf       = plot_model_calltf; 
    tf.compiletf    = NULL; 
    tf.sizetf       = 0; 
    tf.user_data    = 0; 

    vpi_register_systf(&tf); 

    tf.type         = vpiSysFunc; 
    tf.sysfunctype  = vpiSysFuncSized; 
    tf.tfname       = "$stimuli_gen"; 
    tf.calltf       = stimuli_gen_calltf; 
    tf.compiletf    = NULL; 
    tf.sizetf       = stimuli_gen_sizetf; 
    tf.user_data    = NULL; 

    vpi_register_systf(&tf); 

    tf.type         = vpiSysFunc; 
    tf.sysfunctype  = vpiSysFuncSized; 
    tf.tfname       = "$state"; 
    tf.calltf       = state_calltf; 
    tf.compiletf    = NULL; 
    tf.sizetf       = state_sizetf; 
    tf.user_data    = NULL; 

    vpi_register_systf(&tf); 
}

PLI_INT32 cb_val_chg(s_cb_data *cb_data) { 
    vpiHandle rst_h; 
    rst_h = vpi_handle_by_name("tb_core.rst_sync", NULL); 
    s_vpi_value val; 
    val.format = vpiIntVal; 
    vpi_get_value(rst_h, &val); 
    if (val.value.integer==0) { 
        p_hook_record hook = (p_hook_record) cb_data->user_data; 
        p[hook->p_key].sensitized[hook->key] = ~p[hook->p_key].sensitized[hook->key]; 
    } 
    return 0; 
} 

void register_cb_val_chg(int p_key, int key, char *name) {
    char str[256]; 
    strcpy(str, "tb_core.core."); 
    int idx = get_index(name).start_idx; 
    strcat(str, name); 
    // printf("%s\n", str); 

    vpiHandle reg_h; 
    reg_h = vpi_handle_by_name(str, NULL); 
    if (idx>=0) 
        reg_h = vpi_handle_by_index(reg_h, idx); 

    p_hook_record hook = (p_hook_record) malloc(sizeof(s_hook_record)); 
    hook->obj = reg_h; 
    hook->p_key = p_key; 
    hook->key = key; 
    hook->user_data = name; 

    s_cb_data cb_data; 
    cb_data.reason      = cbValueChange; 
    cb_data.cb_rtn      = cb_val_chg; 
    cb_data.obj         = reg_h; 
    cb_data.time        = NULL; 
    cb_data.value       = NULL; 
    cb_data.user_data   = (PLI_BYTE8 *) hook; 
    hook->cb = vpi_register_cb(&cb_data); 
}

PLI_INT32 cb_sim_start(s_cb_data *cb_data) { 
    stimuli_init(); 
    get_paths(p, n_path); 
    get_monitored(monitored, n_mon); 
    m = (int *) malloc(n_mon*sizeof(int)); 
    printf("INFO-VPI:\t%d Critical Paths Registered\n", n_path); 
    // for (int i=0; i<n_path; i++) { 
    for (int i=0; i<0; i++) { 
        for (int j=0; j<p[i].len; j++) 
            register_cb_val_chg(i, j, p->path[j]); 
    } 
    printf("INFO-VPI:\tTask/Function/Callback Registered\n"); 
    return 0; 
}

void register_cb_sim_start() {
    vpiHandle cb_h; 
    s_cb_data cb_data; 
    cb_data.reason      = cbStartOfSimulation; 
    cb_data.cb_rtn      = cb_sim_start; 
    cb_data.user_data   = NULL; 
    cb_h = vpi_register_cb(&cb_data); 
    vpi_free_object(cb_h); 
} 

void (*vlog_startup_routines[])() = { 
    tf_register, 
    register_cb_sim_start, 
    0
}; 
