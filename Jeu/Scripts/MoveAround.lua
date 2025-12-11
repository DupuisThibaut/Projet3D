local function degToRad(d) return d * math.pi / 180 end
local function radToDeg(r) return r * 180 / math.pi end
local function atan2(y, x) return math.atan(y, x) end
local function length(v) return math.sqrt(v[1]*v[1] + v[2]*v[2] + v[3]*v[3]) end
local function normalize(v)
    local L = length(v)
    if L == 0 then return {0,0,0} end
    return {v[1]/L, v[2]/L, v[3]/L}
end

function onInit()
    this.transform.position = {0.0, 0.0, -5.0}
    startTime = os.clock()
    dayDuration = 20
end

function onUpdate(dt)
    if not startTime then
        startTime = os.clock()
        return
    end

    local elapsed = os.clock() - startTime
    local t = (elapsed % dayDuration) / dayDuration
    local angle = t * 2 * math.pi
    local radius = 10.0

    this.transform.position = { radius * math.cos(angle),0.0 , radius * math.sin(angle)}
end
