import os
import sys

# Load DLL and .lua scripts from this folder
os.chdir(os.path.dirname(os.path.abspath(__file__)))

from rlbot.agents.base_agent import BaseAgent, SimpleControllerState
from rlbot.utils.structures.game_data_struct import GameTickPacket

from rlbot_lua import LuaBot


class LuaAgent(BaseAgent):
    def initialize_agent(self):
        print("Init called!")
        self.lua_bot = LuaBot(self.index)
        print("Init finished!")

    def get_output(self, game_tick_packet: GameTickPacket) -> SimpleControllerState:
        args = self.lua_bot.get_output(game_tick_packet)
        return SimpleControllerState(*args)
