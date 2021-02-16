#include "DlmsReader.h"
#include <ArduinoJson.h>
#include <unity.h>
#include "data/testdata.h"

char resultStr[DLMS_READER_BUFFER_SIZE] = {0};
StaticJsonDocument<DLMS_READER_BUFFER_SIZE*4> jsonData;

void testGenericJson(uint8_t *data, uint32_t len) {
        
    DlmsReader dmr;
    dmr.setJson(&jsonData);
    bool result;
    result = dmr.ParseData(data, len);
    serializeJson(jsonData, resultStr, DLMS_READER_BUFFER_SIZE);
    printf("%s\n", resultStr);
    TEST_ASSERT_TRUE(result);
}

void testAXDR1(void) {
    jsonData.clear();
    testGenericJson(axdr1Data, sizeof(axdr1Data));
    TEST_ASSERT_EQUAL(66401, jsonData["payload"]["0100040800FF"].getElement(0));
}

void testAXDR2(void) {
    jsonData.clear();
    testGenericJson(axdr2Data, sizeof(axdr2Data));
    TEST_ASSERT_EQUAL(233, jsonData["payload"]["Kamstrup_V0001"].getElement(23));
}

void testAXDR3(void) {
    jsonData.clear();
    testGenericJson(axdr3Data, sizeof(axdr3Data));
    TEST_ASSERT_EQUAL(1316, jsonData["payload"]["data"].getElement(0));
}

void testAXDR4(void) {
    jsonData.clear();
    testGenericJson(axdr4Data, sizeof(axdr4Data));
    TEST_ASSERT_EQUAL(2402, jsonData["payload"]["4B464D5F303031"].getElement(11));
}

void testASCII1(void) {
    jsonData.clear();
    testGenericJson(ascii1Data, sizeof(ascii1Data));
    TEST_ASSERT_EQUAL(473.789, jsonData["payload"]["KFM5KAIFA-METER"]["0-1:24.2.1"].getElement(1));
}

void testASCII2(void) {
    jsonData.clear();
    testGenericJson(ascii2Data, sizeof(ascii2Data));
    TEST_ASSERT_EQUAL(10.2, jsonData["payload"]["ELL5_253833635_A"]["1-0:71.7.0"].getElement(0));
}
void testASCII3(void) {
    DlmsReader dmr;
    dmr.setJson(&jsonData);
    bool result;
    result = dmr.ParseData(ascii3Data, sizeof(ascii3Data));
    serializeJson(jsonData, resultStr, DLMS_READER_BUFFER_SIZE);
    printf("%s\n", resultStr);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(10.2, jsonData["payload"]["ELL5_253833635_A"]["1-0:71.7.0"].getElement(0));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(testAXDR1);
    RUN_TEST(testAXDR2);
    RUN_TEST(testAXDR3);
    RUN_TEST(testAXDR4);
    RUN_TEST(testASCII1);
    RUN_TEST(testASCII2);
    RUN_TEST(testASCII3);
    UNITY_END();

    return 0;
}
