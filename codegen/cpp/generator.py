from contextlib import contextmanager
from pathlib import Path

import sys

sys.path.append(str(Path(__file__).parent.parent.parent))

from convoy import Convoy

import shutil


class CPPGenerator:
    def __init__(self):
        self.__code = ""
        self.__indents = 0
        self.__doc = False

    def __call__(self, line: str, /, *, indent: int | None = None, unique_line: bool = False) -> None:
        if unique_line and line in self.__code:
            return

        tabs = " " * (self.__indents if indent is None else indent)
        pfix = " * " if self.__doc else ""
        self.__code += f"{tabs}{pfix}{line}\n"

    @property
    def code(self) -> str:
        return self.__code

    def disclaimer(self, ffile: str, /) -> None:
        self(f"// Generated by Convoy's code generation script: '{ffile}'", unique_line=True)

    def include(self, header: str, /, *, quotes: bool = False) -> None:
        self(f'#include "{header}"' if quotes else f"#include <{header}>", unique_line=True)

    def comment(self, msg: str, /) -> None:
        msgs = msg.split("\n")
        for msg in msgs:
            self(f"// {msg}")

    def brief(self, msg: str, /) -> None:
        self.__assert_doc()
        self(f"@brief {msg}")
        self("")

    def param(self, param: str, msg: str, /) -> None:
        self.__assert_doc()
        self(f"@param {param} {msg}")

    def tparam(self, param: str, msg: str, /) -> None:
        self.__assert_doc()
        self(f"@tparam {param} {msg}")

    def ret(self, msg: str, /) -> None:
        self.__assert_doc()
        self(f"@return {msg}")

    @contextmanager
    def doc(self):
        try:
            self("/**")
            self.__doc = True
            yield
        finally:
            self.__doc = False
            self(" */")

    def spacing(self, n: int = 1, /) -> None:
        for _ in range(n):
            self("")

    @contextmanager
    def scope(
        self,
        name: str | None = None,
        /,
        *,
        opener: str = "{",
        closer: str = "}",
        delimiters: bool = True,
        indent: int = 4,
    ):
        try:
            if name is not None:
                self(name)
            if delimiters and opener:
                self(opener)
            self.__indents += indent
            yield
        finally:
            self.__indents -= indent
            if delimiters and closer:
                self(closer)

    def write(self, path: Path, /) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w") as file:
            file.write(self.__code)

        Convoy.log(
            f"Exported generated code to <underline>{path.resolve()}</underline>. Attempting to format with <bold>clang-format</bold>."
        )
        cfpath = shutil.which("clang-format")
        if cfpath is None:
            Convoy.warning("<bold>clang-format</bold> was not found.")
            return

        if Convoy.run_process_success([str(cfpath), "-i", str(path)]):
            Convoy.log("Successfully formatted.")
        else:
            Convoy.warning("Failed to run <bold>clang-format</bold>.")

    def __assert_doc(self) -> None:
        if not self.__doc:
            Convoy.exit_error("Cannot call documentation methods when not in a <bold>doc()</bold> block.")
