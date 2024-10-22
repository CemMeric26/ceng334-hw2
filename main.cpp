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
    // Condition leftLane;
    // Condition rightLane;
    pthread_cond_t leftLane;
    pthread_cond_t rightLane;
    int travelTime;
    int maxWaitTime;
    int currentPassingLane; // 0 for one way, 1 for the other
    vector<int> carsOnBridge;
    // int carsOnBridge;
    // Lock specialLock;
    pthread_mutex_t  mut; 
    struct timespec ts;


    queue<int> WaitingCars[2];
    

public:
    NarrowBridgeMonitor(int _travelTime, int _maxWaitTime) : currentPassingLane(-1),  maxWaitTime(_maxWaitTime) { // , specialLock(this) , leftLane(this), rightLane(this)
        carsOnBridge = {0, 0};
        travelTime = _travelTime;
        pthread_mutex_init(&mut, NULL);
        pthread_cond_init(&leftLane, NULL);
        pthread_cond_init(&rightLane, NULL);
    }
    // pthread_mutex_lock(&owner->mut); 
    // pthread_mutex_unlock(&owner->mut);

    void pass(Car& car, Path& path) {
      
        // WHILE INTIT lock is auto set, so need to unlock it
        // specialLock.unlock();

        // LOCK SET
        // specialLock.lock();
        pthread_mutex_lock(&mut);

        if(currentPassingLane == -1){

            // first car time set
            realTime(ts); // time set
            //printf("Car %d is the first car timestamp: %llu\n", car.carID, GetTimestamp());
            currentPassingLane = path.from;
        } 

        WaitingCars[path.from].push(car.carID);
        WriteOutput(car.carID, path.connectorType, path.connectorID, ARRIVE);

         while(true){
             
            if(currentPassingLane == path.from){
                // if it is not the front car, wait
                // if There is car passing the Wait, can be inside currentPassingLane == path.from

                // if it is not the front car or there is car passing then wait
                while(carsOnBridge[!path.from] > 0  || WaitingCars[path.from].front() != car.carID){ // there is a problem here

                    // path.from == 0  ? leftLane.wait() : rightLane.wait(); // i am hoping that wait will auotamatically release the lock
                    path.from == 0 ? pthread_cond_wait(&leftLane, &mut) : pthread_cond_wait(&rightLane, &mut);
                }

                  // UNLOCK SET
                // specialLock.unlock(); 
                pthread_mutex_unlock(&mut);

                // if it is the front car, PASS DELAY
                if(carsOnBridge[path.from]>0){

                    // printf("Car %d is PASS_DELAYING timestamp: %llu\n", car.carID, GetTimestamp());
                    sleep_milli(PASS_DELAY);
                }


                //LOCK SET
                //specialLock.lock();
                pthread_mutex_lock(&mut);


                WaitingCars[path.from].pop();
                WriteOutput(car.carID, path.connectorType, path.connectorID, START_PASSING);
                

                carsOnBridge[path.from]++;

                // NOTIFY THE NEXT CAR

                if(path.from == 0){
                    //printf("Car %d is NOTIFIED the same direction timestamp: %llu\n", car.carID, GetTimestamp());
                    //leftLane.notifyAll();
                    pthread_cond_broadcast(&leftLane);
                }else{
                    //printf("Car %d is NOTIFIED the same direction timestamp: %llu\n", car.carID, GetTimestamp());
                    //rightLane.notifyAll();
                    pthread_cond_broadcast(&rightLane);
                }

                // specialLock.unlock();   // UNLOCK SET
                pthread_mutex_unlock(&mut);


                // SLEEP FOR TRAVEL TIME
                sleep_milli(travelTime);
                
                // LOCK SET
                //specialLock.lock();
                pthread_mutex_lock(&mut);

                // FINISH PASSING & DECREASE THE CARS ON BRIDGE
                WriteOutput(car.carID, path.connectorType, path.connectorID, FINISH_PASSING);
                carsOnBridge[path.from]--;


                // if i am the last car of our lane notify the opposite lane only if:
                // ** other direction is not empty
                // ** our lane is empty and no car is on the bridge
                if( !(WaitingCars[!path.from].empty()) && carsOnBridge[path.from]==0){  // WaitingCars[path.from].empty() && 
                    
                    // change the passing lane to opposite direction
                    
                    // path.from == 0 ? rightLane.notifyAll() : leftLane.notifyAll();
                    path.from == 0 ? pthread_cond_broadcast(&rightLane) : pthread_cond_broadcast(&leftLane);
                    
                }
                //specialLock.unlock();   // UNLOCK SET 
                pthread_mutex_unlock(&mut);    

                break;
                
            }// timeout condition check
            // there no car on the passing lane
            else if( carsOnBridge[!path.from]==0 && WaitingCars[!path.from].empty()){ // carsOnBridge[!path.from]==0 && 

                path.from == 0 ? currentPassingLane = 0 : currentPassingLane = 1;
                realTime(ts); //time set again
                
                // notify the other direction cars
                // path.from == 0 ? leftLane.notifyAll() : rightLane.notifyAll();
                path.from == 0 ? pthread_cond_broadcast(&leftLane) : pthread_cond_broadcast(&rightLane);
                //specialLock.unlock();
                pthread_mutex_unlock(&mut);
                continue;

            }// there are cars on the other lane
            else{       

                // time set
                // printf("Car %d is waiting for opposite direction timestamp: %llu\n", car.carID, GetTimestamp());
                // int timeout_return = path.from == 0 ? leftLane.timedwait(&ts) : rightLane.timedwait(&ts);
                int timeout_return = path.from == 0 ? pthread_cond_timedwait(&leftLane, &mut, &ts) : pthread_cond_timedwait(&rightLane, &mut, &ts);
                
                // timeout condition check
                if(timeout_return == ETIMEDOUT){
                    //printf("TIMEOUT HAPPENED: timestamp: %llu\n", GetTimestamp());

                    currentPassingLane = path.from;
                    realTime(ts); //time set again
 
                    //specialLock.unlock();
                    pthread_mutex_unlock(&mut);
                    continue;

                }
                //specialLock.unlock();
                pthread_mutex_unlock(&mut);
                continue;


            }         

        }


    }

    // ts.tv_sec += maxWaitTime / 1000;  // Add seconds part of maxWaitTime
        // ts.tv_nsec += (maxWaitTime % 1000) * 1000000;  // Add milliseconds part of maxWaitTime converted to nanoseconds

    void realTime(timespec& ts){
        clock_gettime(CLOCK_REALTIME, &ts);  // Get the current time
        
        ts.tv_sec += maxWaitTime/1000;
        ts.tv_nsec += (maxWaitTime%1000)*1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }

    }


   
           
};

