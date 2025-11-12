function add(a, b)
    return {a[1]+b[1], a[2]+b[2], a[3]+b[3]}
end

function subtract(a, b)
    return {a[1]-b[1], a[2]-b[2], a[3]-b[3]}
end

function scale(v, s)
    return {v[1]*s, v[2]*s, v[3]*s}
end

function length(v)
    return math.sqrt(v[1]^2 + v[2]^2 + v[3]^2)
end

function normalize(v)
    local len = length(v)
    if len == 0 then return {0,0,0} end
    return {v[1]/len, v[2]/len, v[3]/len}
end

function cross(a, b)
    return {
        a[2]*b[3] - a[3]*b[2],
        a[3]*b[1] - a[1]*b[3],
        a[1]*b[2] - a[2]*b[1]
    }
end

function math.rad(deg)
    return deg * math.pi / 180
end

function math.deg(rad)
    return rad * 180 / math.pi
end

function onInit()
    print("[Lua] Script chargé : MoveCam.lua")
end

function onUpdate(dt)
    -- Rien à faire ici pour l'instant
end

function onInput(event)
    local speed = 5.0
    local sensitivity = 0.1
    -- Calcul des directions
    local forward = normalize(camera.target)
    local right = normalize(cross(forward, {0,1,0}))
    local scrollY = event.scroll

    -- Mouvement translation
    for i, btn in ipairs(event.buttons) do
        if btn == "Forward" then
            transform.position = add(transform.position, scale(forward, speed * event.dt))
        elseif btn == "Backward" then
            transform.position = subtract(transform.position, scale(forward, speed * event.dt))
        elseif btn == "Right" then
            transform.position = add(transform.position, scale(right, speed * event.dt))
        elseif btn == "Left" then
            transform.position = subtract(transform.position, scale(right, speed * event.dt))
        end

        -- Zoom avec la molette
        if scrollY ~= 0 and btn == "RightMouse" then
            sensitivity = sensitivity + scrollY * 0.01
            if sensitivity < 0.01 then sensitivity = 0.01 end
            if sensitivity > 1.0 then sensitivity = 1.0 end
            scrollY = 0
        end
    end

    -- Mouvement souris pour rotation caméra
    if event.mouseMoved then
        camera.yaw   = camera.yaw   + event.mouseDeltaX * sensitivity
        camera.pitch = camera.pitch + event.mouseDeltaY * sensitivity

        if camera.pitch > 89.0 then camera.pitch = 89.0 end
        if camera.pitch < -89.0 then camera.pitch = -89.0 end

        local dir = {
            math.cos(math.rad(camera.yaw)) * math.cos(math.rad(camera.pitch)),
            math.sin(math.rad(camera.pitch)),
            math.sin(math.rad(camera.yaw)) * math.cos(math.rad(camera.pitch))}
        camera.target = normalize(dir)
    end

    -- Translation finale avec scroll
    if scrollY ~= 0 then
        transform.position = add(transform.position, {0,0,-scrollY*0.5})
    end
end
