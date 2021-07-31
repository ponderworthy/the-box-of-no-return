# Benchmark gig::Engine Synthesis
#
# Hand crafted Makefile as a starting point for benchmarking the sampler's
# Gigasampler format engine. You have to manually adjust the CFLAGS macro
# below to achieve the best results on your system!
#
# Call 'make' to compile and then './gigsynth' to run the benchmark.

#CFLAGS=-O3 --param max-inline-insns-single=50 -ffast-math -march=pentium4 -mtune=pentium4 -funroll-loops -fomit-frame-pointer -mfpmath=sse
#CFLAGS=-xW -O3 -march=pentium4
#CFLAGS=-O3 -march=pentium4
#CFLAGS=-O3 -march=pentium4m -ffast-math -funroll-loops -fomit-frame-pointer -fpermissive -mfpmath=sse
CFLAGS=-O3 -march=i686 -mmmx -msse -ffast-math -funroll-loops -fomit-frame-pointer -fpermissive -mfpmath=sse
#CFLAGS=-O3 -g3 -ffast-math -march=pentium4 -funroll-loops -fomit-frame-pointer -mno-fp-ret-in-387 -fpermissive
#CFLAGS=-O3 -ffast-math -funroll-loops -fomit-frame-pointer
CPP=g++
OBJFILES=*.o

# In order to be able to compile the actual Sampler source files, we need to
# define compile time configuration macros.
INCLUDES=-include ../config.h

.PHONY: all gigsynth.o Synthesizer.o RTMath.o

all: Synthesizer.o RTMath.o gigsynth.o Filter.o
	$(CPP) $(CFLAGS) -o gigsynth gigsynth.o Synthesizer.o RTMath.o Filter.o

clean:
	rm -f gigsynth $(OBJFILES)

gigsynth.o:
	$(CPP) $(INCLUDES) $(CFLAGS) -c gigsynth.cpp

Synthesizer.o:
	$(CPP) $(INCLUDES) $(CFLAGS) -c ../src/engines/gig/Synthesizer.cpp

Filter.o:
	$(CPP) $(INCLUDES) $(CFLAGS) -c ../src/engines/gig/Filter.cpp

RTMath.o:
	$(CPP) $(INCLUDES) $(CFLAGS) -c ../src/common/RTMath.cpp
