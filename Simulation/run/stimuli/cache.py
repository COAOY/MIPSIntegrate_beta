import numpy as np 

import random  
from collections import deque

from .stimuli_mapping import * 

class NBitRandrange(object): 
    def __init__(self, n_bit, start=0, step=1): 
        self.n_bit = n_bit 
        self.start = start 
        self.step = step 
    
    def __call__(self): 
        template = '{0:0%db}'%(self.n_bit)
        return bitarray(template.format(random.randrange(self.start, 2**self.n_bit, self.step)))

def nbit_randrange(n_bit, start=0, step=1): 
    return NBitRandrange(n_bit, start, step)() 

class Cache(object): 
    def __init__(self, size, hit_rate, lamb, n_bit, start=0, step=1): 
        self.hit_rate = hit_rate  
        self.prob = np.fromfunction(lambda i: lamb*(1-lamb)**i, (size, ))
        self.rand = NBitRandrange(n_bit, start, step) 

        self.size = size 
        self.mem = deque(maxlen=size) 

    def read(self): 
        if random.random() > self.hit_rate:  
            return self.rand() 
        idx = np.random.choice(self.size, p=self.prob)
        return self.mem[idx] if idx<len(self.mem) else self.rand()

    def write(self, data): 
        self.mem.appendleft(data)
            
        