class CrossRoadMonitor: public Monitor {
    int travelTime;
    int maxWaitTime;
    // Lock specialLock;
    pthread_mutex_t  mut; 
    int currentPassingLane; // four possible option

    // Condition zeroLane; Condition oneLane; Condition twoLane; Condition threeLane;
    pthread_cond_t zeroLane; pthread_cond_t oneLane; pthread_cond_t twoLane; pthread_cond_t threeLane;

    // vector<Condition*> crossLanes;
    vector<pthread_cond_t*> crossLanes;

    queue<int> WaitingCars[4];
    vector<int> carsOnBridge; // 4 carsOnBridge for each lane
    struct timespec ts;

public:
    CrossRoadMonitor(int travelTime, int maxWaitTime) { // specialLock(this)
        this->travelTime = travelTime;
        this->maxWaitTime = maxWaitTime;
        carsOnBridge = {0, 0, 0, 0};
        currentPassingLane = -1;
        pthread_mutex_init(&mut, NULL);
        
        // Condition* zeroLane = new Condition(this); Condition* oneLane = new Condition(this); Condition* twoLane = new Condition(this); Condition* threeLane = new Condition(this);


        //crossLanes.push_back(zeroLane); crossLanes.push_back(oneLane); crossLanes.push_back(twoLane); crossLanes.push_back(threeLane);
        pthread_cond_init(&zeroLane, NULL); pthread_cond_init(&oneLane, NULL); pthread_cond_init(&twoLane, NULL); pthread_cond_init(&threeLane, NULL);
        crossLanes.push_back(&zeroLane); crossLanes.push_back(&oneLane); crossLanes.push_back(&twoLane); crossLanes.push_back(&threeLane);
        
        
    }



