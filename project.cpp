#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <fstream>
#include <iomanip>
using namespace std;

class Vehicle {
public:
    int id;
    int arrivalTime;
    bool priority;
    Vehicle(int _id, int _arrivalTime, bool _priority = false)
        : id(_id), arrivalTime(_arrivalTime), priority(_priority) {}
};

class Node {
public:
    Vehicle* v;
    Node* next;
    Node(Vehicle* _v) : v(_v), next(nullptr) {}
};

class Queue {
private:
    Node* frontPtr;
    Node* rearPtr;
    int sz;
public:
    Queue() : frontPtr(nullptr), rearPtr(nullptr), sz(0) {}
    ~Queue() {
        while (!isEmpty()) {
            Vehicle* vv = dequeue();
            delete vv;
        }
    }
    bool isEmpty() const { return frontPtr == nullptr; }
    void enqueue(Vehicle* vehicle) {
        Node* temp = new Node(vehicle);
        if (vehicle->priority) {
            if (isEmpty()) {
                frontPtr = rearPtr = temp;
            } else {
                temp->next = frontPtr;
                frontPtr = temp;
            }
        } else {
            if (rearPtr == nullptr) {
                frontPtr = rearPtr = temp;
            } else {
                rearPtr->next = temp;
                rearPtr = temp;
            }
        }
        sz++;
    }
    Vehicle* dequeue() {
        if (isEmpty()) return nullptr;
        Node* tmp = frontPtr;
        Vehicle* ret = tmp->v;
        frontPtr = frontPtr->next;
        if (frontPtr == nullptr) rearPtr = nullptr;
        delete tmp;
        sz--;
        return ret;
    }
    Vehicle* peek() const {
        if (isEmpty()) return nullptr;
        return frontPtr->v;
    }
    int size() const { return sz; }
};

class TrafficLight {
public:
    int currentLane;
    int greenTime;
    int timer;
    TrafficLight(int gtime = 5) : currentLane(0), greenTime(gtime), timer(0) {}
    void nextSignal(int totalLanes) {
        currentLane = (currentLane + 1) % totalLanes;
        timer = 0;
    }
    void tick() { timer++; }
    bool greenExpired() const { return timer >= greenTime; }
};

