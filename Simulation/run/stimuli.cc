#include <Python.h> 
#include <dlfcn.h>

#include "stimuli.h"

static PyObject *pTop; 
Path *pyobj2carray(PyObject *, int &); 

int stimuli_init() { 
    PyObject *pName, *pModule, *pFunc; 
    PyObject *pArgs, *pValue; 

    dlopen("libpython3.6m.so.1.0", RTLD_LAZY | RTLD_GLOBAL); 
    Py_Initialize(); 
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append(\"..\")");

    pName = PyUnicode_DecodeFSDefault("stimuli_top"); 
    
    pModule = PyImport_Import(pName);
    Py_DECREF(pName); 

    if (pModule!=NULL) { 
        pFunc = PyObject_GetAttrString(pModule, "StimuliTop"); 
        if (pFunc&&PyCallable_Check(pFunc)) { 
            pArgs = PyTuple_New(1);  
            pValue = PyUnicode_FromString("../../../Hardware/verilog/antares_alu.path"); 
            if (!pValue) { 
                Py_DECREF(pArgs); 
                Py_DECREF(pModule);
                fprintf(stderr, "Cannot get argument\n");
                return 1;
            }
            PyTuple_SetItem(pArgs, 0, pValue);
            pTop= PyObject_CallObject(pFunc, pArgs); 
            Py_DECREF(pArgs);
            if (pTop!=NULL) { 
                printf("INFO-PYTHON:\tStimuli top module Initialized\n"); 
            } else { 
                PyErr_Print();
                fprintf(stderr,"Stimuli top module initialized failed\n");
                return 1;
            } 
        } else { 
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Failed to load module \"StimuliTop\"\n"); 
        }
    } else { 
        PyErr_Print();
        fprintf(stderr, "Failed to load \"stimuli_top\"\n");
        return 1;
    }

    return 0; 
} 

int stimuli_gen(int state, int &next_state, int &inst) { 
    PyObject *pValue, *pState, *pInst; 

    if (pTop!=NULL) { 
        pValue = PyObject_CallMethod(pTop, "act", "(i)", state); 
        if (pValue&&PyTuple_Check(pValue)) { 
            pState = PyTuple_GetItem(pValue, 0); 
            pInst = PyTuple_GetItem(pValue, 1); 
            next_state = (int) PyLong_AsLong(pState); 
            inst = (int) PyLong_AsLong(pInst); 
        } else { 
            PyErr_Print();
            fprintf(stderr,"Call failed\n");
            return 1;
        }
    } else { 
        PyErr_Print();
        fprintf(stderr,"Stimuli top module initialized failed\n");
        return 1;
    }
    return 0; 
}

int remember(int state, int action, int *monitored, int n_mon) { 
    if (pTop!=NULL) { 
        PyObject *m = PyList_New(0); 
        for (int n=0; n<n_mon; n++) 
            PyList_Append(m, PyLong_FromLong(monitored[n])); 
        PyObject_CallMethod(pTop, "remember", "(iiO)", state, \
                            action, m); 
    } else { 
        PyErr_Print();
        fprintf(stderr,"Stimuli top module initialized failed\n");
        return 1;
    }
    return 0; 
} 

int plot_model() { 
    if (pTop!=NULL) { 
        PyObject_CallMethod(pTop, "plot_model", "");  
    } else { 
        PyErr_Print();
        fprintf(stderr,"Stimuli top module initialized failed\n");
        return 1;
    }
    return 0; 
}

int get_paths(Path *&p, int &n_path) { 
    PyObject *pPaths; 

    if (pTop!=NULL) { 
        pPaths = PyObject_GetAttrString(pTop, "paths"); 
        if (pPaths != NULL) {
            p = pyobj2carray(pPaths, n_path); 
        } else { 
            PyErr_Print();
            fprintf(stderr,"Call failed\n");
            return 1;
        }       
    } else { 
        PyErr_Print();
        fprintf(stderr,"Stimuli top module initialized failed\n");
        return 1;
    }
    return 0; 
}

int get_monitored(char **&monitored, int &n_mon) { 
    if (pTop!=NULL) { 
        PyObject *pMonitor = PyObject_GetAttrString(pTop, "monitored"); 
        if (pMonitor!= NULL) {
            n_mon = PySequence_Size(pMonitor); 
            PyObject *m = PySequence_Fast(pMonitor, "expected a sequence"); 
            if (m && PyList_Check(m) && n_mon>0) { 
                monitored = (char **) malloc(n_mon*sizeof(char *)); 
                for (int k=0; k<n_mon; k++) { 
                    PyObject *pReg = PySequence_Fast_GET_ITEM(m, k); 
                    monitored[k] = strdup(PyUnicode_AsUTF8(pReg)); 
                    // printf("%s ", monitored[k]); 
                }
                // printf("\n"); 
            }
        } else { 
            PyErr_Print();
            fprintf(stderr,"Call failed\n");
            return 1;
        }       
    } else { 
        PyErr_Print();
        fprintf(stderr,"Stimuli top module initialized failed\n");
        return 1;
    }
    return 0; 
}

Path *pyobj2carray(PyObject *pObj, int &n_path) {
    Path *p = NULL; 
    PyObject *seq = PySequence_Fast(pObj, "expected a sequence");  
    n_path = PySequence_Size(pObj); 
    p = (Path *) malloc(n_path * sizeof(Path)); 
    if (seq && PySequence_Check(seq) && n_path>0) { 
        for (int i=0; i<n_path; i++) { 
            PyObject *pPath = PySequence_Fast_GET_ITEM(seq, i); 

            PyObject *pList = PyObject_GetAttrString(pPath, "path");  
            PyObject *pStr = PyObject_GetAttrString(pPath, "end_point"); 
            
            int len = PySequence_Size(pList); 
            PyObject *path = PySequence_Fast(pList, "expected a sequence"); 
            if (path && PyList_Check(path) && len>0) { 
                p[i].path = (char **) malloc(len*sizeof(char *)); 
                p[i].end_point = strdup(PyUnicode_AsUTF8(pStr)); 
                p[i].sensitized = (bool *) malloc(len*sizeof(bool)); 
                // p->sensitized = {false}; 
                p[i].len = len; 

                for (int j=0; j<len; j++) { 
                    PyObject *pNet = PySequence_Fast_GET_ITEM(path, j); 
                    p[i].path[j] = strdup(PyUnicode_AsUTF8(pNet)); 
                }

            } else { 
                if (PyErr_Occurred()) PyErr_Print();
                fprintf(stderr, "Expected a List\n"); 
            }
            Py_DECREF(path); 
        }
    } else { 
        if (PyErr_Occurred()) PyErr_Print();
        fprintf(stderr, "Expected a Sequence\n"); 
    }
    Py_DECREF(seq); 

    return p; 
} 
