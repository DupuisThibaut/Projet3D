function add(a, b) return {a[1]+b[1], a[2]+b[2], a[3]+b[3]} end
function subtract(a, b) return {a[1]-b[1], a[2]-b[2], a[3]-b[3]} end
function scale(v, s) return {v[1]*s, v[2]*s, v[3]*s} end
function length(v) return math.sqrt(v[1]*v[1] + v[2]*v[2] + v[3]*v[3]) end
function normalize(v)
    local L = length(v)
    if L == 0 then return {0,0,0} end
    return {v[1]/L, v[2]/L, v[3]/L}
end

function deg(rad) return rad * 180 / math.pi end
function rad(deg) return deg * math.pi / 180 end

local function atan2(y, x)
    return math.atan(y, x)
end

function onInit()
    print("[Lua] MoveCam.lua loaded (orbit + anim switches)")
    this.state = this.state or {}
    local s = this.state

    s.currentAnim = s.currentAnim or "idle"
    s.availableAnims = s.availableAnims or {"idle","walk","run","jump"}
    s.animIndex = 1

    if this.camera then
        this.camera.yaw   = this.camera.yaw   or 0.0
        this.camera.pitch = this.camera.pitch or -10.0
        this.camera.distance = this.camera.distance or 5.0
    end

    local ent = get_entity(2)
    if ent then
        play_animation(ent.id, s.currentAnim)
    end
end

local function faceDirection(transform, dir)
    if not transform or length(dir) == 0 then return end
    local angleRad = atan2(dir[3], dir[1])
    local angleDeg = deg(angleRad)
    transform.rotation = transform.rotation or {0,0,0}
    transform.rotation[2] = -angleDeg + 90.0
end

-- mapping common button names / keys to animations
local animMap = {
    ["1"] = "idle",
    ["2"] = "walk",
    ["3"] = "jump",
    ["4"] = "dance",
    ["AnimIdle"] = "idle",
    ["AnimWalk"] = "walk",
    ["AnimJump"] = "jump",
    ["AnimDance"] = "dance",
    ["Space"] = "jump"
}

function onInput(event)
    local cam  = this.camera
    local camTr= this.transform
    if not cam or not camTr then return end

    local springtrap = get_entity(2)
    if not springtrap or not springtrap.transform then return end

    local s = this.state
    local dt = event.dt or 0.016

    if this.controller then
        this.camera.distance = this.controller.distance or this.camera.distance
        this.controller.sensitivity = this.controller.sensitivity or 0.1
    end
    local sensitivity = (this.controller and this.controller.sensitivity) or 0.12

    -- handle animation switches (pressed buttons)
    for _, btn in ipairs(event.buttons or {}) do
        if btn.state == "PRESSED" then
            local name = btn.name
            if animMap[name] then
                local anim = animMap[name]
                play_animation(springtrap.id, anim)
                s.currentAnim = anim
                -- update animIndex if present in list
                for i,a in ipairs(s.availableAnims) do if a == anim then s.animIndex = i; break end end
            elseif name == "NextAnim" then
                s.animIndex = (s.animIndex % #s.availableAnims) + 1
                local anim = s.availableAnims[s.animIndex]
                play_animation(springtrap.id, anim)
                s.currentAnim = anim
            elseif name == "PrevAnim" then
                s.animIndex = (s.animIndex - 2) % #s.availableAnims + 1
                local anim = s.availableAnims[s.animIndex]
                play_animation(springtrap.id, anim)
                s.currentAnim = anim
            end
        end
    end

    -- keep default idle if nothing set
    if not s.currentAnim or s.currentAnim == "" then
        s.currentAnim = "idle"
        play_animation(springtrap.id, "idle")
    end

    -- Make SpringTrap face opposite camera forward for visuals
    local camForward = nil
    if this.camera and this.camera.target then
        camForward = normalize(this.camera.target)
    end
    if not camForward or length(camForward) == 0 then
        local yaw = (cam.yaw or 0)
        local yawRad = rad(yaw)
        camForward = { math.cos(yawRad), 0, math.sin(yawRad) }
    end
    camForward[2] = 0
    if length(camForward) > 0 then camForward = normalize(camForward) end
    local backDir = { -camForward[1], 0, -camForward[3] }
    faceDirection(springtrap.transform, backDir)

    -- camera orbit controls (mouse)
    local rightMouseHeld = false
    for _, btn in ipairs(event.buttons or {}) do
        if btn.name == "RightMouse" and (btn.state == "PRESSED" or btn.state == "REPEAT" or btn.state == "DOWN") then
            rightMouseHeld = true
            break
        end
    end
    if rightMouseHeld and event.mouseMoved then
        cam.yaw = (cam.yaw or 0) + (event.mouseDeltaX or 0) * sensitivity
        cam.pitch = (cam.pitch or 0) + (event.mouseDeltaY or 0) * sensitivity
        if cam.pitch > 89 then cam.pitch = 89 end
        if cam.pitch < -89 then cam.pitch = -89 end
    end

    local scrollY = event.scroll or 0
    if scrollY ~= 0 then
        local zoomSpeed = (this.controller and this.controller.zoomSpeed) or 0.5
        this.camera.distance = math.max(1.0, (this.camera.distance or 5.0) - scrollY * zoomSpeed)
    end

    -- update camera position to orbit around character
    local target = { springtrap.transform.position[1], springtrap.transform.position[2] + 1.5, springtrap.transform.position[3] }
    local pitchRad = rad(cam.pitch or 0)
    local yawRad2  = rad(cam.yaw or 0)
    local dir = {
        math.cos(yawRad2) * math.cos(pitchRad),
        math.sin(pitchRad),
        math.sin(yawRad2) * math.cos(pitchRad)
    }
    local dist = cam.distance or 5.0
    local camPos = subtract(target, scale(normalize(dir), dist))
    this.transform.position = camPos
    this.camera.target = normalize({ target[1] - camPos[1], target[2] - camPos[2], target[3] - camPos[3] })
    this.camera.update = true
end