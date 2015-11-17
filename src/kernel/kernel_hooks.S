
.section ".kernel_code"
    .globl SaveAndResetDataBATs_And_SRs_hook
SaveAndResetDataBATs_And_SRs_hook:
    # restore the kernel instructions that we replaced
    li r7, 1
    mtsr 0, r7
    mtsr 1, r7
    mtsr 2, r7
    mtsr 3, r7
    mtsr 4, r7
    mtsr 5, r7
    mtsr 6, r7
    mtsr 7, r7
    mtsr 8, r7
    mtsr 9, r7
    mtsr 10, r7
    mtsr 11, r7
    mtsr 12, r7
    mtsr 13, r7
    mtsr 14, r7
    mtsr 15, r4
    isync

    # call back the position in kernel after our patch
    lis r7, 0xFFF1
    ori r7, r7, 0xD78C
    mtctr r7
    bctr

.extern my_PrepareTitle
.section ".kernel_code"
    .globl my_PrepareTitle_hook
my_PrepareTitle_hook:
    # store all registers on stack to avoid issues with the call to C functions
    stwu r1, -0x90(r1)
    # registers for our own usage
    # only need r31 and rest is from tests before, just leaving it for later tests
    stw r28, 0x20(r1)
    stw r29, 0x24(r1)
    stw r30, 0x28(r1)
    stw r31, 0x2C(r1)

    stw r3, 0x30(r1)
    stw r4, 0x34(r1)
    stw r5, 0x38(r1)
    stw r6, 0x3C(r1)
    stw r7, 0x40(r1)
    stw r8, 0x44(r1)
    stw r9, 0x48(r1)
    stw r10, 0x4C(r1)
    stw r11, 0x50(r1)
    stw r12, 0x54(r1)
    stw r13, 0x58(r1)
    stw r14, 0x5C(r1)
    stw r15, 0x60(r1)
    stw r16, 0x64(r1)
    stw r17, 0x68(r1)
    stw r18, 0x6C(r1)
    stw r19, 0x70(r1)
    stw r20, 0x74(r1)
    stw r21, 0x78(r1)
    stw r22, 0x7C(r1)

    # the cos.xml/app.xml structure is at the location 0x68 of r11
    # there are actually many places that can be hooked for it
    # e.g. 0xFFF16130 and r27 points to this structure
    addi r3, r11, 0x68
    bl my_PrepareTitle

    # setup CTR to jump back to kernel code
    lis r31, 0xFFF1
    ori r31, r31, 0x855C
    mtspr CTR, r31

    # restore all original values of registers from stack
    lwz r28, 0x20(r1)
    lwz r29, 0x24(r1)
    lwz r30, 0x28(r1)
    lwz r31, 0x2C(r1)

    lwz r3, 0x30(r1)
    lwz r4, 0x34(r1)
    lwz r5, 0x38(r1)
    lwz r6, 0x3C(r1)
    lwz r7, 0x40(r1)
    lwz r8, 0x44(r1)
    lwz r9, 0x48(r1)
    lwz r10, 0x4C(r1)
    lwz r11, 0x50(r1)
    lwz r12, 0x54(r1)
    lwz r13, 0x58(r1)
    lwz r14, 0x5C(r1)
    lwz r15, 0x60(r1)
    lwz r16, 0x64(r1)
    lwz r17, 0x68(r1)
    lwz r18, 0x6C(r1)
    lwz r19, 0x70(r1)
    lwz r20, 0x74(r1)
    lwz r21, 0x78(r1)
    lwz r22, 0x7C(r1)

    # restore the stack
    addi r1, r1, 0x90

    # restore original instruction that we replaced in the kernel
    clrlwi    r7, r12, 0

    # jump back
    bctr
