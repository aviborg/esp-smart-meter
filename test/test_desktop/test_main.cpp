#include "DlmsReader.h"
#include <ArduinoJson.h>
#include <unity.h>
#include "data/testdata.h"

char resultStr[4096] = "";

void testAXDR(void) {
    StaticJsonDocument<8192> jsonData;
    DlmsReader dmr;
    dmr.setJson(&jsonData); 
    dmr.ParseData(axdr1Data, 581);
    serializeJson(jsonData, resultStr, 4096);
    printf(resultStr);
    printf("\n");
    TEST_ASSERT_EQUAL(0, strcmp(resultStr, axdr1Str));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(testAXDR);
    UNITY_END();

    return 0;
}
