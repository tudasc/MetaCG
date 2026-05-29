program main
    implicit none

    abstract interface
        function func(x) result(y)
            real, intent(in) :: x
        end function func
    end interface

    procedure(func), pointer :: func_ptr => null()
    real :: result

    result = 0.0
    func_ptr => square
    result = func_ptr(2.0)
    print *, "Square of 2.0 is: ", result

    func_ptr => cube
    result = func_ptr(2.0)
    print *, "Cube of 2.0 is: ", result

contains

    function square(x) result(y)
        real, intent(in) :: x
        real :: y
        y = x*x
    end function square

    function cube(x) result(y)
        real, intent(in) :: x
        real :: y
        y = x*x*x
    end function cube

end program main
