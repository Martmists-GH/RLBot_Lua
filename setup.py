import glob
import sys

from setuptools import setup, Extension

if sys.platform == "win32":
    libs = ["lua53"]
else:
    libs = ["lua"]


with open("README.md") as f:
    long_description = f.read()


setup(
    name="rlbot_lua",
    version="0.1.1",
    description="Lua wrapper for RLBot",
    long_description=long_description,
    url="https://github.com/martmists/RLBot_Lua",
    author="Martmists",
    author_email="mail@martmists.com",
    license="MIT",
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
        'Programming Language :: Python :: 3 :: Only',
        'Programming Language :: Python :: Implementation :: CPython',
        'Topic :: Games/Entertainment',
        'Topic :: Software Development :: Libraries :: Python Modules'
    ],
    ext_modules=[
        Extension('rlbot_lua',
                  sources=glob.glob("src/*.cpp"),
                  include_dirs=["lib/lua", "src/"],
                  libraries=libs)
    ],
    install_requires=[
        "rlbot"
    ],
    keywords=["rlbot", "lua", "cpp", "extension"],
)
