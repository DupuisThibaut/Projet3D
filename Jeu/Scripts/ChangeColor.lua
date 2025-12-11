local vie = 100


function onInit()
    if this.material then
        this.material.color = { 1.0, 0.0, 0.0 }
        --set_global("vie", vie)
        --print("[Lua] Script chargé : ChangeColor.lua")
    end
end

function onUpdate(dt)
    if this.material then
        local time = os.clock()
        local speed = 1.0
        this.material.color = {
            r = (math.sin(time * speed) + 1) / 2,
            g = (math.sin(time * speed + 2*math.pi/3) + 1) / 2,
            b = (math.sin(time * speed + 4*math.pi/3) + 1) / 2
        }
    end
end
local created = false
function onInput(event)
    for _, btn in ipairs(event.buttons) do
        if btn.name == "C" and btn.state == "PRESSED" then
            if not created then
                --local t = create_entity()
                --add_component(t, "Transform", { position = {-0.5, -9.0, 0.0} })
                --local mesh = add_component(t, "Mesh", { type = "primitive", name = "SPHERE" })
                --local m = add_component(t, "Material", { color = { 1.0,0.0,1.0 }, type = "color" })
                --print("[Lua] Created new entity with id: " .. t)
                created = true
                change_scene("cornelBox.json")
            end
        end
        if btn.name == "C" and btn.state == "RELEASED" and created then
            created = false
        end
        if btn.name == "1" and btn.state == "PRESSED" then
            if not created then
                --local t = create_entity()
                --add_component(t, "Transform", { position = {-0.5, -9.0, 0.0} })
                --local mesh = add_component(t, "Mesh", { type = "primitive", name = "SPHERE" })
                --local m = add_component(t, "Material", { color = { 1.0,0.0,1.0 }, type = "color" })
                --print("[Lua] Created new entity with id: " .. t)
                created = true
                change_scene("scene.json")
            end
        end
        if btn.name == "1" and btn.state == "RELEASED" and created then
            created = false
        end
        if btn.name == "2" and btn.state == "PRESSED" then
            if not created then
                --local t = create_entity()
                --add_component(t, "Transform", { position = {-0.5, -9.0, 0.0} })
                --local mesh = add_component(t, "Mesh", { type = "primitive", name = "SPHERE" })
                --local m = add_component(t, "Material", { color = { 1.0,0.0,1.0 }, type = "color" })
                --print("[Lua] Created new entity with id: " .. t)
                created = true
                change_scene("cornelBox.json")
            end
        end
        if btn.name == "2" and btn.state == "RELEASED" and created then
            created = false
        end
        if btn.name == "3" and btn.state == "PRESSED" then
            if not created then
                --local t = create_entity()
                --add_component(t, "Transform", { position = {-0.5, -9.0, 0.0} })
                --local mesh = add_component(t, "Mesh", { type = "primitive", name = "SPHERE" })
                --local m = add_component(t, "Material", { color = { 1.0,0.0,1.0 }, type = "color" })
                --print("[Lua] Created new entity with id: " .. t)
                created = true
                change_scene("Particules.json")
            end
        end
        if btn.name == "3" and btn.state == "RELEASED" and created then
            created = false
        end
    end
end