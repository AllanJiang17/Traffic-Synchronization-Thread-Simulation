UTHREAD = ./uthreads
TARGETS = traffic traffic_sem

OBJS = $(UTHREAD)/uthread.o $(UTHREAD)/uthread_mutex_cond.o $(UTHREAD)/uthread_sem.o
JUNKF = $(OBJS)
JUNKD = *.dSYM *~

override CFLAGS  += -g -std=gnu11 -I$(UTHREAD)

UNAME = $(shell uname)
ifeq ($(UNAME), Linux)
override LDFLAGS += -pthread
endif

JC = javac
override JFLAGS += -g

all: $(TARGETS)  SearchFileContents.class
$(TARGETS): $(OBJS)

SearchFileContents.class: SearchFileContents.java
	$(JC) $(JFLAGS) $^

clean:
	-rm -f $(JUNKF) $(TARGETS)  SearchFileContents.class
tidy: clean
	rm -rf $(JUNKD)