    void pass(Car& car, Path& path) {
        // WHILE INTIT lock is auto set, so need to unlock it
        // specialLock.unlock();


        // LOCK SET
        // specialLock.lock();
        pthread_mutex_lock(&mut);

        if(currentPassingLane == -1){

            // first car time set
            realTime(ts); // time set
            currentPassingLane = path.from;
        } 

        WaitingCars[path.from].push(car.carID);
        WriteOutput(car.carID, path.connectorType, path.connectorID, ARRIVE);

        while(true)
        {
            if(currentPassingLane == path.from){

                // if it is not the front car or there is car passing then wait
                while(carsOnBridge[ (path.from+1) % 4] > 0 || carsOnBridge[ (path.from+2) % 4] || carsOnBridge[ (path.from+3) % 4] || WaitingCars[path.from].front() != car.carID){ 

                    // crossLanes[path.from]->wait(); 
                    pthread_cond_wait(crossLanes[path.from], &mut);
                }



                  // UNLOCK SET
                // specialLock.unlock(); 
                pthread_mutex_unlock(&mut);

                // if it is the front car, PASS DELAY
                if(carsOnBridge[path.from]>0){
                    // printf("Car %d is PASS_DELAYING timestamp: %llu\n", car.carID, GetTimestamp());
                    sleep_milli(PASS_DELAY);
                }


                //LOCK SET
                // specialLock.lock();
                pthread_mutex_lock(&mut);


                WaitingCars[path.from].pop();
                WriteOutput(car.carID, path.connectorType, path.connectorID, START_PASSING);
                

                carsOnBridge[path.from]++;

                // NOTIFY THE NEXT CAR
                // crossLanes[path.from]->notifyAll();
                pthread_cond_broadcast(crossLanes[path.from]);

                //specialLock.unlock();   // UNLOCK SET
                pthread_mutex_unlock(&mut);

                // SLEEP FOR TRAVEL TIME
                sleep_milli(travelTime);
                
                // LOCK SET
                // specialLock.lock();
                pthread_mutex_lock(&mut);
                // FINISH PASSING & DECREASE THE CARS ON BRIDGE
                WriteOutput(car.carID, path.connectorType, path.connectorID, FINISH_PASSING);
                carsOnBridge[path.from]--;


                // if i am the last car of our lane notify the opposite lane only if:
                // need to check the waiting  in order from path.from 
                if( !(WaitingCars[(path.from+1) % 4].empty()) && carsOnBridge[path.from]==0){  // WaitingCars[path.from].empty() && 
                    
                    // change the passing lane to that direction
                    // crossLanes[(path.from+1) % 4]->notifyAll();
                    pthread_cond_broadcast(crossLanes[(path.from+1) % 4]);
                    
                }
                else if( !(WaitingCars[(path.from+2) % 4].empty()) && carsOnBridge[path.from]==0){  // WaitingCars[path.from].empty() && 
                    
                    // change the passing lane to that direction
                    // crossLanes[(path.from+2) % 4]->notifyAll();
                    pthread_cond_broadcast(crossLanes[(path.from+2) % 4]);
                    
                    
                }
                else if( !(WaitingCars[(path.from+3) % 4].empty()) && carsOnBridge[path.from]==0){  // WaitingCars[path.from].empty() && 
                    
                    // change the passing lane to opposite direction
                    // crossLanes[(path.from+3) % 4]->notifyAll();
                    pthread_cond_broadcast(crossLanes[(path.from+3) % 4]);
                    
                }

                // specialLock.unlock();   // UNLOCK SET     
                pthread_mutex_unlock(&mut);
                break;
                
            }
            // there are no cars on the current passing lane, check the current possible direciton
            else if(carsOnBridge[currentPassingLane % 4] == 0 && WaitingCars[currentPassingLane % 4].empty() ){ // carsOnBridge[!path.from]==0 && 

            /*
            carsOnBridge[(path.from+1) % 4] == 0 && WaitingCars[(path.from+1) % 4].empty() && 
                     carsOnBridge[(path.from+2) % 4] == 0 && WaitingCars[(path.from+2) % 4].empty() && 
                     carsOnBridge[(path.from+3) % 4] == 0 && WaitingCars[(path.from+3) % 4].empty()
            */

                // There are no more cars left on the current Direction then
                // Switch the passing lane to the next non-empty Direction
                // currentPassingLane = path.from;
                if(!WaitingCars[(currentPassingLane+1) % 4].empty()){
                    currentPassingLane = (currentPassingLane+1) % 4;
                }
                else if(!WaitingCars[(currentPassingLane+2) % 4].empty()){
                    currentPassingLane = (currentPassingLane+2) % 4;
                }
                else if(!WaitingCars[(currentPassingLane+3) % 4].empty()){
                    currentPassingLane = (currentPassingLane+3) % 4;
                }
                else{
                    currentPassingLane = path.from;
                }
                realTime(ts); //time set again
                
                // notify the other direction cars
                // crossLanes[path.from]->notifyAll();
                // crossLanes[currentPassingLane]->notifyAll();
                pthread_cond_broadcast(crossLanes[currentPassingLane]);
                // specialLock.unlock();
                pthread_mutex_unlock(&mut);
                continue;

            }
            // timeout condition check
            else{
                // we need to check the waiting cars in order from path.from
                // time set
                // printf("Car %d is waiting for opposite direction timestamp: %llu\n", car.carID, GetTimestamp());

                // int timeout_return = crossLanes[path.from]->timedwait(&ts);
                int timeout_return = pthread_cond_timedwait(crossLanes[path.from], &mut, &ts);
                
                // timeout condition check
                if(timeout_return == ETIMEDOUT){
                    // printf("TIMEOUT HAPPENED: timestamp: %llu\n", GetTimestamp());
                    // Switch the passing lane to the next non-empty Direction;

                    // currentPassingLane = path.from;
                    if(!WaitingCars[(currentPassingLane+1) % 4].empty()){
                        currentPassingLane = (currentPassingLane+1) % 4;
                    }
                    else if(!WaitingCars[(currentPassingLane+2) % 4].empty()){
                        currentPassingLane = (currentPassingLane+2) % 4;
                    }
                    else if(!WaitingCars[(currentPassingLane+3) % 4].empty()){
                        currentPassingLane = (currentPassingLane+3) % 4;
                    }
                    else{
                        currentPassingLane = path.from;
                    }

                    realTime(ts); //time set again
 
                    // specialLock.unlock();
                    pthread_mutex_unlock(&mut);
                    continue;

                }
                // specialLock.unlock();
                pthread_mutex_unlock(&mut);
                continue;
            }


        }
    }

