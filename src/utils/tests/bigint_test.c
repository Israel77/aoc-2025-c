#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../macros.h"

#undef ALLOC_STD_IMPL
#undef ALLOC_ARENA_IMPL
#define ALLOC_STD_IMPL
#define ALLOC_ARENA_IMPL
#include "../allocator.h"

#undef STRING_UTILS_IMPL
#define STRING_UTILS_IMPL
#include "../string_utils.h"

#undef BIGINT_IMPL
#define BIGINT_IMPL
#include "../bigint.h"

static int tests_passed = 0;
static int tests_failed = 0;

static void test_encoding_decoding() {
    error_t err = {0};

    char *values[] = {
        "6154021310635472942320286609226814949822194572662423616250",
        "-1",
        "28491",
        "16",
        "4294967296",
        "123456789123456789123456789",
        "18446744073709551617",
        "120000000000000000000000000",
        "0",
    };

    bool ok = true;
    for (size_t i = 0; i < sizeof(values)/sizeof(char*); ++i) {

        const char *value = values[i];
        string_t sized_value = string_from_cstr(value);

        bigint_t num = bigint_from_cstr(value, &global_std_allocator, NULL, &err);

        if (err.is_error) {
            TEST_FAIL("");
            fprintf(stderr, "Error while parsing%s",err.error_msg);
        }
        
        string_builder_t sb = bigint_to_sb(&num, &global_std_allocator, NULL, &global_std_allocator, NULL);

        string_t str = sb_build(&sb);

        /* string_println(&str); */

        ok &= string_equals(&str, &sized_value); 

        if (!ok) {
            fprintf(stderr, "Expected: %s, got %.*s\n", value, (int)str.count, str.chars);
        }

        da_free(sb.items, &sb.array_info);
        da_free(num.items, &num.array_info);
    }

    if (ok) {
        TEST_OK("Creating and decoding big integers from strings");
    } else {
        TEST_FAIL("Creating and decoding big integers from strings");
    }
}

static void test_addition() {

    error_t err;
    bigint_t result = bigint_from_cstr("0", &global_std_allocator, NULL, &err);
    bigint_t expected_result;

    bigint_t small_positive = bigint_from_cstr("256", &global_std_allocator, NULL, &err);
    bigint_t small_negative = bigint_from_cstr("-255", &global_std_allocator, NULL, &err);
    bigint_t big_positive = bigint_from_cstr("100000000000", &global_std_allocator, NULL, &err);
    bigint_t big_negative = bigint_from_cstr("-10000000000", &global_std_allocator, NULL, &err);
    

    expected_result = bigint_from_cstr("256", &global_std_allocator, NULL, &err);
    bigint_add_in(&result, &small_positive);
    if (bigint_equals(&expected_result, &result)) {
        TEST_OK("Add positive integers");
    } else {
        TEST_FAIL("Add positive integers");
        printf("%s\n", err.error_msg);
    }

    /* Reset result for the next test */
    da_free(expected_result.items, &expected_result.array_info);

    /* Test adding a small negative integer */
    expected_result = bigint_from_cstr("1", &global_std_allocator, NULL, &err); /* 256 + (-255) = 1 */
    bigint_add_in(&result, &small_negative);
    if (bigint_equals(&expected_result, &result)) {
        TEST_OK("Add positive and negative integers");
    } else {
        TEST_FAIL("Add positive and negative integers");
        printf("%s\n", err.error_msg);
    }
    
    /* Reset result for the next test */
    result.array_info.count = 0;
    bigint_normalize(&result);
    da_free(expected_result.items, &expected_result.array_info);

    /* Test adding a big positive integer */
    expected_result = bigint_from_cstr("100000000000", &global_std_allocator, NULL, &err); /* 0 + 100000000000 = 100000000000 */
    bigint_add_in(&result, &big_positive);
    if (bigint_equals(&expected_result, &result)) {
        TEST_OK("Add big positive integer");
    } else {
        TEST_FAIL("Add big positive integer");
        printf("%s\n", err.error_msg);
    }
    
    /* Reset result for the next test */
    result.array_info.count = 0;
    bigint_normalize(&result);
    da_free(expected_result.items, &expected_result.array_info);

    /* Test adding a big negative integer */
    expected_result = bigint_from_cstr("-10000000000", &global_std_allocator, NULL, &err); /* 0 + (-10000000000) = -10000000000 */
    bigint_add_in(&result, &big_negative);
    if (bigint_equals(&expected_result, &result)) {
        TEST_OK("Add big negative integer");
    } else {
        TEST_FAIL("Add big negative integer");
        printf("%s\n", err.error_msg);
    }

    da_free(big_negative.items, &big_negative.array_info);
    da_free(big_positive.items, &big_positive.array_info);
    da_free(small_negative.items, &small_negative.array_info);
    da_free(small_positive.items, &small_positive.array_info);
    da_free(result.items, &result.array_info);
    da_free(expected_result.items, &expected_result.array_info);
}

