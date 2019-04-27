from visdom import Visdom
import numpy as np 

from logging import warning
import subprocess
from time import gmtime, strftime

env = strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ' ' \
    + subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode().strip()
env = 'main'

viz = Visdom(env=env) 
if not viz.check_connection(timeout_seconds=3): 
    viz = None
    warning('No connection could be formed quickly')

def clean_envs(): 
    if viz is None: 
        return 
    for env in viz.get_env_list()[1:]: 
        viz.delete_env(env)

class Window: 
    step = 0 
    def __init__(self, title='', ppp=128, pool='avg'): 
        self.title = title
        self.ppp = max(64, ppp) if viz is None else ppp # TODO 
        self.pool = pool 

        if viz is not None: 
            self.win = viz.line(X=np.array([0]), 
                                Y=np.array([0]), 
                                env=env, 
                                opts=dict(title=title))

        self.buffer = [] 

    def append(self, step, value): 
        self.buffer.append(value)
        if len(self.buffer)<self.ppp: 
            return 
        
        if self.pool=='avg': 
            y = np.mean(self.buffer)
        elif self.pool=='max': 
            y = np.max(self.buffer)
            
        if viz is None: 
            if step>Window.step: 
                Window.step = step
                print('\ntime={}: '.format(step), end='') 
            print('{}={} '.format(self.title, y), end='')
        else: 
            viz.line(X=np.array([step]), 
                     Y=np.array([y]), 
                     win=self.win, 
                     env=env, 
                     update='append')

        self.buffer = []
