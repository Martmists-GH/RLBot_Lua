//
// Created by martmists on 8/15/19.
//

extern "C" {
    #include <Python.h>
    #include <cstdio>
    #include <cstring>
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

#include <iostream>

static void stackDump (lua_State *L, bool verbose=false) {
    int i;
    int top = lua_gettop(L);
    for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {

            case LUA_TSTRING:  /* strings */
                printf("`%s'", lua_tostring(L, i));
                break;

            case LUA_TBOOLEAN:  /* booleans */
                printf(lua_toboolean(L, i) ? "true" : "false");
                break;

            case LUA_TNUMBER:  /* numbers */
                printf("%g", lua_tonumber(L, i));
                break;

            case LUA_TTABLE:
                if (verbose) {
                    lua_getglobal(L, "dump");
                    lua_pushvalue(L, i);
                    lua_call(L, 1, 0);
                } else {
                    printf("%s", lua_typename(L, t));
                }

                break;

            default:  /* other values */
                printf("%s", lua_typename(L, t));
                break;

        }
        printf("  ");  /* put a separator */
    }
    printf("\n");  /* end the listing */
}

int PyModule_AddType(PyObject *module, const char *name, PyTypeObject *type) {
    if (PyType_Ready(type)) {
        return -1;
    }
    Py_INCREF(type);
    if (PyModule_AddObject(module, name, (PyObject *)type)) {
        Py_DECREF(type);
        return -1;
    }
    return 0;
}

void run_file(lua_State *L, char* filename, int ret){
    if (luaL_loadfile(L, filename) || lua_pcall(L, 0, ret, 0))
        luaL_error(L, "cannot load required file: %s", lua_tostring(L, -1));
}

void getInt(lua_State* L, PyObject* parent, char* name){
    PyObject* prop = PyObject_GetAttrString(parent, name);
    long x = PyLong_AsLong(prop);
    Py_DECREF(prop);
    lua_pushnumber(L, x);
    lua_setfield(L, -2, name);
}

void getDouble(lua_State* L, PyObject* parent, char* name){
    PyObject* prop = PyObject_GetAttrString(parent, name);
    double x = PyFloat_AsDouble(prop);
    Py_DECREF(prop);
    lua_pushnumber(L, x);
    lua_setfield(L, -2, name);
}

void getBool(lua_State* L, PyObject* parent, char* name){
    PyObject* prop = PyObject_GetAttrString(parent, name);
    bool x = PyObject_IsTrue(prop);
    Py_DECREF(prop);
    lua_pushboolean(L, x);
    lua_setfield(L, -2, name);
}

void getString(lua_State* L, PyObject* parent, char* name){
    PyObject* prop = PyObject_GetAttrString(parent, name);
    const char* x = PyUnicode_AsUTF8(prop);
    Py_DECREF(prop);
    lua_pushstring(L, x);
    lua_setfield(L, -2, name);
}

void getVector(lua_State* L, PyObject* parent, char* name){
    lua_newtable(L);
    PyObject* vec = PyObject_GetAttrString(parent, name);
    getDouble(L, vec, (char*)"x");
    getDouble(L, vec, (char*)"y");
    getDouble(L, vec, (char*)"z");
    Py_DECREF(vec);
    lua_setfield(L, -2, name);
}

void getRotation(lua_State* L, PyObject* parent, char* name){
    lua_newtable(L);
    PyObject* vec = PyObject_GetAttrString(parent, name);
    getDouble(L, vec, (char*)"pitch");
    getDouble(L, vec, (char*)"yaw");
    getDouble(L, vec, (char*)"roll");
    Py_DECREF(vec);
    lua_setfield(L, -2, name);
}

struct LuaAgent {
    PyObject_HEAD
    PyObject* bot;
    lua_State *L;
};

