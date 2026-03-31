#include <gtest/gtest.h>
#include "array.h"


// После создания все элементы должны быть нулями
TEST(ArrayTest, InitializedWithZeros) {
    Array arr(10);
    for (int i = 0; i < arr.getSize(); i++)
        EXPECT_EQ(arr.getElement(i), 0)
        << "Element at index " << i << " should be 0 after init";
}

// Размер должен совпадать с переданным
TEST(ArrayTest, SizeIsCorrect) {
    Array arr(7);
    EXPECT_EQ(arr.getSize(), 7);
}

// Нельзя создать массив с нулевым или отрицательным размером
TEST(ArrayTest, ThrowsOnZeroSize) {
    EXPECT_THROW(Array arr(0), std::invalid_argument);
}

TEST(ArrayTest, ThrowsOnNegativeSize) {
    EXPECT_THROW(Array arr(-5), std::invalid_argument);
}


TEST(ArrayTest, SetAndGetElement) {
    Array arr(5);
    arr.setElement(0, 42);
    EXPECT_EQ(arr.getElement(0), 42);
}

TEST(ArrayTest, SetMultipleElements) {
    Array arr(5);
    arr.setElement(0, 1);
    arr.setElement(2, 3);
    arr.setElement(4, 5);
    EXPECT_EQ(arr.getElement(0), 1);
    EXPECT_EQ(arr.getElement(1), 0); 
    EXPECT_EQ(arr.getElement(2), 3);
    EXPECT_EQ(arr.getElement(3), 0);
    EXPECT_EQ(arr.getElement(4), 5);
}


// Зачистка удаляет все вхождения нужного значения
TEST(ArrayTest, ClearByValueRemovesAllOccurrences) {
    Array arr(6);
    arr.setElement(0, 2);
    arr.setElement(2, 2);
    arr.setElement(4, 2);

    arr.clearByValue(2);

    EXPECT_EQ(arr.getElement(0), 0);
    EXPECT_EQ(arr.getElement(2), 0);
    EXPECT_EQ(arr.getElement(4), 0);
}

// Зачистка не трогает чужие ячейки
TEST(ArrayTest, ClearByValueDoesNotTouchOthers) {
    Array arr(6);
    arr.setElement(0, 1); // маркер 1
    arr.setElement(1, 2); // маркер 2
    arr.setElement(2, 1); // маркер 1
    arr.setElement(3, 3); // маркер 3

    arr.clearByValue(1); // убиваем маркер 1

    EXPECT_EQ(arr.getElement(0), 0);
    EXPECT_EQ(arr.getElement(1), 2); 
    EXPECT_EQ(arr.getElement(2), 0);
    EXPECT_EQ(arr.getElement(3), 3); 
}

// Зачистка несуществующего значения не меняет массив
TEST(ArrayTest, ClearByValueNothingToClean) {
    Array arr(4);
    arr.setElement(0, 1);
    arr.setElement(1, 2);

    arr.clearByValue(99); 

    EXPECT_EQ(arr.getElement(0), 1);
    EXPECT_EQ(arr.getElement(1), 2);
}

// Зачистка всего массива
TEST(ArrayTest, ClearByValueEntireArray) {
    Array arr(5);
    for (int i = 0; i < 5; i++)
        arr.setElement(i, 7);

    arr.clearByValue(7);

    for (int i = 0; i < 5; i++)
        EXPECT_EQ(arr.getElement(i), 0);
}

// После зачистки маркера массив частично заполнен другими
TEST(ArrayTest, AfterCleanupOtherMarkersRemain) {
    Array arr(6);
    arr.setElement(0, 1);
    arr.setElement(1, 2);
    arr.setElement(2, 1);
    arr.setElement(3, 2);
    arr.setElement(4, 1);
    arr.setElement(5, 2);

    arr.clearByValue(1);

    EXPECT_EQ(arr.getElement(1), 2);
    EXPECT_EQ(arr.getElement(3), 2);
    EXPECT_EQ(arr.getElement(5), 2);

    EXPECT_EQ(arr.getElement(0), 0);
    EXPECT_EQ(arr.getElement(2), 0);
    EXPECT_EQ(arr.getElement(4), 0);
}


TEST(ArrayTest, MarkerDoesNotOverwriteNonZero) {
    Array arr(5);
    arr.setElement(2, 1); 

    if (arr.getElement(2) == 0)
        arr.setElement(2, 2);

    EXPECT_EQ(arr.getElement(2), 1);
}

TEST(ArrayTest, MarkerWritesToFreeCell) {
    Array arr(5);
    if (arr.getElement(3) == 0)
        arr.setElement(3, 2); 

    EXPECT_EQ(arr.getElement(3), 2);
}


TEST(ArrayTest, FullScenario_TwoMarkersThenCleanup) {
    Array arr(10);

    arr.setElement(0, 1);
    arr.setElement(2, 1);
    arr.setElement(4, 1);

    arr.setElement(1, 2);
    arr.setElement(3, 2);
    arr.setElement(5, 2);

    arr.clearByValue(1);

    EXPECT_EQ(arr.getElement(0), 0);
    EXPECT_EQ(arr.getElement(2), 0);
    EXPECT_EQ(arr.getElement(4), 0);

    EXPECT_EQ(arr.getElement(1), 2);
    EXPECT_EQ(arr.getElement(3), 2);
    EXPECT_EQ(arr.getElement(5), 2);

    arr.clearByValue(2);

    for (int i = 0; i < arr.getSize(); i++)
        EXPECT_EQ(arr.getElement(i), 0)
        << "Expected all zeros after both markers cleaned up, index=" << i;
}