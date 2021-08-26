#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

//path to Temp file
char path[] = "/tmp/monitor";
bool infinite = false;
const int buffer = 256;
const int pathBuffer = 500;
string statisticsData;
string interfacePath = "/sys/class/net/";

//Handle SIGNINT
void signalHandler(int sig){

    switch (sig)
    {
    case SIGINT:
        cout << "SIGNAL: Recieved SIGINT, shutting down safely." << endl;
        infinite = false;
        break;
    
    default:
        cout << "SIGNAL: Unhandled signal" << endl;

    }
}

//Retrieve and return string from file
string intfFileHandler(char* InterfaceName, string interfacePathway){

	ifstream file;
	file.open(interfacePathway);
	string fileData;

	if (file.is_open())
	{
		file >> fileData;
	}

	file.close();
	return fileData;
	
}

//Retrieve and return int from file
int integerFileHandler(char* InterfaceName, string interfacePathway){

	ifstream file;
	file.open(interfacePathway);
	int fileData;

	if (file.is_open())
	{
		file >> fileData;
	}

	file.close();
	return fileData;
	
}


int main(int argc, char *argv[]){

	ifstream file;
	char interface[buffer];
	char networkStatPath[pathBuffer];
	struct ifreq ifr;
	int num = 0;
	string operstate;
	int carrier_up_count = 0;
	int carrier_down_count = 0;
	int tx_bytes = 0;
	int tx_packets = 0;
	int rx_packets = 0;
	int tx_dropped = 0;
	int tx_errors = 0;
	int rx_bytes = 0;
	int rx_dropped = 0;
	int rx_errors = 0;

	//copy requested interface into variable
	strncpy(interface, argv[1], buffer);

	struct sockaddr_un address;
	char readInput[buffer];
	int len;
	int fd, rc;

	cout << "Interface is running: " << getpid() << endl;
	memset(&address, 0, sizeof(address));

	fd = socket(AF_UNIX, SOCK_STREAM, 0); 

	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, path);
	connect(fd, (struct sockaddr *)&address, sizeof(address));

	sprintf(readInput, "Ready");
	
	//send "Ready" command to network monitor
	 write(fd, readInput, buffer);

	 //read "Monitor" command
	 read(fd, readInput, buffer); 

	cout << "INTERFACE MONITOR: Message Recieved: " << readInput << endl;

	if (strcmp(readInput, "Monitor") == 0)
	{
		infinite = true;
		sprintf(readInput, "Monitoring");
		//Send "Monitoring to network monitor"
	 	write(fd, readInput, buffer);
	}
	

	while (infinite)
	{
		//Shut down if read input recieves "Shut Down"
		if (strcmp(readInput, "Shut Down") == 0)
		{
			cout << "INTERFACE MONITOR: Shutting down" << endl;
			sprintf(readInput, "Done");
			infinite = false;
		}

		//Read Data from interface files
		operstate = intfFileHandler(interface, interfacePath + interface + "/" + "operstate");
		carrier_up_count = integerFileHandler(interface, interfacePath + interface + "/" + "carrier_up_count");
		carrier_down_count = integerFileHandler(interface, interfacePath + interface + "/" + "carrier_down_count");
		rx_bytes = integerFileHandler(interface, interfacePath + interface + "/statistics/" + "rx_bytes");
		rx_dropped = integerFileHandler(interface, interfacePath + interface + "/statistics/" + "rx_dropped");
		rx_errors = integerFileHandler(interface, interfacePath + interface + "/statistics/" + "rx_errors");
		rx_packets = integerFileHandler(interface, interfacePath + interface + "/statistics/" + "rx_packets");
		tx_bytes = integerFileHandler(interface, interfacePath + interface + "/statistics/" + "tx_bytes");
		tx_dropped = integerFileHandler(interface, interfacePath + interface + "/statistics/" + "tx_dropped");
		tx_errors = integerFileHandler(interface, interfacePath + interface + "/statistics/" + "tx_errors");
		tx_packets = integerFileHandler(interface, interfacePath + interface + "/statistics/" + "tx_packets");

	//Restart interface if it goes down, for some reason this is not working, code for bringing the interface back up seems correct based on links given in instructions.
	if (operstate == "down")
	{
		sprintf(readInput, "Link Down");
		write(fd, readInput, buffer);
		read(fd, readInput, buffer);
		cout << readInput << endl;

			memset(&ifr, 0, sizeof(ifr));

			strncpy(ifr.ifr_name, interface, IF_NAMESIZE);
	
			ifr.ifr_flags |= IFF_UP;
			ioctl(fd, SIOCSIFFLAGS, &ifr);

	}

		statisticsData = "Interface:" + string(interface) + " state:" + string(operstate) + " up_count:" + to_string(carrier_up_count) + " down_count:" + to_string(carrier_down_count) + "\n" +
		   				+"rx_bytes:" + to_string(rx_bytes) + " rx_dropped:" + to_string(rx_dropped) + " rx_errors:" + to_string(rx_errors) + " rx_packets:" + to_string(rx_packets) + "\n" + 
						"tx_bytes:" + to_string(tx_bytes) + " tx_dropped:" + to_string(tx_dropped) + " tx_errors:" + to_string(tx_errors) + " tx_packets:" + to_string(tx_packets) +" \n\n";

		//Print out the data 
		memset(readInput, 0, buffer);
		strcpy(readInput, statisticsData.c_str());
		cout << statisticsData;
		write(fd, readInput, buffer);

		sleep(1);
	}
	close(fd);
	return 0;
}
