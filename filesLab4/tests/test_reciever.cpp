#include "gtest/gtest.h"
#include "SharedQueue.h"
#include <thread>
#include <vector>
#include <string>

// push + pop возвращает то же сообщение
TEST(BasicOps, PushThenPopReturnsMessage) {
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_basic1.bin", 4, 0));

    std::string out;
    EXPECT_TRUE(q.PushMessage("hello", false));
    EXPECT_TRUE(q.PopMessage(out, false));
    EXPECT_EQ(out, "hello");

    q.SignalShutdown();
}

// несколько сообщений читаются в том же порядке
TEST(BasicOps, FifoOrderPreserved) {
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_fifo.bin", 4, 0));

    EXPECT_TRUE(q.PushMessage("first", false));
    EXPECT_TRUE(q.PushMessage("second", false));
    EXPECT_TRUE(q.PushMessage("third", false));

    std::string out;
    EXPECT_TRUE(q.PopMessage(out, false)); EXPECT_EQ(out, "first");
    EXPECT_TRUE(q.PopMessage(out, false)); EXPECT_EQ(out, "second");
    EXPECT_TRUE(q.PopMessage(out, false)); EXPECT_EQ(out, "third");

    q.SignalShutdown();
}

// пустое сообщение допустимо
TEST(BasicOps, EmptyStringMessage) {
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_empty.bin", 2, 0));

    std::string out;
    EXPECT_TRUE(q.PushMessage("", false));
    EXPECT_TRUE(q.PopMessage(out, false));
    EXPECT_EQ(out, "");

    q.SignalShutdown();
}

// сообщение ровно MAX_MESSAGE_LEN-1
TEST(BasicOps, MaxLengthMessageAccepted) {
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_maxlen.bin", 2, 0));

    std::string maxMsg(MAX_MESSAGE_LEN - 1, 'x');
    std::string out;
    EXPECT_TRUE(q.PushMessage(maxMsg, false));
    EXPECT_TRUE(q.PopMessage(out, false));
    EXPECT_EQ(out, maxMsg);

    q.SignalShutdown();
}

// сообщение длиной MAX_MESSAGE_LEN
TEST(BasicOps, TooLongMessageRejected) {
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_toolong.bin", 2, 0));

    std::string tooLong(MAX_MESSAGE_LEN, 'y');
    EXPECT_FALSE(q.PushMessage(tooLong, false));

    q.SignalShutdown();
}

// PopMessage после SignalShutdown
TEST(Shutdown, PopOnEmptyAfterShutdownReturnsFalse) {
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_sd1.bin", 2, 0));

    q.SignalShutdown();

    std::string out;
    EXPECT_FALSE(q.PopMessage(out, false));
}

// сообщения записанные до shutdown всё равно читаются
TEST(Shutdown, MessagesWrittenBeforeShutdownAreReadable) {
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_sd2.bin", 4, 0));

    EXPECT_TRUE(q.PushMessage("msg1", false));
    EXPECT_TRUE(q.PushMessage("msg2", false));
    q.SignalShutdown();

    std::string out;

    EXPECT_TRUE(q.PopMessage(out, false));
    EXPECT_EQ(out, "msg1");
    EXPECT_TRUE(q.PopMessage(out, false));
    EXPECT_EQ(out, "msg2");
}

// PushMessage после SignalShutdown
TEST(Shutdown, PushAfterShutdownReturnsFalse) {
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_sd3.bin", 4, 0));

    q.SignalShutdown();
    EXPECT_FALSE(q.PushMessage("late", false));
}

// IsShuttingDown после SignalShutdown
TEST(Shutdown, IsShuttingDownFlag) {
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_sd4.bin", 2, 0));

    EXPECT_FALSE(q.IsShuttingDown());
    q.SignalShutdown();
    EXPECT_TRUE(q.IsShuttingDown());
}

// очередь до capacity
TEST(Capacity, FillAndDrainExactCapacity) {
    const unsigned int CAP = 3;
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_cap1.bin", CAP, 0));

    for (unsigned int i = 0; i < CAP; i++) {
        std::string msg = "m" + std::to_string(i);
        EXPECT_TRUE(q.PushMessage(msg, false));
    }

    for (unsigned int i = 0; i < CAP; i++) {
        std::string out;
        std::string expected = "m" + std::to_string(i);
        EXPECT_TRUE(q.PopMessage(out, false));
        EXPECT_EQ(out, expected);
    }

    q.SignalShutdown();
}

// кольцевой буфер
TEST(Capacity, RingBufferWraparound) {
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_wrap.bin", 3, 0));

    EXPECT_TRUE(q.PushMessage("a", false));
    EXPECT_TRUE(q.PushMessage("b", false));
    EXPECT_TRUE(q.PushMessage("c", false));

    std::string out;
    EXPECT_TRUE(q.PopMessage(out, false)); EXPECT_EQ(out, "a");
    EXPECT_TRUE(q.PopMessage(out, false)); EXPECT_EQ(out, "b");

    // слоты освободились — пишем ещё два поверх старых позиций
    EXPECT_TRUE(q.PushMessage("d", false));
    EXPECT_TRUE(q.PushMessage("e", false));

    EXPECT_TRUE(q.PopMessage(out, false)); EXPECT_EQ(out, "c");
    EXPECT_TRUE(q.PopMessage(out, false)); EXPECT_EQ(out, "d");
    EXPECT_TRUE(q.PopMessage(out, false)); EXPECT_EQ(out, "e");

    q.SignalShutdown();
}


// значение переданное при создании
TEST(Metadata, CapacityMatchesCreationParam) {
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_meta1.bin", 5, 0));
    EXPECT_EQ(q.Capacity(), 5u);
    q.SignalShutdown();
}

// IsValid() 
TEST(Metadata, IsValidAfterCreate) {
    SharedQueue uninit;
    EXPECT_FALSE(uninit.IsValid());

    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_meta2.bin", 2, 0));
    EXPECT_TRUE(q.IsValid());
    q.SignalShutdown();
}

// один поток пишет N сообщений, другой читает 
TEST(Threaded, SingleProducerSingleConsumer) {
    const int N = 20;
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_mt1.bin", 8, 0));

    std::vector<std::string> received;
    received.reserve(N);

    std::thread producer([&] {
        for (int i = 0; i < N; i++) {
            std::string msg = "msg" + std::to_string(i);
            q.PushMessage(msg, false);
        }
        });

    std::thread consumer([&] {
        for (int i = 0; i < N; i++) {
            std::string out;
            q.PopMessage(out, false);
            received.push_back(out);
        }
        });

    producer.join();
    consumer.join();

    ASSERT_EQ((int)received.size(), N);
    for (int i = 0; i < N; i++) {
        EXPECT_EQ(received[i], "msg" + std::to_string(i));
    }

    q.SignalShutdown();
}

