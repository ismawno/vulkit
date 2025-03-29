import sys
import platform
import subprocess
import os

from time import perf_counter
from pathlib import Path


class _Style:

    RESET = "\033[0m"
    BOLD = "\033[1m"
    DIM = "\033[2m"
    ITALIC = "\033[3m"  # Not widely supported
    UNDERLINE = "\033[4m"
    BLINK = "\033[5m"
    REVERSE = "\033[7m"
    HIDDEN = "\033[8m"
    STRIKETHROUGH = "\033[9m"

    FG_BLACK = "\033[30m"
    FG_RED = "\033[31m"
    FG_GREEN = "\033[32m"
    FG_YELLOW = "\033[33m"
    FG_BLUE = "\033[34m"
    FG_MAGENTA = "\033[35m"
    FG_CYAN = "\033[36m"
    FG_WHITE = "\033[37m"
    FG_DEFAULT = "\033[39m"

    FG_BRIGHT_BLACK = "\033[90m"
    FG_BRIGHT_RED = "\033[91m"
    FG_BRIGHT_GREEN = "\033[92m"
    FG_BRIGHT_YELLOW = "\033[93m"
    FG_BRIGHT_BLUE = "\033[94m"
    FG_BRIGHT_MAGENTA = "\033[95m"
    FG_BRIGHT_CYAN = "\033[96m"
    FG_BRIGHT_WHITE = "\033[97m"

    BG_BLACK = "\033[40m"
    BG_RED = "\033[41m"
    BG_GREEN = "\033[42m"
    BG_YELLOW = "\033[43m"
    BG_BLUE = "\033[44m"
    BG_MAGENTA = "\033[45m"
    BG_CYAN = "\033[46m"
    BG_WHITE = "\033[47m"
    BG_DEFAULT = "\033[49m"

    BG_BRIGHT_BLACK = "\033[100m"
    BG_BRIGHT_RED = "\033[101m"
    BG_BRIGHT_GREEN = "\033[102m"
    BG_BRIGHT_YELLOW = "\033[103m"
    BG_BRIGHT_BLUE = "\033[104m"
    BG_BRIGHT_MAGENTA = "\033[105m"
    BG_BRIGHT_CYAN = "\033[106m"
    BG_BRIGHT_WHITE = "\033[107m"

    @staticmethod
    def format(text: str, /) -> str:
        translator = _Style.__create_format_dict()
        styles = set()

        splitted = text.split("<")
        formatted = splitted[0]
        for segment in splitted[1:]:
            for key, value in translator.items():
                if segment.startswith(key):
                    styles.add(value)
                    formatted += segment.replace(key, value, 1)
                    break
                if segment.startswith(f"/{key}"):
                    styles.remove(value)
                    formatted += segment.replace(
                        f"/{key}", f"{_Style.RESET}{''.join(styles)}", 1
                    )
                    break
            else:
                formatted += segment
        return formatted + _Style.RESET

    @staticmethod
    def __create_format_dict() -> dict[str, str]:
        result = {}
        for name, value in _Style.__dict__.items():
            if callable(value) or not isinstance(value, str) or name.startswith("_"):
                continue
            prefix = ""
            if name.startswith("FG_"):
                prefix += "f"
            elif name.startswith("BG_"):
                prefix += "b"
            if "BRIGHT" in name:
                prefix += "b"
            name = (
                name.removeprefix("FG_")
                .removeprefix("BG_")
                .removeprefix("BRIGHT_")
                .lower()
            )

            result[f"{prefix}{name}>"] = value
        return result


