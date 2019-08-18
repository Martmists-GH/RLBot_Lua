function dump(o, r, indent, n, noprint)
    local r = r or false
    local indent = indent or 4
    local n = n or 0
    local noprint = noprint or false

    local s = "{\n"
    for k, v in pairs(o) do
        local val
        if type(v) == "table" and ((r and v.__name ~= nil) or k == "__identity") then
            val = dump(v, r, indent, n+1)
        elseif type(v) == "string" then
            val = ('%q'):format(v)
        else
            val = tostring(v)
        end
        s = s .. (" "):rep(indent*(1+n)) .. k .. ": " .. val .. ",\n"
    end
    s = s .. (" "):rep(indent*n) .. "}"
    if n == 0 and not noprint then
        print(s)
    end
    return s
end

local function create_instance(_class)
    local _obj = {
        __class=_class
    }

    local _obj_meta = {}
    setmetatable(_obj, _obj_meta)
    local _addr = tostring(_obj):sub(8)  -- Doing this inside __tostring would cause recursion
    _obj_meta.__tostring = function() return ("<object %q at %s>"):format(_class.__name, _addr) end

    return function(_, ...)
        for k, v in pairs(_class.__identity) do
            if k:sub(1, 2) == "__" then
                _obj_meta[k] = v
            end
            _obj[k] = v
        end

        if _obj.__ctr == nil then
            _obj.__ctr = function(self) end
        end

        _obj:__ctr(...)

        return _obj
    end
end

local function create_class(_typedef)
    local _class = {
        __name = _typedef.__name,
        __parent = nil,
        __identity = nil
    }

    if (_typedef.__parent ~= nil) then
        _class.__parent = _G[_typedef.__parent]
    end

    local _class_meta = {}
    setmetatable(_class, _class_meta)
    _class_meta.__call = function(...) return create_instance(_class)(...) end
    _class_meta.__tostring = function() return ("<class %q>"):format(_typedef.__name) end

    return function(_, tab)
        _class.__identity = tab
        _G[_class.__name] = _class
        _class_meta.__newindex = function(_, key, val) _class.__identity[key] = val end
        _class_meta.__index = function(_, key) return _class.__identity[key] end
        return _class
    end
end

function class(cname)
    local _typedef = {
        __name = cname,
        __parent = nil
    }

    local _typedef_meta = {}
    setmetatable(_typedef, _typedef_meta)
    _typedef_meta.__call = create_class(_typedef)
    _typedef_meta.__tostring = function() return ("<typedef %q>"):format(cname) end

    function _typedef:extends(parent)
        _typedef.__parent = parent
        _typedef.extends = nil

        -- Redo this because of how create_class works
        _typedef_meta.__call = create_class(_typedef)
        return _typedef
    end

    return _typedef
end

function super(self)
    -- TODO: Clean up this function

    local parent = self.__class.__parent
    if parent == nil then
        error("No parent class to call super on")
    end

    local _obj_as_parent = {
        __class = parent
    }
    for k, v in pairs(parent.__identity) do
        if type(v) == 'function' then
            _obj_as_parent[k] = v
        end
    end
    setmetatable(_obj_as_parent, {__index=self, __newindex=self, __tostring=getmetatable(self).__tostring})

    local _super = {}
    return setmetatable(_super, {
        __index=function(_, attr)
            if _ == _super then
                return setmetatable({}, {
                    __call=function(_, __, ...)
                        return parent.__identity[attr](_obj_as_parent, ...)
                    end
                })
            end
            return parent.__identity[attr]
        end,
        __tostring=function()
            return ("<super of %s>"):format(tostring(self))
        end
    })
end

return class, super, dump
