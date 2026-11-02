const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .thumb,
        .os_tag = .freestanding,
        .abi = .eabihf,
        .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_m33 },
    });
    const optimize = b.standardOptimizeOption(.{});

    // st's device header, the same one the c programs use. nix develop links
    // it and the cmsis core headers into ../.cmsis
    const regs = b.addTranslateC(.{
        .root_source_file = b.path("src/regs.h"),
        .target = target,
        .optimize = optimize,
        .link_libc = false,
    });
    regs.defineCMacro("STM32H533xx", null);
    regs.addIncludePath(b.path("src/libc"));
    regs.addIncludePath(b.path("../.cmsis/device"));
    regs.addIncludePath(b.path("../.cmsis/core"));

    const exe = b.addExecutable(.{
        .name = "zig-blink",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
            .imports = &.{.{ .name = "regs", .module = regs.createModule() }},
        }),
    });
    exe.setLinkerScript(b.path("stm32h533re.ld"));
    exe.entry = .{ .symbol_name = "Reset_Handler" };
    // debug builds keep every section and bundle ubsan otherwise, 360K of it
    exe.link_gc_sections = true;
    exe.bundle_ubsan_rt = false;

    // same spot as the make programs, <prefix>/zig-blink.elf
    const elf = b.addInstallFileWithDir(exe.getEmittedBin(), .prefix, "zig-blink.elf");
    b.getInstallStep().dependOn(&elf.step);
}