    void realTime(timespec& ts){
        clock_gettime(CLOCK_REALTIME, &ts);  // Get the current time
        
        ts.tv_sec += maxWaitTime/1000;
        ts.tv_nsec += (maxWaitTime%1000)*1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }

    }
        
  // ------------------------
};


class FerryMonitor: public Monitor {
    // left conditions
    Condition leftWay;
    Condition departFerry;

    Condition rightWay;
    Condition departFerryRight;

    int travelTime;
    int maxWaitTime;
    int capacity;
    vector<int> carsOnFerry;


    struct timespec ts;
    struct timespec ts_right;
    bool ferryPassing;
    bool ferryPassingRight;




public:
    FerryMonitor(int travelTime, int maxWaitTime, int capacity): leftWay(this), departFerry(this), rightWay(this), departFerryRight(this) {  
        this->travelTime = travelTime;
        this->maxWaitTime = maxWaitTime;
        this->capacity = capacity;
        carsOnFerry = {0, 0};
        ferryPassing = false;
        ferryPassingRight = false;
        
        
    }

    void pass(Car& car, Path& path){
        
        if(path.from == 0){
            //printf("Car %d is passing from the left\n", car.carID);
            loadCar_checkCapacity(car, path);
            sleep_milli(travelTime);
            finishPass(car, path);

        }
        else{
            //printf("Car %d is passing from the right\n", car.carID);
            loadCar_checkCapacityRight(car, path);
            sleep_milli(travelTime);
            finishPassRight(car, path);
        }
        

    }

