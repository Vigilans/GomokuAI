import sys
import os

os_map = {
    "win32": "Windows_NT",
    "linux": "Linux"
}

is64bit_map = {
    True: "x64",
    False: "x86"
}

# Import path dectection
os_name  = os_map[sys.platform]
platform = is64bit_map[sys.maxsize > 2**32]
config   = "Release"

module_path = os.path.join(
    os.path.dirname(__file__), os_name, platform, config
)

# Add core module to sys path for import
sys.path.insert(0, module_path)