static void test_subtraction() {

    error_t err;
    const allocator_t *allocator = &arena_allocator;
    arena_context_t test_ctx = arena_init(4096, ARENA_FAST_ALLOC | ARENA_VIRTUAL_BACKEND | ARENA_GROWABLE, NULL, NULL);

    bigint_t num1, num2, result, expected_result;

    /* minuend|subtrahend|result */
    const char *calculations[] = {
        "-869778674252","241477624685", "-1111256298937",
        "368181377157","999150299902", "-630968922745",
        "-135562838317","-167903500320", "32340662003",
        "0","3912048938109", "-3912048938109",
        "3912048938109","0", "3912048938109",
        "715773801670","599845648847", "115928152823",
        "268013970004","-352100064784", "620114034788",
        "73162772897","251281108032", "-178118335135",
        "661231055445","-304307266301", "965538321746",
        "-458723363939","210762841458", "-669486205397",
        "459730131016","-160152645103", "619882776119",
        "-147278430243","221598153934", "-368876584177",
    };

    for (size_t i = 0; i < sizeof (calculations) / sizeof (char*); i += 3) {
        
        num1 = bigint_from_cstr(calculations[i], allocator, &test_ctx, &err);
        num2 = bigint_from_cstr(calculations[i+1], allocator, &test_ctx, &err);
        expected_result = bigint_from_cstr(calculations[i+2], allocator, &test_ctx, &err);
        result = bigint_with_capacity(num1.array_info.count, allocator, &test_ctx);

        bigint_copy(&result, &num1);
        bigint_sub_in(&result, &num2);

        if (bigint_equals(&expected_result, &result)) {
            TEST_OK("Random subtraction");
        } else {
            TEST_FAIL("Random subtraction");
            string_builder_t num1_sb, num2_sb, result_sb, exp_result_sb;
            string_t num1_str, num2_str, result_str, exp_result_str;

            num1_sb = bigint_to_sb(&num1, allocator, &test_ctx, allocator, &test_ctx);
            num2_sb = bigint_to_sb(&num2, allocator, &test_ctx, allocator, &test_ctx);
            result_sb = bigint_to_sb(&result, allocator, &test_ctx, allocator, &test_ctx);
            exp_result_sb = bigint_to_sb(&expected_result, allocator, &test_ctx, allocator, &test_ctx);

            num1_str = sb_build(&num1_sb);
            num2_str = sb_build(&num2_sb);
            result_str = sb_build(&result_sb);
            exp_result_str = sb_build(&exp_result_sb);

            fprintf(stderr, "%.*s - %.*s = %.*s\n %.*s\n",
                    (int)num1_str.count, num1_str.chars,
                    (int)num2_str.count, num2_str.chars,
                    (int)exp_result_str.count, exp_result_str.chars,
                    (int)result_str.count, result_str.chars);
        }

        arena_reset(&test_ctx);
    }

    /* Clean up */
    allocator->free_all(&test_ctx);
}


