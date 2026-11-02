{
  description = "firmware and dev tools for a NUCLEO-H533RE, probably other stm32 boards too";

  inputs.nixpkgs.url = "https://channels.nixos.org/nixos-unstable/nixexprs.tar.zst";

  outputs =
    {
      self,
      nixpkgs,
      ...
    }:
    let
      inherit (nixpkgs) lib;

      # stm32cubemx only ships for x86_64 linux
      systems = [ "x86_64-linux" ];
      forAll =
        f:
        lib.genAttrs systems (
          system:
          f (
            import nixpkgs {
              inherit system;
              config.allowUnfreePredicate = pkg: lib.getName pkg == "stm32cubemx";
            }
          )
        );

      treefmt =
        pkgs:
        pkgs.treefmt.withConfig {
          configFile = ./treefmt.toml;
          runtimeInputs = with pkgs; [
            clang-tools
            nixfmt
            prettier
            ruff
            zig_0_17
          ];
        };
    in
    {
      formatter = forAll treefmt;
      checks = forAll (pkgs: {
        formatting = (treefmt pkgs).check self;
      });
      devShells = forAll (
        pkgs:
        let
          # the H5 device pack for pyocd. `pyocd pack install` finds no H5
          # parts in keil's index, so pyocd.yaml loads this copy instead
          stm32h5-dfp = pkgs.fetchurl {
            url = "https://github.com/Open-CMSIS-Pack/STM32H5xx_DFP/releases/download/v2.3.1/Keil.STM32H5xx_DFP.2.3.1.pack";
            hash = "sha256-kGsviAKEXtIcS8MVEqgWBlmRvZg58UQdASamZ6UDR7E=";
          };
          # register and bit names for c (and zig through translate-c), like
          # msp430.h: st's device header and arm's core header under it
          cmsis-device-h5 = pkgs.fetchFromGitHub {
            owner = "STMicroelectronics";
            repo = "cmsis-device-h5";
            tag = "v1.7.0";
            hash = "sha256-ezyi3C/VY0izdn6Ol21x0Vl0UKq9MQQaMMcx9roWTJI=";
          };
          cmsis-core = pkgs.fetchFromGitHub {
            owner = "ARM-software";
            repo = "CMSIS_6";
            tag = "v6.4.0";
            hash = "sha256-35dWmCph2DFwZ6EwG+BCkwTo/57Ubnbjee0GicUWxDg=";
          };
          # register names for debuggers, from the same pack
          stm32h533-svd = pkgs.runCommand "STM32H533.svd" { nativeBuildInputs = [ pkgs.unzip ]; } ''
            unzip -p ${stm32h5-dfp} CMSIS/SVD/STM32H533.svd > $out
          '';
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              # generates the HAL, FreeRTOS and the CMake project
              stm32cubemx
              # arm-none-eabi-gcc, newlib and arm-none-eabi-gdb
              gcc-arm-embedded
              gnumake
              # clangd, reads build/compile_commands.json
              clang-tools
              asm-lsp
              # only for projects CubeMX generates, ours use make
              cmake
              ninja
              just
              zig_0_17
              zls_0_17

              # flashing and debugging, upstream openocd 0.12 has no stm32h5
              pyocd
              stlink
              gdb
              # gdb frontend, x11 only so it runs on xwayland
              gf
              # the board's serial port
              tio
              usbutils

              # usb logic analyzer
              sigrok-cli
              pulseview

              (treefmt pkgs)
            ];

            # java guis come up blank under non-reparenting window managers
            _JAVA_AWT_WM_NONREPARENTING = 1;

            shellHook = /* sh */ ''
              # only in a terminal, nix develop -c from an editor skips the
              # second it takes
              if [ -t 1 ]; then
                printf '%-18s %s\n' \
                  arm-none-eabi-gcc "$(arm-none-eabi-gcc -dumpversion)" \
                  arm-none-eabi-gdb "$(arm-none-eabi-gdb --version | head -n1 | awk '{print $NF}')" \
                  make "$(make --version | head -n1 | cut -d' ' -f3)" \
                  cmake "$(cmake --version | head -n1 | cut -d' ' -f3)" \
                  ninja "$(ninja --version)" \
                  pyocd "$(pyocd --version)" \
                  st-info "$(st-info --version)" \
                  zig "$(zig version)"
              fi

              # where pyocd.yaml looks for the pack. nix develop works from any
              # folder below the flake, so walk up to it
              root=$PWD
              while [ "$root" ] && [ ! -e "$root/flake.nix" ]; do root=''${root%/*}; done
              root=''${root:-$PWD}
              mkdir -p "$root/.pyocd"
              ln -sfn ${stm32h5-dfp} "$root/.pyocd/Keil.STM32H5xx_DFP.pack"
              ln -sfn ${stm32h533-svd} "$root/.pyocd/STM32H533.svd"

              # where the programs look for the headers
              mkdir -p "$root/.cmsis"
              ln -sfn ${cmsis-device-h5}/Include "$root/.cmsis/device"
              ln -sfn ${cmsis-core}/CMSIS/Core/Include "$root/.cmsis/core"
              # st's startup, SystemInit and linker scripts
              ln -sfn ${cmsis-device-h5}/Source/Templates "$root/.cmsis/templates"
            '';
          };
        }
      );
    };
}
