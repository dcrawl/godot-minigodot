signal ping(value, label)

function _ready()
    emit ping 123, "ok"
end function
