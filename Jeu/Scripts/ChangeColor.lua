function onInit()
    -- print("[Lua] Script chargé : ChangeColor.lua")
    -- Initialisation du script
    material.color = {r = 1.0, g = 0.0, b = 0.0} -- Rouge initial
end

function onUpdate(dt)
    -- Changement de couleur en fonction du temps
    local time = os.clock()
    local speed = 1.0
    material.color = {
            r = (math.sin(time * speed) + 1) / 2,           -- varie entre 0 et 1
            g = (math.sin(time * speed + 2 * math.pi / 3) + 1) / 2, -- déphasé
            b = (math.sin(time * speed + 4 * math.pi / 3) + 1) / 2  -- déphasé
        }
end

function onInput(event)
    -- Gérer les entrées si nécessaire
    for i, btn in ipairs(event.buttons) do
        if btn == "C" then
            material.color = {r = math.random(), g = math.random(), b = math.random()}
        end
    end
end