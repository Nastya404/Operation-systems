#include "gtest/gtest.h"
#include "../headers/SharedQueue.h"
using namespace solution;
TEST(SenderBehavior, SendShortMessages)
{
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_send.bin", 3, 0));
    EXPECT_TRUE(q.PushMessage("a", false));
    EXPECT_TRUE(q.PushMessage("b", false));
    EXPECT_TRUE(q.PushMessage("c", false));
    string out;
    EXPECT_TRUE(q.PopMessage(out, false));
    EXPECT_EQ(out, "a");
    EXPECT_TRUE(q.PopMessage(out, false));
    EXPECT_EQ(out, "b");
    EXPECT_TRUE(q.PopMessage(out, false));
    EXPECT_EQ(out, "c");
    q.SignalShutdown();
}

TEST(SenderBehavior, RejectsTooLongMessage)
{
    SharedQueue q;
    ASSERT_TRUE(q.CreateAsReceiver("test_send2.bin", 3, 0));
    string longMsg(MAX_MESSAGE_LEN, 'x');
    EXPECT_FALSE(q.PushMessage(longMsg, false));
    string okMsg(MAX_MESSAGE_LEN - 1, 'y');
    EXPECT_TRUE(q.PushMessage(okMsg, false));
    string out;
    EXPECT_TRUE(q.PopMessage(out, false));
    EXPECT_EQ(out, okMsg);
    q.SignalShutdown();
}