    void finishPass(Car& car, Path& path){
        __synchronized__

        WriteOutput(car.carID, path.connectorType , path.connectorID, FINISH_PASSING);

    }
    void finishPassRight(Car& car, Path& path){
        __synchronized__

        WriteOutput(car.carID, path.connectorType , path.connectorID, FINISH_PASSING);

    }

    void loadCar_checkCapacity(Car& car, Path& path){
        __synchronized__

        if(carsOnFerry[0] == 0){
            realTime(ts);
        }


        if(ferryPassing){
            // printf("Car %d is the waits the next ferry\n", car.carID);
            departFerry.wait(); 
            // printf("Car %d is woken up\n", car.carID);
        }


        // load the car
        WriteOutput(car.carID, path.connectorType, path.connectorID , ARRIVE); //arrive once olabilir
        carsOnFerry[0]++;
        // printf("Car %d is loaded carCount is: %d\n", car.carID, carsOnFerry[0]);

        // no more room for the cars
        if(carsOnFerry[0] == capacity){

            // printf("Car %d is completed capacity check will notify everyone\n", car.carID);

            WriteOutput(car.carID, path.connectorType , path.connectorID, START_PASSING);
            carsOnFerry[0]--; // check here
            ferryPassing = true;

            // printf("Car %d filled the ferry, carsOnferr: %d, ferrypasing:%d \n", car.carID, carsOnFerry[0], ferryPassing);
            leftWay.notifyAll();
        
            return;

        }
        else { // waiting for the ferry to be full, or timeout

                // wait for both timeout and previous ferry to depart 
                // printf("Car %d is waiting for the ferry to be full\n", car.carID);
                int timeout_return = leftWay.timedwait(&ts);

                if(timeout_return == ETIMEDOUT){
                    // printf("TIMEOUT HAPPENED: timestamp: %llu\n", GetTimestamp());
                    // printf("Car %d is TIMEOUT\n", car.carID);

                    // time set
                    realTime(ts);
                    WriteOutput(car.carID, path.connectorType , path.connectorID, START_PASSING);
                    carsOnFerry[0]--;
                    ferryPassing = true;

                    if(carsOnFerry[0] == 0){
                        ferryPassing = false;  
                        departFerry.notifyAll();
                    }

                    return;
                }
                else{
                    WriteOutput(car.carID, path.connectorType , path.connectorID, START_PASSING);

                    carsOnFerry[0]--;
                    // printf("Car %d is unloaded carCount is: %d\n", car.carID, carsOnFerry[0]);

                    if(carsOnFerry[0] == 0){
                        // printf("Car %d is the last car\n", car.carID);
                        ferryPassing = false;
                        departFerry.notifyAll();
                    }
                    return;

                }
       
                
            }
    
    }

