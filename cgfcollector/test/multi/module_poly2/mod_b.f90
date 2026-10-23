module mod_b
    use mod_a
    implicit none

    type, extends(base) :: middle
    contains
        procedure :: op => op_middle
    end type middle

contains
    subroutine op_middle(this)
        class(middle), intent(in) :: this
        print *, "middle"
    end subroutine op_middle
end module mod_b
