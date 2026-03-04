function inner()
    print "MiniScript isolated step-into inner line"
    return 7
end function

function outer()
    call inner
    return 123
end function