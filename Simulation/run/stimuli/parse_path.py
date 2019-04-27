from logging import warning
import sys 

import re

IGNORE_SIG = ['clk', 'rst']

def get_name(lhs): 
    ret = re.split('\[([^[\]]*)\]', lhs)
    if len(ret)==3: 
        return ret[0], ret[1]
    elif len(ret)==1: 
        return ret[0], ''
    else:     
        assert False, 'lhs `{}\' not supported'.format(lhs)

class Const(object): 
    def __init__(self, lhs, rhs): 
        assert len(rhs)>0, 'failed to parse empty rhs'
        self.name, idx_str = get_name(lhs)
        self.const = {} 

        self.nbit = 0
        idx = self.__parse_idx(idx_str, len(rhs))
        assert len(idx)==len(rhs), 'found bit length unmatched between rhs `{}\' \
            ({}) and {}'.format(rhs, len(rhs), lhs)
            
        rhs = rhs[::-1]
        bits = [i for i, b in enumerate(rhs) if b.isdigit()]
        for i in bits: 
            self.const[idx[i]] = 0 if rhs[i]=='0' else 1 

    def update(self, lhs, rhs): 
        name, idx_str = get_name(lhs)
        assert self.name==name, \
            'wrong signal name `{}\', expect `{}\''.format(name, self.name)

        idx = self.__parse_idx(idx_str, len(rhs))
        assert len(idx)==len(rhs), 'found bit length unmatched between rhs `{}\' \
            ({}) and {}'.format(rhs, len(rhs), lhs)

        rhs = rhs[::-1]
        bits = [i for i, b in enumerate(rhs) if b.isdigit()]
        for i in bits: 
            if idx[i] in self.const:  
                if (not rhs[i]=='x') and self.const[idx[i]]!=(0 if rhs[i]=='0' else 1): 
                    warning('updating rhs `%s\' conflicts with original `%s\''%(rhs[::-1], self.tostring()[::-1]))
                    # return False # TODO  
                    return True 
            else: 
                self.const[idx[i]] = 0 if rhs[i]=='0' else 1 

        return True

    def __parse_idx(self, idx_str, len_rhs): 
        idx = idx_str.split(':')
        if len(idx)==1 and (not idx[0]==''): 
            ret = [int(idx[0])]
            nbit = int(idx[0]) + 1
        elif len(idx)==2: 
            nbit = int(idx[0]) + 1
            ret  = list(range(int(idx[1]), nbit))
        elif len(idx)>2: 
            assert False, \
                'error index format `{}\' not supported'.format(lhs)
        else: 
            nbit = len_rhs 
            ret =  list(range(nbit))

        self.nbit = nbit if nbit>self.nbit else self.nbit
        
        return ret 

    def tostring(self): 
        ret = ['x'] * (self.nbit)
        for i in self.const: 
            ret[i] = str(self.const[i])
        return ''.join(ret)

    def split(self): 
        pass #TODO

class Path(object): 
    monitored = {}

    def __init__(self, name, path_lines): 
        self.name = name 
        self.path = path_lines 
        # self.end_point = path_lines[-1].rsplit('.', 1)[0]
        self.end_point = path_lines[-1]
        self.consts0 = {} 
        self.consts1 = {} 

        self.sensitized= False 

    def parse_const(self, lhs, rhs): 
        sig, _ = get_name(lhs)
        if sig in IGNORE_SIG: 
            return True 

        if sig not in self.consts0: 
            self.consts0[sig] = Const(lhs, rhs)
            return True
        else: 
            return self.consts0[sig].update(lhs, rhs)

    def final(self): 
        for sig in self.consts0: 
            if self.consts0[sig].nbit>6: #TODO 
                continue
            # self.consts0[sig].split() 
            Path.monitored[sig] = self.consts0[sig]

    def dump(self): 
        print('name: '+self.name)
        print('from: '+self.path[0]) 
        print('to: '+self.end_point) 
        print('transition: ')
        for p in self.path: 
            print('\t'+p) 

        print('constraints: ')
        for s in self.consts0: 
            print('\t'+s+'({}): '.format(self.consts0[s].nbit)\
                +self.consts0[s].tostring()[::-1])

        # print(self.consts1)
        print('')

def get_paths(lines): 
    paths = [] 

    pos = 0; 

    while pos<len(lines): 
        line = lines[pos] 
        if line.strip()=="":
            pos+=1 
            continue
        keyword = line.split()[0].strip() 
        if keyword=="$transition": 
            path_lines = [] 
            pos+=1 
            const_is_parsed = False 
            while pos<len(lines): 
                line = lines[pos] 
                pos+=1 
                if line.strip()=="": 
                    continue; 
                path_line = line.split()[0].strip('"')
                if not const_is_parsed: 
                    const_line1 = line.split()[-1].strip('()')
                    const_line0 = line.split()[-2].strip('()')
                    const_is_parsed = True

                if path_line=="}":  
                    break 
                else:   
                    if path_line[0]=='\\': 
                        path_lines.append(path_line[1:].replace('/', '.'))
            if pos==len(lines) and line.strip()!="}": 
                print("Unexpected EOF. Expected \"}\"")
                sys.exit(1) 
            path = Path('internal_'+str(len(paths)), path_lines)
            for const in const_line0.split('&'): 
                if not const[0]=='\\':  
                    continue 
                lhs, rhs = const.strip('\\ ').split('=')
                if not path.parse_const(lhs, rhs): 
                    const_is_parsed = False 
                    break

            for const in const_line1.split('&'): 
                if not const_is_parsed: 
                    break 
                if not const[0]=='\\':  
                    continue 
                lhs, rhs = const.strip('\\ ').split('=')
                path.consts1[lhs] = rhs

            if const_is_parsed: 
                path.final()
                paths.append(path)
        pos+=1
        
    return paths 

def parse_path(fname): 
    lines = [] 

    fi = open(fname, 'r') 
    for line in fi: 
        lines.append(line)
    fi.close() 

    return get_paths(lines) 

if __name__ == '__main__': 
    if len(sys.argv) != 2: 
        exit(-1) 
    
    fname = sys.argv[1] 
    parse_path(fname)[0].dump()
