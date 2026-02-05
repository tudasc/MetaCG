program main
    use iso_c_binding
    implicit none

    interface
        function add(a, b) bind(C) result(res)
            import :: C_INT
            implicit none
            integer(C_INT), value :: a, b
            integer(C_INT) :: res
        end function add
    end interface

    integer(C_INT) :: result
    integer(C_INT) :: var1
    integer(C_INT) :: var2
    var1 = 3
    var2 = 23

    result = add(var1, var2)
    print *, "Result from C add function:", result

end program main

