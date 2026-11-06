module shapes
    use shape
    implicit none

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

