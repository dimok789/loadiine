     .globl Syscall_0x36
Syscall_0x36:
    mflr r0
    stwu r1, -0x10(r1)
    stw r30, 0x4(r1)
    stw r31, 0x8(r1)
    mr r5, r0
    mr r6, r1
    li r0, 0x3600
    sc
    nop
    mr r0, r5
    mr r1, r6
    lwz r30, 0x04(r1)
    lwz r31, 0x08(r1)
    addi r1, r1, 0x10
    mtlr r0
    blr

    .globl KernelPatches
KernelPatches:
    # store the old DBAT0
    mfdbatu r30, 0
    mfdbatl r31, 0

    # setup DBAT0 for access to kernel code memory
    lis r3, 0xFFF0
    ori r3, r3, 0x0002
    mtdbatu 0, r3
    lis r3, 0xFFF0
    ori r3, r3, 0x0032
    mtdbatl 0, r3

    # memory barrier
    eieio
    isync

    # SaveAndResetDataBATs_And_SRs hook setup, but could be any BAT function though
    # just chosen because its simple
    lis r3, 0xFFF1
    ori r3, r3, 0xD744

    # make the kernel setup our section in IBAT4 and
    # jump to our function to restore the replaced instructions
    lis r4, 0x3ce0      #   lis r7, 0x2C80
    ori r4, r4, 0x2c80
    stw r4, 0x00(r3)
    lis r4, 0x60e7      #   ori r7, r7, 0x0013
    ori r4, r4, 0x0013
    stw r4, 0x04(r3)
    lis r4, 0x7cf1      #   mtspr 561, r7
    ori r4, r4, 0x8ba6
    stw r4, 0x08(r3)
    lis r4, 0x3ce0      #   lis r7, 0x0080
    ori r4, r4, 0x0080
    stw r4, 0x0C(r3)
    lis r4, 0x60e7      #   ori r7, r7, 0x00FF
    ori r4, r4, 0x00ff
    stw r4, 0x10(r3)
    lis r4, 0x7cf0      #   mtspr 560, r7
    ori r4, r4, 0x8ba6
    stw r4, 0x14(r3)
    lis r4, 0x7c00      #   eieio
    ori r4, r4, 0x06ac
    stw r4, 0x18(r3)
    lis r4, 0x4c00      #   isync
    ori r4, r4, 0x012c
    stw r4, 0x1C(r3)
    lis r4, 0x48ae      #   ba 0x00AE1000
    ori r4, r4, 0x1002
    stw r4, 0x20(r3)

    # flush and invalidate the replaced instructions
    lis r3, 0xFFF1
    ori r3, r3, 0xD740
    dcbf 0, r3
    icbi 0, r3

    # write "nop" to some positions
    lis r4, 0x6000

    # nop on IBATU 4 set/reset
    lis r3, 0xFFF1
    ori r3, r3, 0xD558
    stw r4, 0(r3)
    dcbf 0, r3
    icbi 0, r3

    lis r3, 0xFFF1
    ori r3, r3, 0xD73C
    stw r4, 0(r3)
    dcbf 0, r3
    icbi 0, r3

    # nop on IBATL 4 set/reset
    lis r3, 0xFFF1
    ori r3, r3, 0xD550
    stw r4, 0(r3)
    dcbf 0, r3
    icbi 0, r3

    # nop on remove of supervisor level from IBATU 4
    lis r3, 0xFFF0
    ori r3, r3, 0x6A14
    stw r4, 0(r3)
    dcbf 0, r3
    icbi 0, r3

    lis r3, 0xFFF0
    ori r3, r3, 0x6AA0
    stw r4, 0(r3)
    dcbf 0, r3
    icbi 0, r3
    isync

    # setup IBAT4 for core 1 at this position (not really required but wont hurt)
    # IBATL 4
    lis r3, 0x2C80
    ori r3, r3, 0x0013
    mtspr 561, r3

    # IBATU 4
    lis r3, 0x0080
    ori r3, r3, 0x00FF
    mtspr 560, r3


    # while we are at it, let's give IBATU0 kernel permissions, maybe it will come in handy
    # write "nop" to some positions that set/reset IBATU0
    lis r4, 0x6000

    lis r3, 0xFFF1
    ori r3, r3, 0xD518
    stw r4, 0(r3)
    dcbf 0, r3
    icbi 0, r3

    lis r3, 0xFFF1
    ori r3, r3, 0xD72C
    stw r4, 0(r3)
    dcbf 0, r3
    icbi 0, r3

    # add code execution permission for supervisor to IBATU0
    # IBATU 0
    lis r3, 0x0100
    ori r3, r3, 0x00FF
    mtibatu 0,r3

    # Now lets hook the kernel function for result printing of PrepareTitle add jump to our function
    # at location 0x00AE1058
    lis r3, 0xFFF1
    ori r3, r3, 0x8558
    lis r4, 0x48AE
    ori r4, r4, 0x105A
    stw r4, 0(r3)

    # memory barrier
    eieio
    isync

    # restore DBAT 0 and return from interrupt
    mtdbatu 0, r30
    mtdbatl 0, r31

    # memory barrier
    eieio
    isync

    rfi

