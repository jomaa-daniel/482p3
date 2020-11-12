CC=g++ -g -Wall -fno-builtin

# List of source files for your pager
PAGER_SOURCES=pager.cpp Arena.cpp Virtual_Page.cpp utils.cpp

# Generate the names of the pager's object files
PAGER_OBJS=${PAGER_SOURCES:.cpp=.o}

all: pager test_evict_writeback_NS

# Compile the pager and tag this compilation
pager: ${PAGER_OBJS} libvm_pager_core.o
	./autotag.sh
	${CC} -o $@ $^

# Compile an application program
test_evict_writeback_NS: test_evict_writeback_NS.4.cpp libvm_app_core.o
	${CC} -o $@ $^ -ldl

# Generic rules for compiling a source file to an object file
%.o: %.cpp
	${CC} -c $<
%.o: %.cc
	${CC} -c $<

clean:
	rm -f ${PAGER_OBJS} pager 