class _MetaConvoy(type):
    def __init__(self, *args, **kwargs) -> None:
        self.__t1 = perf_counter()
        super().__init__(*args, **kwargs)
        self.is_verbose = False
        self.all_yes = False
        self.safe = False

        self.__log_label = ""
        self.__indent = 0

    @property
    def is_windows(self) -> bool:
        return sys.platform.startswith("win")

    @property
    def is_linux(self) -> bool:
        return sys.platform.startswith("linux")

    @property
    def is_macos(self) -> bool:
        return sys.platform.startswith("darwin")

    @property
    def is_arm(self) -> bool:
        return platform.machine().lower() in ["arm64", "aarch64"]

    @property
    def operating_system(self) -> str:
        if self.is_windows:
            return "Windows"
        if self.is_linux:
            return "Linux"
        if self.is_macos:
            return "MacOS"
        return sys.platform

    @property
    def architecure(self) -> str:
        similars = {"i386": "x86", "amd64": "x86_64", "x32": "x86", "x64": "x86_64"}
        try:
            return similars[platform.machine().lower()]
        except KeyError:
            return platform.machine().lower()

    @property
    def log_label(self) -> str:
        return self.__log_label

    @log_label.setter
    def log_label(self, msg: str, /) -> None:
        self.__log_label = self.__create_log_label(msg)

    def linux_distro(self) -> str | None:
        try:
            with open("/etc/os-release") as f:
                for line in f:
                    if line.startswith("ID="):
                        return line.split("=")[1].strip().strip('"')
        except FileNotFoundError:
            return None
        return None

    def linux_version(self) -> str | None:
        try:
            with open("/etc/os-release") as f:
                for line in f:
                    if line.startswith("VERSION_ID="):
                        return line.split("=")[1].strip().strip('"')
        except FileNotFoundError:
            return None
        return None

    def push_indent(self) -> None:
        self.__indent += 1

    def pop_indent(self) -> None:
        self.__indent -= 1

    def log(self, msg: str, /, *args, **kwargs) -> None:
        print(
            _Style.format(f"{self.__log_label}{'  '*self.__indent}{msg}"),
            *args,
            **kwargs,
        )

    def verbose(self, msg: str, /, *args, **kwargs) -> None:
        if self.is_verbose:
            print(_Style.format(f"{self.__log_label}{msg}"), *args, **kwargs)

    def exit_ok(self, msg: str | None = None, /) -> None:
        if msg is not None:
            self.log(f"<fgreen>{msg}</fgreen>")

        self.__exit(0)

    def exit_error(
        self,
        msg: str = "Something went wrong! Likely because something happened or the user declined a prompt.",
        /,
    ) -> None:
        self.log(f"<fred>Error: {msg}", file=sys.stderr)
        self.__exit(1)

    def exit_declined(self) -> None:
        self.exit_error("Operation declined by user.")

    def prompt(self, msg: str, /, *, default: bool = True) -> bool:
        if self.all_yes:
            return True

        msg = _Style.format(
            f"{self.__log_label}{'  '*self.__indent}<fcyan>{msg} [Y]/N "
            if default
            else f"{msg} Y/[N] "
        )

        while True:
            answer = input(msg).strip().lower()
            if answer in ["y", "yes"] or (default and answer == ""):
                return True
            elif answer in ["n", "no"] or (not default and answer == ""):
                return False

    def run_process(
        self, command: str | list[str], /, *args, exit_on_decline: bool = True, **kwargs
    ) -> subprocess.CompletedProcess | None:
        if self.safe and not self.prompt(
            f"The command <bold>{command if isinstance(command, str) else ' '.join(command)}</bold> is about to be executed. Do you wish to continue?"
        ):
            if exit_on_decline:
                self.exit_declined()
            return None

        return subprocess.run(command, *args, **kwargs)

    def run_process_success(self, command: str | list[str], /, *args, **kwargs) -> bool:
        result = self.run_process(command, *args, **kwargs, exit_on_decline=False)
        return result is not None and result.returncode == 0

    def run_file(self, path: Path | str, /) -> None:
        if self.safe and not self.prompt(
            f"The file '{path}' is about to be executed. Do you wish to continue?"
        ):
            self.exit_declined()

        if isinstance(path, Path):
            path = str(path.resolve())
        if self.is_windows:
            os.startfile(path)
        elif self.is_linux:
            self.run_process(["xdg-open", path])
        elif self.is_macos:
            self.run_process(["open", path])

    def __exit(self, code: int, /) -> None:
        elapsed = perf_counter() - self.__t1
        self.log(f"Finished in <bold>{elapsed:.2f}</bold> seconds.")
        sys.exit(code)

    def __create_log_label(self, msg: str, /) -> str:
        return f"<fblue>[{msg}]</fblue> "


class Convoy(metaclass=_MetaConvoy):
    pass
