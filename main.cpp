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
    // Lock myLock;


    queue<int> WaitingCars[2];
    

public:
    NarrowBridgeMonitor(int _travelTime, int _maxWaitTime) : currentPassingLane(-1), carsOnBridge(0), maxWaitTime(_maxWaitTime), leftLane(this), rightLane(this) {
        travelTime = _travelTime;
    }
    

    void pass(Car& car, Path& path) {
 
        // get the direction of the car
        int carDirection = path.from; // 0 for one way, 1 for the other

        // pushToWaitingCars(car.carID, carDirection, path);
        
        carDirection == 0 ? pushToLeftWaitingCars(car.carID, path) : pushToWaitingCars(car.carID, path);
        while(true){
            
    
            // lock.unlock();

            // lock.lock();
   
            // it it is not the first car in the bridge, and there is car from the opposite direction, wait for it to pass
            if(currentPassingLane != -1 && currentPassingLane != carDirection){
                waitForOppositeSwitch(carDirection);
            }

            // if the car is from the left can pass
            if(carDirection == 0){

                waitForFrontPassLeft(car.carID);

                // sleep if car has passed before
                if(carsOnBridge > 0){
                    sleep_milli(PASS_DELAY);
                }

                enterBridgeLeft(car.carID, path);
                notifyLeftLane();
            }
            else{

                waitForFrontPassRight(car.carID);

                // sleep if car has passed before
                if(carsOnBridge > 0){
                    sleep_milli(PASS_DELAY);
                }

                enterBridgeRight(car.carID, path);
                notifyRightLane();
            }

            sleep_milli(travelTime);

            if(carDirection == 0){
                exitBridgeLeft(car.carID, path);
                notifyRightLane();
            }
            else{
                exitBridgeRight(car.carID, path);
                notifyLeftLane();
            }


            break;

            

        }
   
           
    }

    void switchDirectionToCarDirection(int carDirection){
        __synchronized__;
        currentPassingLane = carDirection;
        printf("passing lane SWITCHED to %d\n", currentPassingLane);
    
    }

    void waitForOppositeSwitch(int carDirection){
        __synchronized__;
        while(currentPassingLane != carDirection && carsOnBridge > 0) {            
                carDirection == 0 ? leftLane.wait() : rightLane.wait();

        }
    }

    void waitForFrontPassLeft(int carID){
        __synchronized__;
        while(WaitingCars[0].front() != carID){
            leftLane.wait();
        }

    }

    void waitForFrontPassRight(int carID){
        __synchronized__;
        while(WaitingCars[1].front() != carID){
                rightLane.wait();
        }

    }

    void pushToLeftWaitingCars(int carID,Path& path){
        __synchronized__;
        if(currentPassingLane == -1){
            currentPassingLane = 0;
        }
        WaitingCars[0].push(carID);
        WriteOutput(carID, path.connectorType, path.connectorID, ARRIVE);

    }

    void pushToWaitingCars(int carID,Path& path){
        __synchronized__;
        if(currentPassingLane == -1){
            currentPassingLane = 1;
        }
        WaitingCars[1].push(carID);
        WriteOutput(carID, path.connectorType, path.connectorID, ARRIVE);
    }

   void enterBridgeLeft(int carID, Path& path){
        __synchronized__;

        printf("CarID: %d, is ENTERING LEFt the bridge, timestamp: %llu\n", carID, GetTimestamp());

        // remove the car from the waiting cars
        WaitingCars[0].pop();
       
        // now car on the bridge increment carsOnBridge
        carsOnBridge++;  

        WriteOutput(carID, path.connectorType, path.connectorID, START_PASSING);

    }

    void enterBridgeRight(int carID, Path& path){
        __synchronized__;

        // remove the car from the waiting cars
        WaitingCars[1].pop();
        // now car on the bridge increment carsOnBridge
        carsOnBridge++;   

        WriteOutput(carID, path.connectorType, path.connectorID, START_PASSING);

    }

    void notifyLeftLane(){
        __synchronized__;
        
        if(!WaitingCars[0].empty()){
            leftLane.notifyAll();
        }
    }

    void notifyRightLane(){
        __synchronized__;
        
        if(!WaitingCars[1].empty()){
            rightLane.notifyAll();
        }
    }

    
    void exitBridgeLeft(int carID, Path& path){
        __synchronized__;

        carsOnBridge--; 

        // if i am the last car in the bridge, notify the other lane
        if(carsOnBridge == 0 && !(WaitingCars[1].empty())){
       
            WriteOutput(carID, path.connectorType, path.connectorID, FINISH_PASSING);

            // notifyTheOtherLane();
            currentPassingLane = 1 ;  // switch the direction        
            rightLane.notifyAll();
            printf("CarID: %d, notified the other Lane %d\n", carID, currentPassingLane);

            return;
            
        }
        else if(carsOnBridge == 0 && WaitingCars[1].empty() && WaitingCars[0].empty()){
            printf("CarID: %d, is the last car in the bridge\n", carID);
            currentPassingLane = -1;
            WriteOutput(carID, path.connectorType, path.connectorID, FINISH_PASSING);
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
            currentPassingLane = 0 ;  // switch the direction
            printf("CarID: %d, notified the other Lane %d\n", carID, currentPassingLane);
            leftLane.notifyAll(); 

            return;
        
        }
        else if(carsOnBridge == 0 && WaitingCars[1].empty() && WaitingCars[0].empty()){
            printf("CarID: %d, is the last car in the bridge\n", carID);
            currentPassingLane = -1;
            WriteOutput(carID, path.connectorType, path.connectorID, FINISH_PASSING);
            return;
        }

        printf("CarID: %d, is NOT notified the opposite bridge, timestamp: %llu\n", carID, GetTimestamp());


        WriteOutput(carID, path.connectorType, path.connectorID, FINISH_PASSING);
        return;
        
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

