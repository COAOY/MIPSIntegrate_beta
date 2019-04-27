#ifndef _LEARNER_H_ 
#define _LEARNER_H_ 

typedef struct {
    char **path; 
    char *end_point; 

    // char **monitored; 
    bool *sensitized; 

    int len; 
} Path;  

extern int stimuli_init(); 
extern int get_paths(Path *&, int &); 
extern int get_monitored(char **&, int &);  

extern int stimuli_gen(int, int &, int &); 
extern int remember(int, int, int*, int); 
extern int plot_model(); 

#endif //_LEARNER_H_ 
