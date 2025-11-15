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
    -- on utilise 'this.camera' et 'this.transform' directement
end

function onInput(event)
    local cam = this.camera
    local tr  = this.transform
    if not cam or not tr then return end

    local speed = 5.0
    local sensitivity = 0.1
    -- forward est un tableau {x,y,z} retourné par la métatable camera.target
    local forward = normalize(cam.target)
    local right = normalize(cross(forward, {0,1,0}))
    local scrollY = event.scroll

    for _, btn in ipairs(event.buttons) do
        if btn == "Forward" then 
            tr.position = add(tr.position, scale(forward, speed * event.dt))
        elseif btn == "Backward" then tr.position = subtract(tr.position, scale(forward, speed * event.dt))
        elseif btn == "Right" then tr.position = add(tr.position, scale(right, speed * event.dt))
        elseif btn == "Left" then tr.position = subtract(tr.position, scale(right, speed * event.dt)) end

        if scrollY ~= 0 and btn == "RightMouse" then
            sensitivity = sensitivity + scrollY * 0.01
            scrollY = 0
        end
    end

    if event.mouseMoved then
        cam.yaw   = cam.yaw   + event.mouseDeltaX * sensitivity
        cam.pitch = cam.pitch + event.mouseDeltaY * sensitivity
        if cam.pitch > 89.0 then cam.pitch = 89.0 end
        if cam.pitch < -89.0 then cam.pitch = -89.0 end

        local dir = {
            math.cos(math.rad(cam.yaw)) * math.cos(math.rad(cam.pitch)),
            math.sin(math.rad(cam.pitch)),
            math.sin(math.rad(cam.yaw)) * math.cos(math.rad(cam.pitch))
        }
        cam.target = normalize(dir)
    end

    if scrollY ~= 0 then
        tr.position = add(tr.position, {0,0,-scrollY*0.5})
    end
end
