all: target
FORCE: ;
.PHONY: FORCE

target:	btnd    \
	ledd        \
	iod         \
	iou         \
	io2         \
	mp3_to_wav  \
	minimad 	\
	nfcd        \
	pn532-util

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

iod: iod.o hex.o cmd.o uart.o
	$(CC) $(LDFLAGS) $^ -o $@

iou: iou.o hex.o cmd.o
	$(CC) $(LDFLAGS) $^ -o $@

io2: io2.o hex.o cmd.o uart.o
	$(CC) $(LDFLAGS) $^ -o $@

mp3_to_wav: mp3_to_wav.o
	$(CC) $(LDFLAGS) $^ -o $@

minimad: minimad.o
	$(CC) $(LDFLAGS) $^ -o $@

nfcd: pn532-drv.o nfcd.o
	$(CC) $(LDFLAGS) $^ -o $@

pn532-util: pn532-drv.o pn532-util.o
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o $(target)

