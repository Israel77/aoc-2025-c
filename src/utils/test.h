#define TEST_OK(msg)    do { printf("\033[32m [  OK ] %s \033[0m\n", msg); ++tests_passed; } while (0)
#define TEST_FAIL(msg)  do { printf("\033[31m [ FAIL] %s \033[0m\n", msg); ++tests_failed; } while (0)
