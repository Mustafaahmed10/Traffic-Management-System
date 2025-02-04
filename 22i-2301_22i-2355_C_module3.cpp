#include <SFML/Graphics.hpp>
#include "22i-2301_22i-2355_C_challan.h"
#include "22i-2301_22i-2355_C_user.h"
#include "22i-2301_22i-2355_C_payment.h"
#include <thread>
#include <iostream>
#include <unistd.h>
#include <map>
#include <string>
#include <thread>
#include <queue>
#include <sstream>
#include <vector>
#include <mutex>
#include <iostream>
#include <chrono>
#include <random>
using namespace std;

//use for challan generation and creating db for itnerface.
map<int, Challan> challanDatabase;
mutex challanMutex;
mutex spriteMutex;

//parameter for implementation of bakers algorithm is here:
const int numResources = 3;
const int numProcesses = 4;

int available[numResources];
int max[numProcesses][numResources];
int allocation[numProcesses][numResources];
int need[numProcesses][numResources];
//-------------------------------------------------------------



std::mutex resourceMutex;

//------------------------- Vehicle structure------------------------------------------
struct Vehicle 
{
    sf::Sprite sprite;
    string type; // "RegularCar", "RegularTruck", or "Emergency"
    float speed;      // Movement speed
};

// Global variables
vector<Vehicle> northVehicles, southVehicles, eastVehicles, westVehicles;
sf::Texture roadTexture, carTexture, truckTexture, emergencyTexture;
sf::Sprite roadSprite;
int currentGreenLight = 0; // 0: North, 1: West, 2: South, 3: East
bool emergencyOverride = false;

// Load textures
void loadTextures() 
{
    if (!roadTexture.loadFromFile("/home/hp/Desktop/22i-2301_22i-2355_C/images/road.png") ||
        !carTexture.loadFromFile("/home/hp/Desktop/22i-2301_22i-2355_C/images/car.png") || 
        !truckTexture.loadFromFile("/home/hp/Desktop/22i-2301_22i-2355_C/images/truck.png") || 
        !emergencyTexture.loadFromFile("/home/hp/Desktop/22i-2301_22i-2355_C/images/emergency.png")) 
        {
        cerr << "Error loading textures.\n";
        exit(1);
         }
    roadSprite.setTexture(roadTexture);
    roadSprite.setPosition(0, 0);
}

//-----------------------BAkers algortihm to prevent deadlock-----------------------------------------------

