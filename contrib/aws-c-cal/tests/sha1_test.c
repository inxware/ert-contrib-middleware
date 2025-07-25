/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */
#include <aws/cal/hash.h>
#include <aws/common/byte_buf.h>
#include <aws/testing/aws_test_harness.h>

#include <test_case_helper.h>
/*
 * these are the NIST test vectors, as compiled here:
 * https://www.di-mgt.com.au/sha_testvectors.html
 */

static int s_sha1_nist_test_case_1_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    struct aws_byte_cursor input = aws_byte_cursor_from_c_str("abc");
    uint8_t expected[] = {
        0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e,
        0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d,
    };
    struct aws_byte_cursor expected_buf = aws_byte_cursor_from_array(expected, sizeof(expected));

    return s_verify_hash_test_case(allocator, &input, &expected_buf, aws_sha1_new);
}

AWS_TEST_CASE(sha1_nist_test_case_1, s_sha1_nist_test_case_1_fn)

static int s_sha1_nist_test_case_2_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    struct aws_byte_cursor input = aws_byte_cursor_from_c_str("");
    uint8_t expected[] = {
        0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
        0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09,
    };
    struct aws_byte_cursor expected_buf = aws_byte_cursor_from_array(expected, sizeof(expected));

    return s_verify_hash_test_case(allocator, &input, &expected_buf, aws_sha1_new);
}

AWS_TEST_CASE(sha1_nist_test_case_2, s_sha1_nist_test_case_2_fn)

static int s_sha1_nist_test_case_3_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    struct aws_byte_cursor input =
        aws_byte_cursor_from_c_str("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
    uint8_t expected[] = {
        0x84, 0x98, 0x3e, 0x44, 0x1c, 0x3b, 0xd2, 0x6e, 0xba, 0xae,
        0x4a, 0xa1, 0xf9, 0x51, 0x29, 0xe5, 0xe5, 0x46, 0x70, 0xf1,
    };
    struct aws_byte_cursor expected_buf = aws_byte_cursor_from_array(expected, sizeof(expected));

    return s_verify_hash_test_case(allocator, &input, &expected_buf, aws_sha1_new);
}

AWS_TEST_CASE(sha1_nist_test_case_3, s_sha1_nist_test_case_3_fn)

static int s_sha1_nist_test_case_4_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    struct aws_byte_cursor input = aws_byte_cursor_from_c_str("abcdefghbcdefghicdefghijdefghijkefghijklfghij"
                                                              "klmghijklmnhijklmnoijklmnopjklmnopqklm"
                                                              "nopqrlmnopqrsmnopqrstnopqrstu");
    uint8_t expected[] = {
        0xa4, 0x9b, 0x24, 0x46, 0xa0, 0x2c, 0x64, 0x5b, 0xf4, 0x19,
        0xf9, 0x95, 0xb6, 0x70, 0x91, 0x25, 0x3a, 0x04, 0xa2, 0x59,
    };
    struct aws_byte_cursor expected_buf = aws_byte_cursor_from_array(expected, sizeof(expected));

    return s_verify_hash_test_case(allocator, &input, &expected_buf, aws_sha1_new);
}

AWS_TEST_CASE(sha1_nist_test_case_4, s_sha1_nist_test_case_4_fn)

static int s_sha1_nist_test_case_5_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    aws_cal_library_init(allocator);

    struct aws_hash *hash = aws_sha1_new(allocator);
    ASSERT_NOT_NULL(hash);
    struct aws_byte_cursor input = aws_byte_cursor_from_c_str("a");

    for (size_t i = 0; i < 1000000; ++i) {
        ASSERT_SUCCESS(aws_hash_update(hash, &input));
    }

    uint8_t output[AWS_SHA1_LEN] = {0};
    struct aws_byte_buf output_buf = aws_byte_buf_from_array(output, sizeof(output));
    output_buf.len = 0;
    ASSERT_SUCCESS(aws_hash_finalize(hash, &output_buf, 0));

    uint8_t expected[] = {
        0x34, 0xaa, 0x97, 0x3c, 0xd4, 0xc4, 0xda, 0xa4, 0xf6, 0x1e,
        0xeb, 0x2b, 0xdb, 0xad, 0x27, 0x31, 0x65, 0x34, 0x01, 0x6f,
    };
    struct aws_byte_cursor expected_buf = aws_byte_cursor_from_array(expected, sizeof(expected));
    ASSERT_BIN_ARRAYS_EQUALS(expected_buf.ptr, expected_buf.len, output_buf.buffer, output_buf.len);

    aws_hash_destroy(hash);

    aws_cal_library_clean_up();

    return AWS_OP_SUCCESS;
}

