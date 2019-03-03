# Target library
lib := libfs.a
objs := disk.o fs.o

AR := ar rcs

CC := gcc
CCFLAGS := -Wall -Werror

all: $(lib)

## TODO: Phase 1
libfs.a: $(objs)
	$(AR) $@ $^ $(objs)

%.o: %.c
	$(CC) $(CCFLAGS) -c -o $@ $<

clean:
	rm -rf $(lib) $(objs)
	
