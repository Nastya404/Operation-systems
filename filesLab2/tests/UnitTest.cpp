#include "CppUnitTest.h"
#include "threads.h"
#include <cmath>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
    TEST_CLASS(MinMaxThreadTests)
    {
    public:

        TEST_METHOD(BasicArray)
        {
            SharedData data;
            data.array = { 3, 1, 4, 1, 5, 9, 2, 6 };
            MinMaxThread(&data);

            Assert::AreEqual(1, data.minValue);
            Assert::AreEqual(1, data.minIndex);
            Assert::AreEqual(9, data.maxValue);
            Assert::AreEqual(5, data.maxIndex);
        }

        TEST_METHOD(SingleElement)
        {
            SharedData data;
            data.array = { 42 };
            MinMaxThread(&data);

            Assert::AreEqual(42, data.minValue);
            Assert::AreEqual(42, data.maxValue);
            Assert::AreEqual(0, data.minIndex);
            Assert::AreEqual(0, data.maxIndex);
        }

        TEST_METHOD(AllEqualElements)
        {
            SharedData data;
            data.array = { 5, 5, 5, 5 };
            MinMaxThread(&data);

            Assert::AreEqual(5, data.minValue);
            Assert::AreEqual(5, data.maxValue);
        }

        TEST_METHOD(NegativeValues)
        {
            SharedData data;
            data.array = { -3, -1, -7, 0, 2 };
            MinMaxThread(&data);

            Assert::AreEqual(-7, data.minValue);
            Assert::AreEqual(2, data.maxValue);
            Assert::AreEqual(2, data.minIndex);
            Assert::AreEqual(4, data.maxIndex);
        }
    };

    TEST_CLASS(AverageThreadTests)
    {
    public:

        TEST_METHOD(BasicArray)
        {
            SharedData data;
            data.array = { 2, 4, 6, 8 };
            AverageThread(&data);

            Assert::AreEqual(5.0, data.average, 0.001);
        }

        TEST_METHOD(SingleElement)
        {
            SharedData data;
            data.array = { 10 };
            AverageThread(&data);

            Assert::AreEqual(10.0, data.average, 0.001);
        }

        TEST_METHOD(NegativeValues)
        {
            SharedData data;
            data.array = { -10, 10 };
            AverageThread(&data);

            Assert::AreEqual(0.0, data.average, 0.001);
        }
    };

    TEST_CLASS(FullWorkflowTests)
    {
    public:

        TEST_METHOD(ReplaceMinMaxWithAverage)
        {
            SharedData data;
            data.array = { 10, 3, 7, 15, 2, 8 };

            MinMaxThread(&data);
            AverageThread(&data);

            int avgInt = static_cast<int>(std::round(data.average));
            data.array[data.minIndex] = avgInt;
            data.array[data.maxIndex] = avgInt;

            Assert::AreEqual(8, data.array[3]);
            Assert::AreEqual(8, data.array[4]);
            Assert::AreEqual(10, data.array[0]);
            Assert::AreEqual(3, data.array[1]);
            Assert::AreEqual(7, data.array[2]);
            Assert::AreEqual(8, data.array[5]);
        }
    };
}