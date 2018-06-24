#include "stable.h"
#include "example/example.h"


int main(int /*argc*/, char* /*argv*/[])
{
    example_throttle();
    example_snowflake();
    example_event_delegate();
    example_datetime();
    example_workerpool();
    example_strings();
    example_buffer();
    return 0;
}
