#include <gtest/gtest.h>
#include "headers/sharedQueue.h"

#include <cstdio>
#include <string>

namespace {

const char* kTestFile = "test_queue.bin";

void RemoveTestFile() {
    std::remove(kTestFile);
}

} 

TEST(SharedQueueSmoke, CreatePushPopSingleMessage) {
    RemoveTestFile();

    SharedQueue receiver;
    ASSERT_TRUE(receiver.CreateAsReceiver(kTestFile, 4, 1));

    SharedQueue sender;
    ASSERT_TRUE(sender.OpenAsSender(kTestFile));

    ASSERT_TRUE(sender.PushMessage("hello", false));

    std::string out;
    ASSERT_TRUE(receiver.PopMessage(out, false));
    EXPECT_EQ(out, "hello");
}

TEST(SharedQueueSmoke, FifoOrderIsPreserved) {
    RemoveTestFile();

    SharedQueue receiver;
    ASSERT_TRUE(receiver.CreateAsReceiver(kTestFile, 8, 1));

    SharedQueue sender;
    ASSERT_TRUE(sender.OpenAsSender(kTestFile));

    ASSERT_TRUE(sender.PushMessage("one",   false));
    ASSERT_TRUE(sender.PushMessage("two",   false));
    ASSERT_TRUE(sender.PushMessage("three", false));

    std::string out;
    ASSERT_TRUE(receiver.PopMessage(out, false));
    EXPECT_EQ(out, "one");
    ASSERT_TRUE(receiver.PopMessage(out, false));
    EXPECT_EQ(out, "two");
    ASSERT_TRUE(receiver.PopMessage(out, false));
    EXPECT_EQ(out, "three");
}

TEST(SharedQueueSmoke, ShutdownFlagIsVisibleToSender) {
    RemoveTestFile();

    SharedQueue receiver;
    ASSERT_TRUE(receiver.CreateAsReceiver(kTestFile, 2, 1));

    SharedQueue sender;
    ASSERT_TRUE(sender.OpenAsSender(kTestFile));

    EXPECT_FALSE(sender.IsShuttingDown());
    receiver.SignalShutdown();
    EXPECT_TRUE(sender.IsShuttingDown());
}

TEST(SharedQueueSmoke, AllSendersExitUnblocksReceiver) {
    RemoveTestFile();

    SharedQueue receiver;
    ASSERT_TRUE(receiver.CreateAsReceiver(kTestFile, 4, 2));

    SharedQueue sender1;
    SharedQueue sender2;
    ASSERT_TRUE(sender1.OpenAsSender(kTestFile));
    ASSERT_TRUE(sender2.OpenAsSender(kTestFile));

    sender1.SignalSenderExit();
    sender2.SignalSenderExit();

    std::string out;
    EXPECT_FALSE(receiver.PopMessage(out, false));
    EXPECT_TRUE(out.empty());
}
