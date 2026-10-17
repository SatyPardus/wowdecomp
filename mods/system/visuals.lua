-- ==============================================================================
-- wowdecomp Visuals Engine (Danger Zones, Weapon Trails, Scripted Missiles)
-- ==============================================================================

local Visuals = {}
local activeDecals = {}

-- Create a boss encounter telegraph decal
function Visuals.CreateDecal(params)
    if not params or not params.position then
        ModCore.LogError("Visuals.CreateDecal: Missing position parameter")
        return nil
    end

    local decal = {
        id = #activeDecals + 1,
        shape = params.shape or "CIRCLE",
        position = {
            x = params.position.x or 0,
            y = params.position.y or 0,
            z = params.position.z or 0
        },
        radius = params.radius or 5.0,
        innerRadius = params.innerRadius or 0.0,
        coneAngle = params.coneAngle or 90.0,
        color = params.color or { r = 1.0, g = 0.2, b = 0.2, a = 0.6 },
        duration = params.duration or 3.0,
        elapsed = 0.0,
        alive = true
    }

    function decal:SetPosition(pos)
        self.position.x = pos.x or self.position.x
        self.position.y = pos.y or self.position.y
        self.position.z = pos.z or self.position.z
    end

    function decal:SetRadius(r)
        self.radius = r
    end

    function decal:Destroy()
        self.alive = false
    end

    table.insert(activeDecals, decal)
    return decal
end

-- Update all active visual telegraphs each tick
ModCore.Hooks.Register("OnWorldUpdate", function(dt)
    for i = #activeDecals, 1, -1 do
        local d = activeDecals[i]
        d.elapsed = d.elapsed + dt
        if not d.alive or (d.duration > 0 and d.elapsed >= d.duration) then
            table.remove(activeDecals, i)
        end
    end
end, 100, "Visuals tick update")

-- Render decals in 3D world space
ModCore.Hooks.Register("OnWorldRender", function(worldFrame)
    for _, d in ipairs(activeDecals) do
        -- Rendering primitive hook: C++ engine renders active decals
    end
end, 100, "Visuals render pass")

-- Weapon trail ribbon attachment
function Visuals.AttachWeaponTrail(unit, hand, params)
    if not unit then return end
    ModCore.LogDebug("Visuals: Attaching weapon trail to hand %d", hand)
    -- Dispatches to native WeaponTrailPlayback if registered
end

return Visuals