AWS_TEST_CASE(sha1_nist_test_case_5, s_sha1_nist_test_case_5_fn)

static int s_sha1_nist_test_case_5_truncated_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    aws_cal_library_init(allocator);

    struct aws_hash *hash = aws_sha1_new(allocator);
    ASSERT_NOT_NULL(hash);
    struct aws_byte_cursor input = aws_byte_cursor_from_c_str("a");

    for (size_t i = 0; i < 1000000; ++i) {
        ASSERT_SUCCESS(aws_hash_update(hash, &input));
    }

    uint8_t expected[] = {
        0x34, 0xaa, 0x97, 0x3c, 0xd4, 0xc4, 0xda, 0xa4, 0xf6, 0x1e, 0xeb, 0x2b, 0xdb, 0xad, 0x27, 0x31};
    struct aws_byte_cursor expected_buf = aws_byte_cursor_from_array(expected, sizeof(expected));
    uint8_t output[AWS_SHA1_LEN] = {0};
    struct aws_byte_buf output_buf = aws_byte_buf_from_array(output, expected_buf.len);
    output_buf.len = 0;
    ASSERT_SUCCESS(aws_hash_finalize(hash, &output_buf, 16));

    ASSERT_BIN_ARRAYS_EQUALS(expected_buf.ptr, expected_buf.len, output_buf.buffer, output_buf.len);

    aws_hash_destroy(hash);

    aws_cal_library_clean_up();

    return AWS_OP_SUCCESS;
}

AWS_TEST_CASE(sha1_nist_test_case_5_truncated, s_sha1_nist_test_case_5_truncated_fn)

static int s_sha1_nist_test_case_6_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    aws_cal_library_init(allocator);

    struct aws_hash *hash = aws_sha1_new(allocator);
    ASSERT_NOT_NULL(hash);
    struct aws_byte_cursor input =
        aws_byte_cursor_from_c_str("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno");

    for (size_t i = 0; i < 16777216; ++i) {
        ASSERT_SUCCESS(aws_hash_update(hash, &input));
    }

    uint8_t output[AWS_SHA1_LEN] = {0};
    struct aws_byte_buf output_buf = aws_byte_buf_from_array(output, sizeof(output));
    output_buf.len = 0;
    ASSERT_SUCCESS(aws_hash_finalize(hash, &output_buf, 0));

    uint8_t expected[] = {
        0x77, 0x89, 0xf0, 0xc9, 0xef, 0x7b, 0xfc, 0x40, 0xd9, 0x33,
        0x11, 0x14, 0x3d, 0xfb, 0xe6, 0x9e, 0x20, 0x17, 0xf5, 0x92,
    };

    struct aws_byte_cursor expected_buf = aws_byte_cursor_from_array(expected, sizeof(expected));
    ASSERT_BIN_ARRAYS_EQUALS(expected_buf.ptr, expected_buf.len, output_buf.buffer, output_buf.len);

    aws_hash_destroy(hash);

    aws_cal_library_clean_up();

    return AWS_OP_SUCCESS;
}

AWS_TEST_CASE(sha1_nist_test_case_6, s_sha1_nist_test_case_6_fn)

static int s_sha1_test_invalid_buffer_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    aws_cal_library_init(allocator);

    struct aws_byte_cursor input = aws_byte_cursor_from_c_str("abcdefghbcdefghicdefghijdefghijkefghijklfghij"
                                                              "klmghijklmnhijklmnoijklmnopjklmnopqklm"
                                                              "nopqrlmnopqrsmnopqrstnopqrstu");
    uint8_t output[AWS_SHA1_LEN] = {0};
    struct aws_byte_buf output_buf = aws_byte_buf_from_array(output, sizeof(output));
    output_buf.len = 1;

    ASSERT_ERROR(AWS_ERROR_SHORT_BUFFER, aws_sha1_compute(allocator, &input, &output_buf, 0));

    aws_cal_library_clean_up();

    return AWS_OP_SUCCESS;
}

