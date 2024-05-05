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
    vector<int> carsOnBridge;
    // int carsOnBridge;
    Lock specialLock;


    queue<int> WaitingCars[2];
    

public:
    NarrowBridgeMonitor(int _travelTime, int _maxWaitTime) : currentPassingLane(-1),  maxWaitTime(_maxWaitTime), leftLane(this), rightLane(this), specialLock(this) {
        carsOnBridge = {0, 0};
        travelTime = _travelTime;
    }
    

    void pass(Car& car, Path& path) {

        struct timespec ts;
        realTime(ts); // time set
        /* clock_gettime(CLOCK_REALTIME, &ts);  // Get the current time
        ts.tv_sec += maxWaitTime / 1000;  // Add seconds part of maxWaitTime
        ts.tv_nsec += (maxWaitTime % 1000) * 1000000;  // Add milliseconds part of maxWaitTime converted to nanoseconds

         */
        // WHILE INTIT lock is auto set, so need to unlock it
        specialLock.unlock();

        int carDirection = path.from;

        // LOCK SET
        specialLock.lock();

        if(currentPassingLane == -1){
            currentPassingLane = carDirection;
        } 

        WaitingCars[carDirection].push(car.carID);
        WriteOutput(car.carID, path.connectorType, path.connectorID, ARRIVE);

         while(true){

            // if There is car passing the Wait
            if(carsOnBridge[!carDirection] > 0 && WaitingCars[!carDirection].empty()){        
                // specialLock.unlock();          
                carDirection == 0 ? leftLane.wait() : rightLane.wait();
            } 

            if(currentPassingLane == carDirection){
                // if it is not the front car, wait

                printf("Car %d is same direction timestamp: %llu\n", car.carID, GetTimestamp());

                // LOCK SET
                // specialLock.lock();
                // if there are cars on the bridge, wait
                /* if(carsOnBridge[!carDirection] > 0){        
                    // specialLock.unlock();          
                    carDirection == 0 ? leftLane.wait() : rightLane.wait();
                } */


                // if it is not the front car, wait
                while(WaitingCars[carDirection].front() != car.carID){ // there is a problem here
                    //specialLock.unlock();
                    printf("Car %d is waiting for FRONT timestamp: %llu\n", car.carID, GetTimestamp());
                    
                    // carDirection == 0 ? leftLane.notifyAll() : rightLane.notifyAll(); //not sure here
                    
                    // specialLock.unlock();
                    carDirection == 0  ? leftLane.wait() : rightLane.wait(); // i am hoping that wait will auotamatically release the lock
                }

                // UNLOCK SET
                specialLock.unlock();   

                // if it is the front car, PASS DELAY
                if(carsOnBridge[carDirection]>0){
                    printf("Car %d is PASS_DELAYING timestamp: %llu\n", car.carID, GetTimestamp());
                    sleep_milli(PASS_DELAY);
                }


                //LOCK SET
                specialLock.lock();


                WaitingCars[carDirection].pop();
                WriteOutput(car.carID, path.connectorType, path.connectorID, START_PASSING);
                

                carsOnBridge[carDirection]++;

                printf("Car %d is passing timestamp: %llu\n", car.carID, GetTimestamp());
                // NOTIFY THE NEXT CAR
                carDirection == 0 ? leftLane.notifyAll() : rightLane.notifyAll();

                printf("Car %d is NOTIFIED the same direction timestamp: %llu\n", car.carID, GetTimestamp());

                specialLock.unlock();   // UNLOCK SET


                // SLEEP FOR TRAVEL TIME
                printf("Car %d is sleeping travel timestamp: %llu\n", car.carID, GetTimestamp());
                sleep_milli(travelTime);
                
                // LOCK SET
                specialLock.lock();
                // FINISH PASSING & DECREASE THE CARS ON BRIDGE
                WriteOutput(car.carID, path.connectorType, path.connectorID, FINISH_PASSING);
                carsOnBridge[carDirection]--;


                // if i am the last car of our lane notify the opposite lane only if:
                // ** other direction is not empty
                // ** our lane is empty and no car is on the bridge
                if( !(WaitingCars[!carDirection].empty()) && WaitingCars[carDirection].empty() && carsOnBridge[carDirection]==0){  
                    
                    // change the passing lane to opposite direction
                    printf("Car %d the last car of its direction NOTIFIYING OTHER LANE: %llu\n", car.carID, GetTimestamp());
           
                    carDirection == 0 ? currentPassingLane = 1 : currentPassingLane = 0;
                    
                    carDirection == 0 ? rightLane.notifyAll() : leftLane.notifyAll();
                    
                }
                specialLock.unlock();   // UNLOCK SET     

                break;
                
            }
            // timeout condition check
            else if( currentPassingLane== 0 ? leftLane.timedwait(&ts) : rightLane.timedwait(&ts) == ETIMEDOUT){
                printf("Car %d TIMEOUT happaned to lane %d: %llu\n", car.carID, currentPassingLane ,GetTimestamp());
                specialLock.lock();
                
                currentPassingLane = !currentPassingLane;
                
                // notify the other direction cars
                currentPassingLane == 0 ? rightLane.notifyAll() : leftLane.notifyAll();
                
                realTime(ts); //time set again
                specialLock.unlock();
                continue;

            }
            // there no car on the passing lane
            else if( carsOnBridge[!carDirection]==0 && WaitingCars[!carDirection].empty()){ // carsOnBridge[!carDirection]==0 && 
                printf("Car %d opposite direction EMPTY: %llu\n", car.carID, GetTimestamp());
                specialLock.lock();
                carDirection == 0 ? currentPassingLane = 0 : currentPassingLane = 1;
                
                // notify the other direction cars
                carDirection == 0 ? rightLane.notifyAll() : leftLane.notifyAll();
                specialLock.unlock();
                continue;

            }
            // there are cars on the other lane
            else{
                printf("Car %d opposite direction NOT EMPTY: %llu\n", car.carID, GetTimestamp());
                // if the other lane is not empty, wait
                carDirection == 0 ? leftLane.wait() : rightLane.wait();
                continue;
                
            }

        }


    }

    void realTime(timespec& ts){
        clock_gettime(CLOCK_REALTIME, &ts);  // Get the current time
        ts.tv_sec += maxWaitTime / 1000;  // Add seconds part of maxWaitTime
        ts.tv_nsec += (maxWaitTime % 1000) * 1000000;  // Add milliseconds part of maxWaitTime converted to nanoseconds

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
    Condition leftWay;
    Condition rightWay;
    int travelTime;
    int maxWaitTime;
    int capacity;
    vector<int> carsOnFerry;
    Lock specialLockLeft;
    // Lock specialLockRight;



public:
    FerryMonitor(int travelTime, int maxWaitTime, int capacity): leftWay(this), rightWay(this), specialLockLeft(this){
        this->travelTime = travelTime;
        this->maxWaitTime = maxWaitTime;
        this->capacity = capacity;
        carsOnFerry = {0, 0};
        
        
    }

    void pass(Car& car, Path& path) {
        //__synchronized__
        // struct timespec ts;
        // clock_gettime(CLOCK_REALTIME, &ts);
        // ts.tv_sec += maxWaitTime;

        specialLockLeft.unlock();
        // load the car
        WriteOutput(car.carID, path.connectorType, path.from , ARRIVE);
        
        if(path.from == 0){
            // load the car to the ferry
            specialLockLeft.lock();
            carsOnFerry[0]++;
            specialLockLeft.unlock();


            // no more room for the cars
            specialLockLeft.lock();
            if(carsOnFerry[0] == capacity){

                printf("Car %d is completed capacity check will notify everyone\n", car.carID);
                WriteOutput(car.carID, path.connectorType , path.connectorID, START_PASSING);
                specialLockLeft.unlock();

                leftWay.notifyAll();
                sleep_milli(travelTime);
                WriteOutput(car.carID, path.connectorType , path.connectorID, FINISH_PASSING);
                // leftWayPass(car, path);
                return;

            } 
            else{
                printf("Car %d is waiting for leftWay\n", car.carID);
                leftWay.wait();
                WriteOutput(car.carID, path.connectorType , path.connectorID, START_PASSING);

                leftWay.notifyAll();
                sleep_milli(travelTime);
                WriteOutput(car.carID, path.connectorType , path.connectorID, FINISH_PASSING);

        }
   

    }
    }


    void leftWayPass(Car& car, Path& path) {
       // __synchronized__;
        WriteOutput(car.carID, path.connectorType , path.connectorID, START_PASSING);

        leftWay.notifyAll();
        sleep_milli(travelTime);
        WriteOutput(car.carID, path.connectorType , path.connectorID, FINISH_PASSING);
        return;
       
    }

    void rightWayPass(Car& car, Path& path) {

        WriteOutput(car.carID, path.connectorType , path.connectorID, START_PASSING);

        // rightWay.notifyAll();
        sleep_milli(travelTime);
        WriteOutput(car.carID, path.connectorType , path.connectorID, FINISH_PASSING);
        
    }

    void loadToLeftFerry(Car& car, Path& path) {
        __synchronized__;
        printf("Car %d increased the  left count\n", car.carID);
        carsOnFerry[0]++;
    }
    void loadToRightFerry(Car& car, Path& path) {
        __synchronized__;
        
        carsOnFerry[1]++;
    }

    bool leftCapacityCheck(Car& car , Path& path) {
        __synchronized__;

        if(carsOnFerry[0] == capacity){
            printf(" Left capacity is full\n" );
            carsOnFerry[0] = 0;
            return true;
        }

        return false;
    }


    bool rightCapacityCheck(Car& car , Path& path) {
        __synchronized__;
        if(carsOnFerry[1] == capacity){
            carsOnFerry[1] = 0;
            return true;
            
        }

        return false;
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
            ferryMonitors[path.connectorID]->pass(*car, path);
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

