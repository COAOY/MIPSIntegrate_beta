
from bitarray import bitarray
from .stimuli_mapping import*

class TestClass:
	def test_OPcode(self):
		assert OP_ADDI == bitarray("001000")