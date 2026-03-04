function inner()
    print "MiniScript isolated call-stack inner line"
    return 1
end function

function middle()
    call inner
    return 2
end function

function outer()
    call middle
    return 3
end function