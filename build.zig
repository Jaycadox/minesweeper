const std = @import("std");
const raylib = @import("raylib");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardOptimizeOption(.{});
   
    const r = raylib.addRaylib(b, target, mode, .{}) catch unreachable; 

    const game = b.addExecutable(.{
        .name = "minesweeper",
        .target = target,
        .optimize = mode,
    });
    game.linkLibC();
    game.addCSourceFiles(.{ 
        .files = &.{
            "main.c",
        },
        .flags = &.{
            "-std=c99",
            "-pedantic",
            "-Wall",
            "-W",
            "-Wno-missing-field-initializers",
            "-fno-sanitize=undefined",
            "-Wno-gnu-binary-literal",
        }
    });
    game.addIncludePath(b.path("raylib/src"));
    game.linkLibrary(r);
    b.installArtifact(game);
}
