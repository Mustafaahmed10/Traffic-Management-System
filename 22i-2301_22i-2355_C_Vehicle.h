#ifndef VEHICLE_H
#define VEHICLE_H

#include <string>

class Vehicle {
public:
    Vehicle(float speed, bool isEmergency);
    
    // Getters and setters
    std::string getNumberPlate() const;
    float getSpeed() const;
    float getMaxSpeed() const;
    bool isEmergencyVehicle() const;
    void setChallanStatus(bool status);
    bool hasChallan() const;

private:
    std::string numberPlate;
    float speed;
    bool emergency;
    bool challanStatus;  // Whether this vehicle has been issued a challan
};

#endif

