from pathlib import Path
from argparse import ArgumentParser, Namespace
from dataclasses import dataclass, field
from collections.abc import Callable
from generator import CPPGenerator

import sys
import copy
import xml.etree.ElementTree as ET
import difflib


sys.path.append(str(Path(__file__).parent.parent.parent))

from convoy import Convoy


def parse_arguments() -> Namespace:
    desc = """
    The purpose of this script is to generate the code needed to load all available vulkan functions and organize
    them in such a way that it is clear which functions/structs are available and which are not.
    """

    parser = ArgumentParser(description=desc)
    parser.add_argument(
        "-i",
        "--input",
        type=Path,
        default=None,
        help="The input path where the 'vk.xml' file is located. If not provided, the script will download the file.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        required=True,
        help="The output file path where the generated code will be saved.",
    )
    parser.add_argument(
        "-a",
        "--api",
        type=str,
        default="vulkan",
        help="The API to generate code for. Can be 'vulkan' or 'vulkansc'.",
    )
    parser.add_argument(
        "--guard-version",
        action="store_true",
        default=False,
        help="If set, the generated code will be guarded by the VKIT_API_VERSION macro when necessary.",
    )
    parser.add_argument(
        "--guard-extension",
        action="store_true",
        default=False,
        help="If set, the generated code will be guarded by extension macros when necessary.",
    )
    parser.add_argument(
        "--export-timeline",
        type=Path,
        default=None,
        help="The path where a small .txt will be exported alongside the generated code with information about when/by which extension a function/type was added. If not provided, the timeline will not be exported.",
    )
    parser.add_argument("--sdk-version", type=str, default="v1.4.328", help="The version of the vulkan sdk to use.")
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Print more information.",
    )

    return parser.parse_args()


@dataclass
class GuardGroup:
    guards: list[list[str]] = field(default_factory=list)

    def and_guards(self, guards: str | list[str], /) -> None:
        if isinstance(guards, str):
            guards = [guards]
        guards = [g.strip() for g in guards]
        self.guards.append([f"defined({g})" if " " not in g else g for g in guards])

    def parse_guards(self) -> str:
        if not self.guards:
            return ""

        or_guards: list[str] = []
        for group in self.guards:
            or_guards.append(" || ".join(group))

        return " && ".join([f"({g})" if "||" in g else g for g in or_guards if g]).strip()


broken_functions = {
    "vkGetLatencyTimingsNV": 271,  # Changed API parameters
    "vkCmdSetDiscardRectangleEnableEXT": 241,  # new function in older extension
    "vkCmdSetDiscardRectangleModeEXT": 241,  # new function in older extension
    "vkCmdSetExclusiveScissorEnableNV": 241,  # Changed API parameters
    "vkCmdInitializeGraphScratchMemoryAMDX": 298,  # Changed API parameters
    "vkCmdDispatchGraphAMDX": 298,  # Changed API parameters
    "vkCmdDispatchGraphIndirectAMDX": 298,  # Changed API parameters
    "vkCmdDispatchGraphIndirectCountAMDX": 298,  # Changed API parameters
}


@dataclass
class Parameter:
    name: str
    tp: str
    array: str | None = None

    def as_string(self) -> str:
        if self.array is not None:
            return f"{self.tp} {self.name}{self.array}"
        return f"{self.tp} {self.name}"


@dataclass
class Type:
    name: str
    available_since: str | None = None
    guards: list[GuardGroup] = field(default_factory=list)

    def parse_guards(self) -> str:
        guards = [g.parse_guards() for g in self.guards]
        return fix_version_macro(" || ".join([f"({g})" if "&&" in g else g for g in guards if g]).strip())


