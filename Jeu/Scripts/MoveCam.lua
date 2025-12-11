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
    print("[Lua] MoveCam.lua loaded")
    this.state = this.state or {}
    local s = this.state

    local ctrl = this.controller or {}
    s.speed      = s.speed      or (ctrl.moveSpeed or 3.0)
    s.jumpSpeed  = s.jumpSpeed  or 20.0
    s.gravity    = s.gravity    or 18.0
    s.velY       = s.velY       or 0.0
    s.grounded   = (s.grounded == nil) and true or s.grounded
    s.currentAnim= s.currentAnim or "idle"
    s.jumpPlayed = s.jumpPlayed or false       -- new flag to avoid double play

    if this.camera then
        this.camera.yaw   = this.camera.yaw   or 0.0
        this.camera.pitch = this.camera.pitch or -10.0
        this.camera.distance = this.camera.distance or 5.0
    end
end

local function faceDirection(transform, dir)
    if not transform or length(dir) == 0 then return end
    local angleRad = atan2(dir[3], dir[1])
    local angleDeg = deg(angleRad)
    transform.rotation = transform.rotation or {0,0,0}
    transform.rotation[2] = -angleDeg + 90.0
end

function onInput(event)
    local cam  = this.camera
    local camTr= this.transform
    if not cam or not camTr then return end

    local springtrap = get_entity(1)
    if not springtrap or not springtrap.transform then return end

    local s = this.state
    local dt = event.dt or 0.016

    -- controller overrides
    if this.controller then
        s.speed = this.controller.moveSpeed or s.speed
        this.camera.distance = this.controller.distance or this.camera.distance
        this.controller.sensitivity = this.controller.sensitivity or 0.1
    end
    local sensitivity = (this.controller and this.controller.sensitivity) or 0.12

    -- compute camera forward: prefer actual camera target if available (more accurate than yaw)
    local camForward = nil
    if this.camera and this.camera.target then
        camForward = normalize(this.camera.target)
    end
    if not camForward or length(camForward) == 0 then
        local yaw = (cam.yaw or 0)
        local yawRad = rad(yaw)
        camForward = { math.cos(yawRad), 0, math.sin(yawRad) }
    end
    -- ensure XZ-only for movement / facing
    camForward[2] = 0
    if length(camForward) > 0 then camForward = normalize(camForward) end
    local camRight = { -camForward[3], 0, camForward[1] }

    -- inputs -> movement/jump
    local move = {0,0,0}
    local moving = false
    local jumpPressed = false
    for _, btn in ipairs(event.buttons) do
        local name = btn.name
        local state = btn.state
        local held = (state == "PRESSED" or state == "REPEAT")
        if name == "Forward" and held then
            move = add(move, camForward); moving = true
        elseif name == "Backward" and held then
            move = subtract(move, camForward); moving = true
        elseif name == "Right" and held then
            move = add(move, camRight); moving = true
        elseif name == "Left" and held then
            move = subtract(move, camRight); moving = true
        elseif name == "Space" and state == "PRESSED" then
            jumpPressed = true
        end
    end

    move[2] = 0
    if length(move) > 0 then move = normalize(move) end

    local rb = springtrap.rigidbody
    local hasRB = rb and rb.velocity

    -- HORIZONTAL MOVEMENT
    if hasRB then
        -- set horizontal velocity (preserve vertical velocity)
        local vx = 0; local vz = 0
        if moving and length(move) > 0 then
            vx = move[1] * s.speed
            vz = move[3] * s.speed
        end
        rb.velocity[1] = vx
        rb.velocity[3] = vz
    else
        if moving then
            local delta = scale(move, s.speed * dt)
            springtrap.transform.position = add(springtrap.transform.position, delta)
        end
    end

    -- JUMP
    if jumpPressed and s.grounded then
        s.grounded = false
        s.jumpPlayed = true
        if hasRB then
            -- apply vertical velocity through rigidbody
            rb.velocity[2] = s.jumpSpeed
            -- small nudge to avoid immediate re-contact
            springtrap.transform.position[2] = springtrap.transform.position[2] + 0.01
        else
            s.velY = s.jumpSpeed
        end
        play_animation(springtrap.id, "jump")
        s.currentAnim = "jump"
    end

    -- GROUNDED CHECK & VERTICAL INTEGRATION
    local groundY = 0.0
    local eps = 0.05

    if hasRB then
        -- prefer a grounded flag from physics if available
        if rb.isGrounded ~= nil then
            s.grounded = rb.isGrounded
        elseif rb.onGround ~= nil then
            s.grounded = rb.onGround
        else
            -- fallback: use transform.y
            s.grounded = (springtrap.transform.position[2] <= groundY + eps)
        end

        -- clamp on ground and zero vertical vel when grounded
        if s.grounded then
            springtrap.transform.position[2] = groundY
            rb.velocity[2] = rb.velocity[2] and math.max(rb.velocity[2], 0) or 0
            if s.jumpPlayed then s.jumpPlayed = false end
        end
    else
        -- manual integration
        s.velY = s.velY - s.gravity * dt
        local newY = springtrap.transform.position[2] + s.velY * dt
        if newY <= groundY + eps then
            if not s.grounded then s.grounded = true; s.velY = 0; s.jumpPlayed = false end
            newY = groundY
        else
            s.grounded = false
        end
        springtrap.transform.position[2] = newY
    end

    -- ANIMATIONS: play jump only once (s.jumpPlayed prevents double plays)
    if not s.grounded then
        -- already played at jump start, don't replay
    else
        if moving then
            if s.currentAnim ~= "walk" then play_animation(springtrap.id, "walk"); s.currentAnim = "walk" end
        else
            if s.currentAnim ~= "idle" then play_animation(springtrap.id, "idle"); s.currentAnim = "idle" end
        end
    end

    -- Make SpringTrap keep its back to the camera (opposite of camera forward)
    local backDir = { -camForward[1], 0, -camForward[3] }
    if length(backDir) > 0 then
        faceDirection(springtrap.transform, backDir)
    end

    -- camera orbit controls
    local rightMouseHeld = false
    for _, btn in ipairs(event.buttons) do
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
