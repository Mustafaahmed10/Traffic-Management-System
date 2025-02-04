#ifndef CHALLAN_GENERATOR_H
#define CHALLAN_GENERATOR_H
using namespace std;
// Data structure for Challan
struct Challan {
    int challanId;
    string vehicleNumber;
    string vehicleType;
    float amount;
    float serviceCharges;
    string issueDate;
    string dueDate;
    string paymentStatus; // unpaid, paid, overdue
};

// Global challan database
extern map<int, Challan> challanDatabase;

// Function declarations
string getCurrentDate();
string calculateDueDate();
void challanGenerator(int writePipe[2]);
void challanGeneratorWindow();

#endif // CHALLAN_GENERATOR_H

