all: target
FORCE: ;
.PHONY: FORCE

target: btnd

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	@$(CXX) $(CFLAGS) -c $< -o $@

%.o: %.cxx
	@$(CXX) $(CFLAGS) -c $< -o $@

btnd: button.o
	$(CXX) $(LDFLAGS) button.o -o btnd

clean:
	rm -f *.o btnd

