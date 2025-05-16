TTE_MountSystem("<Data>/", "/Users/lucassaragosa/Desktop/extract/Bone/", true)

local my_Props = ResourceGetNames("*.scene")
TTE_Log("All resource names:")
for k,v in ipairs(my_Props) do
    TTE_Log(v)
end

TTE_Log("Finshed! Did we have any memory leaks? ...")