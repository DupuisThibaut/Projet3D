function onInit()
    print("[Lua] Script chargé : CycleJourNuit.lua")
    transform.position = {0.0, 10.0, 0.0} 
    startTime = os.clock()
    dayDuration = 60
end

function onUpdate(dt)
    local elapsed = os.clock() - startTime
    local t = (elapsed % dayDuration) / dayDuration

    local angle = t * 2 * math.pi
    local radius = 20.0

    transform.position = {radius * math.cos(angle),radius * math.sin(angle),0.0}
end

function onInput(event)
    for i, btn in ipairs(event.buttons) do
        if btn == "C" then
            transform.position = {math.random()*20-10,math.random()*20-10, math.random()*20-10}
        end
    end
end
