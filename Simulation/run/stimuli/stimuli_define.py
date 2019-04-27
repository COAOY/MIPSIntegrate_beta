from abc import ABC, abstractmethod

from .cache import * 

class Stimuli(ABC): 
    state_space = [] 
    reg_cache = Cache(32, 0.8, 0.8, 5)
    def __init__(self, name=''): 
        super().__init__() 
        Stimuli.state_space.append(self)
	
        self.name = name

    @abstractmethod
    def __call__(self): 
        pass
     
class RTypeInst(Stimuli): 
    def __init__(self, ops, name=''): 
        super().__init__(name) 
        self.ops = ops 

    def __call__(self): 
        inst = bitarray(32) 
        inst.setall(False) 

        inst[ANTARES_INSTR_OPCODE] = OP_TYPE_R
        inst[ANTARES_INSTR_RS] = Stimuli.reg_cache.read()
        inst[ANTARES_INSTR_RT] = nbit_randrange(5)
        inst[ANTARES_INSTR_RD] = nbit_randrange(5, start=1)
        inst[ANTARES_INSTR_FUNCT] = random.choice(self.ops)
        Stimuli.reg_cache.write(inst[ANTARES_INSTR_RD])

        inst.reverse()

        return inst.tobytes().hex()

class R2TypeInst(Stimuli): 
    def __init__(self, ops, name=''): 
        super().__init__(name) 
        self.ops = ops 

    def __call__(self): 
        inst = bitarray(32) 
        inst.setall(False) 

        inst[ANTARES_INSTR_OPCODE] = OP_TYPE_R2
        inst[ANTARES_INSTR_RS] = Stimuli.reg_cache.read()
        inst[ANTARES_INSTR_RT] = nbit_randrange(5)
        inst[ANTARES_INSTR_RD] = nbit_randrange(5, start=1)
        inst[ANTARES_INSTR_FUNCT] = random.choice(self.ops)
        Stimuli.reg_cache.write(inst[ANTARES_INSTR_RD])

        inst.reverse()

        return inst.tobytes().hex()

class ITypeInst(Stimuli): 
    def __init__(self, ops, name=''): 
        super().__init__(name) 
        self.ops = ops

    def __call__(self): 
        inst = bitarray(32) 
        inst.setall(False) 

        inst[ANTARES_INSTR_OPCODE] = random.choice(self.ops)
        inst[ANTARES_INSTR_RS] = Stimuli.reg_cache.read()
        inst[ANTARES_INSTR_RT] = nbit_randrange(5, start=1)
        inst[ANTARES_INSTR_IMM16] = nbit_randrange(16)
        Stimuli.reg_cache.write(inst[ANTARES_INSTR_RT])

        inst.reverse()

        return inst.tobytes().hex()

class JTypeInst(Stimuli): 
     def __init__(self, ops, name=''): 
        super().__init__(name) 
        self.ops = ops 

     def __call__(self): 
        inst = bitarray(32) 
        inst.setall(False) 

        inst[ANTARES_INSTR_OPCODE] = random.choice(self.ops) 
        inst[ANTARES_INSTR_IMM26] = nbit_randrange(26) 

        inst.reverse() 

        return inst.tobytes().hex() 


# j = JTypeInst(OP_TYPE_J, 'jump') 
# j = JTypeInst([OP_J], 'j') 
# jal = JTypeInst([OP_JAL], 'jal') 

# i = ITypeInst(OP_TYPE_I, 'i') 
addi = ITypeInst([OP_ADDI], 'addi')
addiu = ITypeInst([OP_ADDIU], 'addiu')
andi = ITypeInst([OP_ANDI], 'andi') 
ori = ITypeInst([OP_ORI], 'ori') 
xori = ITypeInst([OP_XORI], 'xori') 
slti = ITypeInst([OP_SLTI], 'slti') 
sltiu = ITypeInst([OP_SLTIU], 'sltiu') 

# b = ITypeInst(OP_BRANCH, 'b') 
beq = ITypeInst([OP_BEQ], 'beq') 
bgtz = ITypeInst([OP_BGTZ], 'bgtz') 
blez = ITypeInst([OP_BLEZ], 'blez') 
bne = ITypeInst([OP_BNE], 'bne') 

# arith = RTypeInst(FUNCTION_OP_ARITH, 'arith')    
add = RTypeInst([FUNCTION_OP_ADD], 'add')    
addu = RTypeInst([FUNCTION_OP_ADDU], 'addu')    
sub = RTypeInst([FUNCTION_OP_SUB], 'sub')    
subu = RTypeInst([FUNCTION_OP_SUBU], 'subu')    
div = RTypeInst([FUNCTION_OP_DIV], 'div')    
divu = RTypeInst([FUNCTION_OP_DIVU], 'divu')    
mult = RTypeInst([FUNCTION_OP_MULT], 'mult')    
multu = RTypeInst([FUNCTION_OP_MULTU], 'multu')    

# logic = RTypeInst(FUNCTION_OP_LOGIC, 'logic') 
And = RTypeInst([FUNCTION_OP_AND], 'and') 
nor = RTypeInst([FUNCTION_OP_NOR], 'nor') 
Or = RTypeInst([FUNCTION_OP_OR], 'or') 
xor = RTypeInst([FUNCTION_OP_XOR], 'xor') 

# comp = RTypeInst(FUNCTION_OP_COMP, 'comp') 
slt = RTypeInst([FUNCTION_OP_SLT], 'slt') 
sltu = RTypeInst([FUNCTION_OP_SLTU], 'sltu') 

# shift = RTypeInst(FUNCTION_OP_SHIFT, 'shift') 
sll = RTypeInst([FUNCTION_OP_SLL], 'sll') 
sllv = RTypeInst([FUNCTION_OP_SLLV], 'sllv') 
sra = RTypeInst([FUNCTION_OP_SRA], 'sra') 
srav = RTypeInst([FUNCTION_OP_SRAV], 'srav') 
srl = RTypeInst([FUNCTION_OP_SRL], 'srl') 
srlv = RTypeInst([FUNCTION_OP_SRLV], 'srlv') 

# mov = RTypeInst(FUNCTION_OP_MOV, 'mov')
mfhi = RTypeInst([FUNCTION_OP_MFHI], 'mfhi') 
mflo = RTypeInst([FUNCTION_OP_MFLO], 'mflo') 
movn = RTypeInst([FUNCTION_OP_MOVN], 'movn') 
movz = RTypeInst([FUNCTION_OP_MOVZ], 'movz') 
mthi = RTypeInst([FUNCTION_OP_MTHI], 'mthi') 
mtlo = RTypeInst([FUNCTION_OP_MTLO], 'mtlo') 

clo = R2TypeInst([FUNCTION_OP_CLO], 'clo')
clz = R2TypeInst([FUNCTION_OP_CLZ], 'clz')
madd = R2TypeInst([FUNCTION_OP_MADD], 'madd')
maddu = R2TypeInst([FUNCTION_OP_MADDU], 'maddu')
msub = R2TypeInst([FUNCTION_OP_MSUB], 'msub')
msubu = R2TypeInst([FUNCTION_OP_MSUBU], 'msubu')

# jumpr = RTypeInst(FUNCTION_OP_J, 'jumpr') 
# jalr = RTypeInst([FUNCTION_OP_JALR], 'jalr') 
# jr = RTypeInst([FUNCTION_OP_JR], 'jr') 
