# RLBot_Lua

This extension creates a native bridge between Python and Lua, implementing its own class system to simplify usage.

## Installation

```
$ python -m pip install rlbot_lua
``` 

> Note: Only supports 64-bit OS/Python

## Bot setup
Copy the following files to your bot folder:

- Lua53.dll (Not needed on Linux when support arrives)
- classes.lua
- structs.lua
- lua_bot.py

Then, in your bot.cfg, set bot path to `lua_bot.py`

## Functions provided

Functions:
- `class` - A keyword to create classes (see bot_example.lua and structs.lua for reference)
- `super` - A function useful with inheritance; calls the parent function (see structs.lua for reference)
- `dump` - A function that can be used to dump information about a table

Classes:
- `GameTickPacket` - The game tick packet, this completely copies the python packet
- `GameObject` - Base class for all entities in the game, this should generally not be used
- `GameCar` - The class used for cars in packet.game_cars
- `GameBall` - The class used for the ball
- `GameInfo` - The class used to hold information about the current game
- `GameBoost` - The class used for boost pads in packet.game_boosts
- `Team` - The class used for team information
- `Hitbox` - Container class for hitbox data
- `ControllerState` - The class used to hold controller data, defaults to neutral
- `LuaBot` - The class a bot written in Lua must inherit and implement
- `Vector` - The class used for all 3-dimensional vectors, has utility methods
- `Rotation` - The class used for rotation data

These classes can be modified as shown in example_bot.lua

## TODO

- Proper classes for Ball attributes
- FieldInfo
- BallPrediction
- Figure out why it segfaults when there are more than 2 players on the field
