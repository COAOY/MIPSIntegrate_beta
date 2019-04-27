import numpy as np 

from sklearn.preprocessing import OneHotEncoder
from xgboost import XGBClassifier
# from sklearn.multioutput import MultiOutputRegressor

from .window import * 
from .stimuli_define import * 

# win_r_avg= Window(title='Reward-Avg', ppp=250)
# win_r_max= Window(title='Reward-Max', ppp=50, pool='max')
win_loss = Window(title='Error Rate') 

batch_size = 64

class StimuliGen(object): 
    def __init__(self, nbit): 
        self.exploration_rate = 1.0

        self.action_space = len(Stimuli.state_space)
        self.mem = deque(maxlen=256)

        nval = [range(2**n) for n in nbit]
        self.enc = OneHotEncoder(categories=nval, \
            handle_unknown='ignore') 
        self.model = XGBClassifier(objective='multi:softmax')

        self.is_fitted = False 
        self.step = 0 
        
    def remember(self, state, action, monitored): 
        self.mem.append((state, action, monitored))
        self.step+=1 
        # win_r_avg.append(self.step, (float) (monitored==goal))
        # win_r_max.append(self.step, (float) (monitored==goal))

    def act(self, state, goal): 
        if (not self.is_fitted) or random.random() < self.exploration_rate: 
            next_state = random.randrange(self.action_space) 
        else: 
            prob = self.policy(state, goal)
            next_state = np.random.choice(self.model.classes_, p=prob)

        inst = Stimuli.state_space[next_state]()
        return next_state, int(inst, 16)

    def policy(self, state, goal): 
        return self.model.predict_proba(self.enc.transform([goal]))[0]

    def experience_replay(self): 
        if len(self.mem)<batch_size:  
            return 
        batch = random.sample(self.mem, batch_size) 
        X = np.stack([i[2] for i in batch]) #monitored 
        y = np.fromiter((i[0] for i in batch), np.int64)
        # action = np.fromiter((i[1] for i in batch), np.int64).reshape(-1, 1)  

        X_1hot = self.enc.fit_transform(X)

        y_pred = self.model.fit(X_1hot, y).predict(X_1hot)
        self.is_fitted = True 

        win_loss.append(self.step, (y!=y_pred).sum()/batch_size)
        self.exploration_rate *= 0.995
        self.exploration_rate = max(0.1, self.exploration_rate)  

    def plot_model(self): 
        pass 
                
if __name__ == "__main__": 
    gen = StimuliGen()
    state = 0 
    for _ in range(10): 
        next_state, inst = gen.act(state) 
        reward = np.random.rand()
        gen.remember(state, next_state, reward)
        gen.experience_replay()
        state = next_state
        print('%d\t0x%s' % (state, inst))

    gen.plot_model()
