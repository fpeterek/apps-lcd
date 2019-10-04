CFLAGS+=$(shell pkg-config --cflags opencv)
LDFLAGS+=$(shell pkg-config --libs opencv)

PROGRAM=main-lcd-opencv

all: $(PROGRAM)

$(PROGRAM): $(PROGRAM).cpp
	g++ -std=c++11 $(PROGRAM).cpp -o $@ $(CFLAGS) $(LDFLAGS)

clean: 
	rm -rf $(PROGRAM)

