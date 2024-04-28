#include <iostream>
#include <pthread.h>
#include "helper.h"
#include "WriteOutput.h"
#include <vector>
#include "monitor.h"

using namespace std;

struct NarrowBridge
{
    int travelTime;
    int maxWaitTime;
};

struct Ferry
{
    int travelTime;
    int maxWaitTime;
    int capacity;

};

struct CrossRoad
{
    int travelTime;
    int maxWaitTime;
};

struct Path
{
    char connectorType;
    int connectorID;
    int from;
    int to;
};

struct Car
{
    int carID;
    int travelTime;
    int pathLength;
    vector<Path> path;
};

// storing inputs in these variables
vector<NarrowBridge> narrowBridges;
vector<Ferry> ferries;
vector<CrossRoad> crossRoads;
vector<Car> cars;

// global monitor variables
vector<NarrowBridgeMonitor> narrowBridgeMonitors;
vector<FerryMonitor> ferryMonitors;
vector<CrossRoadMonitor> crossRoadMonitors;

// input parsing function
void parseInput() {
    int N_N, F_N, C_N, C_AN;

    // number of narrow bridges and its details
    cin >> N_N;
    narrowBridges.resize(N_N);
    for (int i = 0; i < N_N; i++) {
        cin >> narrowBridges[i].travelTime >> narrowBridges[i].maxWaitTime;
    }

    // number of ferries and its details
    cin >> F_N;
    ferries.resize(F_N);
    for (int i = 0; i < F_N; i++) {
        cin >> ferries[i].travelTime >> ferries[i].maxWaitTime >> ferries[i].capacity;
    }

    // number of crossroads and its details
    cin >> C_N;
    crossRoads.resize(C_N);
    for (int i = 0; i < C_N; i++) {
        cin >> crossRoads[i].travelTime >> crossRoads[i].maxWaitTime;
    }

    // number of cars and its details
    cin >> C_AN;
    cars.resize(C_AN);

    for(int i=0; i< C_AN; i++) {
        cars[i].carID = i;
        cin >> cars[i].travelTime >> cars[i].pathLength;
        cars[i].path.resize(cars[i].pathLength);

        for(int j=0; j<cars[i].pathLength; j++) {
            string connector;
            
            cin >> connector >> cars[i].path[j].from >> cars[i].path[j].to;

            cars[i].path[j].connectorType = connector[0];
            cars[i].path[j].connectorID = stoi(connector.substr(1)); 

        }
    }

    
}


void printInput(){

    cout << "Narrow Bridges: " << endl;
    for (int i = 0; i < narrowBridges.size(); i++) {
        cout << "Travel Time: " << narrowBridges[i].travelTime << " Max Wait Time: " << narrowBridges[i].maxWaitTime << endl;
    }

    cout << "Ferries: " << endl;
    for (int i = 0; i < ferries.size(); i++) {
        cout << "Travel Time: " << ferries[i].travelTime << " Max Wait Time: " << ferries[i].maxWaitTime << " Capacity: " << ferries[i].capacity << endl;
    }

    cout << "Cross Roads: " << endl;
    for (int i = 0; i < crossRoads.size(); i++) {
        cout << "Travel Time: " << crossRoads[i].travelTime << " Max Wait Time: " << crossRoads[i].maxWaitTime << endl;
    }

    cout << "Cars: " << endl;
    for (int i = 0; i < cars.size(); i++) {
        cout << "Travel Time: " << cars[i].travelTime << " Path Length: " << cars[i].pathLength << endl;
        for (int j = 0; j < cars[i].path.size(); j++) {
            cout << "Connector Type: " << cars[i].path[j].connectorType << " Connector ID: " << cars[i].path[j].connectorID << " From: " << cars[i].path[j].from << " To: " << cars[i].path[j].to << endl;
        }
    }

}

class NarrowBridgeMonitor: public Monitor {
    Condition directionChange;
    int travelTime;
    int maxWaitTime;
    int currentDirection; // 0 for one way, 1 for the other
    int carsInQueue;
    int waitingCars[2]; // 0 for one way, 1 for the other

public:
    NarrowBridgeMonitor(int _travelTime, int _maxWaitTime) : currentDirection(false), carsInQueue(0), maxWaitTime(_maxWaitTime), directionChange(this) {
        waitingCars[0] = 0; waitingCars[1] = 0;
    }

    void pass(Car& car, Path& path) {
        __synchronized__;
        // path will provide the direction, connectorType, connectorID, info
        int direction = path.from; // 0 for one way, 1 for the other
        

    }

};

class CrossRoadMonitor: public Monitor {
    int travelTime;
    int maxWaitTime;

public:
    CrossRoadMonitor(int travelTime, int maxWaitTime) {
        this->travelTime = travelTime;
        this->maxWaitTime = maxWaitTime;
        
    }

    void pass(int carId) {
        __synchronized__;
        ;
    }
    
};


class FerryMonitor: public Monitor {
    int travelTime;
    int maxWaitTime;
    int capacity;

public:
    FerryMonitor(int travelTime, int maxWaitTime, int capacity) {
        this->travelTime = travelTime;
        this->maxWaitTime = maxWaitTime;
        this->capacity = capacity;
        
    }

    void pass(int carId) {
        __synchronized__;
        ;
    }
    
};



// car thread function
void* carThread(void *arg) {
    
    Car *car = (Car *)arg;

    for (int i = 0; i < car->path.size(); i++){

        Path path = car->path[i];
        int carID = car->carID;
        // travel to the connector
        WriteOutput(carID, path.connectorType, path.connectorID, TRAVEL);

        // Sleep for TravelTime milliseconds
        sleep_milli(car->travelTime); 

        // arrive at the connector
        WriteOutput(carID, path.connectorType, path.connectorID, ARRIVE);

        // pass the connector
        if (path.connectorType == 'N') {
            // narrow bridge
            // direction of the path
            narrowBridgeMonitors[path.connectorID].pass(car, path);
            printf("Narrow Bridge passing\n");

        } else if (path.connectorType == 'F') {
            // ferry
            ferryMonitors[path.connectorID].pass(carID);
            printf("Ferry passing\n");
        } else if (path.connectorType == 'C') {
            // cross road
            crossRoadMonitors[path.connectorID].pass(carID);
            printf("Cross Road passing\n");
        }


        
    }

    return NULL;
}

void initalizeMonitors() {
    for (int i = 0; i < narrowBridges.size(); i++) {
        narrowBridgeMonitors[i] = NarrowBridgeMonitor(narrowBridges[i].travelTime, narrowBridges[i].maxWaitTime);
    }

    for (int i = 0; i < ferries.size(); i++) {
        ferryMonitors[i] = FerryMonitor(ferries[i].travelTime, ferries[i].maxWaitTime, ferries[i].capacity);
    }

    for (int i = 0; i < crossRoads.size(); i++) {
        crossRoadMonitors[i] = CrossRoadMonitor(crossRoads[i].travelTime, crossRoads[i].maxWaitTime);
    }
}

int main(){

    // will parse the input here
    parseInput();

    // print the parsed input
    printInput();

    InitWriteOutput();

    // initalize the monitors
    initalizeMonitors();

    pthread_t threads[cars.size()]; // car threads array

    for (int i = 0; i < cars.size(); i++) {
        // Create a thread for each car
        pthread_create(&threads[i], NULL, carThread, (void *)&cars[i]);
    }

    // Join threads to make sure all car threads finish before program exits
    for (int i = 0; i < cars.size(); i++) {
        pthread_join(threads[i], NULL);
    }

    
    return 0;
}