@dataclass
class Function:
    name: str
    return_type: str
    params: list[Parameter]
    dispatchable: bool
    available_since: str | None = None
    guards: list[GuardGroup] = field(default_factory=list)

    def parse_guards(self) -> str:
        guards = [g.parse_guards() for g in self.guards]
        guards = fix_version_macro(" || ".join([f"({g})" if "&&" in g else g for g in guards if g]))

        if self.name not in broken_functions:
            return guards.strip()

        hv = f"VK_HEADER_VERSION >= {broken_functions[self.name]}"
        return hv if not guards else f"{hv} && ({guards})".strip()

    def as_string(
        self,
        *,
        namespace: str | None = None,
        vk_prefix: bool = True,
        no_discard: bool = False,
        api_macro: bool = False,
        semicolon: bool = True,
        const: bool = False,
    ) -> str:
        params = ", ".join(p.as_string() for p in self.params)
        fname = self.name if vk_prefix else self.name.removeprefix("vk")
        if namespace is not None:
            fname = f"{namespace}::{fname}"

        rtype = self.return_type
        if no_discard and rtype != "void":
            rtype = f"VKIT_LOADER_NO_DISCARD {rtype}"
        if api_macro:
            rtype = f"VKIT_API {rtype}"
        modifiers = " const" if const else ""

        return f"{rtype} {fname}({params}){modifiers};" if semicolon else f"{rtype} {fname}({params}){modifiers}"

    def as_fn_pointer_declaration(self, *, modifier: str | None = None, null: bool = False) -> str:
        if modifier is None:
            modifier = ""
        return (
            f"{modifier} PFN_{self.name} {self.name};".strip()
            if not null
            else f"{modifier} PFN_{self.name} {self.name} = VK_NULL_HANDLE;".strip()
        )

    def as_fn_pointer_type(self) -> str:
        return f"PFN_{self.name}"

    def is_instance_function(self) -> bool:
        return (
            fn.name
            not in [
                "vkGetInstanceProcAddr",
                "vkCreateInstance",
            ]
            and self.dispatchable
            and (
                self.params[0].tp == "VkInstance"
                or self.params[0].tp == "VkPhysicalDevice"
                or fn.name == "vkGetDeviceProcAddr"
            )
        )

    def is_device_function(self) -> bool:
        return (
            fn.name != "vkGetDeviceProcAddr"
            and self.dispatchable
            and self.params[0].tp != "VkInstance"
            and self.params[0].tp != "VkPhysicalDevice"
        )


def download_vk_xml() -> Path:
    import urllib.request

    url = f"https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/refs/tags/{args.sdk_version}/xml/vk.xml"

    vendor = rpath / "vendor"
    vendor.mkdir(exist_ok=True)

    location = vendor / "vk.xml"
    if location.exists():
        return location

    try:
        urllib.request.urlretrieve(url, str(location))
        Convoy.verbose(
            f"Downloaded <bold>vk.xml</bold> from <underline>{url}</underline> to <underline>{location}</underline>."
        )
    except Exception as e:
        Convoy.exit_error(
            f"Failed to download <bold>vk.xml</bold> from <underline>{url}</underline> because of <bold>{e}</bold>. Please download it manually and place it in <underline>{location}</underline>, or provide the path with the <bold>-i</bold> or <bold>--input</bold> argument."
        )

    return location


Convoy.program_label = "VK-LOADER"
rpath = Path(__file__).parent.resolve()

args = parse_arguments()
Convoy.is_verbose = args.verbose
vulkan_api: str = args.api


def check_api(element: ET.Element, /, *, strict: bool = False) -> bool:
    for k in ["api", "supported", "export"]:
        api = element.get(k)
        if api is None:
            continue

        if vulkan_api not in api.split(","):
            return False

    return not strict


def fix_version_macro(version: str, /) -> str:
    shits = ["VK_VERSION", "VK_BASE_VERSION", "VK_COMPUTE_VERSION", "VK_GRAPHICS_VERSION"]
    for shit in shits:
        version = version.replace(shit, "VKIT_API_VERSION")
    return version


vkxml_path: Path | None = args.input
output: Path = args.output.resolve()

if vkxml_path is None:
    vkxml_path = download_vk_xml()

vkxml_path = vkxml_path.resolve()

with open(vkxml_path, "r") as f:
    vkxml = f.read()

