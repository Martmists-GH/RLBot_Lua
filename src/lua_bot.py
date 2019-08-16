from rlbot.agents.base_agent import BaseAgent, SimpleControllerState
from rlbot.utils.structures.game_data_struct import GameTickPacket

from rlbot_lua import LuaBot


class LuaAgent(BaseAgent):
    def initialize_agent(self):
        self.lua_bot = LuaBot(self.index)

    def get_output(self, game_tick_packet: GameTickPacket) -> SimpleControllerState:
        args = self.lua_bot.get_output(game_tick_packet)
        return SimpleControllerState(*args)
