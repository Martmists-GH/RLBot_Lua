import glob
import sys

from setuptools import setup, Extension

if "develop" in sys.argv:
    args = ["-g3"]
else:
    args = ["-O3"]

lua_files = [
    'lapi.c',
    'lcode.c',
    'lctype.c',
    'ldebug.c',
    'ldo.c',
    'ldump.c',
    'lfunc.c',
    'lgc.c',
    'llex.c',
    'lmem.c',
    'lobject.c',
    'lopcodes.c',
    'lparser.c',
    'lstate.c',
    'lstring.c',
    'ltable.c',
    'ltm.c',
    'lundump.c',
    'lvm.c',
    'lzio.c',
    'ltests.c',
    'lauxlib.c',
    'lbaselib.c',
    'ldblib.c',
    'liolib.c',
    'lmathlib.c',
    'loslib.c',
    'ltablib.c',
    'lstrlib.c',
    'lutf8lib.c',
    'lbitlib.c',
    'loadlib.c',
    'lcorolib.c',
    'linit.c',
]

setup(
    name="rlbot_lua",
    ext_modules=[
        Extension('rlbot_lua',
                  sources=glob.glob("src/*.cpp"),
                  include_dirs=["lib/lua"],
                  extra_compile_args=args)
    ],
    ext_libraries=[
        ['lua', {
            "sources": ["lib/lua/"+src for src in lua_files],
            "include_dirs": ["lib/lua"]
        }]
    ]
)
