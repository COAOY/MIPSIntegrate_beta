from stimuli.parse_path import * 
from stimuli.stimuli_gen import * 

from itertools import product

inst_cnt = np.zeros(len(Stimuli.state_space))
win_cover = Window(title='Coverage', ppp=1, pool='max')
# win_r_avg= Window(title='Reward-Avg', ppp=250)
# win_r_max= Window(title='Reward-Max', ppp=50, pool='max')

class StimuliTop(object): 
    def __init__(self, fname): 
        self.paths = parse_path(fname)
        self.monitored = list(Path.monitored) # TODO 

        self.idx_cnt = {} 

        self.cur_path_idx = 0 
        self.cur_goal = self.__get_goal(0)
        self.sampled_goal = self.__sample_goal() 
        self.idx_cnt[0] = 1

        self.gen = StimuliGen(\
            [c.nbit for c in Path.monitored.values()])

        self.finished = False 
        self.step = 0 

    def remember(self, state, action, monitored): 
        self.gen.remember(state, action, monitored)
        self.gen.experience_replay()
        self.step+=1 
        # win_r_avg.append(self.step, float(monitored==goal))
        # win_r_max.append(self.step, float(monitored==goal))
        inst_cnt[state]+=1
        if not self.finished: 
            win_cover.append(self.step, self.check_sens(monitored)) 

    def act(self, state): 
        return self.gen.act(state, self.sampled_goal) 

    def check_sens(self, monitored): 
        idx_updated = False 
        sensitized = 0
        for i in range(len(self.paths)): 
            goal = self.__get_goal(i)
            if self.paths[i].sensitized: 
                sensitized+=1
            elif self.__is_sens(monitored, goal): 
                self.paths[i].sensitized = True 
                sensitized+=1
            else: 
                targeted_cnt = self.idx_cnt.get(i)
                if idx_updated or (targeted_cnt is not None and targeted_cnt>=128): 
                    continue 
                self.cur_path_idx = i 
                self.cur_goal = self.__get_goal(i)
                self.sampled_goal = self.__sample_goal() 
                idx_updated = True 
                if targeted_cnt is None: 
                    self.idx_cnt[i] = 0 
                self.idx_cnt[i]+=1 
        
        self.finished = (sensitized==len(self.paths))
        return sensitized/len(self.paths)

    def __is_sens(self, monitored, goal): #TODO 
        for i, g in enumerate(goal): 
            if g is None: 
                continue 
            for j, b in enumerate(g): 
                if not b.isdigit(): 
                    continue
                gbit = 1 if b=='1' else 0 
                mbit = (monitored[i]>>j)&1
                if gbit!=mbit: 
                    return False  
        
        return True  

    def __get_goal(self, idx): #TODO 
        # return [self.paths[idx].consts0.get(str, \
        #     None) for str in Path.monitored]
        ret = [] 
        for sig in Path.monitored: 
            const = self.paths[idx].consts0.get(sig, None) 
            if const is not None: 
                ret.append(const.tostring())
            else:  
                ret.append(None)

        return ret

    def __sample_goal(self): #TODO
        return [self.__bitstr2int(str) for str in self.cur_goal]

    def plot_model(self): 
        if viz is None: 
            return 

        # plot generated inst dist
        label = [i.name for i in Stimuli.state_space]
        viz.bar(X=inst_cnt, opts=dict(title='Instructions', 
                rownames=label))

        # plot targeted PDFs dist
        viz.bar(X=list(self.idx_cnt.values()), 
            opts=dict(title='Targeted PDFs', 
            rownames=list(self.idx_cnt)))

        # plot goal mapping 
        seq = [] 
        for i in range(len(self.paths)): 
            # if not self.paths[i].sensitized: 
            g = self.__get_goal(i)
            seq.append(self.gen.policy(g, g))

        label = [Stimuli.state_space[i].name for i in self.gen.model.classes_]
        T = np.vstack(seq)
        viz.heatmap(X=T, opts=dict(title='Goal Mapping', 
                         columnnames=label))

        self.gen.plot_model()

    def __bitstr2int(self, str): 
        if str is None: 
            return None 
        x_count = str.count('x') 
        if x_count==0: 
            return int(str, 2) 
        v = str
        x = nbit_randrange(x_count).to01()
        for i in range(x_count): 
            v = v.replace('x', x[i], 1) 

        return int(v, 2)

if __name__ == '__main__': 
    if len(sys.argv) != 2: 
        print('USAGE: python stimuli_top.py <filename>')
        exit(-1) 
    
    top= StimuliTop(sys.argv[1]) 
    state = 0 
    for _ in range(256): 
        next_state, inst = top.act(state) 
        monitored = [] 
        for nbit in [c.nbit for c in Path.monitored.values()]: 
            monitored.append(random.randrange(2**nbit))        

        top.remember(state, next_state, monitored)
        state = next_state
        print('%d\t0x%s' % (state, inst))
    
    top.plot_model()

