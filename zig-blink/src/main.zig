// blinks LD2 (PA5) at 1 Hz with plain register writes, like ../blink but
// with the startup code in zig too. names come from st's device header

const std = @import("std");
const c = @import("regs");

// no room for stack traces, a failed safety check just traps
pub const panic = std.debug.no_panic;

// NOTE: translate-c drops the volatile off the struct fields, so every
// register access goes through this
fn io(reg: *u32) *volatile u32 {
    return reg;
}

// the core runs off HSI after reset, 64 MHz divided by 2
const cpu_hz = 32_000_000;

extern var _sidata: u32;
extern var _sdata: u32;
extern var _edata: u32;
extern var _sbss: u32;
extern var _ebss: u32;
extern var _estack: u32;

const Handler = *const fn () callconv(.c) void;

// only the cortex-m33 core exceptions, the blink takes no interrupts
const VectorTable = extern struct {
    initial_sp: *const u32,
    reset: Handler,
    nmi: Handler = defaultHandler,
    hard_fault: Handler = defaultHandler,
    mem_manage: Handler = defaultHandler,
    bus_fault: Handler = defaultHandler,
    usage_fault: Handler = defaultHandler,
    secure_fault: Handler = defaultHandler,
    reserved: [3]usize = .{ 0, 0, 0 },
    svcall: Handler = defaultHandler,
    debug_monitor: Handler = defaultHandler,
    reserved2: usize = 0,
    pendsv: Handler = defaultHandler,
    systick: Handler = defaultHandler,
};

export const vectors: VectorTable linksection(".isr_vector") = .{
    .initial_sp = &_estack,
    .reset = Reset_Handler,
};

export fn Reset_Handler() callconv(.c) void {
    // full access to the fpu, the target is eabihf
    io(&c.SCB.*.CPACR).* |= (3 << 20) | (3 << 22);
    asm volatile ("dsb\n isb" ::: .{ .memory = true });

    const data_len = @intFromPtr(&_edata) - @intFromPtr(&_sdata);
    const data: [*]u8 = @ptrCast(&_sdata);
    const init: [*]const u8 = @ptrCast(&_sidata);
    @memcpy(data[0..data_len], init[0..data_len]);

    const bss_len = @intFromPtr(&_ebss) - @intFromPtr(&_sbss);
    const bss: [*]u8 = @ptrCast(&_sbss);
    @memset(bss[0..bss_len], 0);

    main();
}

fn defaultHandler() callconv(.c) void {
    while (true) {}
}

fn delayMs(ms: u32) void {
    const systick = c.SysTick;
    io(&systick.*.LOAD).* = cpu_hz / 1000 - 1;
    io(&systick.*.VAL).* = 0;
    io(&systick.*.CTRL).* = c.SysTick_CTRL_CLKSOURCE_Msk | c.SysTick_CTRL_ENABLE_Msk;
    for (0..ms) |_| {
        while (io(&systick.*.CTRL).* & c.SysTick_CTRL_COUNTFLAG_Msk == 0) {}
    }
    io(&systick.*.CTRL).* = 0;
}

export fn main() callconv(.c) void {
    io(&c.RCC.*.AHB2ENR).* |= c.RCC_AHB2ENR_GPIOAEN;
    // NOTE: the read back makes the write land before GPIOA is touched
    _ = io(&c.RCC.*.AHB2ENR).*;

    const moder = io(&c.GPIOA.*.MODER);
    moder.* = (moder.* & ~@as(u32, c.GPIO_MODER_MODE5)) | c.GPIO_MODER_MODE5_0;

    const bsrr = io(&c.GPIOA.*.BSRR);
    while (true) {
        bsrr.* = c.GPIO_BSRR_BS5;
        delayMs(500);
        bsrr.* = c.GPIO_BSRR_BR5;
        delayMs(500);
    }
}