AWS_TEST_CASE(sha1_test_invalid_buffer, s_sha1_test_invalid_buffer_fn)

static int s_sha1_test_oneshot_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    aws_cal_library_init(allocator);

    struct aws_byte_cursor input = aws_byte_cursor_from_c_str("abcdefghbcdefghicdefghijdefghijkefghijklfghij"
                                                              "klmghijklmnhijklmnoijklmnopjklmnopqklm"
                                                              "nopqrlmnopqrsmnopqrstnopqrstu");
    uint8_t expected[] = {
        0xa4, 0x9b, 0x24, 0x46, 0xa0, 0x2c, 0x64, 0x5b, 0xf4, 0x19,
        0xf9, 0x95, 0xb6, 0x70, 0x91, 0x25, 0x3a, 0x04, 0xa2, 0x59,
    };

    uint8_t output[AWS_SHA1_LEN] = {0};
    struct aws_byte_buf output_buf = aws_byte_buf_from_array(output, sizeof(output));
    output_buf.len = 0;

    ASSERT_SUCCESS(aws_sha1_compute(allocator, &input, &output_buf, 0));
    ASSERT_BIN_ARRAYS_EQUALS(expected, sizeof(expected), output_buf.buffer, output_buf.len);

    aws_cal_library_clean_up();

    return AWS_OP_SUCCESS;
}

AWS_TEST_CASE(sha1_test_oneshot, s_sha1_test_oneshot_fn)

static int s_sha1_test_invalid_state_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    aws_cal_library_init(allocator);

    struct aws_byte_cursor input = aws_byte_cursor_from_c_str("abcdefghbcdefghicdefghijdefghijkefghijklfghij"
                                                              "klmghijklmnhijklmnoijklmnopjklmnopqklm"
                                                              "nopqrlmnopqrsmnopqrstnopqrstu");

    struct aws_hash *hash = aws_sha1_new(allocator);
    ASSERT_NOT_NULL(hash);

    uint8_t output[AWS_SHA1_LEN] = {0};
    struct aws_byte_buf output_buf = aws_byte_buf_from_array(output, sizeof(output));
    output_buf.len = 0;

    ASSERT_SUCCESS(aws_hash_update(hash, &input));
    ASSERT_SUCCESS(aws_hash_finalize(hash, &output_buf, 0));
    ASSERT_ERROR(AWS_ERROR_INVALID_STATE, aws_hash_update(hash, &input));
    ASSERT_ERROR(AWS_ERROR_INVALID_STATE, aws_hash_finalize(hash, &output_buf, 0));

    aws_hash_destroy(hash);

    aws_cal_library_clean_up();

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(sha1_test_invalid_state, s_sha1_test_invalid_state_fn)

static int s_sha1_test_extra_buffer_space_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    aws_cal_library_init(allocator);

    struct aws_byte_cursor input = aws_byte_cursor_from_c_str("123456789012345678901234567890123456789012345"
                                                              "67890123456789012345678901234567890");

    struct aws_byte_buf digest_size_buf;
    struct aws_byte_buf super_size_buf;

    aws_byte_buf_init(&digest_size_buf, allocator, AWS_SHA1_LEN);
    aws_byte_buf_init(&super_size_buf, allocator, AWS_SHA1_LEN + 100);

    aws_sha1_compute(allocator, &input, &digest_size_buf, 0);
    aws_sha1_compute(allocator, &input, &super_size_buf, 0);

    ASSERT_TRUE(aws_byte_buf_eq(&digest_size_buf, &super_size_buf));
    ASSERT_TRUE(super_size_buf.len == AWS_SHA1_LEN);

    aws_byte_buf_clean_up(&digest_size_buf);
    aws_byte_buf_clean_up(&super_size_buf);

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(sha1_test_extra_buffer_space, s_sha1_test_extra_buffer_space_fn)
