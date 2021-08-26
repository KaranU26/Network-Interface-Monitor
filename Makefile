FILE1=interfaceMonitor.cpp
FILE2=networkMonitor.cpp


networkMonitor: $(FILE2)
	g++ $(FILE2) -o $@  

intfMonitor: $(FILE1)
	g++ $(FILE1) -o $@

all: networkMonitor interfaceMonitor


clean: 
	rm  networkMonitor interfaceMonitor

