#ifndef UNITY_INCLUDE_CONFIG_H
#include <unity.h>
#endif

#include <stdint.h>
#include <LocoNet.h>

void test_ln_buffer_init() {
	LnBuf buff;
	initLnBuf(&buff);
	for(size_t i = 0; i < LN_BUF_SIZE ; i++) {
		TEST_ASSERT_EQUAL(0, buff.Buff[i]);
	}
	TEST_ASSERT_EQUAL(0, buff.WriteIndex);
	TEST_ASSERT_EQUAL(0, buff.ReadIndex);
	TEST_ASSERT_EQUAL(0, buff.ReadPacketIndex);
	TEST_ASSERT_EQUAL(0, buff.CheckSum);
	TEST_ASSERT_EQUAL(0, buff.ReadExpLen);
	TEST_ASSERT_EQUAL(0, buff.Stats.RxPackets);
	TEST_ASSERT_EQUAL(0, buff.Stats.RxErrors);
	TEST_ASSERT_EQUAL(0, buff.Stats.TxPackets);
	TEST_ASSERT_EQUAL(0, buff.Stats.TxErrors);
	TEST_ASSERT_EQUAL(0, buff.Stats.Collisions);
}

int main() {
	ThisThread::wait(2000);
	UNITY_BEGIN();
	RUN_TEST(test_ln_buffer_init);
	UNITY_END();
}

