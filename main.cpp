#include <iostream>
#include <pthread.h>
#include "helper.h"
#include "WriteOutput.h"
#include <vector>
#include "monitor.h"
#include <queue>

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
    int travelTime;
    int maxWaitTime;
    int currentPassingLane; // 0 for one way, 1 for the other
    int carsOnBridge;


    queue<int> WaitingCars[2];
    

public:
    NarrowBridgeMonitor(int _travelTime, int _maxWaitTime) : currentPassingLane(1), carsOnBridge(0), maxWaitTime(_maxWaitTime), leftLane(this), rightLane(this) {
        travelTime = _travelTime;
    }
    

    void pass(Car& car, Path& path) {
        struct timespec ts;

        WriteOutput(car.carID, path.connectorType, path.connectorID, ARRIVE);
        // __synchronized__;


        // get the direction of the car
        int carDirection = path.from; // 0 for one way, 1 for the other

        pushToWaitingCars(car.carID, carDirection);

        while(true){

            printf("CarID: %d, is entered loop again current lane: %d\n", car.carID, currentPassingLane);
            // this is very first car
            if( currentPassingLane != carDirection && !(WaitingCars[carDirection].size() > 1) && WaitingCars[!carDirection].empty() && carsOnBridge == 0){

                printf("CarID: %d, is the first car in the bridge\n", car.carID);
                // if the bridge is empty, the car can pass

                // currentPassingLane = carDirection;
                switchDirectionToCarDirection(carDirection);

                enterBridge(car.carID, carDirection, path);

                sleep_milli(travelTime);

                exitBridge(car.carID, carDirection, path);
                break;


            }
            // if the car is the first car in the queue, it can pass
            // check if the bridge is empty
            if(carDirection == currentPassingLane){

                printf("CarID: %d, is in the same direction of %d lane\n", car.carID, carDirection);

                // if car is not the first of the waiting cars in the queue
                if((WaitingCars[carDirection].front() != car.carID) && (WaitingCars[carDirection].size()>1 ) ){
                    // if the bridge is not empty, wait for the direction change
                    printf("CarID: %d, is not the front of the %d lane, time stamp: %llu \n", car.carID, carDirection ,GetTimestamp());
                    if(carDirection == 0){
                        
                        leftLane.wait();
                        printf("CarID: %d, woke up from the left lane time stamp: %llu\n", car.carID, GetTimestamp());
                    }
                    else{
                        
                        rightLane.wait();
                        printf("CarID: %d, woke up from the right lane time stamp: %llu\n", car.carID, GetTimestamp());
                    }
                }

                printf("CarID: %d, is the front of the %d lane timestamp: %llu\n", car.carID, carDirection, GetTimestamp());
                // if it is NOT the first car in the queue
                if(carsOnBridge > 0){
                    printf("CarID: %d, is PASS DELAYING, timestamp: %llu\n", car.carID, GetTimestamp());
                    sleep_milli(PASS_DELAY);                 
                }

                printf("CarID: %d, is will enter the BRIDGE, timestamp: %llu\n", car.carID, GetTimestamp());
                enterBridge(car.carID, carDirection, path);
                printf("CarID: %d, is ENTERED the BRIDGE, timestamp: %llu\n", car.carID, GetTimestamp());

                // notify the next car in the queue this is the for the switch direction and then starting to pass in order case
                carDirection == 0 ? leftLane.notifyAll() : rightLane.notifyAll();
                sleep_milli(travelTime);

                exitBridge(car.carID, carDirection, path);

                break;

            }
            // I am the car from the opposite direction
            else{
                if(WaitingCars[!carDirection].empty() && carsOnBridge == 0){
                    // if the bridge is empty, the car can pass                    
                    continue;
                }
                else{
                    // if the bridge is not empty, wait for the direction change
                    if(carDirection == 0){
                        printf("CarID: %d, waiting for the left lane time stamp: %llu\n", car.carID, GetTimestamp());
                        leftLane.wait();
                        printf("CarID: %d, woke up from the left lane, time stamp: %llu\n", car.carID, GetTimestamp());
                        continue;
                    }
                    else{
                        printf("CarID: %d, waiting for the right lane time stamp: %llu\n", car.carID, GetTimestamp());
                        rightLane.wait();
                        printf("CarID: %d, woke up from the right lane time stamp: %llu\n", car.carID , GetTimestamp() );
                        continue;
                    }
                }
                
            }

        }
     
           
    }

    void switchDirectionToCarDirection(int carDirection){
        __synchronized__;
        currentPassingLane = carDirection;
        printf("passing lane SWITCHED to %d\n", currentPassingLane);
    
    }

   void enterBridge(int carID, int carDirection, Path& path){
        __synchronized__;

        printf("CarID: %d, is ENTERING the bridge, timestamp: %llu\n", carID, GetTimestamp());
        // remove the car from the waiting cars
        WaitingCars[carDirection].pop();

        // now car on the bridge increment carsOnBridge
        carsOnBridge++; // carsOnBridgeIncrement();      
        WriteOutput(carID, path.connectorType, path.connectorID, START_PASSING);

    }

    void exitBridge(int carID, int carDirection, Path& path){
        __synchronized__;
        // remove the car from the bridge

        carsOnBridge--; // carsOnBridgeDecrement();
        
        WriteOutput(carID, path.connectorType, path.connectorID, FINISH_PASSING);

        // if i am the last car in the bridge, notify the other lane
        if(carsOnBridge == 0 && !(WaitingCars[!carDirection].empty())){

            // notifyTheOtherLane();
            currentPassingLane = !currentPassingLane ;  // switch the direction
            printf("CarID: %d, notified the other Lane %d\n", carID, currentPassingLane);
            currentPassingLane == 0 ? leftLane.notifyAll() : rightLane.notifyAll();
            
        }

        

        
    }

    // if the passing car is the last car in the current Lane notified the other lane
    void notifyTheOtherLane(){
        // __synchronized__;
        if(currentPassingLane == 0){
            rightLane.notifyAll();
        }
        else{
            leftLane.notifyAll();
        }
    }
    
    void pushToWaitingCars(int carID, int carDirection){
        __synchronized__;
        WaitingCars[carDirection].push(carID);
        printf("Car %d is PUSHED to  %d waitLane\n", carID, carDirection);
    }

    void removeCarFromWaitingCars(int carID, int carDirection){
        __synchronized__;
        WaitingCars[carDirection].pop();
        printf("Car %d is REMOVED from  %d waitLane\n", carID, carDirection);
    }


    void switchTimeoutDirection(struct timespec &ts) {
        __synchronized__;
        
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += maxWaitTime / 1000;
        ts.tv_nsec += (maxWaitTime % 1000) * 1000000;
        normalize_timespec(ts);

        currentPassingLane = !currentPassingLane;

        printf("passing lane SWITCHED %d\n", currentPassingLane);
        
        if (currentPassingLane == 0) {
            leftLane.notifyAll();
        } else {
            rightLane.notifyAll();
        }

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
vector<NarrowBridgeMonitor*> narrowBridgeMonitors;
vector<FerryMonitor*> ferryMonitors;
vector<CrossRoadMonitor*> crossRoadMonitors;


void initalizeMonitors() {
    for (int i = 0; i < narrowBridges.size(); i++) {
        NarrowBridgeMonitor *narrowBridgeMonitor = new NarrowBridgeMonitor(narrowBridges[i].travelTime, narrowBridges[i].maxWaitTime);
        narrowBridgeMonitors.push_back(narrowBridgeMonitor);
    }

    for (int i = 0; i < ferries.size(); i++) {
        FerryMonitor *ferryMonitor = new FerryMonitor(ferries[i].travelTime, ferries[i].maxWaitTime, ferries[i].capacity);
        ferryMonitors.push_back(ferryMonitor);

    }

    for (int i = 0; i < crossRoads.size(); i++) {
        CrossRoadMonitor *crossRoadMonitor = new CrossRoadMonitor(crossRoads[i].travelTime, crossRoads[i].maxWaitTime);
        crossRoadMonitors.push_back(crossRoadMonitor);
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

        // pass the connector
        if (path.connectorType == 'N') {
            // narrow bridge
            // direction of the path
            narrowBridgeMonitors[path.connectorID]->pass(*car, path);
            // printf("Narrow Bridge passing\n");

        } else if (path.connectorType == 'F') {
            // ferry
            ferryMonitors[path.connectorID]->pass(carID);
            // printf("Ferry passing\n");
        } else if (path.connectorType == 'C') {
            // cross road
            crossRoadMonitors[path.connectorID]->pass(carID);
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

    

    // initalize the monitors
    initalizeMonitors();

    InitWriteOutput();

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