static int getBallPrediction(lua_State *L){
    // TODO: Use Ball Prediction DLL?

    // Stack: [..., Bot]
    lua_getfield(L, -1, "___agentptr");
    // Stack: [..., Bot, <agent>]
    auto agent = (LuaAgent*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    // Stack: [..., Bot]
    PyObject* ball_pred_struct = PyObject_CallMethod(agent->bot, "get_ball_prediction_struct", nullptr);
    lua_newtable(L);
    // Stack: [..., Bot, {table ball_prediction}]

    PyObject* num_slices = PyObject_GetAttrString(ball_pred_struct, "num_slices");
    long num_slices_i = PyLong_AsLong(num_slices);
    lua_pushinteger(L, num_slices_i);
    // Stack: [..., Bot, {table ball_prediction}, num_slices]
    lua_setfield(L, -2, "num_slices");
    // Stack: [..., Bot, {table ball_prediction}]

    PyObject* slices = PyObject_GetAttrString(ball_pred_struct, "slices");

    lua_newtable(L);
    // Stack: [..., Bot, {table ball_prediction}, {table slices}]
    for (long i = 0; i < num_slices_i; i++){
        PyObject* index = PyLong_FromLong(i);
        PyObject* slice = PyObject_GetItem(slices, index);
        Py_DECREF(index);
        lua_newtable(L);

        PyObject* physics = PyObject_GetAttrString(slice, "physics");
        // Stack: [..., Bot, {table ball_prediction}, {table slices}, {table slice}]
        getVector(L, physics, (char*)"location");
        getVector(L, physics, (char*)"velocity");
        getVector(L, physics, (char*)"angular_velocity");
        getRotation(L, physics, (char*)"rotation");
        Py_DECREF(physics);

        getDouble(L, slice, (char*)"game_seconds");

        Py_DECREF(slice);
        lua_rawseti(L, -2, i+1);
        // Stack: [..., Bot, {table ball_prediction}, {table slices}]
    }
    // Stack: [..., Bot, {table ball_prediction}, {table slices}]
    lua_setfield(L, -2, "slices");
    // Stack: [..., Bot, {table ball_prediction}]

    Py_DECREF(slices);
    Py_DECREF(ball_pred_struct);

    lua_getglobal(L, "BallPrediction");
    lua_insert(L, -2);
    lua_call(L, 1, 1);

    return 1;
}

lua_State* createAgent(LuaAgent* agent, int index){
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    // Add _G to the stack
    lua_getglobal(L, "_G");

    // Register `class` and `super`
    run_file(L, (char*)"classes.lua", 3);
    lua_setfield(L, 1, "dump");
    lua_setfield(L, 1, "super");
    lua_setfield(L, 1, "class");
    lua_settop(L, 0);

    // Register structs
    run_file(L, (char*)"structs.lua", 0);
    // Load bot onto the stack
    // THIS FILE MUST RETURN AN INSTANCE OR IT WONT WORK!
    run_file(L, (char*)"bot.lua", 1);
    lua_pushlightuserdata(L, agent);
    lua_setfield(L, -2, "___agentptr");
    lua_pushvalue(L, -1);

    lua_pushcfunction(L, getBallPrediction);
    lua_setfield(L, -2, "get_ball_prediction");
    // Add methods

    // Call bot_init
    lua_getfield(L, -1, "bot_init");
    lua_insert(L, -2);
    lua_pushnumber(L, index+1);
    lua_call(L, 2, 0);

    // Pop _G from the stack
    return L;
}

void createLuaPacket(lua_State *L, PyObject* packet){
    // Get _G for the classes
    // stack: [Bot, <function get_output>, Bot]
    lua_getglobal(L, "_G");
    // stack: [Bot, <function get_output>, Bot, _G]
    lua_getfield(L, -1, "GameTickPacket");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>]
    lua_newtable(L);

    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}]
    PyObject* num_cars = PyObject_GetAttrString(packet, "num_cars");
    long num_cars_i = PyLong_AsLong(num_cars);
    lua_pushinteger(L, num_cars_i);
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {int num_cars}]
    lua_setfield(L, -2, "num_cars");
    Py_DECREF(num_cars);

    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}]
    PyObject* cars = PyObject_GetAttrString(packet, "game_cars");
    lua_newtable(L);
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_cars}]
    for (long i = 0; i < num_cars_i; i++){
        // Create car
        PyObject* index = PyLong_FromLong(i);
        PyObject* car = PyObject_GetItem(cars, index);
        Py_DECREF(index);
        lua_newtable(L);

        // Physics
        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_cars}, {table car_n}]
        PyObject* physics = PyObject_GetAttrString(car, "physics");
        lua_newtable(L);
        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_cars}, {table car_n}, {table car_n_physics}]
        char* physics_keys[] = {(char*)"location", (char*)"velocity", (char*)"angular_velocity", (char*)"rotation"};

        for (char* physics_key : physics_keys) {
            if (strcmp(physics_key, "rotation") == 0){
                getRotation(L, physics, physics_key);
            } else {
                getVector(L, physics, physics_key);
            }
        }
        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_cars}, {table car_n}, {table car_n_physics}]
        lua_setfield(L, -2, "physics");
        Py_DECREF(physics);

        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_cars}, {table car_n}]
        char* bool_keys[] = {
                (char*)"is_demolished",
                (char*)"has_wheel_contact",
                (char*)"is_super_sonic",
                (char*)"is_bot",
                (char*)"jumped",
                (char*)"double_jumped"
        };
        for (char* key : bool_keys){
            getBool(L, car, key);
        }

        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_cars}, {table car_n}]
        getString(L, car, (char*)"name");
        getInt(L, car, (char*)"team");
        getDouble(L, car, (char*)"boost");

        lua_newtable(L);
        PyObject* hitbox = PyObject_GetAttrString(car, "hitbox");
        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_cars}, {table car_n}, {table hitbox}]
        getInt(L, hitbox, (char*)"length");
        getInt(L, hitbox, (char*)"width");
        getInt(L, hitbox, (char*)"height");

        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_cars}, {table car_n}, {table hitbox}]
        lua_setfield(L, -2, "hitbox");
        Py_DECREF(hitbox);

        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_cars}, {table car_n}]
        Py_DECREF(car);
        lua_rawseti(L, -2, i+1);
        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_cars}]
    }
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_cars}]
    lua_setfield(L, -2, "game_cars");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}]

    PyObject* num_boost = PyObject_GetAttrString(packet, "num_boost");
    long num_boost_i = PyLong_AsLong(num_boost);
    lua_pushinteger(L, num_boost_i);
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {int num_boost}]
    lua_setfield(L, -2, "num_boost");
    Py_DECREF(num_boost);

    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}]
    lua_newtable(L);
    PyObject* boosts = PyObject_GetAttrString(packet, (char*)"game_boosts");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table boosts}]
    for (long i = 0; i < num_boost_i; i++) {
        PyObject* index = PyLong_FromLong(i);
        PyObject* boost_obj = PyObject_GetItem(boosts, index);
        Py_DECREF(index);
        lua_newtable(L);
        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table boosts}, {table boost_<i>}]
        getBool(L, boost_obj, (char*)"is_active");
        getDouble(L, boost_obj, (char*)"timer");
        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table boosts}, {table boost_<i>}]
        lua_rawseti(L, -2, i+1);
        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table boosts}]
        Py_DECREF(boost_obj);
    }
    Py_DECREF(boosts);

    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table boosts}]
    lua_setfield(L, -2, "game_boosts");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}]

    lua_newtable(L);
    PyObject* ball = PyObject_GetAttrString(packet, (char*)"game_ball");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}]
    PyObject* physics = PyObject_GetAttrString(ball, "physics");
    lua_newtable(L);
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table physics}]
    char* physics_keys[] = {(char*)"location", (char*)"velocity", (char*)"angular_velocity", (char*)"rotation"};

    for (char* physics_key : physics_keys) {
        if (strcmp(physics_key, "rotation") == 0){
            getRotation(L, physics, physics_key);
        } else {
            getVector(L, physics, physics_key);
        }
    }
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table physics}]
    lua_setfield(L, -2, "physics");
    Py_DECREF(physics);
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}]

    lua_newtable(L);
    PyObject* last_touch = PyObject_GetAttrString(ball, "latest_touch");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table last_touch}]
    getString(L, last_touch, (char*)"player_name");
    getDouble(L, last_touch, (char*)"time_seconds");
    getInt(L, last_touch, (char*)"team");
    getInt(L, last_touch, (char*)"player_index");
    getVector(L, last_touch, (char*)"hit_location");
    getVector(L, last_touch, (char*)"hit_normal");

    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table last_touch}]
    Py_DECREF(last_touch);
    lua_setfield(L, -2, "latest_touch");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}]

    lua_newtable(L);
    PyObject* dropshot_info = PyObject_GetAttrString(ball, "drop_shot_info");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table dropshot}]
    getInt(L, dropshot_info, (char*)"damage_index");
    getInt(L, dropshot_info, (char*)"absorbed_force");
    getInt(L, dropshot_info, (char*)"force_accum_recent");
    Py_DECREF(dropshot_info);
    lua_setfield(L, -2, "drop_shot_info");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}]

    lua_newtable(L);
    PyObject* collision = PyObject_GetAttrString(ball, "collision_shape");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table collision}]
    getInt(L, collision, (char*)"type");

    lua_newtable(L);
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table collision}, {table box}]
    PyObject* box = PyObject_GetAttrString(collision, (char*)"box");
    getDouble(L, box, (char*)"length");
    getDouble(L, box, (char*)"width");
    getDouble(L, box, (char*)"height");
    Py_DECREF(box);
    lua_setfield(L, -2, "box");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table collision}]

    lua_newtable(L);
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table collision}, {table sphere}]
    PyObject* sphere = PyObject_GetAttrString(collision, (char*)"sphere");
    getDouble(L, sphere, (char*)"diameter");
    Py_DECREF(sphere);
    lua_setfield(L, -2, "sphere");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table collision}]

    lua_newtable(L);
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table collision}, {table cylinder}]
    PyObject* cylinder = PyObject_GetAttrString(collision, (char*)"cylinder");
    getDouble(L, cylinder, (char*)"diameter");
    getDouble(L, cylinder, (char*)"height");
    Py_DECREF(cylinder);
    lua_setfield(L, -2, "cylinder");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}, {table collision}]
    Py_DECREF(collision);
    lua_setfield(L, -2, "collision_shape");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table ball}]

    lua_setfield(L, -2, "game_ball");
    Py_DECREF(ball);
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}]

    lua_newtable(L);
    PyObject* game_info = PyObject_GetAttrString(packet, (char*)"game_info");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table game_info}]
    getDouble(L, game_info, (char*)"seconds_elapsed");
    getDouble(L, game_info, (char*)"game_time_remaining");
    getDouble(L, game_info, (char*)"world_gravity_z");
    getDouble(L, game_info, (char*)"game_speed");
    getBool(L, game_info, (char*)"is_overtime");
    getBool(L, game_info, (char*)"is_unlimited_time");
    getBool(L, game_info, (char*)"is_round_active");
    getBool(L, game_info, (char*)"is_kickoff_pause");
    getBool(L, game_info, (char*)"is_match_ended");

    Py_DECREF(game_info);
    lua_setfield(L, -2, "game_info");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}]

    PyObject* num_teams = PyObject_GetAttrString(packet, "num_teams");
    long num_teams_i = PyLong_AsLong(num_cars);
    lua_pushinteger(L, num_teams_i);
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {int num_teams}]
    lua_setfield(L, -2, "num_teams");
    Py_DECREF(num_teams);

    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}]
    lua_newtable(L);
    PyObject* teams = PyObject_GetAttrString(packet, "teams");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table teams}]

    for (long i = 0; i < num_teams_i; i++){
        PyObject* index = PyLong_FromLong(i);
        PyObject* team = PyObject_GetItem(teams, index);
        Py_DECREF(index);
        lua_newtable(L);
        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table teams}, {table team_n}]

        getInt(L, team, (char*)"team_index");
        getInt(L, team, (char*)"score");
        lua_rawseti(L, -2, i+1);
        // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table teams}]
        Py_DECREF(team);
    }
    Py_DECREF(teams);
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}, {table teams}]
    lua_setfield(L, -2, "teams");
    // stack: [Bot, <function get_output>, Bot, <class GameTickPacket>, {table packet}]

    // Packet is now on top the stack

    lua_call(L, 1, 1);
    // stack: [Bot, <function get_output>, Bot, _G, <object GameTickPacket>]
    // Remove _G again, we need to move it to the top first
    lua_insert(L, -2);
    lua_pop(L, 1);
    // stack: [Bot, <function get_output>, Bot, <object GameTickPacket>]
}

