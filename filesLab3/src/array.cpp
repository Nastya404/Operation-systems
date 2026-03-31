#include "array.h"
#include <iostream>
using namespace std;

Array::Array(int size) {
    if (size <= 0) throw std::invalid_argument("The array size must be positive");
    this->size = size;
    data = new int[size];
    for (int i = 0; i < size; i++)
        data[i] = 0;
}

Array::~Array() {
    delete[] data;
}

int Array::getSize() const { return size; }

int Array::getElement(int index) const { return data[index]; }

void Array::setElement(int index, int value) { data[index] = value; }

void Array::clearByValue(int value) {
    for (int i = 0; i < size; i++)
        if (data[i] == value)
            data[i] = 0;
}

void Array::print() const {
    for (int i = 0; i < size; i++)
        cout << "[" << i << "]=" << data[i] << " ";
    cout << endl;
}