static void test_multiplication() {

    error_t err = {0};
    arena_context_t test_ctx = arena_init(4096, ARENA_FAST_ALLOC | ARENA_VIRTUAL_BACKEND | ARENA_GROWABLE, NULL, NULL);

    bigint_t num1 = bigint_from_cstr("4294967296", &arena_allocator, &test_ctx, &err);
    bigint_t num2 = bigint_from_cstr("2", &arena_allocator, &test_ctx, &err);
    bigint_t expected_result = bigint_from_cstr("8589934592", &arena_allocator, &test_ctx, &err);

    bigint_mul_in(&num1, &num2);

    if (bigint_equals(&expected_result, &num1)) {
        TEST_OK("Multiply positive numbers");
    } else {
        TEST_FAIL("Multiply positive numbers");
    }

    arena_reset(&test_ctx);

    num1 = bigint_from_cstr("10", &arena_allocator, &test_ctx, &err);
    num2 = bigint_from_cstr("100000000000", &arena_allocator, &test_ctx, &err);
    expected_result = bigint_from_cstr("1000000000000", &arena_allocator, &test_ctx, &err);

    bigint_mul_in(&num1, &num2);

    if (bigint_equals(&expected_result, &num1)) {
        TEST_OK("Multiply positive numbers with (binary) carry");
    } else {
        TEST_FAIL("Multiply positive numbers with (binary) carry");
    }

    arena_reset(&test_ctx);

    num1 = bigint_from_cstr("-10", &arena_allocator, &test_ctx, &err);
    num2 = bigint_from_cstr("-100000000000", &arena_allocator, &test_ctx, &err);
    expected_result = bigint_from_cstr("1000000000000", &arena_allocator, &test_ctx, &err);

    bigint_mul_in(&num1, &num2);

    if (bigint_equals(&expected_result, &num1)) {
        TEST_OK("Multiply negative numbers");
    } else {
        TEST_FAIL("Multiply negative numbers");
    }

    arena_reset(&test_ctx);

    num1 = bigint_from_cstr("-10", &arena_allocator, &test_ctx, &err);
    num2 = bigint_from_cstr("100000000000", &arena_allocator, &test_ctx, &err);
    expected_result = bigint_from_cstr("-1000000000000", &arena_allocator, &test_ctx, &err);

    bigint_mul_in(&num1, &num2);

    if (bigint_equals(&expected_result, &num1)) {
        TEST_OK("Multiply positive and negative numbers");
    } else {
        TEST_FAIL("Multiply positive and negative numbers");
    }

    arena_reset(&test_ctx);

    arena_destroy(&test_ctx);
}

