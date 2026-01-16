/*
 * tests/host/test_gdt.c - Host-side tests for GDT entry encoding
 *
 * Tests the ACTUAL kernel gdt_set_gate implementation to verify
 * GDT descriptors are encoded correctly per Intel SDM Vol 3, Section 3.4.5.
 *
 * Uses Unity test framework.
 */

#include "unity/unity.h"
#include <string.h>
#include <gdt.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/*
 * Test structure sizes match Intel spec
 */
void test_structure_sizes(void)
{
    TEST_ASSERT_EQUAL(8, sizeof(struct gdt_entry));
    TEST_ASSERT_EQUAL(6, sizeof(struct gdt_ptr));
}

/*
 * Test null descriptor
 */
void test_null_descriptor(void)
{
    struct gdt_entry entry;
    memset(&entry, 0xFF, sizeof(entry));

    gdt_set_gate(&entry, 0, 0, 0, 0);

    TEST_ASSERT_EQUAL_HEX16(0, entry.limit_low);
    TEST_ASSERT_EQUAL_HEX16(0, entry.base_low);
    TEST_ASSERT_EQUAL_HEX8(0, entry.base_middle);
    TEST_ASSERT_EQUAL_HEX8(0, entry.access);
    TEST_ASSERT_EQUAL_HEX8(0, entry.granularity);
    TEST_ASSERT_EQUAL_HEX8(0, entry.base_high);
}

/*
 * Test kernel code segment (selector 0x08)
 * Base=0, Limit=0xFFFFF, Access=0x9A, Flags=0xC
 */
void test_kernel_code_segment(void)
{
    struct gdt_entry entry;

    gdt_set_gate(&entry, 0, 0xFFFFF, 0x9A, 0xC);

    TEST_ASSERT_EQUAL_HEX16(0, entry.base_low);
    TEST_ASSERT_EQUAL_HEX8(0, entry.base_middle);
    TEST_ASSERT_EQUAL_HEX8(0, entry.base_high);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, entry.limit_low);
    TEST_ASSERT_EQUAL_HEX8(0xCF, entry.granularity);
    TEST_ASSERT_EQUAL_HEX8(0x9A, entry.access);
}

/*
 * Test kernel data segment (selector 0x10)
 */
void test_kernel_data_segment(void)
{
    struct gdt_entry entry;

    gdt_set_gate(&entry, 0, 0xFFFFF, 0x92, 0xC);

    TEST_ASSERT_EQUAL_HEX16(0xFFFF, entry.limit_low);
    TEST_ASSERT_EQUAL_HEX8(0xCF, entry.granularity);
    TEST_ASSERT_EQUAL_HEX8(0x92, entry.access);
}

/*
 * Test user code segment (selector 0x18)
 */
void test_user_code_segment(void)
{
    struct gdt_entry entry;

    gdt_set_gate(&entry, 0, 0xFFFFF, 0xFA, 0xC);

    TEST_ASSERT_EQUAL_HEX8(0xFA, entry.access);
    TEST_ASSERT_EQUAL_HEX8(0xCF, entry.granularity);
}

/*
 * Test user data segment (selector 0x20)
 */
void test_user_data_segment(void)
{
    struct gdt_entry entry;

    gdt_set_gate(&entry, 0, 0xFFFFF, 0xF2, 0xC);

    TEST_ASSERT_EQUAL_HEX8(0xF2, entry.access);
}

/*
 * Test non-zero base address encoding
 * Base=0x12345678 should be split across three fields
 */
void test_base_encoding(void)
{
    struct gdt_entry entry;

    gdt_set_gate(&entry, 0x12345678, 0, 0, 0);

    TEST_ASSERT_EQUAL_HEX16(0x5678, entry.base_low);
    TEST_ASSERT_EQUAL_HEX8(0x34, entry.base_middle);
    TEST_ASSERT_EQUAL_HEX8(0x12, entry.base_high);
}

/*
 * Test limit encoding
 */
void test_limit_encoding(void)
{
    struct gdt_entry entry;

    gdt_set_gate(&entry, 0, 0x12345, 0, 0);

    TEST_ASSERT_EQUAL_HEX16(0x2345, entry.limit_low);
    TEST_ASSERT_EQUAL_HEX8(0x01, entry.granularity & 0x0F);
}

/*
 * Test raw bytes match expected encoding
 */
void test_raw_bytes(void)
{
    struct gdt_entry entry;
    uint8_t *bytes = (uint8_t *)&entry;

    gdt_set_gate(&entry, 0, 0xFFFFF, 0x9A, 0xC);

    TEST_ASSERT_EQUAL_HEX8(0xFF, bytes[0]);  /* limit_low LSB */
    TEST_ASSERT_EQUAL_HEX8(0xFF, bytes[1]);  /* limit_low MSB */
    TEST_ASSERT_EQUAL_HEX8(0x00, bytes[2]);  /* base_low LSB */
    TEST_ASSERT_EQUAL_HEX8(0x00, bytes[3]);  /* base_low MSB */
    TEST_ASSERT_EQUAL_HEX8(0x00, bytes[4]);  /* base_middle */
    TEST_ASSERT_EQUAL_HEX8(0x9A, bytes[5]);  /* access */
    TEST_ASSERT_EQUAL_HEX8(0xCF, bytes[6]);  /* granularity */
    TEST_ASSERT_EQUAL_HEX8(0x00, bytes[7]);  /* base_high */
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_structure_sizes);
    RUN_TEST(test_null_descriptor);
    RUN_TEST(test_kernel_code_segment);
    RUN_TEST(test_kernel_data_segment);
    RUN_TEST(test_user_code_segment);
    RUN_TEST(test_user_data_segment);
    RUN_TEST(test_base_encoding);
    RUN_TEST(test_limit_encoding);
    RUN_TEST(test_raw_bytes);

    return UNITY_END();
}
