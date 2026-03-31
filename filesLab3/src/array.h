#pragma once
#include <stdexcept>

class Array {
private:

    int* data;   
    int  size;  

public:
    Array(int size);      
    ~Array();             
    int  getSize() const;
    int  getElement(int index) const;
    void setElement(int index, int value);
    void print() const;          
    void clearByValue(int value);
};