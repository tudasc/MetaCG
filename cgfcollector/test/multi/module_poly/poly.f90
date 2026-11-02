program main
    use shapes
    implicit none
    call test_poly()
contains
    subroutine test_poly()
        implicit none

        class(Shape), allocatable :: s
        type(Circle) :: c
        type(Rectangle) :: r

        c%radius = 5.0
        s = c
        call s%draw_shape()

        r%width = 10.0
        r%height = 20.0
        s = r
        call s%draw_shape()
    end subroutine test_poly
end program main

