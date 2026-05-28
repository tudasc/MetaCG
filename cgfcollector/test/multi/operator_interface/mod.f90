module mod

    implicit none

    type :: integer_wrapper
        integer :: value
    end type integer_wrapper

    interface operator(+)
        procedure add_stuff
        module procedure unary_plus
    end interface

    interface operator(.NEGX.)
        module procedure negx
    end interface

contains

    function add_stuff(x, y) result(res)
        type(integer_wrapper), intent(in) :: x, y
        type(integer_wrapper) :: res
        res%value = x%value + y%value
        print *, "Hello from add_stuff"
    end function add_stuff

    function unary_plus(x) result(res)
        type(integer_wrapper), intent(in) :: x
        type(integer_wrapper) :: res
        res%value = x%value
        print *, "Hello from unary_plus"
    end function unary_plus

    function negx(x) result(res)
        real, intent(in) :: x
        real :: res
        res = -x
        print *, "Hello from negx"
    end function negx

end module mod

