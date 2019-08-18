class "Vector" {
    __tostring = function(self)
        return ("Vector([%.2f, %.2f, %.2f])"):format(self.x, self.y, self.z)
    end,

    __ctr = function(self, x, y, z)
        if type(x) == "table" then
            y = x.y or x[2]
            z = x.z or x[3]
            x = x.x or x[1]
        end
        self.x = x or 0
        self.y = y or 0
        self.z = z or 0
    end,

    -- Magic functions --

    __unm = function(self)
        return self * -1
    end,

    __add = function(self, other)
        return Vector(self.x + other.x, self.y + other.y, self.z + other.z)
    end,

    __sub = function(self, other)
        return Vector(self.x - other.x, self.y - other.y, self.z - other.z)
    end,

    __mul = function(self, other)
        return Vector(self.x * other, self.y * other, self.z * other)
    end,

    __div = function(self, other)
        return self * (1/other)
    end,

    __eq = function(self, other)
        return self.x == other.x and self.y == other.y and self.z == other.z
    end,

    -- Useful methods --

    normalized = function(self)
        if self:length() == 0 then
            return Vector(1, 1, 1)
        end
        return self / self:length()
    end,

    length = function(self)
        return math.sqrt(self.x*self.x + self.y*self.y + self.z*self.z)
    end,

    flat = function(self)
        return Vector(self.x, self.y, 0)
    end,

    rescale = function(self, length)
        return self:normalized() * length
    end
}

class "Rotation" {
    __ctr = function(self, pitch, yaw, roll)
        if type(pitch) == "table" then
            roll  = pitch.roll  or pitch[3]
            yaw   = pitch.yaw   or pitch[2]
            pitch = pitch.pitch or pitch[1]
        end
        self.pitch = pitch or 0
        self.yaw   = yaw   or 0
        self.roll  = roll  or 0
    end
}

class "Hitbox" {
    __ctr = function(self, hitbox)
        self.length = hitbox.length
        self.width = hitbox.width
        self.height = hitbox.height
    end
}

class "GameObject" {
    __ctr = function(self, physics)
        self.location = Vector(physics.location)
        self.velocity = Vector(physics.velocity)
        self.rotation = Rotation(physics.rotation)
        self.angular_velocity = Vector(physics.angular_velocity)
    end
}

class "GameCar" : extends "GameObject" {
    __ctr = function(self, car)
        super(self):__ctr(car.physics)
        self.is_demolished = car.is_demolished
        self.has_wheel_contact = car.has_wheel_contact
        self.is_super_sonic = car.is_super_sonic
        self.is_bot = car.is_bot
        self.jumped = car.jumped
        self.double_jumped = car.double_jumped
        self.name = car.name
        self.team = car.team
        self.boost = car.boost
        self.hitbox = Hitbox(car.hitbox)
    end
}

class "GameBoost" {
    __ctr = function(self, boost)
        self.is_active = boost.is_active
        self.timer = boost.timer
    end
}

class "Team" {
    __ctr = function(self, team)
        self.team_index = team.team_index
        self.score = team.score
    end
}

class "GameBall" : extends "GameObject" {
    __ctr = function(self, ball)
        super(self):__ctr(ball.physics)
        -- TODO
        self.latest_touch = ball.latest_touch
        self.drop_shot_info = ball.drop_shot_info
        self.collision_shape = ball.collision_shape
    end
}

class "GameInfo" {
    __ctr = function(self, info)
        self.seconds_elapsed = info.seconds_elapsed
        self.game_time_remaining = info.game_time_remaining
        self.is_overtime = info.is_overtime
        self.is_unlimited_time = info.is_unlimited_time
        self.is_round_active = info.is_round_active
        self.is_kickoff_pause = info.is_kickoff_pause
        self.is_match_ended = info.is_match_ended
        self.world_gravity_z = info.world_gravity_z
        self.game_speed = info.game_speed
    end
}

class "GameTickPacket" {
    __ctr = function(self, packet)
        self.game_cars = {}
        self.num_cars = packet.num_cars
        for i, car in ipairs(packet.game_cars) do
            self.game_cars[i] = GameCar(car)
        end
        self.game_boosts = {}
        self.num_boost = packet.num_boost
        for i, boost in ipairs(packet.game_boosts) do
            self.game_boosts[i] = GameBoost(boost)
        end
        self.teams = {}
        self.num_teams = packet.num_teams
        for i, team in ipairs(packet.teams) do
            self.teams[i] = Team(team)
        end
        self.game_ball = GameBall(packet.game_ball)
        self.game_info = GameInfo(packet.game_info)
    end
}

class "BallPredictionSlice" : extends "GameObject" {
    __ctr = function(self, slice)
        super(self):__ctr(slice)
        self.game_seconds = slice.game_seconds
    end
}

class "BallPrediction" {
    __ctr = function(self, prediction)
        self.num_slices = prediction.num_slices
        self.slices = {}
        for i, slice in ipairs(prediction.slices) do
            self.slices[i] = BallPredictionSlice(slice)
        end
    end
}

class "ControllerState" {
    __ctr = function(self, throttle, steer, pitch, yaw, roll, jump, boost, handbrake, use_item)
        self.throttle = throttle or 0
        self.steer = steer or 0
        self.pitch = pitch or 0
        self.yaw = yaw or 0
        self.roll = roll or 0
        self.jump = jump or false
        self.boost = boost or false
        self.handbrake = handbrake or false
        self.use_item = use_item or false
    end
}

class "LuaBot" {
    bot_init = function(self, index)
        self.index = index
    end,

    get_output = function(self, packet)
        return ControllerState()
    end
}