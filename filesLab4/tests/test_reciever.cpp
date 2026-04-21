#include "gtest/gtest.h"
#include "../headers/SharedQueue.h"
using namespace solution;
TEST(ReceiverBehavior, ReadFromEmptyBlocksThenReads)
{
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_recv.bin", 2, 0));
    string out;
    EXPECT_TRUE(q.PushMessage("hello", false));
    EXPECT_TRUE(q.PopMessage(out, false));
    EXPECT_EQ(out, "hello");
    q.SignalShutdown();
}

TEST(ReceiverBehavior, ShutdownUnblocksReceiver)
{
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_recv2.bin", 2, 0));
    q.SignalShutdown();
    string out;
    EXPECT_FALSE(q.PopMessage(out, false));
}