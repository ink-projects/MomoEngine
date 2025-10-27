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
    -- print on space
    if KeyIsDown(KEYBOARD.SPACE) then
        print("BEEP!")
    end

    -- quit on escape
    if KeyIsDown(KEYBOARD.ESCAPE) then
        Quit()
    end
end