tree = ET.ElementTree(ET.fromstring(vkxml))
root = tree.getroot()
if root is None:
    Convoy.exit_error("Failed to get the root of the vulkan's XML.")


def ncheck_text(param: ET.Element | None, /) -> str:
    if param is None or param.text is None:
        Convoy.exit_error("Found a None entry when trying to parse the vulkan's XML.")
    return param.text


dispatchables: set[str] = set()
for h in root.findall("types/type[@category='handle']"):
    if not check_api(h):
        continue

    text = "".join(h.itertext())
    if "VK_DEFINE_HANDLE" in text:
        hname = h.get("name") or ncheck_text(h.find("name"))
        dispatchables.add(hname)

functions: dict[str, Function] = {}


def parse_commands(root: ET.Element, /, *, alias_sweep: bool) -> None:
    for command in root.findall("commands/command"):
        if not check_api(command):
            continue

        alias = command.get("alias")
        if alias_sweep:

            if alias is None:
                continue

            fname = Convoy.ncheck(command.get("name"))
            if fname in functions:
                continue

            fn = copy.deepcopy(functions[alias])
            fn.name = fname
            for i, param in enumerate(fn.params):
                fntp = param.tp
                clean_fntp = fntp.replace("const", "").replace("*", "").replace("struct", "").strip()
                if clean_fntp not in type_aliases:
                    continue

                tpal = type_aliases[clean_fntp]
                closest = difflib.get_close_matches(fname, tpal, n=1, cutoff=0.0)[0]
                fn.params[i].tp = fntp.replace(clean_fntp, closest)

            functions[fname] = fn

        if alias is not None:
            continue

        proto = Convoy.ncheck(command.find("proto"))
        fname = ncheck_text(proto.find("name"))
        return_type = ncheck_text(proto.find("type"))

        params: list[Parameter] = []
        for param in command.findall("param"):
            if not check_api(param):
                continue

            param_name = ncheck_text(param.find("name"))
            full = "".join(param.itertext()).strip()
            param_type = full.rsplit(param_name, 1)[0].strip()

            idx1 = full.find("[")
            idx2 = full.find("]")

            p = Parameter(
                param_name,
                param_type,
                full[idx1 : idx2 + 1].strip() if idx1 != -1 and idx2 != -1 else None,
            )
            params.append(p)

        fn = Function(
            fname,
            return_type,
            params,
            bool(
                params and params[0].tp in dispatchables,
            ),
        )
        functions[fname] = fn
        Convoy.verbose(f"Parsed vulkan function <bold>{fn.as_string()}</bold>.")


types: dict[str, Type] = {}
type_aliases: dict[str, list[str]] = {}


def parse_types(root: ET.Element, lookup: str, /, *, alias_sweep: bool) -> None:
    for tp in root.findall(lookup):
        if not check_api(tp):
            continue

        alias = tp.get("alias")

        if alias_sweep:
            if alias is None:
                continue
            tname = Convoy.ncheck(tp.get("name"))
            if tname in types:
                continue

            type_aliases.setdefault(alias, []).append(tname)
            t = copy.deepcopy(types[alias])
            t.name = tname
            types[tname] = t
            Convoy.verbose(f"Parsed vulkan type <bold>{tname}</bold> as an alias of <bold>{alias}</bold>.")
            continue

        if alias is not None:
            continue

        tname = tp.get("name")
        if tname is None:
            tname = ncheck_text(tp.find("name"))

        types[tname] = Type(tname)
        Convoy.verbose(f"Parsed vulkan type <bold>{tname}</bold>.")


parse_types(root, "types/type", alias_sweep=False)
parse_types(root, "enums/enum", alias_sweep=False)
parse_types(root, "types/type", alias_sweep=True)
parse_types(root, "enums/enum", alias_sweep=True)

parse_commands(root, alias_sweep=False)
parse_commands(root, alias_sweep=True)

Convoy.log(f"Found <bold>{len(types)}</bold> types in the vk.xml file.")
Convoy.log(f"Found <bold>{len(functions)}</bold> {vulkan_api} functions in the vk.xml file.")