static void test_division() {

    error_t err = {0};
    const allocator_t *allocator = &arena_allocator;
    arena_context_t test_ctx = arena_init(4096, ARENA_FAST_ALLOC | ARENA_VIRTUAL_BACKEND | ARENA_GROWABLE, NULL, NULL);

    bigint_t num1, num2;
    divmod_t expected, actual;

    /* dividend|divisor|quotient|remainder */
    const char *positive_by_positive[] = {
        "57197825656076", "789372", "72459912", "812",
        "199199199199199199199199199199","2727272727","73039706380343677011","202020202",
        "31385508697394426750658929512418883039131020459666374144060","1701411834921604967429270619762735448065","18446744082299486211","12345",
        "1979026408232494142227078758414788119204702576506353221271275415293620212723","4134192603734347520","478697196266296943814151161389731428759230825882002427639","4078565605941107443",
        "28706531079966129691235470577159592992911547996822603662045522967612776730454","9744546976443352686","2945907198083382077818459795887643922629261622178601352319","6300848994915751620",
        "63401465876617411051179760504581181676711997943381852778690159322046768342800", "8389561948681566786","7557184304072163605870742328257457322086085843851197036870", "1496072614558942980",
        "84348255","720401", "117", "61338",
        "86726178","246841", "351", "84987",
        "90238261","632418", "142", "434905",
        "8589934592"            ,"2"          ,"4294967296"      , "0",
        "12398419017212341"     ,"6"          ,"2066403169535390", "1",
        "1931782798091231477871","12903848971","149705936766"    ,"691310085",
        "17179869185"           ,"4"          ,"4294967296"      ,"1",
    };

    const char *negative_by_negative[] = {
        "-10", "-2", "5", "0",
        "-34742785","-970221", "35", "-785050",
        "-72980724","-967316", "75", "-432024",
        "-55442016","-464269", "119", "-194005",
        "-3938014315338810647927150882072151618427334538799386124378169010605325432781","-17879370809558263552","220254636322747933915236770907331891881430757454949242944","-9715744563697055693",
        "-48812829131006068995559606950859946443360252086525484235746425079681975920493","-13485463270289000159","3619662754823549038066470760697451318700587562723475851098","-11849915221993595911",
    };

    const char *negative_by_positive[] = {
        "-50", "7", "-8", "6",
        "-25011636","47531", "-527", "37201",
        "-95659540","823236", "-117", "659072",
        "-63401465876617411051179760504581181676711997943381852778690159322046768342800", "8389561948681566786","-7557184304072163605870742328257457322086085843851197036871", "6893489334122623806",
        "-39922612229048928892405681325606272653163571665243720064243510659666219811636","760874683782934478","-52469365954667798097438967040487667082044514777113979378469","646531845171142546",
    };

    const char *positive_by_negative[] = {
        "3953740","-713259", "-6", "-325814",
        "85268996","-301197", "-284", "-270952",
        "17179869185","-4","-4294967297","-3",
        "50"         ,"-7","-8"         ,"-6",
        "75666770409524863562481996169183033502372244896568279867801144104043718811969", "-13806448924332836533", "-5480538176342203678795437846734246489090059731768585946780", "-5874265515058901771",
        "47872221034391769985875292426062864197168596810225194061600955266145086356223","-5018556480745580414","-9539041997048451491085444790148194359099005883583418909322","-2061514362638863085",
        "63121802689723392023318459150964876062204299147426604171942038636939647636749","-5040419222445851725","-12523125538572500686244378361439225579995605163161453818757","-488948714998169076",
        "21245970522266790799675693778586022286609194777403842214145406101150949077854","-12741426539964319476","-1667471884378677041783234453264258900347316764849463885722","-598108167413843818",
    };

    for (size_t i = 0; i < sizeof (positive_by_positive) / sizeof (char*); i += 4) {
        
        num1 = bigint_from_cstr(positive_by_positive[i], allocator, &test_ctx, &err);
        num2 = bigint_from_cstr(positive_by_positive[i+1], allocator, &test_ctx, &err);
        expected.quotient = bigint_from_cstr(positive_by_positive[i+2], allocator, &test_ctx, &err);
        expected.remainder = bigint_from_cstr(positive_by_positive[i+3], allocator, &test_ctx, &err);

        actual = bigint_divmod(&num1, &num2, allocator, &test_ctx, &err);

        if (bigint_equals(&expected.quotient, &actual.quotient) && bigint_equals(&expected.remainder, &actual.remainder)) {
            TEST_OK("Divide positive numbers");
        } else {
            TEST_FAIL("Divide positive numbers");
            string_builder_t num1_sb, num2_sb, exp_q_sb, exp_r_sb, act_q_sb, act_r_sb;
            string_t num1_str, num2_str, exp_q_str, exp_r_str, act_q_str, act_r_str;

            num1_sb = bigint_to_sb(&num1, allocator, &test_ctx, allocator, &test_ctx);
            num2_sb = bigint_to_sb(&num2, allocator, &test_ctx, allocator, &test_ctx);
            exp_q_sb = bigint_to_sb(&expected.quotient, allocator, &test_ctx, allocator, &test_ctx);
            exp_r_sb = bigint_to_sb(&expected.remainder, allocator, &test_ctx, allocator, &test_ctx);
            act_q_sb = bigint_to_sb(&actual.quotient, allocator, &test_ctx, allocator, &test_ctx);
            act_r_sb = bigint_to_sb(&actual.remainder, allocator, &test_ctx, allocator, &test_ctx);

            num1_str = sb_build(&num1_sb);
            num2_str = sb_build(&num2_sb);
            exp_q_str = sb_build(&exp_q_sb);
            exp_r_str = sb_build(&exp_r_sb);
            act_q_str = sb_build(&act_q_sb);
            act_r_str = sb_build(&act_r_sb);

            fprintf(stderr, "%.*s / %.*s = (%.*s,%.*s),\n got (%.*s, %.*s)\n",
                    (int)num1_str.count, num1_str.chars,
                    (int)num2_str.count, num2_str.chars,
                    (int)exp_q_str.count, exp_q_str.chars,
                    (int)exp_r_str.count, exp_r_str.chars,
                    (int)act_q_str.count, act_q_str.chars,
                    (int)act_r_str.count, act_r_str.chars
                    );
        }

        arena_reset(&test_ctx);
    }


    for (size_t i = 0; i < sizeof (negative_by_negative) / sizeof (char*); i += 4) {
        
        num1 = bigint_from_cstr(negative_by_negative[i], allocator, &test_ctx, &err);
        num2 = bigint_from_cstr(negative_by_negative[i+1], allocator, &test_ctx, &err);
        expected.quotient = bigint_from_cstr(negative_by_negative[i+2], allocator, &test_ctx, &err);
        expected.remainder = bigint_from_cstr(negative_by_negative[i+3], allocator, &test_ctx, &err);

        actual = bigint_divmod(&num1, &num2, allocator, &test_ctx, &err);

        if (bigint_equals(&expected.quotient, &actual.quotient) && bigint_equals(&expected.remainder, &actual.remainder)) {
            TEST_OK("Divide negative numbers");
        } else {
            TEST_FAIL("Divide negative numbers");
            string_builder_t num1_sb, num2_sb, exp_q_sb, exp_r_sb, act_q_sb, act_r_sb;
            string_t num1_str, num2_str, exp_q_str, exp_r_str, act_q_str, act_r_str;

            num1_sb = bigint_to_sb(&num1, allocator, &test_ctx, allocator, &test_ctx);
            num2_sb = bigint_to_sb(&num2, allocator, &test_ctx, allocator, &test_ctx);
            exp_q_sb = bigint_to_sb(&expected.quotient, allocator, &test_ctx, allocator, &test_ctx);
            exp_r_sb = bigint_to_sb(&expected.remainder, allocator, &test_ctx, allocator, &test_ctx);
            act_q_sb = bigint_to_sb(&actual.quotient, allocator, &test_ctx, allocator, &test_ctx);
            act_r_sb = bigint_to_sb(&actual.remainder, allocator, &test_ctx, allocator, &test_ctx);

            num1_str = sb_build(&num1_sb);
            num2_str = sb_build(&num2_sb);
            exp_q_str = sb_build(&exp_q_sb);
            exp_r_str = sb_build(&exp_r_sb);
            act_q_str = sb_build(&act_q_sb);
            act_r_str = sb_build(&act_r_sb);

            fprintf(stderr, "%.*s / %.*s = (%.*s,%.*s),\n got (%.*s, %.*s)\n",
                    (int)num1_str.count, num1_str.chars,
                    (int)num2_str.count, num2_str.chars,
                    (int)exp_q_str.count, exp_q_str.chars,
                    (int)exp_r_str.count, exp_r_str.chars,
                    (int)act_q_str.count, act_q_str.chars,
                    (int)act_r_str.count, act_r_str.chars
                    );
        }

        arena_reset(&test_ctx);
    }


    for (size_t i = 0; i < sizeof (negative_by_positive) / sizeof (char*); i += 4) {
        
        num1 = bigint_from_cstr(negative_by_positive[i], allocator, &test_ctx, &err);
        num2 = bigint_from_cstr(negative_by_positive[i+1], allocator, &test_ctx, &err);
        expected.quotient = bigint_from_cstr(negative_by_positive[i+2], allocator, &test_ctx, &err);
        expected.remainder = bigint_from_cstr(negative_by_positive[i+3], allocator, &test_ctx, &err);

        actual = bigint_divmod(&num1, &num2, allocator, &test_ctx, &err);

        if (bigint_equals(&expected.quotient, &actual.quotient) && bigint_equals(&expected.remainder, &actual.remainder)) {
            TEST_OK("Divide negative by positive");
        } else {
            TEST_FAIL("Divide negative by positive");
            string_builder_t num1_sb, num2_sb, exp_q_sb, exp_r_sb, act_q_sb, act_r_sb;
            string_t num1_str, num2_str, exp_q_str, exp_r_str, act_q_str, act_r_str;

            num1_sb = bigint_to_sb(&num1, allocator, &test_ctx, allocator, &test_ctx);
            num2_sb = bigint_to_sb(&num2, allocator, &test_ctx, allocator, &test_ctx);
            exp_q_sb = bigint_to_sb(&expected.quotient, allocator, &test_ctx, allocator, &test_ctx);
            exp_r_sb = bigint_to_sb(&expected.remainder, allocator, &test_ctx, allocator, &test_ctx);
            act_q_sb = bigint_to_sb(&actual.quotient, allocator, &test_ctx, allocator, &test_ctx);
            act_r_sb = bigint_to_sb(&actual.remainder, allocator, &test_ctx, allocator, &test_ctx);

            num1_str = sb_build(&num1_sb);
            num2_str = sb_build(&num2_sb);
            exp_q_str = sb_build(&exp_q_sb);
            exp_r_str = sb_build(&exp_r_sb);
            act_q_str = sb_build(&act_q_sb);
            act_r_str = sb_build(&act_r_sb);

            fprintf(stderr, "%.*s / %.*s = (%.*s,%.*s),\n got (%.*s, %.*s)\n",
                    (int)num1_str.count, num1_str.chars,
                    (int)num2_str.count, num2_str.chars,
                    (int)exp_q_str.count, exp_q_str.chars,
                    (int)exp_r_str.count, exp_r_str.chars,
                    (int)act_q_str.count, act_q_str.chars,
                    (int)act_r_str.count, act_r_str.chars
                    );
        }

        arena_reset(&test_ctx);
    }

    for (size_t i = 0; i < sizeof (positive_by_negative) / sizeof (char*); i += 4) {
        
        num1 = bigint_from_cstr(positive_by_negative[i], allocator, &test_ctx, &err);
        num2 = bigint_from_cstr(positive_by_negative[i+1], allocator, &test_ctx, &err);
        expected.quotient = bigint_from_cstr(positive_by_negative[i+2], allocator, &test_ctx, &err);
        expected.remainder = bigint_from_cstr(positive_by_negative[i+3], allocator, &test_ctx, &err);

        actual = bigint_divmod(&num1, &num2, allocator, &test_ctx, &err);

        if (bigint_equals(&expected.quotient, &actual.quotient) && bigint_equals(&expected.remainder, &actual.remainder)) {
            TEST_OK("Divide positive by negative");
        } else {
            TEST_FAIL("Divide positive by negative");
            string_builder_t num1_sb, num2_sb, exp_q_sb, exp_r_sb, act_q_sb, act_r_sb;
            string_t num1_str, num2_str, exp_q_str, exp_r_str, act_q_str, act_r_str;

            num1_sb = bigint_to_sb(&num1, allocator, &test_ctx, allocator, &test_ctx);
            num2_sb = bigint_to_sb(&num2, allocator, &test_ctx, allocator, &test_ctx);
            exp_q_sb = bigint_to_sb(&expected.quotient, allocator, &test_ctx, allocator, &test_ctx);
            exp_r_sb = bigint_to_sb(&expected.remainder, allocator, &test_ctx, allocator, &test_ctx);
            act_q_sb = bigint_to_sb(&actual.quotient, allocator, &test_ctx, allocator, &test_ctx);
            act_r_sb = bigint_to_sb(&actual.remainder, allocator, &test_ctx, allocator, &test_ctx);

            num1_str = sb_build(&num1_sb);
            num2_str = sb_build(&num2_sb);
            exp_q_str = sb_build(&exp_q_sb);
            exp_r_str = sb_build(&exp_r_sb);
            act_q_str = sb_build(&act_q_sb);
            act_r_str = sb_build(&act_r_sb);

            fprintf(stderr, "%.*s / %.*s = (%.*s,%.*s),\n got (%.*s, %.*s)\n",
                    (int)num1_str.count, num1_str.chars,
                    (int)num2_str.count, num2_str.chars,
                    (int)exp_q_str.count, exp_q_str.chars,
                    (int)exp_r_str.count, exp_r_str.chars,
                    (int)act_q_str.count, act_q_str.chars,
                    (int)act_r_str.count, act_r_str.chars
                    );
        }

        arena_reset(&test_ctx);
    }


    arena_destroy(&test_ctx);
}

int main(void)
{
    UNUSED(arena_allocator);

    printf("\n--- Start tests: Bigint ---\n");
    test_encoding_decoding();
    test_addition();
    test_subtraction();
    test_multiplication();
    test_division();

    printf("--- Summary: Bigint ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n");

    return EXIT_SUCCESS;
}
