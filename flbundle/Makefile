# Mac OSX bundle-maker Makefile

INCFLTK = $(shell fltk-config --cxxflags)
IMGLIB  = $(shell fltk-config --use-images --ldstaticflags)
CXX     = $(shell fltk-config --cxx)
OPTS    = -Wall -O3 
STRIP   = strip

#.SILENT:

all: flbundle


flbundle: flbundle.cxx
	@echo Build $@
	$(CXX) -o $@ $< $(OPTS) $(INCFLTK)  $(IMGLIB)
	$(STRIP) $@

clean:
	rm -f flbundle *.o

tidy:
	rm -rf test.app

