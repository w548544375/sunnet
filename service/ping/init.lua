local serviceID

function OnInit(serviceId)
	serviceID = serviceId
	print("ping service " .. serviceId .. " OnInit")
end

function OnServiceMsg(from, buf)
	print("[Lua]recv msg from " .. from .. ":" .. buf)
	print(string.unpack("i4 i4", buf))
	--Sunnet.Send(serviceID, from, " Pong")
end

function OnExit()
	print("[Lua] ping serivce " .. serviceID .. " OnExit!")
end
