all: target
FORCE: ;
.PHONY: FORCE

target: btnd ledd

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	@$(CXX) $(CFLAGS) -c $< -o $@

%.o: %.cxx
	@$(CXX) $(CFLAGS) -c $< -o $@

btnd: button.o
	$(CXX) $(LDFLAGS) button.o -o btnd

ledd: led.o
	$(CXX) $(LDFLAGS) led.o -o ledd

clean:
	rm -f *.o btnd

