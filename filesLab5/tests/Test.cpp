#include <gtest/gtest.h>

#include <windows.h>
#include <cstring>
#include <thread>
#include <atomic>

#include "Employee.h"
#include "FileHandler.h"
#include "Request.h"

// Helpers

static Employee MakeEmployee(int id, const char* name, double hours)
{
    Employee e;
    EmployeeSet(&e, id, name, hours);
    return e;
}

static const char* TEST_FILE = "test_employees.bin";

static void CreateTestFile(const Employee* arr, int count)
{
    FileHandler fh(TEST_FILE);
    fh.CreateFileWithData(arr, count);
}

static void RemoveTestFile()
{
    DeleteFileA(TEST_FILE);
}

// Employee 

TEST(EmployeeTest, InitZerosFields)
{
    Employee e;
    EmployeeInit(&e);
    EXPECT_EQ(e.num, 0);
    EXPECT_DOUBLE_EQ(e.hours, 0.0);
    EXPECT_EQ(e.name[0], '\0');
}

TEST(EmployeeTest, SetStoresValues)
{
    Employee e;
    EmployeeSet(&e, 42, "Alice", 160.5);
    EXPECT_EQ(e.num, 42);
    EXPECT_DOUBLE_EQ(e.hours, 160.5);
    EXPECT_STREQ(e.name, "Alice");
}

TEST(EmployeeTest, SetTruncatesLongName)
{
    Employee e;
    EmployeeSet(&e, 1, "VeryLongName", 10.0);
    EXPECT_EQ(strnlen(e.name, sizeof(e.name)), sizeof(e.name) - 1u);
}

//FileHandler

class FileHandlerTest : public ::testing::Test {
protected:
    void TearDown() override { RemoveTestFile(); }
};

TEST_F(FileHandlerTest, CreateAndReadBack)
{
    Employee src[3] = {
        MakeEmployee(1, "Alice", 100.0),
        MakeEmployee(2, "Bob",   200.0),
        MakeEmployee(3, "Carol", 150.0),
    };
    CreateTestFile(src, 3);

    FileHandler fh(TEST_FILE);
    Employee out;
    EXPECT_TRUE(fh.GetEmployee(1, out));
    EXPECT_EQ(out.num, 1);
    EXPECT_STREQ(out.name, "Alice");
    EXPECT_DOUBLE_EQ(out.hours, 100.0);
}

TEST_F(FileHandlerTest, GetEmployeeNotFound)
{
    Employee src[] = { MakeEmployee(1, "Alice", 100.0) };
    CreateTestFile(src, 1);

    FileHandler fh(TEST_FILE);
    Employee out;
    EXPECT_FALSE(fh.GetEmployee(999, out));
}

TEST_F(FileHandlerTest, UpdateEmployee)
{
    Employee src[] = {
        MakeEmployee(1, "Alice", 100.0),
        MakeEmployee(2, "Bob",   200.0),
    };
    CreateTestFile(src, 2);

    FileHandler fh(TEST_FILE);
    Employee updated = MakeEmployee(1, "Alicia", 120.0);
    EXPECT_TRUE(fh.UpdateEmployee(updated));

    Employee out;
    EXPECT_TRUE(fh.GetEmployee(1, out));
    EXPECT_STREQ(out.name, "Alicia");
    EXPECT_DOUBLE_EQ(out.hours, 120.0);
}

TEST_F(FileHandlerTest, UpdateNonExistentReturnsFalse)
{
    Employee src[] = { MakeEmployee(1, "Alice", 100.0) };
    CreateTestFile(src, 1);

    FileHandler fh(TEST_FILE);
    Employee ghost = MakeEmployee(99, "Ghost", 1.0);
    EXPECT_FALSE(fh.UpdateEmployee(ghost));
}

TEST_F(FileHandlerTest, UpdateDoesNotCorruptOtherRecords)
{
    Employee src[] = {
        MakeEmployee(1, "Alice", 100.0),
        MakeEmployee(2, "Bob",   200.0),
        MakeEmployee(3, "Carol", 150.0),
    };
    CreateTestFile(src, 3);

    FileHandler fh(TEST_FILE);
    fh.UpdateEmployee(MakeEmployee(2, "Bobby", 99.0));

    Employee out;
    EXPECT_TRUE(fh.GetEmployee(1, out)); EXPECT_STREQ(out.name, "Alice");
    EXPECT_TRUE(fh.GetEmployee(3, out)); EXPECT_STREQ(out.name, "Carol");
}

TEST_F(FileHandlerTest, GetFromNonExistentFile)
{
    FileHandler fh("no_such_file.bin");
    Employee out;
    EXPECT_FALSE(fh.GetEmployee(1, out));
}

// PipeSendRaw 

class PipeIOTest : public ::testing::Test {
protected:
    HANDLE hRead = INVALID_HANDLE_VALUE;
    HANDLE hWrite = INVALID_HANDLE_VALUE;

    void SetUp() override
    {
        SECURITY_ATTRIBUTES sa{ sizeof(sa), NULL, TRUE };
        ASSERT_TRUE(CreatePipe(&hRead, &hWrite, &sa, 0));
    }
    void TearDown() override
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
    }
};

