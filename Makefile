CC_FLAGS=-Wall -std=gnu++11
LD_FLAGS=

mercury: obj/main.o obj/mercury.o obj/mercury_exception.o
	g++ $(LD_FLAGS) -o $@ $^

obj/%.o: %.cpp
	g++ $(CC_FLAGS) -c -o $@ $<

all: mercury

.PHONY: clean

clean:
	rm -f obj/*.o mercury

