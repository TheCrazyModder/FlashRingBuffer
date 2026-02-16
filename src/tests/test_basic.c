#include "stdio.h"
#include "../core/flashlog.h"

typedef struct {
    uint32_t number;
    uint32_t another_number;
    uint8_t yet_another_number;
    uint16_t just_one_more;
} TestStruct;

void this_one_works() {
    TestStruct test = {0};
    test.number = 1;
    test.another_number = 2;
    test.yet_another_number = 3;
    
    g_flash_hal.write(0, &test, sizeof(test));
    
    TestStruct t = {0};
    
    g_flash_hal.read(0, &t, sizeof(test));
    printf("%i %i %i\n", t.number, t.another_number, t.yet_another_number);
}

uint8_t error;

int main() {
    FlashlogState state = {0};
    error = flashlog_init(&state);
    
    if (error != 0) {
        printf("Error initializing: %i\n", error);
    }
    
    TestStruct test = {0};
    test.number = 1;
    test.another_number = 2;
    test.yet_another_number = 3;
    test.just_one_more = 5;
    
    error = flashlog_write(&state, &test, sizeof(test));
    
    printf("Write result: %i\n", error);
    
    //printf("State: %i %i %i\n", state.last_record_addr, state.last_record_seq, state.struct_already);
    
    TestStruct read = {0};
    
    printf("Read Error: %i\n", error);
    printf("Read struct number argument: %i %i %i %i\n", read.number, test.another_number, test.yet_another_number, test.just_one_more);
    
    
    test.number = 0;
    test.another_number = 0;
    test.yet_another_number = 1;
    test.just_one_more = 6;
    
    printf("State address and record_already: %u %i\n", state.last_record_addr, state.struct_already);
    
    error = flashlog_write(&state, &test, sizeof(test));
    
    //printf("Write result: %i\n", error);
    
    
    
    error = read_latest(&state, &read, sizeof(read));
    
    printf("Read Error: %i\n", error);
    printf("Read struct number argument: %i %i %i %i\n", read.number, test.another_number, test.yet_another_number, test.just_one_more);
    
    // test the sector skipping
    
    flashlog_deinit();
}