PyObject* runAgent(LuaAgent* agent, PyObject* packet) {
    lua_State *L = agent->L;

    // Stack: [Bot]
    // Create function name
    lua_pushvalue(L, -1);
    lua_getfield(L, -1, "get_output");
    // add Bot as argument
    lua_insert(L, -2);

    // Parse and prepare packet
    createLuaPacket(L, packet);
    // stack: [Bot, <function get_output>, Bot, <object GameTickPacket>]

    // Call function, puts controller state to the stack
    int res = lua_pcall(L, 2, 1, 0);
    if (res != 0) {
        PyErr_SetString(PyExc_RuntimeError, lua_tostring(L, -1));
        lua_settop(L, 1);
        return nullptr;
    }
    // stack: [Bot, <object ControllerState>]

    // Get properties from controller state
    lua_getfield(L, -1, "steer");
    double steer = lua_tonumber(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "throttle");
    double throttle = lua_tonumber(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "pitch");
    double pitch = lua_tonumber(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "yaw");
    double yaw = lua_tonumber(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "roll");
    double roll = lua_tonumber(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "jump");
    bool jump = lua_toboolean(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "boost");
    bool boost = lua_toboolean(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "handbrake");
    bool handbrake = lua_toboolean(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "use_item");
    bool use_item = lua_toboolean(L, -1);
    lua_pop(L, 1);

    PyObject* ret = Py_BuildValue("dddddhhhh", steer, throttle, pitch, yaw, roll, jump, boost, handbrake, use_item);
    // Pop controller state
    lua_pop(L, 1);
    // stack: [Bot]
    return ret;
}

