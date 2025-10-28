-- test.lua
if not initialized then 
    print("Lua script loaded.")

    -- load texture
    if not LoadTexture("momo", "assets/momo.png") then
        print("Failed to load momo.png")
    end

    initialized = true;
end

function Update()
    -- sprite movement
    local speed = 0.05
    local pos = GetPosition(entity)

    if KeyIsDown(KEYBOARD.W) then
        pos.y = pos.y + speed
    end
    if KeyIsDown(KEYBOARD.S) then
        pos.y = pos.y - speed
    end
    if KeyIsDown(KEYBOARD.A) then
        pos.x = pos.x - speed
    end
    if KeyIsDown(KEYBOARD.D) then
        pos.x = pos.x + speed
    end

    SetPosition(entity, pos.x, pos.y)

    -- print on space
    if KeyIsDown(KEYBOARD.SPACE) then
        print("BEEP!")
    end

    -- quit on escape
    if KeyIsDown(KEYBOARD.ESCAPE) then
        Quit()
    end
end
