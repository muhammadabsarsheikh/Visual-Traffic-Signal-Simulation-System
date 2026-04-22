#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <memory>
#include <iostream>
#include <string>
#include <optional>
#include <cmath>
#include <algorithm>

// ==========================================
// Domain Classes
// ==========================================

class Vehicle {
public:
    int id;
    int arrivalTime;
    bool priority;
    int laneIndex;      
    float dist;         
    bool isLeaving;     
    
    Vehicle(int _id, int _arrivalTime, int _laneIndex, bool _priority = false)
        : id(_id), arrivalTime(_arrivalTime), laneIndex(_laneIndex), priority(_priority), 
          dist(-150.0f), isLeaving(false) {} 
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
    ~Queue() { while (!isEmpty()) dequeue(); }
    bool isEmpty() const { return frontPtr == nullptr; }
    
    void enqueue(Vehicle* vehicle) {
        Node* temp = new Node(vehicle);
        if (vehicle->priority) {
            if (isEmpty()) frontPtr = rearPtr = temp;
            else { temp->next = frontPtr; frontPtr = temp; }
        } else {
            if (rearPtr == nullptr) frontPtr = rearPtr = temp;
            else { rearPtr->next = temp; rearPtr = temp; }
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
    
    Vehicle* peek() const { if (isEmpty()) return nullptr; return frontPtr->v; }
    int size() const { return sz; }
    
    std::vector<Vehicle*> getAllVehicles() const {
        std::vector<Vehicle*> result;
        Node* current = frontPtr;
        while (current != nullptr) {
            result.push_back(current->v);
            current = current->next;
        }
        return result;
    }
};

class TrafficLight {
public:
    int currentLane;
    int greenTime;
    int timer;
    TrafficLight(int gtime = 5) : currentLane(0), greenTime(gtime), timer(0) {}
    void nextSignal(int totalLanes) { currentLane = (currentLane + 1) % totalLanes; timer = 0; }
    void tick() { timer++; }
    bool greenExpired() const { return timer >= greenTime; }
    bool isYellow() const { return timer >= (greenTime - 3); }
};

// ==========================================
// INTERSECTION CLASS
// ==========================================
class Intersection {
public:
    int id;
    float cx, cy; 
    int lanes;    
    
    TrafficLight light;
    std::vector<Queue> roads;
    std::vector<Vehicle*> leavingVehicles;
    
    int totalArrived;
    int totalPassed;
    long long totalWaitTime;

    Intersection(int _id, float _cx, float _cy, int _lanes, int _greenTime)
        : id(_id), cx(_cx), cy(_cy), lanes(_lanes), light(_greenTime), 
          totalArrived(0), totalPassed(0), totalWaitTime(0)
    {
        roads.resize(lanes);
    }

    ~Intersection() {
        for(auto v : leavingVehicles) delete v;
    }

    void update(int currentTime, float visualSpeed, const float CAR_LENGTH, const float CAR_GAP) {
        float standardStopDist = 250.0f; 

        for (int i = 0; i < lanes; ++i) {
            auto vehicles = roads[i].getAllVehicles();
            for (size_t j = 0; j < vehicles.size(); ++j) {
                Vehicle* v = vehicles[j];
                float idealDist = standardStopDist - CAR_LENGTH/2 - (j * (CAR_LENGTH + CAR_GAP));
                
                if (v->dist < idealDist) {
                    v->dist += visualSpeed;
                    if (v->dist > idealDist) v->dist = idealDist;
                } else if (v->dist > idealDist) {
                    v->dist -= visualSpeed * 2.0f;
                    if (v->dist < idealDist) v->dist = idealDist;
                }
            }
        }

        for (size_t i = 0; i < leavingVehicles.size(); ) {
            Vehicle* v = leavingVehicles[i];
            v->dist += visualSpeed * 3.0f;
            if (v->dist > 800.0f) { 
                delete v;
                leavingVehicles.erase(leavingVehicles.begin() + i);
            } else {
                ++i;
            }
        }
    }

    void tickLogic(int currentTime, int& globalID, double arrivalProb, double priorityProb) {
        for (int i = 0; i < lanes; ++i) {
            double r = (double)rand() / RAND_MAX;
            if (r < arrivalProb) {
                double p = (double)rand() / RAND_MAX;
                bool isPriority = (p < priorityProb);
                Vehicle* v = new Vehicle(globalID++, currentTime, i, isPriority);
                v->dist = -100.0f; 
                roads[i].enqueue(v);
                totalArrived++;
            }
        }

        int lane = light.currentLane;
        if (!light.greenExpired() && !light.isYellow()) {
             Vehicle* v = roads[lane].dequeue();
             if (v != nullptr) {
                int wait = currentTime - v->arrivalTime;
                totalPassed++;
                totalWaitTime += wait;
                v->isLeaving = true;
                leavingVehicles.push_back(v);
             }
        }

        light.tick();
        if (light.greenExpired()) light.nextSignal(lanes);
    }
    
    int getWaitingCount() {
        int count = 0;
        for(auto& r : roads) count += r.size();
        return count;
    }
};

// ==========================================
// GUI & Application Logic
// ==========================================

struct InputBox {
    sf::RectangleShape rect;
    sf::Text labelText;
    sf::Text inputText;
    std::string value;
    bool isSelected = false;

    InputBox(float x, float y, const std::string& label, const std::string& defaultVal, const sf::Font& font) 
        : labelText(font), inputText(font), value(defaultVal)
    {
        labelText.setString(label);
        labelText.setCharacterSize(18);
        labelText.setFillColor(sf::Color::White);
        labelText.setPosition({x, y});

        rect.setSize({200.f, 30.f});
        rect.setPosition({x + 250.f, y});
        rect.setFillColor(sf::Color(70, 70, 70));
        rect.setOutlineThickness(2);
        rect.setOutlineColor(sf::Color(150, 150, 150));

        inputText.setString(value);
        inputText.setCharacterSize(18);
        inputText.setFillColor(sf::Color::White);
        inputText.setPosition({x + 255.f, y + 4.f});
    }

    void updateVisuals() {
        if (isSelected) {
            rect.setOutlineColor(sf::Color::Cyan);
            rect.setFillColor(sf::Color(90, 90, 90));
        } else {
            rect.setOutlineColor(sf::Color(150, 150, 150));
            rect.setFillColor(sf::Color(70, 70, 70));
        }
        inputText.setString(value);
    }

    void handleInput(uint32_t unicode) {
        if (!isSelected) return;
        if (unicode == 8) { if (!value.empty()) value.pop_back(); }
        else if ((unicode >= '0' && unicode <= '9') || unicode == '.') {
            if (unicode == '.' && value.find('.') != std::string::npos) return;
            if (value.length() < 10) value += static_cast<char>(unicode);
        }
    }
};

class TrafficApp {
    enum State { SPLASH, INPUT, SIMULATION };

private:
    sf::RenderWindow window;
    sf::Font font;
    sf::Texture carTexture;  
    sf::Texture carTexture2; 
    bool hasTexture1;        
    bool hasTexture2;
    
    State currentState;
    sf::Clock splashClock;
    float splashDuration = 2.0f;

    std::vector<InputBox> inputs;
    int selectedInputIndex = 0;
    
    // Config
    int numIntersections; 
    int lanesPerIntersection;
    int simTime;
    int greenTime;
    double arrivalProb;
    double priorityProb;
    float stepDelay;

    // Runtime
    int currentTime;
    std::vector<std::unique_ptr<Intersection>> intersections; 
    int vehicleIDCounter;
    
    // Visual Constants
    float visualSpeed;
    const float CAR_LENGTH = 80.0f; 
    const float CAR_WIDTH = 48.0f; 
    const float CAR_GAP = 20.0f;
    const float ROAD_WIDTH = 100.0f; 
    const float LANE_WIDTH = 50.0f;
    
    bool isPaused;
    bool isRunning;
    sf::Clock stepClock;

public:
    TrafficApp() 
        : currentState(SPLASH), currentTime(0), vehicleIDCounter(1),
          visualSpeed(5.0f),
          isPaused(false), isRunning(true), hasTexture1(false), hasTexture2(false)
    {
        window.create(sf::VideoMode({1200, 800}), "Traffic Signal Simulation System");
        window.setFramerateLimit(60); 
        loadAssets();
        setupInputs();
        splashClock.restart(); 
    }

    void run() {
        while (window.isOpen()) {
            handleEvents();
            switch (currentState) {
                case SPLASH: updateSplash(); renderSplash(); break;
                case INPUT: renderInput(); break;
                case SIMULATION: updateSimulation(); renderSimulation(); break;
            }
        }
    }

private:
    void loadAssets() {
        std::vector<std::string> fontPaths = { "fonts/calibri-regular.ttf", "calibri-regular.ttf", "C:\\Windows\\Fonts\\arial.ttf" };
        bool fontLoaded = false;
        for (const auto& path : fontPaths) {
            if (font.openFromFile(path)) { fontLoaded = true; break; }
        }
        if (!fontLoaded) std::cerr << "Warning: Font not found." << std::endl;

        if (carTexture.loadFromFile("assets/car.png")) { hasTexture1 = true; carTexture.setSmooth(true); }
        if (carTexture2.loadFromFile("assets/car2.png")) { hasTexture2 = true; carTexture2.setSmooth(true); }
    }

    void setupInputs() {
        inputs.reserve(7);
        float startX = 350.f, startY = 150.f, gap = 60.f;
        
        inputs.emplace_back(startX, startY + 0*gap, "Num Intersections (1-4)", "1", font);
        inputs.emplace_back(startX, startY + 1*gap, "Lanes per Inter.. (1-4)", "4", font);
        inputs.emplace_back(startX, startY + 2*gap, "Sim Time Steps (int)", "60", font);
        inputs.emplace_back(startX, startY + 3*gap, "Green Time (steps)", "10", font);
        inputs.emplace_back(startX, startY + 4*gap, "Arrival Prob (0.0-1.0)", "0.5", font);
        inputs.emplace_back(startX, startY + 5*gap, "Priority Prob (0.0-1.0)", "0.05", font);
        
        // CHANGED: Default value from "0.5" to "2.0"
        inputs.emplace_back(startX, startY + 6*gap, "Step Delay (seconds)", "0.5", font); 
        
        inputs[0].isSelected = true;
        selectedInputIndex = 0;
    }

    void handleEvents() {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            
            if (const auto* keyPress = event->getIf<sf::Event::KeyPressed>()) {
                
                // === STATE: INPUT ===
                if (currentState == INPUT) {
                    if (keyPress->code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }
                    else if (keyPress->code == sf::Keyboard::Key::Enter) {
                        startSimulation();
                    }
                    else if (keyPress->code == sf::Keyboard::Key::Tab || keyPress->code == sf::Keyboard::Key::Down) {
                        inputs[selectedInputIndex].isSelected = false;
                        selectedInputIndex = (selectedInputIndex + 1) % inputs.size();
                        inputs[selectedInputIndex].isSelected = true;
                    }
                    else if (keyPress->code == sf::Keyboard::Key::Up) {
                         inputs[selectedInputIndex].isSelected = false;
                         selectedInputIndex = (selectedInputIndex - 1 + inputs.size()) % inputs.size();
                         inputs[selectedInputIndex].isSelected = true;
                    }
                }
                // === STATE: SIMULATION ===
                else if (currentState == SIMULATION) {
                    // ESC: Go Back to Input
                    if (keyPress->code == sf::Keyboard::Key::Escape) {
                        currentState = INPUT;
                        isPaused = false; 
                    }
                    else if (keyPress->code == sf::Keyboard::Key::Space) {
                        isPaused = !isPaused;
                    }
                    else if (keyPress->code == sf::Keyboard::Key::R) {
                        resetSimulation();
                    }
                }
                // === STATE: SPLASH ===
                else if (currentState == SPLASH) {
                    if (keyPress->code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }
                }
            }

            if (currentState == INPUT) {
                if (const auto* textEvent = event->getIf<sf::Event::TextEntered>()) {
                    inputs[selectedInputIndex].handleInput(textEvent->unicode);
                }
            }
        }
    }

    void updateSplash() { if (splashClock.getElapsedTime().asSeconds() > splashDuration) currentState = INPUT; }

    void startSimulation() {
        try {
            numIntersections = std::stoi(inputs[0].value);
            lanesPerIntersection = std::stoi(inputs[1].value);
            simTime = std::stoi(inputs[2].value);
            greenTime = std::stoi(inputs[3].value);
            arrivalProb = std::stod(inputs[4].value);
            priorityProb = std::stod(inputs[5].value);
            stepDelay = std::stof(inputs[6].value);
            
            if (numIntersections < 1) numIntersections = 1; if (numIntersections > 4) numIntersections = 4;
            if (lanesPerIntersection < 1) lanesPerIntersection = 1; if (lanesPerIntersection > 4) lanesPerIntersection = 4;
            
            visualSpeed = (CAR_LENGTH + CAR_GAP) / (stepDelay * 60.0f);
            if(visualSpeed < 0.2f) visualSpeed = 0.2f;
            if(visualSpeed > 20.0f) visualSpeed = 20.0f; 

            resetSimulation();
            currentState = SIMULATION;
        } catch (...) { std::cerr << "Invalid input" << std::endl; }
    }

    void resetSimulation() {
        currentTime = 0; 
        vehicleIDCounter = 1; 
        intersections.clear();
        
        struct Pos { float x, y; };
        std::vector<Pos> positions;
        
        if (numIntersections == 1) { positions.push_back({600.f, 400.f}); } 
        else if (numIntersections == 2) { positions.push_back({350.f, 400.f}); positions.push_back({850.f, 400.f}); } 
        else if (numIntersections == 3) { positions.push_back({350.f, 250.f}); positions.push_back({850.f, 250.f}); positions.push_back({600.f, 600.f}); } 
        else { positions.push_back({350.f, 250.f}); positions.push_back({850.f, 250.f}); positions.push_back({350.f, 550.f}); positions.push_back({850.f, 550.f}); }

        for (int i = 0; i < numIntersections; ++i) {
            intersections.push_back(std::make_unique<Intersection>(i+1, positions[i].x, positions[i].y, lanesPerIntersection, greenTime));
        }

        isPaused = false; isRunning = true; stepClock.restart();
    }

    void updateSimulation() {
        if (!isRunning || isPaused) return;

        for(auto& ints : intersections) {
            ints->update(currentTime, visualSpeed, CAR_LENGTH, CAR_GAP);
        }

        if (stepClock.getElapsedTime().asSeconds() >= stepDelay) {
            if (currentTime >= simTime) { isRunning = false; return; }

            for(auto& ints : intersections) {
                ints->tickLogic(currentTime, vehicleIDCounter, arrivalProb, priorityProb);
            }

            currentTime++;
            stepClock.restart();
        }
    }

    // --- RENDERING ---
    struct Transform { sf::Vector2f pos; float rot; };
    
    Transform getVehicleTransform(float cx, float cy, int lane, float dist) {
        float laneOffset = LANE_WIDTH / 2.0f; 
        float spawnRadius = 350.0f; 
        
        switch(lane) {
            case 0: return { {cx - spawnRadius + dist, cy - laneOffset}, 0.0f };
            case 1: return { {cx + spawnRadius - dist, cy + laneOffset}, 180.0f };
            case 2: return { {cx + laneOffset, cy - spawnRadius + dist}, 90.0f };
            case 3: return { {cx - laneOffset, cy + spawnRadius - dist}, 270.0f };
        }
        return { {0,0}, 0 };
    }

    void renderSplash() {
        window.clear(sf::Color(20, 20, 30));
        sf::Text title(font, "Traffic Intersection Sim", 60);
        title.setFillColor(sf::Color::Cyan);
        sf::FloatRect tr = title.getLocalBounds();
        title.setOrigin({tr.size.x/2, tr.size.y/2});
        title.setPosition({window.getSize().x/2.0f, window.getSize().y/2.0f});
        window.draw(title);
        window.display();
    }

    void renderInput() {
        window.clear(sf::Color(30, 30, 30));
        sf::Text h(font, "Configuration", 36);
        h.setPosition({350.f, 100.f});
        window.draw(h);
        for (auto& b : inputs) { b.updateVisuals(); window.draw(b.labelText); window.draw(b.rect); window.draw(b.inputText); }
        sf::Text i(font, "TAB to switch, ENTER to start", 20);
        i.setFillColor(sf::Color::Yellow); i.setPosition({350.f, 600.f});
        window.draw(i);
        window.display();
    }

    void renderSimulation() {
        window.clear(sf::Color(30, 30, 30)); 

        for(auto& ints : intersections) {
            drawIntersection(*ints);
        }

        drawStatsOverlay(); 
        
        window.display();
    }
    
    void drawIntersection(Intersection& ints) {
        float cx = ints.cx;
        float cy = ints.cy;
        float roadLen = 350.0f; // from center
        
        sf::Color roadColor(50, 50, 50); 
        sf::Color markColor(200, 200, 200); 
        
        if (ints.lanes >= 1) { 
            sf::RectangleShape hRoad({roadLen*2, ROAD_WIDTH});
            hRoad.setOrigin({roadLen, ROAD_WIDTH/2});
            hRoad.setPosition({cx, cy});
            hRoad.setFillColor(roadColor);
            window.draw(hRoad);
            for(float i=-roadLen; i<roadLen; i+=60) {
                sf::RectangleShape dash({30.f, 2.f});
                dash.setOrigin({15.f, 1.f});
                dash.setPosition({cx+i, cy});
                dash.setFillColor(markColor);
                window.draw(dash);
            }
        }
        
        if (ints.lanes >= 3) { 
            sf::RectangleShape vRoad({ROAD_WIDTH, roadLen*2});
            vRoad.setOrigin({ROAD_WIDTH/2, roadLen});
            vRoad.setPosition({cx, cy});
            vRoad.setFillColor(roadColor);
            window.draw(vRoad);
            for(float i=-roadLen; i<roadLen; i+=60) {
                sf::RectangleShape dash({2.f, 30.f});
                dash.setOrigin({1.f, 15.f});
                dash.setPosition({cx, cy+i});
                dash.setFillColor(markColor);
                window.draw(dash);
            }
            sf::RectangleShape box({ROAD_WIDTH, ROAD_WIDTH});
            box.setOrigin({ROAD_WIDTH/2, ROAD_WIDTH/2});
            box.setPosition({cx, cy});
            box.setFillColor(roadColor);
            window.draw(box);
        }

        float stopOffset = ROAD_WIDTH/2 + 15.0f;
        auto drawSignalBox = [&](sf::Vector2f pos, int laneIdx) {
            bool isCurrent = (ints.light.currentLane == laneIdx);
            bool isYellow = ints.light.isYellow();
            sf::CircleShape c(12.f);
            c.setOrigin({12.f, 12.f});
            c.setPosition(pos);
            c.setOutlineThickness(2);
            c.setOutlineColor(sf::Color::White);
            if (isCurrent) c.setFillColor(isYellow ? sf::Color::Yellow : sf::Color::Green);
            else c.setFillColor(sf::Color::Red);
            window.draw(c);
            
            sf::Text t(font, std::to_string(laneIdx + 1), 16);
            t.setOrigin({t.getLocalBounds().size.x/2, t.getLocalBounds().size.y/2});
            t.setPosition({pos.x, pos.y - 5}); 
            t.setFillColor(sf::Color::Black);
            t.setStyle(sf::Text::Bold);
            window.draw(t);
        };

        if (ints.lanes >= 1) { 
            sf::RectangleShape line({4.f, LANE_WIDTH}); line.setOrigin({2.f, LANE_WIDTH/2});
            line.setPosition({cx - stopOffset, cy - LANE_WIDTH/2}); line.setFillColor(sf::Color::White); window.draw(line);
            drawSignalBox({cx - stopOffset, cy - ROAD_WIDTH/2 - 20}, 0);
        }
        if (ints.lanes >= 2) { 
            sf::RectangleShape line({4.f, LANE_WIDTH}); line.setOrigin({2.f, LANE_WIDTH/2});
            line.setPosition({cx + stopOffset, cy + LANE_WIDTH/2}); line.setFillColor(sf::Color::White); window.draw(line);
            drawSignalBox({cx + stopOffset, cy + ROAD_WIDTH/2 + 20}, 1);
        }
        if (ints.lanes >= 3) { 
            sf::RectangleShape line({LANE_WIDTH, 4.f}); line.setOrigin({LANE_WIDTH/2, 2.f});
            line.setPosition({cx + LANE_WIDTH/2, cy - stopOffset}); line.setFillColor(sf::Color::White); window.draw(line);
            drawSignalBox({cx + ROAD_WIDTH/2 + 20, cy - stopOffset}, 2);
        }
        if (ints.lanes >= 4) { 
            sf::RectangleShape line({LANE_WIDTH, 4.f}); line.setOrigin({LANE_WIDTH/2, 2.f});
            line.setPosition({cx - LANE_WIDTH/2, cy + stopOffset}); line.setFillColor(sf::Color::White); window.draw(line);
            drawSignalBox({cx - ROAD_WIDTH/2 - 20, cy + stopOffset}, 3);
        }

        auto drawList = [&](const std::vector<Vehicle*>& list) {
            for (auto* v : list) {
                Transform t = getVehicleTransform(cx, cy, v->laneIndex, v->dist);
                if (std::abs(t.pos.x - cx) > 400 || std::abs(t.pos.y - cy) > 300) continue;
                drawCar(v, t.pos, t.rot);
            }
        };

        for (int i = 0; i < ints.lanes; ++i) drawList(ints.roads[i].getAllVehicles());
        drawList(ints.leavingVehicles);
        
        sf::Text idT(font, "Intersection -" + std::to_string(ints.id), 20);
        idT.setPosition({cx - 30, cy - 10});
        idT.setFillColor(sf::Color(255, 255, 255, 100)); 
        window.draw(idT);
    }
    
    void drawCar(Vehicle* v, sf::Vector2f pos, float rot) {
        sf::Texture* tex = (v->priority && hasTexture2) ? &carTexture2 : (hasTexture1 ? &carTexture : nullptr);
        
        float usedWidth = CAR_WIDTH; 
        if (v->laneIndex == 0 || v->laneIndex == 1) usedWidth = 55.0f; 
        else usedWidth = 48.0f; 

        if (tex) {
            sf::Sprite s(*tex);
            sf::Vector2u size = tex->getSize();
            s.setOrigin({size.x / 2.0f, size.y / 2.0f});
            s.setPosition(pos);
            s.setRotation(sf::degrees(rot)); 
            s.setScale({CAR_LENGTH / size.x, usedWidth / size.y});
            s.setColor(sf::Color::White);
            window.draw(s);
        } else {
            sf::RectangleShape body({CAR_LENGTH, usedWidth});
            body.setOrigin({CAR_LENGTH/2, usedWidth/2});
            body.setPosition(pos);
            body.setRotation(sf::degrees(rot));
            body.setFillColor(v->priority ? sf::Color(255, 215, 0) : sf::Color(100, 149, 237)); 
            body.setOutlineThickness(1);
            body.setOutlineColor(sf::Color::Black);
            window.draw(body);
        }
    }

    void drawStatsOverlay() {
        int totalA = 0, totalP = 0;
        long long totalWait = 0;
        int totalWaitingCars = 0;
        
        for(auto& i : intersections) { 
            totalA += i->totalArrived; 
            totalP += i->totalPassed; 
            totalWait += i->totalWaitTime;
            totalWaitingCars += i->getWaitingCount();
        }
        
        double avgWait = (totalP == 0) ? 0.0 : (double)totalWait / totalP;
        std::ostringstream oss; oss << std::fixed << std::setprecision(1) << avgWait;

        // TOP LEFT: Stats & Exit
        sf::Text esc(font, "ESC: Back", 20); esc.setPosition({10.f, 10.f}); window.draw(esc);
        
        std::string statsStr = "Intersections: " + std::to_string(numIntersections) + "\n" +
                               "Arrived: " + std::to_string(totalA) + "\n" +
                               "Passed: " + std::to_string(totalP) + "\n" +
                               "Waiting: " + std::to_string(totalWaitingCars) + "\n" +
                               "Avg Wait: " + oss.str() + "s\n" + 
                               "Green Time: " + std::to_string(greenTime) + "s";
                               
        sf::Text stats(font, statsStr, 18);
        stats.setPosition({10.f, 40.f});
        stats.setFillColor(sf::Color(200, 200, 200));
        window.draw(stats);

        // TOP CENTER: Step Counter (Red Box)
        float cx = 1200.f / 2.f;
        sf::RectangleShape box({200.f, 40.f});
        box.setOrigin({100.f, 0.f});
        box.setPosition({cx, 10.f});
        box.setFillColor(sf::Color(200, 0, 0));
        box.setOutlineThickness(2);
        box.setOutlineColor(sf::Color::White);
        window.draw(box);
        
        sf::Text stepT(font, "Step: " + std::to_string(currentTime) + " / " + std::to_string(simTime), 20);
        stepT.setOrigin({stepT.getLocalBounds().size.x/2, stepT.getLocalBounds().size.y/2});
        stepT.setPosition({cx, 28.f});
        window.draw(stepT);

        // TOP RIGHT: Reset
        sf::Text rst(font, "R: Reset", 20);
        rst.setPosition({1100.f, 10.f});
        window.draw(rst);

        // BOTTOM CENTER: Pause
        sf::Text pause(font, "SPACE: Pause / Resume", 24);
        pause.setOrigin({pause.getLocalBounds().size.x/2, 0});
        pause.setPosition({cx, 760.f});
        pause.setFillColor(sf::Color::Yellow);
        window.draw(pause);
        
        // Show Active Lane for INT-1 as reference (Center Top, below Red Box)
        if (!intersections.empty()) {
            std::string laneStr = "Intersection Active Lane: " + std::to_string(intersections[0]->light.currentLane + 1);
            sf::Text lT(font, laneStr, 18);
            lT.setOrigin({lT.getLocalBounds().size.x/2, 0});
            lT.setPosition({cx, 60.f});
            window.draw(lT);
        }
    }
};

int main() {
    srand((unsigned)time(nullptr));
    TrafficApp app;
    app.run();
    return 0;
}