static int Agent_tp_init(PyObject *_self, PyObject *args, PyObject *kwargs) {
    auto* self = (LuaAgent*)_self;

    int index;
    PyObject* bot = nullptr;
    char* kwlist[] = {(char*)"bot", (char*)"index", nullptr};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi:__init__", kwlist, &bot, &index)) {
        return -1;
    }

    self->bot = bot;
    self->L = createAgent(self, index);
    return 0;
}

PyObject* Agent_GetOutput(PyObject *_self, PyObject *args, PyObject *kwargs){
    auto* self = (LuaAgent*)_self;

    PyObject* packet = nullptr;
    char* kwlist[] = {(char*)"packet", nullptr};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:get_output", kwlist, &packet)) {
        PyErr_SetString(PyExc_TypeError, "Unable to call GetOutput");
        return nullptr;
    }

    return runAgent(self, packet);
}

static int Agent_tp_clear(PyObject *self) {
    return 0;
}

static void Agent_tp_dealloc(PyObject *self) {
    Agent_tp_clear(self);
    Py_TYPE(self)->tp_free(self);
}

PyMethodDef Agent_Methods[] = {
        {"get_output", (PyCFunction) Agent_GetOutput, METH_VARARGS | METH_KEYWORDS, "Returns a controller state from a GTP"},
        {nullptr}
};