    void loadCar_checkCapacityRight(Car& car, Path& path){
        __synchronized__

        if(carsOnFerry[1] == 0){
            realTimeRight(ts_right);
        }


        if(ferryPassingRight){
            // printf("Car %d is the waits the next ferry\n", car.carID);
            departFerryRight.wait(); 
            // printf("Car %d is woken up\n", car.carID);
        }


        // load the car
        WriteOutput(car.carID, path.connectorType, path.connectorID , ARRIVE); //arrive once olabilir
        carsOnFerry[1]++;
        // printf("Car %d is loaded carCount is: %d\n", car.carID, carsOnFerry[0]);

        // no more room for the cars
        if(carsOnFerry[1] == capacity){

            // printf("Car %d is completed capacity check will notify everyone\n", car.carID);

            WriteOutput(car.carID, path.connectorType , path.connectorID, START_PASSING);
            carsOnFerry[1]--; // check here
            ferryPassingRight = true;

            // printf("Car %d filled the ferry, carsOnferr: %d, ferrypasing:%d \n", car.carID, carsOnFerry[0], ferryPassing);
            rightWay.notifyAll();
        
            return;

        }
        else { // waiting for the ferry to be full, or timeout

                // wait for both timeout and previous ferry to depart 
                // printf("Car %d is waiting for the ferry to be full\n", car.carID);
                int timeout_return = rightWay.timedwait(&ts_right);

                if(timeout_return == ETIMEDOUT){
                    // printf("TIMEOUT HAPPENED: timestamp: %llu\n", GetTimestamp());
                    // printf("Car %d is TIMEOUT\n", car.carID);

                    // time set
                    realTimeRight(ts_right);
                    WriteOutput(car.carID, path.connectorType , path.connectorID, START_PASSING);
                    carsOnFerry[1]--;
                    ferryPassingRight = true;

                    if(carsOnFerry[1] == 0){
                        ferryPassingRight = false;  
                        departFerryRight.notifyAll();
                    }

                    return;
                }
                else{
                    WriteOutput(car.carID, path.connectorType , path.connectorID, START_PASSING);

                    carsOnFerry[1]--;
                    // printf("Car %d is unloaded carCount is: %d\n", car.carID, carsOnFerry[0]);

                    if(carsOnFerry[1] == 0){
                        // printf("Car %d is the last car\n", car.carID);
                        ferryPassingRight = false;
                        departFerryRight.notifyAll();
                    }
                    return;

                }
       
                
            }
    
    }



    void realTime(timespec& ts){
        clock_gettime(CLOCK_REALTIME, &ts);  // Get the current time
        
        ts.tv_sec += maxWaitTime/1000;
        ts.tv_nsec += (maxWaitTime%1000)*1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }

    }

    void realTimeRight(timespec& ts){
        clock_gettime(CLOCK_REALTIME, &ts);  // Get the current time
        
        ts.tv_sec += maxWaitTime/1000;
        ts.tv_nsec += (maxWaitTime%1000)*1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }

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
        // printf("Car %d is traveled to connector %c%d timestamp: %llu\n", carID, path.connectorType, path.connectorID, GetTimestamp()); 

        // pass the connector
        if (path.connectorType == 'N') {
            // narrow bridge
            // direction of the path
            narrowBridgeMonitors[path.connectorID]->pass(*car, path);
            // printf("Narrow Bridge passing\n")

        } else if (path.connectorType == 'F') {
            // ferry
            // path.from == 0 ? ferryMonitors[path.connectorID]->pass1(*car, path) : ferryMonitors[path.connectorID]->pass2(*car, path);
            ferryMonitors[path.connectorID]->pass(*car, path);
            // printf("Ferry passing\n");
        } else if (path.connectorType == 'C') {
            // cross road
            crossRoadMonitors[path.connectorID]->pass(*car, path);
            // printf("Cross Road passing\n");
        }


        
    }

    return NULL;
}



int main(){

    // will parse the input here
    parseInput();

    // print the parsed input
    //printInput();


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

