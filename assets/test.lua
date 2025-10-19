-- test.lua

print("Lua script loaded.")

-- load texture
if not LoadTexture("momo", "assets/momo.png") then
    print("Failed to load momo.png")
end

-- create sprite
local mySprite = Sprite.new("momo", vec3.new(0.0, 0.0, 0.0), vec2.new(1.0, 1.0), 0.0, 1, 1)

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
