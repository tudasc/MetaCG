module mod

    implicit none

    interface add
        module procedure add_int, add_real
    end interface add

contains

    subroutine add_int(x, y)
        implicit none
        integer, intent(inout) :: x, y
        x = x + y
    end subroutine add_int

    subroutine add_real(x, y)
        implicit none
        real, intent(inout) :: x, y
        x = x + y
    end subroutine add_real

end module mod

program main
    use mod

    implicit none

    integer :: x, y
    real :: r1, r2
    x = 1
    y = 2
    r1 = 3.3
    r2 = 4.4

    call add(x, y)
    call add(r1, r2)

    print *, "Integer addition result: ", x
    print *, "Real addition result: ", r1

end program main