class Simulation {
private:
    int lanes;
    int simTime;
    int timeStepMillis;
    vector<Queue> roads;
    TrafficLight light;
    int vehicleIDCounter;
    int totalArrived;
    int totalPassed;
    long long totalWaitTime;
    double arrivalProb;
    double priorityProb;
    ofstream logFile;
public:
    Simulation(int _lanes = 4, int _simTime = 60, int _greenTime = 5,
               double _arrivalProb = 0.5, double _priorityProb = 0.01,
               int _timeStepMillis = 0, const string &logFilename = "simulation_log.txt")
        : lanes(_lanes), simTime(_simTime), timeStepMillis(_timeStepMillis),
          roads(_lanes), light(_greenTime), vehicleIDCounter(1), totalArrived(0),
          totalPassed(0), totalWaitTime(0), arrivalProb(_arrivalProb), priorityProb(_priorityProb)
    {
        logFile.open(logFilename);
        if (!logFile.is_open()) {
            cerr << "Warning: could not open log file. Logging disabled." << endl;
        }
        srand((unsigned)time(nullptr));
    }
    ~Simulation() {
        if (logFile.is_open()) logFile.close();
    }
    void run() {
        printHeader();
        for (int t = 0; t < simTime; ++t) {
            handleArrivals(t);
            processGreenLane(t);
            displayState(t);
            light.tick();
            if (light.greenExpired()) light.nextSignal(lanes);
            if (timeStepMillis > 0) {
                this_thread::sleep_for(chrono::milliseconds(timeStepMillis));
            }
        }
        printSummary();
    }
private:
    void handleArrivals(int currentTime) {
        for (int i = 0; i < lanes; ++i) {
            double r = (double)rand() / RAND_MAX;
            if (r < arrivalProb) {
                double p = (double)rand() / RAND_MAX;
                bool isPriority = (p < priorityProb);
                Vehicle* v = new Vehicle(vehicleIDCounter++, currentTime, isPriority);
                roads[i].enqueue(v);
                totalArrived++;
                if (logFile.is_open()) {
                    logFile << "t=" << currentTime << ": Vehicle " << v->id
                            << " arrived at lane " << i
                            << (isPriority ? " [PRIORITY]" : "") << "\n";
                }
            }
        }
    }
    void processGreenLane(int currentTime) {
        int lane = light.currentLane;
        Vehicle* v = roads[lane].dequeue();
        if (v != nullptr) {
            int wait = currentTime - v->arrivalTime;
            totalPassed++;
            totalWaitTime += wait;
            if (logFile.is_open()) {
                logFile << "t=" << currentTime << ": Vehicle " << v->id
                        << " passed from lane " << lane
                        << " (wait=" << wait << ")\n";
            }
            delete v;
        }
    }
    void displayState(int currentTime) {
        cout << "----------------------------------------\n";
        cout << "Time: " << setw(3) << currentTime << "   | Green Lane: " << light.currentLane << "\n";
        for (int i = 0; i < lanes; ++i) {
            cout << "Lane " << i << " waiting: " << setw(3) << roads[i].size() << "\n";
        }
        cout << "Total arrived: " << totalArrived << "   Total passed: " << totalPassed << "\n";
        if (logFile.is_open()) {
            logFile << "-- t=" << currentTime << " snapshot --\n";
            for (int i = 0; i < lanes; ++i) {
                logFile << "Lane " << i << " size=" << roads[i].size() << "\n";
            }
            logFile << "TotalArrived=" << totalArrived << " TotalPassed=" << totalPassed << "\n";
        }
    }
 void printHeader() {
    cout << "Traffic Signal & Vehicle Flow Simulation\n";
    cout << "Lanes: " << lanes << "  Simulation time steps: " << simTime
         << "  Green time: " << light.greenTime << "\n";
    cout << "Arrival prob per lane per step: " << arrivalProb
         << "  Priority prob: " << priorityProb << "\n\n";

    if (logFile.is_open()) {
        cout << " greenTime=" << light.greenTime
             << " arrivalProb=" << arrivalProb
             << " priorityProb=" << priorityProb << "\n";

        logFile << "Simulation started. lanes=" << lanes
                << " simTime=" << simTime << "\n";
    }
}

    void printSummary() {
        cout << "========================================\n";
        cout << "Simulation finished.\n";
        cout << "Total vehicles arrived: " << totalArrived << "\n";
        cout << "Total vehicles passed:  " << totalPassed << "\n";
        cout << "Vehicles still waiting: ";
        int remaining = 0;
        for (int i = 0; i < lanes; ++i) remaining += roads[i].size();
        cout << remaining << "\n";
        double avgWait = (totalPassed == 0) ? 0.0 : (double)totalWaitTime / totalPassed;
        cout << fixed << setprecision(2);
        cout << "Average wait time (for passed vehicles): " << avgWait << " time steps\n";
        if (logFile.is_open()) {
            logFile << "--- Summary ---\n";
            logFile << "TotalArrived=" << totalArrived << "\n";
            logFile << "TotalPassed=" << totalPassed << "\n";
            logFile << "Remaining=" << remaining << "\n";
            logFile << "AverageWait=" << avgWait << "\n";
            logFile << "Simulation ended.\n";
        }
    }
};

int main() {
    int lanes, simTime, greenTime, timestepMs;
    double arrivalProb, priorityProb;
    cout << "--- Traffic Signal Simulation Configuration ---\n";
    cout << "Number of lanes (e.g., 2 or 4): "; cin >> lanes;
    cout << "Simulation time steps (e.g., 60): "; cin >> simTime;
    cout << "Green time per lane (time steps, e.g., 5): "; cin >> greenTime;
    cout << "Arrival probability per lane per step (0..1, e.g., 0.5): "; cin >> arrivalProb;
    cout << "Priority vehicle probability per arrival (0..1, e.g., 0.01): "; cin >> priorityProb;
    cout << "Real-time delay per step in ms (0 for no delay): "; cin >> timestepMs;
    cout << "\nStarting simulation...\n";
    Simulation sim(lanes, simTime, greenTime, arrivalProb, priorityProb, timestepMs);
    sim.run();
    cout << "\nLog file: simulation_log.txt (if created)\n";
    cout << "Compile with: g++ -std=c++17 traffic_signal_simulation.cpp -o traffic_sim && ./traffic_sim\n";
    return 0;
}
