module mod
    implicit none

    type :: base
        integer :: value
    end type base

    interface operator(*)
        module procedure multiply_base
    end interface

    real, parameter :: num = 2*3.1415926535898
    real, parameter :: num2 = num*0.5, num3 = num2*0.5

    type(base), parameter :: pi = base(3.1415926535898)
    type(base), parameter :: half = base(0.5)
    ! type(base) :: half_pi = pi*half not possible becasue no constant

contains

    function multiply_base(a, b) result(res)
        type(base), intent(in) :: a, b
        type(base) :: res
        res%value = a%value*b%value
        print *, "Multiplying base values: ", a%value, " * ", b%value, " = ", res%value
    end function multiply_base

end module mod

program main
    use mod
    implicit none

end program main