TEST_F(PipeIOTest, SendAndReceiveRequest)
{
    Request sent{};
    sent.type = REQ_READ;
    sent.emp_id = 7;

    EXPECT_TRUE(PipeSendRaw(hWrite, &sent, sizeof(sent)));

    Request recv{};
    EXPECT_TRUE(PipeRecvRaw(hRead, &recv, sizeof(recv)));
    EXPECT_EQ(recv.type, REQ_READ);
    EXPECT_EQ(recv.emp_id, 7);
}

TEST_F(PipeIOTest, SendAndReceiveResponse)
{
    Response sent{};
    sent.success = TRUE;
    EmployeeSet(&sent.data, 3, "Dan", 88.0);
    strncpy_s(sent.message, sizeof(sent.message), "OK", _TRUNCATE);

    EXPECT_TRUE(PipeSendRaw(hWrite, &sent, sizeof(sent)));

    Response recv{};
    EXPECT_TRUE(PipeRecvRaw(hRead, &recv, sizeof(recv)));
    EXPECT_TRUE(recv.success);
    EXPECT_EQ(recv.data.num, 3);
    EXPECT_STREQ(recv.data.name, "Dan");
    EXPECT_STREQ(recv.message, "OK");
}

TEST_F(PipeIOTest, WrongSizeReturnsFalse)
{
    DWORD fakeSize = 999;
    DWORD written = 0;
    WriteFile(hWrite, &fakeSize, sizeof(DWORD), &written, NULL);

    char payload[4]{};
    WriteFile(hWrite, payload, sizeof(payload), &written, NULL);

    Request recv{};
    EXPECT_FALSE(PipeRecvRaw(hRead, &recv, sizeof(recv)));
}

TEST_F(PipeIOTest, SendReceiveRoundtripMultiple)
{
    for (int i = 0; i < 10; ++i) {
        Request s{};
        s.type = REQ_MODIFY_COMMIT;
        s.emp_id = i;
        EXPECT_TRUE(PipeSendRaw(hWrite, &s, sizeof(s)));

        Request r{};
        EXPECT_TRUE(PipeRecvRaw(hRead, &r, sizeof(r)));
        EXPECT_EQ(r.emp_id, i);
    }
}

//LockManager

#include "LockManager.h"

TEST(LockManagerTest, SingleReadAcquireRelease)
{
    LockManager lm;
    EXPECT_TRUE(lm.AcquireRead(1));
    lm.ReleaseRead(1);
}

TEST(LockManagerTest, SingleWriteAcquireRelease)
{
    LockManager lm;
    EXPECT_TRUE(lm.AcquireWrite(1));
    lm.ReleaseWrite(1);
}

TEST(LockManagerTest, MultipleReadersSimultaneous)
{
    LockManager lm;
    EXPECT_TRUE(lm.AcquireRead(5));
    EXPECT_TRUE(lm.AcquireRead(5));
    EXPECT_TRUE(lm.AcquireRead(5));
    lm.ReleaseRead(5);
    lm.ReleaseRead(5);
    lm.ReleaseRead(5);
}

TEST(LockManagerTest, WriterBlocksUntilReadersLeave)
{
    LockManager lm;
    std::atomic<bool> writerAcquired{ false };

    lm.AcquireRead(10);

    std::thread writer([&] {
        lm.AcquireWrite(10);
        writerAcquired = true;
        lm.ReleaseWrite(10);
        });

    Sleep(100);
    EXPECT_FALSE(writerAcquired.load());

    lm.ReleaseRead(10);
    writer.join();
    EXPECT_TRUE(writerAcquired.load());
}

TEST(LockManagerTest, ReadersBlockWhileWriterHoldsLock)
{
    LockManager lm;
    std::atomic<bool> readerAcquired{ false };

    lm.AcquireWrite(20);

    std::thread reader([&] {
        lm.AcquireRead(20);
        readerAcquired = true;
        lm.ReleaseRead(20);
        });

    Sleep(100);
    EXPECT_FALSE(readerAcquired.load());

    lm.ReleaseWrite(20);
    reader.join();
    EXPECT_TRUE(readerAcquired.load());
}

TEST(LockManagerTest, DifferentIdsDoNotBlock)
{
    LockManager lm;
    lm.AcquireWrite(1);

    std::atomic<bool> readerDone{ false };
    std::thread reader([&] {
        lm.AcquireRead(2);
        readerDone = true;
        lm.ReleaseRead(2);
        });

    reader.join();
    EXPECT_TRUE(readerDone.load());

    lm.ReleaseWrite(1);
}

TEST(LockManagerTest, WriteAfterWriteIsSequential)
{
    LockManager lm;
    std::vector<int> order;
    std::mutex orderMu;

    lm.AcquireWrite(30);

    std::thread w2([&] {
        lm.AcquireWrite(30);
        { std::lock_guard<std::mutex> g(orderMu); order.push_back(2); }
        lm.ReleaseWrite(30);
        });

    Sleep(50);
    { std::lock_guard<std::mutex> g(orderMu); order.push_back(1); }
    lm.ReleaseWrite(30);

    w2.join();
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
}

//  main 

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}