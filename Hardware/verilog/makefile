TARGET = cdfg.so 

CC = yosys-config
CFLAGS = --build

.PHONY: default run clean 

default: $(TARGET) 
run: $(TARGET)
	@yosys -m $(TARGET) -s cdfg.ys 

SOURCE = $(wildcard *.cc)

$(TARGET): $(SOURCE) 
	@$(CC) $(CFLAGS) $@ $<

clean: 
	rm *.log 
	rm *.so 
	rm *.d
