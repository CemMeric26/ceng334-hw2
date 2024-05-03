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
    NarrowBridgeMonitor(int _travelTime, int _maxWaitTime) : currentPassingLane(-1), carsOnBridge(0), maxWaitTime(_maxWaitTime), leftLane(this), rightLane(this) {
        travelTime = _travelTime;
    }
    

    void pass(Car& car, Path& path) {
        struct timespec ts;

        WriteOutput(car.carID, path.connectorType, path.connectorID, ARRIVE);
        // __synchronized__;


        // get the direction of the car
        int carDirection = path.from; // 0 for one way, 1 for the other

        // pushToWaitingCars(car.carID, carDirection);

        while(true){

            // this is very first car
            if( currentPassingLane == -1){

                printf("CarID: %d, is the first car in the bridge\n", car.carID);
                // if the bridge is empty, the car can pass

                // currentPassingLane = carDirection;
                switchDirectionToCarDirection(carDirection);

                carDirection == 0 ? enterBridgeLeft(car.carID, path) : enterBridgeRight(car.carID, path);
                sleep_milli(travelTime);
                carDirection == 0 ? exitBridgeLeft(car.carID, path) : exitBridgeRight(car.carID, path);
                
                break;


            }


            

            // I am the car from the opposite direction
            // if the opposite direction is full then wait
            while(currentPassingLane != carDirection && carsOnBridge > 0) {
                printf("CarID: %d, is waiting for the opposite direction to be empty\n", car.carID);
                carDirection == 0 ? leftLane.wait() : rightLane.wait();

            }

            // if the opposite direction is empty, switch the direction
            if(currentPassingLane != carDirection && carsOnBridge == 0){
                switchDirectionToCarDirection(carDirection);
            }
    

            // if the car is the first car in the queue, it can pass
            // check if the bridge is empty
 
            carDirection == 0 ? enterBridgeLeft(car.carID, path) : enterBridgeRight(car.carID, path);
            sleep_milli(travelTime);
            carDirection == 0 ? exitBridgeLeft(car.carID, path) : exitBridgeRight(car.carID, path);
            

            break;

            

        }
     
           
    }

    void switchDirectionToCarDirection(int carDirection){
        __synchronized__;
        currentPassingLane = carDirection;
        printf("passing lane SWITCHED to %d\n", currentPassingLane);
    
    }

    void pushToWaitingCars(int carID, int carDirection){
        __synchronized__;
        WaitingCars[carDirection].push(carID);
        printf("CarID: %d, pushed to the waiting cars, direction: %d\n", carID, carDirection);
    }

   void enterBridgeLeft(int carID, Path& path){
        __synchronized__;

        WaitingCars[0].push(carID);

        // if the car is not the front of the queue, wait for the direction change
        while(WaitingCars[0].front() != carID){
            leftLane.wait();
        }


        // sleep if car has passed before
        if(carsOnBridge > 0){
            sleep_milli(PASS_DELAY);
        }

        printf("CarID: %d, is ENTERING the bridge, timestamp: %llu\n", carID, GetTimestamp());

        // remove the car from the waiting cars
        WaitingCars[0].pop();

        
        // now car on the bridge increment carsOnBridge
        carsOnBridge++;  

        WriteOutput(carID, path.connectorType, path.connectorID, START_PASSING);

        

        if(!WaitingCars[0].empty()){
            leftLane.notifyAll();
        }

        // sleep for travel time
        // sleep_milli(travelTime);

    }

    void enterBridgeRight(int carID, Path& path){
        __synchronized__;

        WaitingCars[1].push(carID);

        // if the car is not the front of the queue, wait for the direction change
        while(WaitingCars[1].front() != carID){
            rightLane.wait();
        }

        

        // sleep if car has passed before
        if(carsOnBridge > 0){
            sleep_milli(PASS_DELAY);
        }


        printf("CarID: %d, is ENTERING the bridge, timestamp: %llu\n", carID, GetTimestamp());

        // remove the car from the waiting cars
        WaitingCars[1].pop();
        // now car on the bridge increment carsOnBridge
        carsOnBridge++;   

        WriteOutput(carID, path.connectorType, path.connectorID, START_PASSING);
        
            

        // sleep for travel time
        // sleep_milli(travelTime);

        if(! WaitingCars[1].empty()){
            rightLane.notifyAll();
        }

        // WriteOutput(carID, path.connectorType, path.connectorID, FINISH_PASSING);

    }


    void exitBridgeLeft(int carID, Path& path){
        __synchronized__;
        // remove the car from the bridge

        carsOnBridge--; // carsOnBridgeDecrement();

        // if i am the last car in the bridge, notify the other lane
        if(carsOnBridge == 0 && !(WaitingCars[1].empty())){
       
            WriteOutput(carID, path.connectorType, path.connectorID, FINISH_PASSING);

            // notifyTheOtherLane();
            currentPassingLane = 1 ;  // switch the direction
            printf("CarID: %d, notified the other Lane %d\n", carID, currentPassingLane);
            rightLane.notifyAll();

            return;
            
        }

        printf("CarID: %d, is NOT notified the opposite bridge, timestamp: %llu\n", carID, GetTimestamp());


        WriteOutput(carID, path.connectorType, path.connectorID, FINISH_PASSING);
        return;
       
        
    }

    void exitBridgeRight(int carID,Path& path){
        __synchronized__;
        // remove the car from the bridge

        carsOnBridge--; 

        // if i am the last car in the bridge, notify the other lane
        if(carsOnBridge == 0 && !(WaitingCars[0].empty())){

            
            WriteOutput(carID, path.connectorType, path.connectorID, FINISH_PASSING);
            
            // notifyTheOtherLane();
            currentPassingLane = 0 ;  // switch the direction
            printf("CarID: %d, notified the other Lane %d\n", carID, currentPassingLane);
            leftLane.notifyAll();

            

            return;


            
        }

        printf("CarID: %d, is NOT notified the opposite bridge, timestamp: %llu\n", carID, GetTimestamp());


        WriteOutput(carID, path.connectorType, path.connectorID, FINISH_PASSING);
        return;
        
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

