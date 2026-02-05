module shapes
    implicit none

    type, abstract :: Shape
    contains
        procedure(draw), deferred :: draw_shape
    end type Shape

    abstract interface
        subroutine draw(self)
            import :: Shape
            class(Shape), intent(in) :: self
        end subroutine draw
    end interface

    type, extends(Shape) :: Circle
        real :: radius
    contains
        procedure :: draw_shape => draw_circle
    end type Circle

    type, extends(Shape) :: Rectangle
        real :: width, height
    contains
        procedure :: draw_shape => draw_rectangle
    end type Rectangle

contains

    subroutine draw_circle(self)
        class(Circle), intent(in) :: self
        print *, "Drawing a circle with radius:", self%radius
    end subroutine draw_circle

    subroutine draw_rectangle(self)
        class(Rectangle), intent(in) :: self
        print *, "Drawing a rectangle with width:", self%width, "and height:", self%height
    end subroutine draw_rectangle

end module shapes

program main
    use shapes
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

end program main
