program main
    use iso_c_binding
    implicit none

    integer(C_INT), external :: add

    integer(C_INT) :: result
    integer(C_INT) :: var1
    integer(C_INT) :: var2
    var1 = 3
    var2 = 23

    result = add(var1, var2)
    print *, "Result from C add function:", result
end program main