for feature in root.findall("feature"):
    if not check_api(feature):
        continue
    version = Convoy.ncheck(feature.get("name"))

    for require in feature.findall("require"):
        if not check_api(require):
            continue
        for command in require.findall("command"):
            if not check_api(command):
                continue
            fname = Convoy.ncheck(command.get("name"))
            fn = functions[fname]
            if fn.available_since is not None:
                Convoy.exit_error(
                    f"Function <bold>{fname}</bold> is already flagged as required since <bold>{fn.available_since}</bold>. Cannot register it again for the <bold>{version}</bold> feature."
                )

            version = fix_version_macro(version)
            fn.available_since = version
            guards = GuardGroup()
            guards.and_guards(version)

            fn.guards.append(guards)
            Convoy.verbose(
                f"Registered availability of function <bold>{fname}</bold> from the <bold>{version}</bold> feature."
            )

        for tp in require.findall("type"):
            if not check_api(tp):
                continue

            tpname = Convoy.ncheck(tp.get("name"))
            t = types[tpname]
            if t.available_since is not None:
                Convoy.exit_error(
                    f"Type <bold>{tpname}</bold> is already flagged as required since <bold>{t.available_since}</bold>. Cannot register it again for the <bold>{version}</bold> feature."
                )

            version = fix_version_macro(version)
            t.available_since = version
            guards = GuardGroup()
            guards.and_guards(version)

            t.guards.append(guards)
            Convoy.verbose(
                f"Registered availability of type <bold>{tpname}</bold> from the <bold>{version}</bold> feature."
            )

spec_version_req = {
    "vkCmdSetDiscardRectangleEnableEXT": 2,
    "vkCmdSetDiscardRectangleModeEXT": 2,
    "vkCmdSetExclusiveScissorEnableNV": 2,
    "vkGetImageViewAddressNVX": 2,
    "vkGetImageViewHandle64NVX": 3,
    "vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI": 2,
}

for extension in root.findall("extensions/extension"):
    extname = Convoy.ncheck(extension.get("name"))
    for require in extension.findall("require"):
        guards = GuardGroup()
        if args.guard_extension:
            guards.and_guards(extname)

        deps = require.get("depends")
        if deps is not None and args.guard_extension:
            for group in deps.split("+"):
                guards.and_guards(group.split(","))

        for command in require.findall("command"):
            fname = Convoy.ncheck(command.get("name"))
            if fname in spec_version_req and args.guard_version:
                guards.and_guards(f"{extname.upper()}_SPEC_VERSION >= {spec_version_req[fname]}")
            functions[fname].guards.append(copy.deepcopy(guards))
            Convoy.verbose(
                f"Registered availability of function <bold>{fname}</bold> from the <bold>{fname}</bold> extension."
            )

        for tp in require.findall("type"):
            tpname = Convoy.ncheck(tp.get("name"))
            types[tpname].guards.append(copy.deepcopy(guards))
            Convoy.verbose(
                f"Registered availability of type <bold>{tpname}</bold> from the <bold>{extname}</bold> extension."
            )


def guard_if_needed(gen: CPPGenerator, text: str | Callable, guards: str, /, *args, **kwargs) -> None:
    def put_code() -> None:
        if isinstance(text, str):
            gen(text)
        else:
            text(gen, *args, **kwargs)

    if guards:
        gen(f"#if {guards}", indent=0)
        put_code()
        gen("#endif", indent=0)
    else:
        put_code()


hpp = CPPGenerator()
hpp.disclaimer("vkloader.py")
hpp("#pragma once")
hpp.include("vkit/vulkan/vulkan.hpp", quotes=True)
hpp("#ifdef TKIT_ENABLE_ASSERTS")
hpp("#define VKIT_LOADER_NO_DISCARD [[nodiscard]]")
hpp("#else")
hpp("#define VKIT_LOADER_NO_DISCARD")
hpp("#endif")