bool isSafeState() {
    int work[numResources];
    bool finish[numProcesses] = {false};

    for (int i = 0; i < numResources; ++i) {
        work[i] = available[i];
    }

    while (true) {
        bool found = false;

        for (int i = 0; i < numProcesses; ++i) {
            if (!finish[i]) {
                bool canProceed = true;

                for (int j = 0; j < numResources; ++j) {
                    if (need[i][j] > work[j]) {
                        canProceed = false;
                        break;
                    }
                }

                if (canProceed) {
                    for (int j = 0; j < numResources; ++j) {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    found = true;
                }
            }
        }

        if (!found) {
            for (int i = 0; i < numProcesses; ++i) {
                if (!finish[i]) {
                    return false;
                }
            }
            return true;
        }
    }
}

bool requestResources(int processID, int request[]) {
    std::lock_guard<std::mutex> lock(resourceMutex);

    for (int i = 0; i < numResources; ++i) {
        if (request[i] > need[processID][i] || request[i] > available[i]) {
            return false;
        }
    }

    for (int i = 0; i < numResources; ++i) {
        available[i] -= request[i];
        allocation[processID][i] += request[i];
        need[processID][i] -= request[i];
    }

    if (isSafeState()) {
        return true;
    } else {
        for (int i = 0; i < numResources; ++i) {
            available[i] += request[i];
            allocation[processID][i] -= request[i];
            need[processID][i] += request[i];
        }
        return false;
    }
}

//-------------------------------------------------------------------------------------------------------


// ------------------------Proority handling and Adjusting vehicle size based on type -----------------------
void adjustVehicleSize(sf::Sprite& sprite, const std::string& type) 
{
    if (type == "RegularCar") 
    {
        sprite.setScale(0.05f, 0.05f); // Smaller size for regular cars
    } else if (type == "RegularTruck")
    {
        sprite.setScale(0.025f, 0.025f); // Larger size for regular trucks
    } else if (type == "Emergency") 
    {
        sprite.setScale(0.025f, 0.025f); // Larger size for emergency vehicles
    }
}

//-----------------------adding via queue management ----------------------------------------

void addVehicle(vector<Vehicle>& lane, const string& type, float x, float y, float rotation) 
{
    std::lock_guard<std::mutex> lock(spriteMutex);
    if (lane.size() >= 10) return; // Limit to 10 vehicles per lane

    Vehicle vehicle;
    vehicle.type = type;
    vehicle.speed = (type == "RegularCar" || type == "RegularTruck") ? 0.3f : 0.4f; // Emergency vehicles are faster
    if (type == "RegularCar") 
    {
        vehicle.sprite.setTexture(carTexture);
    } else if (type == "RegularTruck")
    {
        vehicle.sprite.setTexture(truckTexture);
    } else if (type == "Emergency")
    {
        vehicle.sprite.setTexture(emergencyTexture);
    }
    adjustVehicleSize(vehicle.sprite, type);
    vehicle.sprite.setPosition(x, y);
    vehicle.sprite.setRotation(rotation);

    lane.push_back(vehicle);
}

//-------------------------------- Check for emergency vehicles------------------------------------------------
bool hasEmergencyVehicle(const std::vector<Vehicle>& lane) 
{
    for (const auto& vehicle : lane) 
    {
        if (vehicle.type == "Emergency") 
        {
            return true;
        }
    }
    return false;
}
//------------------------------------------------------------------------------------------------

// ----------------------------Spawn for each direction, probabilty ,speed is adjsuting here-------------------------------------------

void spawnNorthVehicles()
{
    while (true) 
    {
        string regularType = (rand() % 2 == 0) ? "RegularCar" : "RegularTruck";
        addVehicle(northVehicles, regularType, 375, 0, 180); // No rotation for north-bound
        this_thread::sleep_for(std::chrono::milliseconds(1000));

        if (rand() % 100 < 20) 
        { // 20% chance for emergency vehicle
            addVehicle(northVehicles, "Emergency", 375, 0, 180);
            this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
    }
}

void spawnSouthVehicles() 
{
    while (true) {
        string regularType = (rand() % 2 == 0) ? "RegularCar" : "RegularTruck";
        addVehicle(southVehicles, regularType, 450, 600, 0); // 180-degree rotation for south-bound
        this_thread::sleep_for(std::chrono::milliseconds(2000));

        if (rand() % 100 < 5) 
        { // 5% chance for emergency vehicle
            addVehicle(southVehicles, "Emergency", 450, 600, 0);
            this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
    }
}

void spawnEastVehicles() 
{
    while (true) 
    {
        string regularType = (rand() % 2 == 0) ? "RegularCar" : "RegularTruck";
        addVehicle(eastVehicles, regularType, 800, 275, -90); // 90-degree rotation for east-bound
        this_thread::sleep_for(std::chrono::milliseconds(1500));

        if (rand() % 100 < 10) 
        { // 10% chance for emergency vehicle
            addVehicle(eastVehicles, "Emergency", 800, 275, -90);
            this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
    }
}

void spawnWestVehicles() 
{
    while (true) 
    {
        string regularType = (rand() % 2 == 0) ? "RegularCar" : "RegularTruck";
        addVehicle(westVehicles, regularType, 0, 350, 90); // -90-degree rotation for west-bound
        this_thread::sleep_for(std::chrono::milliseconds(2000));

        if (rand() % 100 < 30) { // 30% chance for emergency vehicle
            addVehicle(westVehicles, "Emergency", 0, 350, 90);
            this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------

//--------------- QUEUE USED FOR Update vehicle positions------------------------------------------------------------
void updateVehicles(vector<Vehicle>& queue, float deltaX, float deltaY) {
    lock_guard<mutex> lock(spriteMutex);
    for (auto& vehicle : queue) {
        vehicle.sprite.move(deltaX * vehicle.speed, deltaY * vehicle.speed);
    }
    queue.erase(remove_if(queue.begin(), queue.end(),
                               [](const Vehicle& v) {
                                   return v.sprite.getPosition().x > 800 || v.sprite.getPosition().y > 600 ||
                                          v.sprite.getPosition().x < 0 || v.sprite.getPosition().y < 0;
                               }),
                queue.end());
}

//---------------------------------------------------------------------------------------------------
// ----------------------------------for challan manager,user portal and stripe payment---------------------------------------

string getCurrentDate() {
    time_t now = time(0);
    char buf[80];
    struct tm tstruct;
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return string(buf);
}

std::string calculateDueDate() {
    time_t now = time(0) + 3 * 24 * 60 * 60; // Add 3 days in seconds
    char buf[80];
    struct tm tstruct;
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);
    return string(buf);
}

//----------------------chalan work is happening here-----------------------------------------------

void challanGenerator(int writePipe[2]) {
    close(writePipe[0]); // Close reading end of the pipe
    int challanId = 1;
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> speedDist(50, 80);

    while (true) {
        // -------------------------------------Generate vehicle details
        uniform_int_distribution<int> numberDist(0, 9999);
        string vehicleNumber = "ABC" + to_string(numberDist(gen));
        string vehicleType = (rand() % 2 == 0) ? "RegularVehicle" : "HeavyVehicle";
        string numericPart = vehicleNumber.substr(3); // Extract numeric part
        int number = std::stoi(numericPart);
        float speed = speedDist(gen);

        // =-------------------------Generate challan if speed exceeds limits
        if ((vehicleType == "RegularVehicle" && speed > 60) ||
            (vehicleType == "HeavyVehicle" && speed > 40)) {
            Challan challan;
            challan.challanId = challanId++;
            challan.vehicleNumber = vehicleNumber;
            challan.vehicleType = vehicleType;
            challan.amount = (vehicleType == "RegularVehicle" ? 5000 : 7000);
            challan.serviceCharges = challan.amount * 0.17;
            challan.issueDate = getCurrentDate();
            challan.dueDate = calculateDueDate();
            challan.paymentStatus = "unpaid";

            //------------------------------------- Add challan to database
            {
                lock_guard<mutex> lock(challanMutex);
                challanDatabase[challan.challanId] = challan;
            }

            //----------------------- Send challan to UserPortal
            ostringstream dataStream;
            dataStream << challan.challanId << "|" << challan.vehicleNumber << "|" << challan.vehicleType
                       << "|" << challan.amount + challan.serviceCharges << "|" << challan.issueDate << "|"
                       << challan.dueDate << "|" << challan.paymentStatus;
            string data = dataStream.str();
            write(writePipe[1], data.c_str(), data.size() + 1);

            cout << "Challan Generated: ID=" << challan.challanId
                      << ", Vehicle=" << challan.vehicleNumber
                      << ", Amount=" << challan.amount + challan.serviceCharges << std::endl;
        }
         this_thread::sleep_for(chrono::seconds(5));
    }
}

void challanGeneratorWindow() {
    sf::RenderWindow challanWindow(sf::VideoMode(800, 600), "Challan Generator");
    sf::Font font;
    if (!font.loadFromFile("/home/hp/Desktop/22i-2301_22i-2355_C/images/arial.ttf")) {
        cerr << "Error loading font.\n";
        return;
    }

    sf::Text title("Challan Generator", font, 24);
    title.setPosition(10, 10);
    title.setFillColor(sf::Color::Black);

    while (challanWindow.isOpen()) {
        sf::Event event;
        while (challanWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                challanWindow.close();
            }
        }

        challanWindow.clear(sf::Color::White);
        challanWindow.draw(title);

        float y = 50;
      lock_guard<std::mutex> lock(challanMutex);
        for (const auto& [id, challan] : challanDatabase) {
            sf::Text challanText("ID: " + std::to_string(id) + ", Vehicle: " + challan.vehicleNumber +
                                 ", Status: " + challan.paymentStatus, font, 18);
            challanText.setPosition(10, y);
            challanText.setFillColor(sf::Color::Black);
            y += 30;
            challanWindow.draw(challanText);
        }

        challanWindow.display();
    }
}

//----------------------------user portal working is happeining her------------------------------------

//--------------------------------------------------------------------------------------------------------
/*void userPortal(int readPipe[2], int writePipe[2]) {
    close(writePipe[0]); // Close read end of writePipe
    close(readPipe[1]);  // Close write end of readPipe

    char buffer[256];
    while (true) {
        ssize_t bytesRead = read(readPipe[0], buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            cout << "Received in User Portal: " << buffer << std::endl;
            string paymentRequest = "Processing payment for challan.";
            write(writePipe[1], paymentRequest.c_str(), paymentRequest.size() + 1);
        }
    }
}*/


void userPortalwindow(int readPipe[2], int writePipe[2]) {
    close(writePipe[0]); // Close read end of writePipe
    close(readPipe[1]);  // Close write end of readPipe
   
    sf::RenderWindow portalWindow(sf::VideoMode(800, 600), "User Portal");
    sf::Font font;
    if (!font.loadFromFile("/home/hp/Desktop/22i-2301_22i-2355_C/images/arial.ttf")) {
        cerr << "Error loading font.\n";
        return;
    }

    sf::Text title("User Portal - Challan Details", font, 24);
    title.setPosition(10, 10);
    title.setFillColor(sf::Color::Black);

    vector<Challan> displayedChallans;
    char buffer[256];

    while (portalWindow.isOpen()) {
        // Poll events for user interaction
        sf::Event event;
        while (portalWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                portalWindow.close();
            }

          
        }

        // Read data from pipe
        ssize_t bytesRead = read(readPipe[0], buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            istringstream dataStream(buffer);
            Challan newChallan;
            string field;
            getline(dataStream, field, '|');
            newChallan.challanId = stoi(field);
            getline(dataStream, newChallan.vehicleNumber, '|');
            getline(dataStream, newChallan.vehicleType, '|');
            getline(dataStream, field, '|');
            newChallan.amount = std::stof(field);
            getline(dataStream, newChallan.issueDate, '|');
            getline(dataStream, newChallan.dueDate, '|');
            getline(dataStream, newChallan.paymentStatus, '|');

            {
                lock_guard<std::mutex> lock(challanMutex);
                challanDatabase[newChallan.challanId] = newChallan;
                displayedChallans.push_back(newChallan);
            }
        }

        // Render GUI
        portalWindow.clear(sf::Color::White);
        portalWindow.draw(title);

        float y = 50;
        for (const auto& challan : displayedChallans) 
        {
            sf::Text challanText("ID: " + std::to_string(challan.challanId) + 
                                 ", Vehicle: " + challan.vehicleNumber + 
                                 ", Amount: " + to_string(challan.amount) +
                                 ", Status: " + challan.paymentStatus, font, 18);
            challanText.setPosition(10, y);
            challanText.setFillColor(sf::Color::Blue);
            y += 30;
            portalWindow.draw(challanText);
        }

        portalWindow.display();
    }
}

//----------------------cdoe for stripe payment-----------------------------------------

void stripePayment(int readPipe[2]) {
    close(readPipe[1]); // Close write end of readPipe

    char buffer[256];
    while (true) {
        ssize_t bytesRead = read(readPipe[0], buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            cout << "Stripe Payment System: " << buffer << endl;
            cout << "Payment processed successfully!" << endl;
        }
    }
}


//-----------------------------------------------------------------------------


//-------------------------------Displaying  traffic system along with signals-----------------------------------------

void display(sf::RenderWindow& window) 
{
    sf::CircleShape lights[4];
    for (int i = 0; i < 4; ++i) {
        lights[i].setRadius(20);
        lights[i].setFillColor(sf::Color::Red);
    }
    lights[0].setPosition(390, 100);  // North
    lights[1].setPosition(250, 300); // West
    lights[2].setPosition(390, 500); // South
    lights[3].setPosition(500, 300); // East

    while (window.isOpen()) 
    {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) 
            {
                window.close();
            }
        }

        // ------------------------------Update light colors according to requiremnet in module----------------------------------
        
        if (emergencyOverride) 
        {
            for (int i = 0; i < 4; ++i) 
            {
                lights[i].setFillColor(i == currentGreenLight ? sf::Color::Green : sf::Color::Red);
            }
        } else {
            for (int i = 0; i < 4; ++i) 
            {
                lights[i].setFillColor(i == currentGreenLight ? sf::Color::Green : sf::Color::Red);
            }
        }

        switch (currentGreenLight) 
        {
            case 0: updateVehicles(northVehicles, 0, 2); break;
            case 1: updateVehicles(westVehicles, 2, 0); break;
            case 2: updateVehicles(southVehicles, 0, -2); break;
            case 3: updateVehicles(eastVehicles, -2, 0); break;
        }

        window.clear();
        window.draw(roadSprite);
        for (const auto& light : lights) window.draw(light);

        lock_guard<mutex> lock(spriteMutex);
        for (const auto& vehicle : northVehicles) window.draw(vehicle.sprite);
        for (const auto& vehicle : southVehicles) window.draw(vehicle.sprite);
        for (const auto& vehicle : eastVehicles) window.draw(vehicle.sprite);
        for (const auto& vehicle : westVehicles) window.draw(vehicle.sprite);

        window.display();
    }
}

//--------------------------------------- Traffic light controller
void trafficLightController() {
    while (true) {
        if (!emergencyOverride) {
            currentGreenLight = (currentGreenLight + 1) % 4;
            this_thread::sleep_for(chrono::seconds(8));
        } else {
            this_thread::sleep_for(chrono::seconds(2));
        }
    }
}

//---------------------------------------------- Emergency management
void manageEmergencyVehicles() {
    while (true) {
        bool foundEmergency = false;

        {
            lock_guard<mutex> lock(spriteMutex);
            // Check all lanes for emergency vehicles
            if (hasEmergencyVehicle(northVehicles)) {
                currentGreenLight = 0; // Set light to North
                foundEmergency = true;
            } else if (hasEmergencyVehicle(westVehicles)) {
                currentGreenLight = 1; // Set light to West
                foundEmergency = true;
            } else if (hasEmergencyVehicle(southVehicles)) {
                currentGreenLight = 2; // Set light to South
                foundEmergency = true;
            } else if (hasEmergencyVehicle(eastVehicles)) {
                currentGreenLight = 3; // Set light to East
                foundEmergency = true;
            }
        }

        emergencyOverride = foundEmergency;

        if (foundEmergency) {
          this_thread::sleep_for(chrono::seconds(5)); // Allow emergency vehicles to pass
        } else {
            this_thread::sleep_for(chrono::milliseconds(500)); // Check periodically
        }
    }
}

int main() {

sf::RenderWindow window(sf::VideoMode(800, 600), "Smart Traffic Management System");
    // Load textures
     loadTextures();
    int writePipe[2];
    int readPipe[2];

    pipe(writePipe);
    pipe(readPipe);

    thread challanGenThread(challanGenerator, writePipe);
    thread challanWindowThread(challanGeneratorWindow);
    thread userPortalThread(userPortalwindow, writePipe, readPipe);
    thread stripePaymentThread(stripePayment, readPipe);

    thread northSpawner(spawnNorthVehicles);
    thread southSpawner(spawnSouthVehicles);
    thread eastSpawner(spawnEastVehicles);
    thread westSpawner(spawnWestVehicles);
    thread lightController(trafficLightController);
    // Emergency vehicle management
    thread emergencyManager(manageEmergencyVehicles);
    display(window);
     
      // Traffic light controller
    northSpawner.join();
    southSpawner.join();
    eastSpawner.join();
    westSpawner.join();
    lightController.join();
    emergencyManager.join();  
    challanGenThread.join();
    challanWindowThread.join();
    userPortalThread.join();
    stripePaymentThread.join();

    return 0;
}


