program main
    use mod

    implicit none

    integer :: x, y, res
    real :: e = 5.0, f

    x = 5
    y = 10

    res = x + y

    print *, "Result of x + y = ", res

    f = .NEGX.e

    print *, "Negated value: ", f
end program main

