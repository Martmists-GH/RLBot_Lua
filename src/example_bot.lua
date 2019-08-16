function get_car_facing_vector(car)
    local pitch = float(car.rotation.pitch)
    local yaw = float(car.rotation.yaw)

    local facing_x = math.cos(pitch) * math.cos(yaw)
    local facing_y = math.cos(pitch) * math.sin(yaw)

    return Vector(facing_x, facing_y)
end

function Vector:correction_to(self, ideal)
    local current_rad = math.atan2(self.y, -self.x)
    local ideal_rad = math.atan2(ideal.y, -ideal.x)

    local correction = ideal_rad - current_rad

    if abs(correction) > math.pi then
        if correction > 0 then
            correction = correction - 2*math.pi
        else
            correction = correction + 2*math.pi
        end
    end

    return correction
end

class "MyBot" : extends "LuaBot" {
    bot_init = function(self, index)
        super(self):bot_init(index)
        self.controller_state = ControllerState()
    end,

    get_output = function(self, packet)
        local ball_location = packet.game_ball.location
        local my_car = packet.game_cars[self.index]
        local car_location = my_car.location

        local car_to_ball = ball_location - car_location
        local car_direction = get_car_facing_vector(my_car)

        local steer_correction_rad = car_direction:correction_to(car_to_ball)
        if steer_correction_rad > 0 then
            turn = -1
        else
            turn = 1
        end

        self.controller_state.throttle = 1
        self.controller_state.steer = turn

        return self.controller_state
    end
}

return MyBot()
