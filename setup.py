import glob
import sys

from setuptools import setup, Extension

if "develop" in sys.argv:
    args = ["-g3"]
else:
    args = ["-O3"]

setup(
    name="rlbot_lua",
    version="0.0.1",
    ext_modules=[
        Extension('rlbot_lua',
                  sources=glob.glob("src/*.cpp"),
                  include_dirs=["lib/lua", "src/"],
                  libraries=["lua"],
                  extra_compile_args=args)
    ]
)
