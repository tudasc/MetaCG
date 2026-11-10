module shape
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
end module shape
