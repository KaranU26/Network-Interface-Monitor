#include <iostream>
#include <signal.h>
#include <string.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

using namespace std;

char path[] = "/tmp/monitor";
int buffer = 256;
bool infinite = true;

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

int main(int argc, char *argv[]){

    int clients = 10;
    bool isParent = true;
    int numberOfClients = 0;
    char readInput[buffer];
    int clientCollection[5];
    int numberOfInterfaces, masterFd, recievedFd;
    string nameOfIntf;

    struct sockaddr_un address;
    char strBuffer[buffer];
    fd_set fdset, fdset2;
    vector<string> intfNameStorage;

    //Register Signal Handler
    struct sigaction signalaction;
    signalaction.sa_handler = signalHandler;
    sigemptyset(&signalaction.sa_mask);
    signalaction.sa_flags = 0;
    int actionInt = sigaction(SIGINT, &signalaction, NULL);


    if (actionInt < 0)
    {
        cout << "An error occured when registering the handlers" << endl;
        return -1;
    }

    //Ask user to input choices
    while (true)
    {   
       cout << "Number of interfaces to request: ";
       cin >> numberOfInterfaces;
       for (int i = 0; i < numberOfInterfaces; i++)
       {
           cout << "Input the name of Interface " << i + 1 << " - ";
           cin >> nameOfIntf;
            intfNameStorage.push_back(nameOfIntf);
       }
       break;
       
    }
    
    pid_t collectionOfPid[numberOfInterfaces];

    memset(&address, 0, sizeof(address));

    masterFd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (masterFd < 0)
    {
        cout << "Could not create client" << endl;
        return -1;
    }

    address.sun_family = AF_UNIX;

    //Fork and exec Interface Monitors
    for (int i = 0; i < numberOfInterfaces; i++)
    {
        collectionOfPid[i] = fork();
        if (collectionOfPid[i] == 0)
        {
            isParent = false;
            execlp("./interfaceMonitor", "./interfaceMonitor", intfNameStorage[i].c_str(), NULL);
        }
        
    }

    strcpy(address.sun_path, path);
    unlink(path);

    int binding = bind(masterFd, (struct sockaddr *) &address, sizeof(address));

    if (binding < 0)
    {
        cout << "Binding failed" << endl;
        return -1;
    }

    cout << "Listening....." << endl;
    listen(masterFd, 5);

    FD_ZERO(&fdset2);
    FD_ZERO(&fdset);
    FD_SET(masterFd, &fdset);
    recievedFd = masterFd;

    while (infinite)
    {
        fdset2 = fdset;
        select(recievedFd + 1, &fdset2, NULL, NULL, NULL);
        int fdIsSet = FD_ISSET(masterFd, &fdset2);

        if (fdIsSet)
        {
            clientCollection[numberOfClients] = accept(masterFd, NULL, NULL);

                cout << "NETWORK MONITOR: Connection Recieved from INTERFACE" << endl;
                FD_SET(clientCollection[numberOfClients], &fdset); 
                read(clientCollection[numberOfClients], readInput, buffer);

                cout << "NETWORK MONITOR: recieved " << readInput << " from the Interface Monitor" << endl;
                cout << "Starting Interface Monitor" << endl;
                sprintf(readInput, "Monitor");
                write(clientCollection[numberOfClients], readInput, buffer);

                read(clientCollection[numberOfClients], readInput, buffer);
                
                cout << "NETWORK MONITOR: recieved " << readInput << " from the Interface Monitor" << endl;
                
                if (recievedFd < clientCollection[numberOfClients])
                {
                    recievedFd = clientCollection[numberOfClients]; 
                }

        }else{
            sprintf(readInput, "");
        read(clientCollection[numberOfClients], readInput, buffer);

        //Checks for Link down then sends Link Up
        if (readInput == "Link Down")
        {
            sprintf(readInput, "Set Link Up");
            write(clientCollection[numberOfClients], readInput, buffer);
        }
        }
        

        numberOfClients = numberOfClients + 1;

        sleep(1);
    }
    cout << "NETWORK MONITOR: Shutting Down Interface Monitors" << endl;
    //Safely Shutting down the Interface Monitors
    for (int i = 0; i < numberOfClients; i++)
    {
       sprintf(readInput, "Shut Down");
       write(clientCollection[i], readInput, buffer);

       read(clientCollection[i], readInput, buffer);
        kill(collectionOfPid[i], SIGINT);
       if (readInput == "Done")
       {    
           close(clientCollection[i]);
       }
       

    }

    close(masterFd);
    unlink(path);
    return 0;
    
}
