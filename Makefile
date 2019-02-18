TARGETS=player master

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

player: player.cpp potato.h
	g++ -g -o $@ $<

master: ringmaster.cpp potato.h
	g++ -g -o $@ $<




