/**
 * Configuration for MAX32690-WLP.
 *
 * This file was generated using Analog Devices CodeFusion Studio.
 * https://github.com/analogdevicesinc/codefusion-studio
 *
 * Generated at: 2026-09-23T10:41:19.335Z *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Analog Devices, Inc.
 */


MEMORY {
    ROM         (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00020000 /* 128kB ROM */
    PAL_NVM_DB  (r)   : ORIGIN = 0x00000000, LENGTH = 0x00000000 /* Section not defined. */
    RISCV_FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00000000 /* Section not defined. */
    MAILBOX_0   (rw)  : ORIGIN = 0x00000000, LENGTH = 0x00000000 /* Section not defined. */
    MAILBOX_1   (rw)  : ORIGIN = 0x00000000, LENGTH = 0x00000000 /* Section not defined. */

    /* Note that CS0 address mapping may be reversed using MXC_HPC->mbr0.
     * The following mapping is selected for simplicity.
     */
    HPB_CS0     (rwx) : ORIGIN = 0x60000000, LENGTH = 0x10000000 /* External Hyperbus/Xccelabus chip select 0 */
    /* Note that CS1 address mapping may be reversed using MXC_HPC->mbr1.
     * The following mapping is selected for simplicity.
     */
    HPB_CS1     (rwx) : ORIGIN = 0x70000000, LENGTH = 0x10000000 /* External Hyperbus/Xccelabus chip select 1 */

    FLASH       (rx)  : ORIGIN = 0x10000000, LENGTH = 0x00300000 /* FLASH */
    SRAM        (rw)  : ORIGIN = 0x20000000, LENGTH = 0x00100000 /* SRAM  */
}

INCLUDE max32690.sects.ld
