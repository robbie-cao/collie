all: target
FORCE: ;
.PHONY: FORCE

target: btnd ledd nfcd

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	@$(CXX) $(CFLAGS) -c $< -o $@

%.o: %.cxx
	@$(CXX) $(CFLAGS) -c $< -o $@

btnd: button.o
	$(CC) $(LDFLAGS) $^ -o $@

ledd: led.o
	$(CC) $(LDFLAGS) $^ -o $@

nfcd: pn532-drv.o nfcd.o
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o btnd

