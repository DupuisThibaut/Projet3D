-- ...existing code...
local startTime = nil
local dayDuration = 60

local function getLifeValue(v)
    if not v then return nil end
    if type(v) == "number" then return v end
    if type(v) == "table" then
        if v[1] and type(v[1]) == "number" then return v[1] end
        if v.r and type(v.r) == "number" then return v.r end
    end
    return nil
end

function onInit()
    print("[Lua] Script chargé : SunCycle.lua")
    this.transform.position = {0.0, 10.0, 0.0}
    startTime = os.clock()
    dayDuration = 60

    -- local life = getLifeValue(get_global("vie"))
    --if life then
    --   local t = life / 100.0
    --    if this.material then
    --        this.material.color = { 1.0 - t, t, 0.0 }
    --    end
    this.material.color = {1.0, 1.0, 1.0}
        print("[Lua] Applied initial color with life =", life)
    --end
end

function onUpdate(dt)
    if not startTime then
        -- defensive: ensure we have a startTime
        startTime = os.clock()
        return
    end

    local elapsed = os.clock() - startTime
    local t = (elapsed % dayDuration) / dayDuration
    local angle = t * 2 * math.pi
    local radius = 20.0

    this.transform.position = { radius * math.cos(angle), radius * math.sin(angle), 0.0 }

    --local life = getLifeValue(get_global("vie"))
    --if life and this.material then
    --    local tt = life / 100.0
    --    this.material.color = { 1.0 - tt, tt, 0.0 }
    --end
end

function onInput(event)
    for i, btn in ipairs(event.buttons) do
        if btn == "C" then
            -- set_transform(this.id, { position = { math.random()*20-10, math.random()*20-10, math.random()*20-10 } })
        end
    end
end