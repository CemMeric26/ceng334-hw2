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
    Condition leftLane;
    Condition rightLane;
    Condition directionChange;
    int travelTime;
    int maxWaitTime;
    int currentPassingLane; // 0 for one way, 1 for the other
    int carsOnBridge;
    // int waitingCars[2]; // 0 for one way, 1 for the other
    vector<int> waitingCars;

public:
    NarrowBridgeMonitor(int _travelTime, int _maxWaitTime) : currentPassingLane(false), carsOnBridge(0), maxWaitTime(_maxWaitTime), leftLane(this), rightLane(this), directionChange(this) {
        travelTime = _travelTime;
    }

    void pass(Car& car, Path& path) {
        __synchronized__;

        // bool hasPassed = false;

        // path will provide the direction, connectorType, connectorID, info
        int carDirection = path.from; // 0 for one way, 1 for the other

        bool hasPassed = false;

       // Setup the timespec for the maximum wait time
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += maxWaitTime / 1000;
        ts.tv_nsec += (maxWaitTime % 1000) * 1000000;
        normalize_timespec(ts);

        // get the current time
        long lastCarTime = getCurrentTime();

        long startTime = getCurrentTime();

        printf("current passing lane %d and carId: %d and cardirection: %d\n", currentPassingLane, car.carID, carDirection);

        while (!hasPassed)
        {
            // if currarDently passing lane is the same as the direction of the car
            if(currentPassingLane ==  carDirection){
                printf("Car %d is trying to pass the bridge in direction: %d\n", car.carID, carDirection);
                // if there is car passing then wait
                if(carsOnBridge > 0){
                    // wait 
                    printf("Car %d is waiting, there is a car passing the same lane with sleeping as pass delay\n", car.carID);
                    
                    sleep_milli(PASS_DELAY);                 
    
                } 
                // Direction has no cars left that have arrived before it, no car is passing
                
                printf("Car %d is passing the bridge\n", car.carID);
                // start pass the bridge
                WriteOutput(car.carID, path.connectorType, path.connectorID, START_PASSING);
                carsOnBridge++;
                
                printf("Cars on bridge: %d now will sleep for travel time\n", car.carID);
                // passing time
                sleep_milli(travelTime);

                // finish passing the bridge
                WriteOutput(car.carID, path.connectorType, path.connectorID, FINISH_PASSING);
                carsOnBridge--;

                // update the last car time
                lastCarTime = getCurrentTime();

                hasPassed = true; // break the loop you are done passing             

            }
            else{

                // The maximum wait time has been reached
                // Condition& currentLane = currentPassingLane == 0 ? leftLane : rightLane;

                // maxWaitTimeReached(currentLane);
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += maxWaitTime / 1000;
                ts.tv_nsec += (maxWaitTime % 1000) * 1000000;
                normalize_timespec(ts);

                if (directionChange.timedwait(&ts) == ETIMEDOUT) {
                    printf("Max wait time reached\n");
                    switchDirection();
                }

                // there are no more cars left on the passing lane
                // noMoreCarLeftOnPassingLane();
                else if (carsOnBridge == 0) {
                    printf("No more cars left on the passing lane\n");
                    switchDirection();
                }

                // wait for the current lane to be free
                else {
                    printf("Car %d is waiting for the current lane to be free\n", car.carID);
                    waitingCars.push_back(car.carID);
                    directionChange.wait();
                    waitingCars.pop_back();
                }

            }
            
        }

        printf("Car %d has passed the bridge\n", car.carID);
        

              
    }

    void switchDirection() {
        printf("Switching direction from: %d to: %d\n", currentPassingLane, !currentPassingLane);
        // Condition& laneToNotify = currentPassingLane == 0 ? rightLane : leftLane;
        currentPassingLane = !currentPassingLane;

        printf("new current passing lane %d\n", currentPassingLane);
        
        directionChange.notifyAll();

        printf("Notified all for the lane dir: %d\n", currentPassingLane);
    }


    void normalize_timespec(struct timespec &ts) {
        while (ts.tv_nsec >= 1000000000) {
            ts.tv_nsec -= 1000000000;
            ts.tv_sec++;
        }
    }

    long getCurrentTime() {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
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


// global monitor variables
vector<NarrowBridgeMonitor> narrowBridgeMonitors;
vector<FerryMonitor> ferryMonitors;
vector<CrossRoadMonitor> crossRoadMonitors;


void initalizeMonitors() {
    for (int i = 0; i < narrowBridges.size(); i++) {
        narrowBridgeMonitors.push_back(NarrowBridgeMonitor(narrowBridges[i].travelTime, narrowBridges[i].maxWaitTime));
    }

    for (int i = 0; i < ferries.size(); i++) {
        ferryMonitors.push_back(FerryMonitor(ferries[i].travelTime, ferries[i].maxWaitTime, ferries[i].capacity));
    }

    for (int i = 0; i < crossRoads.size(); i++) {
        crossRoadMonitors.push_back(CrossRoadMonitor(crossRoads[i].travelTime, crossRoads[i].maxWaitTime));
    }
}


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
            narrowBridgeMonitors[path.connectorID].pass(*car, path);
            // printf("Narrow Bridge passing\n");

        } else if (path.connectorType == 'F') {
            // ferry
            ferryMonitors[path.connectorID].pass(carID);
            // printf("Ferry passing\n");
        } else if (path.connectorType == 'C') {
            // cross road
            crossRoadMonitors[path.connectorID].pass(carID);
            // printf("Cross Road passing\n");
        }


        
    }

    return NULL;
}



int main(){

    // will parse the input here
    parseInput();

    // print the parsed input
    printInput();

    // InitWriteOutput();

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