static PyTypeObject PyType_LuaBot = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "rlbot_lua.LuaBot",                                    /* tp_name */
    sizeof(LuaAgent),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor) Agent_tp_dealloc,                              /* tp_dealloc */
    nullptr,                                          /* tp_vectorcall_offset */
    nullptr,                                          /* tp_getattr */
    nullptr,                                          /* tp_setattr */
    nullptr,                                          /* tp_as_async */
    nullptr,                                 /* tp_repr */
    nullptr,                                          /* tp_as_number */
    nullptr,                                          /* tp_as_sequence */
    nullptr,                                          /* tp_as_mapping */
    nullptr,                                          /* tp_hash */
    nullptr,                                          /* tp_call */
    nullptr,                                          /* tp_str */
    nullptr,                             /* tp_getattro */
    nullptr,                                          /* tp_setattro */
    nullptr,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                    /* tp_flags */
    nullptr,                                  /* tp_doc */
    nullptr,                             /* tp_traverse */
    (inquiry) Agent_tp_clear,                                          /* tp_clear */
    nullptr,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    nullptr,                                          /* tp_iter */
    nullptr,                                          /* tp_iternext */
    Agent_Methods,                                          /* tp_methods */
    nullptr,                              /* tp_members */
    nullptr,                                          /* tp_getset */
    nullptr,                                          /* tp_base */
    nullptr,                                          /* tp_dict */
    nullptr,                            /* tp_descr_get */
    nullptr,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc) Agent_tp_init,                                 /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
};

PyObject* RLBot_Lua__module = nullptr;

PyMODINIT_FUNC PyInit_rlbot_lua(){
    static struct PyModuleDef moduledef = {
            PyModuleDef_HEAD_INIT,
            "rlbot_lua",     /* m_name */
            "Lua bridge for rlbot",  /* m_doc */
            -1,                  /* m_size */
            nullptr,   /* m_methods */
            nullptr,                /* m_reload */
            nullptr,                /* m_traverse */
            nullptr,                /* m_clear */
            nullptr,                /* m_free */
    };

    RLBot_Lua__module = PyModule_Create(&moduledef);

    if (RLBot_Lua__module == nullptr){
        std::cerr << "Error loading RLBot Lua-C module.";
        return nullptr;
    }

    PyModule_AddType(RLBot_Lua__module, "LuaBot", &PyType_LuaBot);

    return RLBot_Lua__module;
}
