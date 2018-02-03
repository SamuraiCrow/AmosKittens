
# Version to build
VERSION = 1

#CC     = $(CROSS_COMPILE)gcc 
#CXX    = $(CROSS_COMPILE)c++ 
#AS     = $(CROSS_COMPILE)as 
#LD     = $(CROSS_COMPILE)ld 
#RANLIB = $(CROSS_COMPILE)ranlib 
#RM     = delete
# RM     = rm

warnings = -wall -Wuninitialized


main_objects = commands.cpp
main_SRCS = AmosKittens.cpp

objects = $(main_objects:.cpp=.o)
programs= $(main_SRCS:.cpp=.exe)

%.o:		%.cpp 
	g++ $(warnings) -c -O2 -D__USE_INLINE__ $(@:.o=.cpp) -o $@

%.exe:		%.cpp $(objects)
	g++ $(warnings) -O2 -D__USE_INLINE__ $(@:.exe=.cpp) $(objects) -o $@

all:	 $(programs) $(objects)

clean:
	delete $(programs) $(objects)

.PHONY: revision 
revision:
	bumprev $(VERSION) $(programs)