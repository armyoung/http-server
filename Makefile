SOURCES=io_worker.cpp server.cpp session.cpp task_worker.cpp log.cpp status.cpp http_engine.cpp

CXX=g++ -std=c++11
CFLAGS += -I. 
LIBS += -lpthread
LDFLAGS += -pthread
LIBOBJECTS = $(SOURCES:.cpp=.o)

default: all

PROGS = server_start client

all: $(PROGS)

server_start: server_start.o $(LIBOBJECTS) 
	$(CXX) $(LIBOBJECTS) server_start.o -o $@ $(LIBS)

client: client.o log.o
	$(CXX) log.o client.o -o $@ $(LIBS)

clean:
	rm -rf *.o $(PROGS)
