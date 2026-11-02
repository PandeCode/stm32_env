;; hermes for the board: <leader>mm opens the menu of everything below,
;; <leader>mb builds, <leader>mr flashes, <leader>dm flashes and debugs,
;; <leader>mu opens the serial port. they work on the program of the file you
;; are in (or of the folder nvim started in, else blink) until you pick one in
;; the menu. hermes asks to trust this file the first time and after every
;; change

;; the folder of this file, nvim can start anywhere below it
(local root (vim.fs.dirname (: (. (debug.getinfo 1 :S) :source) :sub 2)))
(local mode :Debug)
(var current nil)
;; a program picked in the menu, files you open don't change it
(var pinned false)

;; outside `nix develop` the tools come from the flake. any other nix shell
;; sets IN_NIX_SHELL too, so look for the compiler instead
(local in-shell (= 1 (vim.fn.executable :arm-none-eabi-gcc)))

;; terminals open in nvim's cwd, which can be outside the repo after a :cd
(fn dev [cmd]
  (.. "cd " (vim.fn.shellescape root) " && "
      (if in-shell cmd (.. "nix develop . -c " cmd))))

(fn dev-cmd [cmd]
  (if in-shell cmd (vim.list_extend [:nix :develop root :-c] cmd)))

(fn just [recipe]
  #(_G.Utils.open_tmp_term (dev (.. "just " recipe))))

;; a program is a top-level folder with one of these, same as the justfile.
;; common/ and lib/ have none. path is a file or folder anywhere in it
(fn program-of [path]
  (let [rel (vim.fs.relpath root path)
        app (and rel (rel:match "^([^/]+)"))]
    (when app
      (accumulate [found nil _ file (ipairs [:Makefile
                                             :build.zig
                                             :CMakePresets.json])
                   &until found]
        (when (vim.uv.fs_stat (vim.fs.joinpath root app file))
          app)))))

(fn programs []
  (doto (icollect [name kind (vim.fs.dir root)]
          (when (= kind :directory)
            (program-of (vim.fs.joinpath root name))))
    (table.sort)))

;; quickfix, terminals and dap windows have no file name, or one that isn't a
;; path. relpath would resolve those against the cwd
(fn buf-program []
  (let [name (vim.api.nvim_buf_get_name 0)]
    (when (and (= "" vim.bo.buftype) (not= "" name))
      (program-of name))))

(var use nil)

(fn pick []
  (vim.ui.select [:follow-the-file (unpack (programs))]
                 {:prompt :program
                  :format_item #(if (= $1 :follow-the-file)
                                    "follow the open file"
                                    $1)}
                 (fn [app]
                   (when app
                     (set pinned (not= app :follow-the-file))
                     (use (if pinned app (or (buf-program) current)))))))

(set use (fn [app]
           (set current app)
           (_G.Mcu.setup {: root
                       :elf (string.format "%s/build/%s/%s.elf" app mode app)
                       :build (dev (string.format "just build %s %s" app mode))
                       :flash (dev (string.format "just flash %s %s" app mode))
                       :server (dev "just gdbserver")
                       :serial (dev "just serial")
                       ;; same as just debug, pyocd shows no peripherals
                       :reset ["set mem inaccessible-by-default off"
                               "monitor reset halt"]
                       :actions [[(.. "Program: " app
                                      (if pinned "" ", follows the open file"))
                                  pick]
                                 [:Size (just (.. "size " app " " mode))]
                                 [:Clean (just (.. "clean " app))]
                                 ["Build every program"
                                  (just (.. "build all " mode))]
                                 ["Reset the board" (just :reset)]
                                 ["Erase the flash" (just :erase)]]})))

;; switching mid debug would point the keys at another elf
(fn debugging []
  (let [dap (. package.loaded :dap)]
    (and dap (dap.session))))

(when _G.Mcu
  (vim.api.nvim_create_autocmd :BufEnter
                               {:group (vim.api.nvim_create_augroup :stm32-program
                                                                    {:clear true})
                                :callback #(let [app (buf-program)]
                                             (when (and app (not pinned)
                                                        (not= app current)
                                                        (not (debugging)))
                                               (use app)))})
  (use (or (program-of (vim.fn.getcwd)) :blink)))

;; clangd reads <app>/build/compile_commands.json, common/ and lib/ get
;; build/compile_commands.json at the root, which every Makefile program adds
;; to. the commands say plain arm-none-eabi-gcc, so clangd needs the flake's
;; PATH. --query-driver lets it ask that gcc for newlib's headers, without it
;; the host's glibc breaks every include
(vim.lsp.config :clangd {:cmd (dev-cmd [:clangd
                                        :--query-driver=/nix/store/*-gcc-arm-embedded-*/bin/arm-none-eabi-*])
                         :cmd_cwd root})

;; asm-lsp runs the commands in the root database from its own cwd, and their
;; paths start at the root. hermes turns it on for every asm file, hold it
;; until the database is there
(vim.lsp.config :asm_lsp {:cmd (dev-cmd [:asm-lsp]) :cmd_cwd root})
(vim.lsp.enable :asm_lsp false)

;; a program's database only exists once it was built, so on a fresh checkout
;; clangd has no flags and every include goes red. each Makefile writes its
;; database without compiling anything, then clangd starts. that's a few ms
;; in the shell, outside it nix develop takes a few seconds. the dev shell
;; also links .cmsis
(local compile-db
       "rc=0; for m in */Makefile; do make -s -f \"$m\" build/compile_commands.json || rc=1; done; exit $rc")

;; before VimEnter lsp.enable skips buffers that are already open, like the
;; file in `nvim uart/src/main.c`
(fn start-lsp [res]
  (when (not= 0 res.code)
    (vim.notify (.. "compile_commands.json: " res.stderr) vim.log.levels.WARN))
  (if (= 1 vim.v.vim_did_enter)
      (vim.lsp.enable [:clangd :asm_lsp])
      (vim.api.nvim_create_autocmd :VimEnter
                                   {:once true
                                    :callback #(vim.lsp.enable [:clangd
                                                                :asm_lsp])})))

(vim.system (dev-cmd [:sh :-c compile-db]) {:cwd root}
            (vim.schedule_wrap start-lsp))
