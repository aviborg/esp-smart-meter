#include "DlmsReader.h"
#include <ArduinoJson.h>
#include <unity.h>
#include "data/testdata.h"

char resultStr[4096] = "";

void testAXDR(void) {
    StaticJsonDocument<8192> jsonData;
    DlmsReader dmr;
    dmr.setJson(&jsonData); 
    TEST_ASSERT_TRUE(dmr.ParseData(axdr1Data, 581));
    serializeJson(jsonData, resultStr, 4096);
    printf(resultStr);
    printf("\n");
    TEST_ASSERT_EQUAL(0, strcmp(resultStr, axdr1Str));
}

void testASCII1(void) {
    StaticJsonDocument<8192> jsonData;
    DlmsReader dmr;
    dmr.setJson(&jsonData);

    TEST_ASSERT_TRUE(dmr.ParseData(ascii1Data, 678));
    serializeJson(jsonData, resultStr, 4096);
    printf(resultStr);
    printf("\n");
    TEST_ASSERT_EQUAL(0, strcmp(resultStr, ascii1Str));
}

void testASCII2(void) {
    StaticJsonDocument<8192> jsonData;
    DlmsReader dmr;
    dmr.setJson(&jsonData);

    TEST_ASSERT_TRUE(dmr.ParseData(ascii2Data, 712));
    serializeJson(jsonData, resultStr, 4096);
    printf(resultStr);
    printf("\n");
    TEST_ASSERT_EQUAL(0, strcmp(resultStr, ascii2Str));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(testAXDR);
    RUN_TEST(testASCII1);
    RUN_TEST(testASCII2);
    UNITY_END();

    return 0;
}