with hpp.scope("namespace VKit::Vulkan", indent=0):
    hpp.spacing()

    hpp("void Load(void *p_Library);")
    hpp.spacing()

    def codefn1(gen: CPPGenerator, fn: Function, /) -> None:
        gen(fn.as_fn_pointer_declaration(modifier="extern"))
        gen(fn.as_string(vk_prefix=False, no_discard=True, api_macro=True))

    for fn in functions.values():
        if fn.is_instance_function() or fn.is_device_function():
            continue
        guards = fn.parse_guards()

        guard_if_needed(hpp, codefn1, guards, fn)
        hpp.spacing()

    hpp.spacing()

    def codefn2(gen: CPPGenerator, fn: Function, /) -> None:
        gen(fn.as_fn_pointer_declaration(null=True))
        gen(fn.as_string(vk_prefix=False, no_discard=True, const=True))

    with hpp.scope("struct VKIT_API InstanceTable", closer="};"):
        hpp("static InstanceTable Create(VkInstance p_Instance);")
        for fn in functions.values():
            if not fn.is_instance_function():
                continue

            guards = fn.parse_guards()
            guard_if_needed(hpp, codefn2, guards, fn)
            hpp.spacing()

    with hpp.scope("struct VKIT_API DeviceTable", closer="};"):
        hpp("static DeviceTable Create(VkDevice p_Device, const InstanceTable &p_InstanceFuncs);")
        for fn in functions.values():
            if not fn.is_device_function():
                continue

            guards = fn.parse_guards()
            guard_if_needed(hpp, codefn2, guards, fn)
            hpp.spacing()


cpp = CPPGenerator()
cpp.disclaimer("vkloader.py")
cpp.include("vkit/core/pch.hpp", quotes=True)
cpp.include("vkit/vulkan/loader.hpp", quotes=True)
cpp.include("tkit/utils/debug.hpp", quotes=True)
cpp("#if defined(TKIT_OS_APPLE) || defined(TKIT_OS_LINUX)", indent=0)
cpp.include("dlfcn.h")
cpp("#elif defined(TKIT_OS_WINDOWS)", indent=0)
cpp.include("tkit/core/windows.hpp", quotes=True)
cpp("#else", indent=0)
cpp('#error "[VULKIT] Unsupported platform to load Vulkan library"', indent=0)
cpp("#endif", indent=0)

with cpp.scope("namespace VKit::Vulkan", indent=0):

    cpp("#ifdef TKIT_ENABLE_ASSERTS", indent=0)
    with cpp.scope("template <typename T> static T validateFunction(const char *p_Name, T &&p_Function)"):
        cpp("TKIT_ASSERT(p_Function, \"The function '{}' is not available for the device being used.\", p_Name);")
        cpp("return p_Function;")
    cpp("#endif", indent=0)

    def codefn3(gen: CPPGenerator, fn: Function, /) -> None:
        gen(fn.as_fn_pointer_declaration(null=True))
        with gen.scope(fn.as_string(vk_prefix=False, semicolon=False)):

            pnames = [p.name for p in fn.params]

            def write_fn(name: str, /) -> None:
                if fn.return_type != "void":
                    gen(f"return {name}({', '.join(pnames)});")
                else:
                    gen(f"{name}({', '.join(pnames)});")

            gen("#ifdef TKIT_ENABLE_ASSERTS", indent=0)
            gen(f'static {fn.as_fn_pointer_type()} fn = validateFunction("{fn.name}", Vulkan::{fn.name});')
            write_fn("fn")
            gen("#else", indent=0)
            write_fn(f"Vulkan::{fn.name}")
            gen("#endif", indent=0)

    cpp.spacing()
    for fn in functions.values():
        if fn.is_instance_function() or fn.is_device_function():
            continue
        guards = fn.parse_guards()
        guard_if_needed(cpp, codefn3, guards, fn)

    cpp.spacing()
    cpp("#if defined(TKIT_OS_APPLE) || defined(TKIT_OS_LINUX)", indent=0)
    cpp("void Load(void *p_Library)")
    cpp("#else", indent=0)
    cpp("void Load(HMODULE p_Library)")
    cpp("#endif", indent=0)

    with cpp.scope():
        cpp("#if defined(TKIT_OS_APPLE) || defined(TKIT_OS_LINUX)", indent=0)
        cpp(
            'Vulkan::vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(p_Library, "vkGetInstanceProcAddr"));'
        )
        cpp("#else", indent=0)
        cpp(
            'Vulkan::vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(p_Library, "vkGetInstanceProcAddr"));'
        )
        cpp("#endif", indent=0)

        cpp.spacing()
        for fn in functions.values():
            if fn.name == "vkGetInstanceProcAddr" or fn.is_instance_function() or fn.is_device_function():
                continue
            guards = fn.parse_guards()
            guard_if_needed(
                cpp,
                f'Vulkan::{fn.name} = reinterpret_cast<{fn.as_fn_pointer_type()}>(GetInstanceProcAddr(VK_NULL_HANDLE, "{fn.name}"));',
                guards,
            )
    cpp.spacing()
    with cpp.scope("InstanceTable InstanceTable::Create(const VkInstance p_Instance)"):
        cpp("InstanceTable table{};")
        for fn in functions.values():
            if not fn.is_instance_function():
                continue
            guards = fn.parse_guards()
            guard_if_needed(
                cpp,
                f'table.{fn.name} = reinterpret_cast<{fn.as_fn_pointer_type()}>(GetInstanceProcAddr(p_Instance, "{fn.name}"));',
                guards,
            )
        cpp("return table;")

    cpp.spacing()
    with cpp.scope("DeviceTable DeviceTable::Create(const VkDevice p_Device, const InstanceTable &p_InstanceFuncs)"):
        cpp("DeviceTable table{};")
        for fn in functions.values():
            if not fn.is_device_function():
                continue
            guards = fn.parse_guards()
            guard_if_needed(
                cpp,
                f'table.{fn.name} = reinterpret_cast<{fn.as_fn_pointer_type()}>(p_InstanceFuncs.GetDeviceProcAddr(p_Device, "{fn.name}"));',
                guards,
            )
        cpp("return table;")

    def codefn4(gen: CPPGenerator, fn: Function, /, *, namespace: str) -> None:
        with gen.scope(
            fn.as_string(
                vk_prefix=False,
                semicolon=False,
                const=True,
                namespace=namespace,
            )
        ):

            pnames = [p.name for p in fn.params]

            def write_fn(name: str, /) -> None:
                if fn.return_type != "void":
                    gen(f"return {name}({', '.join(pnames)});")
                else:
                    gen(f"{name}({', '.join(pnames)});")

            gen("#ifdef TKIT_ENABLE_ASSERTS", indent=0)
            gen(f'static {fn.as_fn_pointer_type()} fn = validateFunction("{fn.name}", this->{fn.name});')
            write_fn("fn")
            gen("#else", indent=0)
            write_fn(f"this->{fn.name}")
            gen("#endif", indent=0)

    cpp.spacing()
    for fn in functions.values():
        if not fn.is_instance_function():
            continue
        guards = fn.parse_guards()
        guard_if_needed(
            cpp,
            codefn4,
            guards,
            fn,
            namespace="InstanceTable",
        )

    cpp.spacing()
    for fn in functions.values():
        if not fn.is_device_function():
            continue
        guards = fn.parse_guards()
        guard_if_needed(
            cpp,
            codefn4,
            guards,
            fn,
            namespace="DeviceTable",
        )

hpp.write(output / "loader.hpp")
cpp.write(output / "loader.cpp")

tmpath: Path | None = args.export_timeline
if tmpath is None:
    Convoy.exit_ok()

tmpath = tmpath.resolve()

with open(tmpath, "w") as f:
    for fn in functions.values():
        guards = fn.parse_guards()
        if guards:
            f.write(f"{fn.name} -> {guards}\n")
        else:
            f.write(f"{fn.name} - No requirements\n")

    for tp in types.values():
        guards = tp.parse_guards()
        if guards:
            f.write(f"{tp.name} -> {guards}\n")
        else:
            f.write(f"{tp.name} - No requirements\n")

Convoy.exit_ok()
